#include "boost/asio/strand.hpp"
#include "core/util.hpp"
#include "network/client.hpp"
#include "network/server.hpp"
#include "core/enum.hpp"

tecmn::server::server(boost::asio::io_context& io, endpoint_t endpoint) : io(io), acceptor(boost::beast::net::make_strand(io)) {
    boost::beast::error_code ec;

    auto err = acceptor.open(endpoint.protocol(), ec);
    if (ec) {
        BOOST_BEAST_FAIL(ec, (util::this_function() + " open").c_str());
        return;
    }

    err = acceptor.set_option(boost::beast::net::socket_base::reuse_address(true), ec);
    if (ec) {
        BOOST_BEAST_FAIL(ec, (util::this_function() + " set_option reuse_address").c_str());
        return;
    }

    err = acceptor.bind(endpoint, ec);
    if (ec) {
        BOOST_BEAST_FAIL(ec, (util::this_function() + " bind").c_str());
        return;
    }

    err = acceptor.listen(boost::beast::net::socket_base::max_listen_connections, ec);
    if (ec) {
        BOOST_BEAST_FAIL(ec, (util::this_function() + " listen").c_str());
        return;
    }

}

void tecmn::server::run() {
    do_accept();
}

void tecmn::server::do_accept() {
    acceptor.async_accept(boost::beast::net::make_strand(io),
        boost::beast::bind_front_handler(&server::on_accept, this)
    );
}

void tecmn::server::on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket) {
    if (ec) {
        BOOST_BEAST_FAIL(ec, util::this_function());
    } else {
        // Wrap the raw socket so it can be read from asynchronously,
        // with the same timeout/cancellation features client_connection and http_session rely on
        auto stream = std::make_shared<boost::beast::tcp_stream>(std::move(socket));
        auto buffer = std::make_shared<boost::beast::flat_buffer>();
        auto req = std::make_shared<http_request_t>();

        boost::beast::http::async_read(*stream, *buffer, *req,
            [this, stream, buffer, req](boost::beast::error_code ec, std::size_t) {
                if (ec) {
                    BOOST_BEAST_FAIL(ec, util::this_function() + "detect");
                    return;
                }

                if (boost::beast::websocket::is_upgrade(*req)) {
                    make_ws_client(std::move(*stream), std::move(*req));
                }
                else {
                    make_http_client(std::move(*stream), std::move(*req));
                }
            });
    }
    do_accept();
}

void tecmn::server::make_ws_client(boost::beast::tcp_stream&& stream, http_request_t&& http_req) {
    util::handle_val<client_connection> client_handle;
    access_ws_map_with_lock([&](auto& client_map){
        client_connection client = client_connection{*this, std::move(stream.socket()), std::move(http_req)};
        int32_t key = client_map.push(std::move(client));
        client_handle = {client_map, key};
        client_handle->this_in_server_storage = client_handle;
        client_connected.fire(client_handle);
        client_handle->run();
    });
}

void tecmn::server::make_http_client(boost::beast::tcp_stream&& stream, http_request_t&& http_req) {
    util::handle_val<http_session> http_handle;
    access_http_map_with_lock([&](auto& http_map){
        http_session session = http_session{*this, std::move(stream.socket()), std::move(http_req), DIST_ROOT};
        int32_t key = http_map.push(std::move(session));
        http_handle = {http_map, key};
        http_handle->this_in_server_storage = http_handle;
        //client_connected.fire(http_handle);
        http_handle->run();
    });
}
