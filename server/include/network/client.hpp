#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <cstdint>
#include <deque>
#include <vector>
#include "core/util.hpp"
#include "core/event.hpp"

namespace tecmn {

using strand_t = boost::beast::websocket::stream<boost::asio::ip::tcp::socket>::executor_type;
using http_request_t = boost::beast::http::request<boost::beast::http::string_body>;

class server;

struct message {
    std::vector<uint8_t> data;
    bool is_text;
    std::string get_string() {
        return std::string{data.begin(), data.end()};
    }
};

class client_connection {
    friend class server;
private:
    boost::beast::websocket::stream<boost::beast::tcp_stream> websocket;
    boost::beast::flat_buffer buff;
    strand_t strand;
    http_request_t request;
    message recieve_buffer;
    std::deque<message> send_buffer_queue;
    server* server_p = nullptr;
    util::handle_val<client_connection> this_in_server_storage;
    bool dead = false;

    bool check_failure(boost::beast::error_code ec, const char* errstr = "");

    void on_run();
    void on_accept(boost::beast::error_code ec);
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void do_write();
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred);
public:
    client_connection(server& server_ref, boost::asio::ip::tcp::socket&& socket, http_request_t&& request);
    client_connection(client_connection&&) noexcept = default;
    client_connection& operator=(client_connection&&) noexcept = delete;
    client_connection(const client_connection&) = delete;
    client_connection& operator=(const client_connection&) = delete;
    void run();
    bool is_alive();
    void exit(int32_t reason);
    void exit_already_locked(int32_t reason);

    event::async_event<int32_t> disconnected;
    event::async_event<message> message_recieved;
    void send(std::string message);
    void send(std::vector<uint8_t> bytes);
};

class http_session {
    friend class server;
private:
    boost::beast::tcp_stream stream;
    boost::beast::flat_buffer buffer;
    std::string doc_root;
    http_request_t request;
    strand_t strand;
    util::handle_val<http_session> this_in_server_storage;
    server* server_p = nullptr;
    bool dead = false;
public:
    http_session(server& server_ref, boost::asio::ip::tcp::socket&& socket, http_request_t&& request, std::string doc_root);

    void run();
    bool is_alive();

    void on_run();
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void send_response(boost::beast::http::message_generator&& msg);
    void on_write(bool keep_alive, boost::beast::error_code ec, std::size_t bytes_transferred);
    void close();
};

}
