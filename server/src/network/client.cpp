#include "network/client.hpp"
#include "boost/asio/strand.hpp"
#include "boost/beast/core/flat_buffer.hpp"
#include "boost/beast/core/flat_static_buffer.hpp"
#include "core/util.hpp"
#include "network/server.hpp"
#include "core/enum.hpp"

tecmn::client_connection::client_connection(tecmn::server& server_ref, boost::asio::ip::tcp::socket&& socket, http_request_t&& request)
: server_p(&server_ref), websocket(std::move(socket)), request(std::move(request)), strand(websocket.get_executor()) {}

void tecmn::client_connection::run() {
    boost::beast::net::dispatch(strand,
        boost::beast::bind_front_handler(&client_connection::on_run, this)
    );
}

void tecmn::client_connection::exit(int32_t reason) {
    boost::asio::dispatch(strand, [this, reason] {
        if (dead) return;
        dead = true;
        if (server_p) {
            disconnected.fire(reason);
            server_p->access_ws_map_with_lock([&](auto& client_map){
                if (this_in_server_storage.has_key()) {
                    client_map.erase(this_in_server_storage.key());
                }
            });
        }
    });
}

void tecmn::client_connection::exit_already_locked(int32_t reason) {
    if (dead) return;
    dead = true;
    if (server_p) {
        disconnected.fire(reason);
        if (this_in_server_storage.has_key()) {
            this_in_server_storage.owner().erase(this_in_server_storage.key());
        }
    }
}

bool tecmn::client_connection::check_failure(boost::beast::error_code ec, const char* errstr) {
    if (dead) return true;
    if (ec) {
        BOOST_BEAST_FAIL(ec, errstr);
        if (ec == boost::beast::websocket::error::closed) {
            exit(CLIENT_DISCONNECTED_NORMALLY);
            return true;
        }
        switch (ec.value()) {
            case boost::asio::error::eof:
                exit(CLIENT_DISCONNECTED_NORMALLY);
                return true;
            case boost::asio::error::operation_aborted:
                exit(CLIENT_DISCONNECTED_SHUT_DOWN_BY_SERVER);
                return true;
            case boost::asio::error::connection_aborted:
            case boost::asio::error::connection_reset:
            case boost::asio::error::broken_pipe:
            case boost::asio::error::not_connected:
            case boost::asio::error::shut_down:
            case boost::asio::error::timed_out:
            case boost::asio::error::network_down:
            case boost::asio::error::network_unreachable:
            case boost::asio::error::host_unreachable:
                exit(CLIENT_DISCONNECTED_UNEXPECTED);
                return true;
            default:
                exit(CLIENT_DISCONNECTED_UNKNOWN);
                return true;
            }
        return true;
    }
    return false;
}

void tecmn::client_connection::on_run() {
    if (dead) return;
    websocket.set_option(
        boost::beast::websocket::stream_base::timeout::suggested(
            boost::beast::role_type::server)
    );
    websocket.set_option(boost::beast::websocket::stream_base::decorator(
        [](boost::beast::websocket::response_type& res) {
            res.set(boost::beast::http::field::server,
                std::string(BOOST_BEAST_VERSION_STRING) + " websocket-server-async");
        })
    );
    websocket.async_accept(
        request,
        boost::beast::bind_front_handler(&client_connection::on_accept, this)
    );
}

void tecmn::client_connection::on_accept(boost::beast::error_code ec) {
    if (dead) return;
    if (check_failure(ec, util::this_function<const char*>())) return;
    do_read();
}

void tecmn::client_connection::do_read() {
    if (dead) return;
    websocket.async_read(
        buff,
        boost::beast::bind_front_handler(&client_connection::on_read, this)
    );
}

void tecmn::client_connection::on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (dead) return;
    boost::ignore_unused(bytes_transferred);

    if (check_failure(ec, util::this_function<const char*>())) return;

    recieve_buffer.data.resize(buff.size());
    memcpy(recieve_buffer.data.data(), buff.data().data(), buff.size());
    recieve_buffer.is_text = websocket.got_text();
    buff.clear();
    message_recieved.fire({recieve_buffer});

    do_read();
}

void tecmn::client_connection::do_write() {
    if (dead) return;
    websocket.text(send_buffer_queue.front().is_text);
    websocket.async_write(
        boost::asio::buffer(send_buffer_queue.front().data),
        boost::beast::bind_front_handler(&client_connection::on_write, this)
    );
}

void tecmn::client_connection::on_write(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (dead) return;
    boost::ignore_unused(bytes_transferred);
    if (check_failure(ec, util::this_function<const char*>())) return;
    send_buffer_queue.pop_front();
    if (!send_buffer_queue.empty()) {
        do_write();
        return;
    }
}

void tecmn::client_connection::send(std::string message) {
    std::vector<uint8_t> buffer{message.begin(), message.end()};
    boost::beast::net::post(
        strand,
        [this, buffer = std::move(buffer)]() mutable {
            if (dead) return;
            send_buffer_queue.push_back({std::move(buffer), true});
            if (send_buffer_queue.size() == 1)
                do_write();
        }
    );
}

void tecmn::client_connection::send(std::vector<uint8_t> bytes) {
    boost::beast::net::post(
        strand,
        [this, buffer = std::move(bytes)]() mutable {
            if (dead) return;

            send_buffer_queue.push_back({std::move(buffer), false});
            if (send_buffer_queue.size() == 1)
                do_write();
        }
    );
}

