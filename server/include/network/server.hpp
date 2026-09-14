#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "boost/beast/core/flat_buffer.hpp"
#include "core/enum.hpp"
#include "core/event.hpp"
#include "core/util.hpp"
#include "network/client.hpp"

namespace tecmn{

using endpoint_t = boost::asio::ip::tcp::endpoint;

class server {
    friend class client_connection;
    friend class http_session;
private:
    boost::asio::io_context& io;
    boost::asio::ip::tcp::acceptor acceptor;
    std::mutex map_ws_mtx;
    std::mutex map_http_mtx;

    void do_accept();
    void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

    void make_ws_client(boost::beast::tcp_stream&& stream, http_request_t&& http_req);
    void make_http_client(boost::beast::tcp_stream&& stream, http_request_t&& http_req);

public:
    server(boost::asio::io_context& io, endpoint_t endpoint);
    void run();

    event::async_event<util::handle_val<client_connection>> client_connected;
    util::sequential_unordered_map<client_connection> clients;
    util::sequential_unordered_map<http_session> http_sessions;

    template <typename func>
    requires (std::is_invocable_v<func, util::handle_val<client_connection>>)
    auto for_each_client(func&& fn) {
        std::lock_guard<std::mutex> lg{map_ws_mtx};
        for (auto& [key, client] : clients.unordered_map()) {
            fn(util::handle_val<client_connection>{clients, key});
        }
    }

    template <typename func>
    requires (std::is_invocable_v<func, util::handle_val<client_connection>>)
    auto for_each_client_without_lock(func&& fn) {
        std::vector<util::handle_val<client_connection>> culled;
        {
            std::lock_guard<std::mutex> lg{map_ws_mtx};
            for (auto& [key, client] : clients.unordered_map()) {
                culled.push_back(util::handle_val<client_connection>{clients, key});
            }
        }
        for (auto& client : culled) {
            fn(client);
        }
    }

    template <typename func>
    requires (std::is_invocable_v<func, util::sequential_unordered_map<client_connection>&>)
    auto access_ws_map_with_lock(func&& fn) {
        std::lock_guard<std::mutex> lg{map_ws_mtx};
        return fn(clients);
    }

    template <typename func>
    requires (std::is_invocable_v<func, util::sequential_unordered_map<http_session>&>)
    auto access_http_map_with_lock(func&& fn) {
        std::lock_guard<std::mutex> lg{map_http_mtx};
        return fn(http_sessions);
    }
};

}