bool tecmn::client_connection::is_alive() {
    return !dead;
}



std::string_view mime_type(std::string_view path)
{
    using boost::beast::iequals;
    auto const ext = [&path] {
        auto const pos = path.rfind(".");
        if(pos == std::string_view::npos)
            return std::string_view{};
        return path.substr(pos);
    }();
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

template <class Body, class Allocator>
boost::beast::http::message_generator handle_request(boost::beast::string_view doc_root, boost::beast::http::request<Body, boost::beast::http::basic_fields<Allocator>>&& req)
{
    auto const bad_request =
    [&req](boost::beast::string_view why)
    {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::bad_request, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = std::string(why);
        res.prepare_payload();
        return res;
    };

    auto const not_found =
    [&req](boost::beast::string_view target)
    {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::not_found, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "The resource '" + std::string(target) + "' was not found.";
        res.prepare_payload();
        return res;
    };

    auto const server_error =
    [&req](boost::beast::string_view what)
    {
        boost::beast::http::response<boost::beast::http::string_body> res{boost::beast::http::status::internal_server_error, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, "text/html");
        res.keep_alive(req.keep_alive());
        res.body() = "An error occurred: '" + std::string(what) + "'";
        res.prepare_payload();
        return res;
    };

    //^^ error helper lambdas

    if( req.method() != boost::beast::http::verb::get &&
        req.method() != boost::beast::http::verb::head)
        return bad_request("Unknown HTTP-method");

    if( req.target().empty() ||
        req.target()[0] != '/' ||
        req.target().find("..") != boost::beast::string_view::npos) // nice catch beast-bros
        return bad_request("Illegal request-target");

    std::string target{req.target()};
    auto qpos = target.find('?');
    if (qpos != std::string::npos)
        target = target.substr(0, qpos);

    std::string path = DIST_ROOT + target;
    if(path.back() == '/')
        path = INDEX_HTML_PATH;

    boost::beast::error_code ec;
    boost::beast::http::file_body::value_type body;
    body.open(path.c_str(), boost::beast::file_mode::scan, ec);

    if(ec == boost::beast::errc::no_such_file_or_directory)
        return not_found(req.target());

    if(ec)
        return server_error(ec.message());

    auto const size = body.size();

    if(req.method() == boost::beast::http::verb::head)
    {
        boost::beast::http::response<boost::beast::http::empty_body> res{boost::beast::http::status::ok, req.version()};
        res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
        res.set(boost::beast::http::field::content_type, mime_type(path));
        res.content_length(size);
        res.keep_alive(req.keep_alive());
        return res;
    }

    boost::beast::http::response<boost::beast::http::file_body> res{
        std::piecewise_construct,
        std::make_tuple(std::move(body)),
        std::make_tuple(boost::beast::http::status::ok, req.version())};
    res.set(boost::beast::http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(boost::beast::http::field::content_type, mime_type(path));
    res.content_length(size);
    res.keep_alive(req.keep_alive());
    return res;
}

tecmn::http_session::http_session(server& server_ref, boost::asio::ip::tcp::socket&& socket, http_request_t&& request, std::string doc_root)
: server_p(&server_ref), stream(std::move(socket)), request(std::move(request)), doc_root(doc_root), strand(stream.get_executor()) {}

void tecmn::http_session::run() {
    boost::beast::net::dispatch(strand,
        boost::beast::bind_front_handler(&http_session::on_run, this)
    );
}

bool tecmn::http_session::is_alive() {
    return !dead;
}

void tecmn::http_session::on_run() {
    send_response(handle_request(doc_root, std::move(request)));
}

void tecmn::http_session::do_read() {
    if (dead) return;
    request = {};
    stream.expires_after(std::chrono::seconds(30));

    boost::beast::http::async_read(stream, buffer, request,
        boost::beast::bind_front_handler(
            &http_session::on_read,
            this)
    );
}

void tecmn::http_session::on_read(boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (dead) return;
    boost::ignore_unused(bytes_transferred);
    if (ec == boost::beast::http::error::end_of_stream) {
        close();
        return;
    }
    if (ec) {
        BOOST_BEAST_FAIL(ec, util::this_function() + "read");
        close();
        return;
    }

    send_response(handle_request(doc_root, std::move(request)));
}

void tecmn::http_session::send_response(boost::beast::http::message_generator&& msg) {
    if (dead) return;
    bool keep_alive = msg.keep_alive();

    boost::beast::async_write(
        stream, std::move(msg),
        boost::beast::bind_front_handler(
            &http_session::on_write, this, keep_alive)
    );
}

void tecmn::http_session::on_write(bool keep_alive, boost::beast::error_code ec, std::size_t bytes_transferred) {
    if (dead) return;
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        BOOST_BEAST_FAIL(ec, util::this_function() + "write");
        close();
        return;
    }

    if (!keep_alive) {
        close();
        return;
    }

    do_read();
}

void tecmn::http_session::close() {
    boost::asio::dispatch(strand, [this]{
        boost::beast::error_code ec;
        if (dead) return;
        dead = true;
        auto err = stream.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
        if (server_p) {
            //disconnected.fire(reason);
            server_p->access_http_map_with_lock([&](auto& http_map){
                if (this_in_server_storage.has_key()) {
                    http_map.erase(this_in_server_storage.key());
                }
            });
        }
        if (ec) {
            BOOST_BEAST_FAIL(ec, util::this_function() + "close");
            return;
        }
    });
}
