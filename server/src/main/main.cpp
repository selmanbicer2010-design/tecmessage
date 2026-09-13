#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "boost/beast/core/flat_buffer.hpp"
#include "boost/beast/core/tcp_stream.hpp"
#include "core/event.hpp"
#include "core/threadpool.hpp"
#include <cstdint> // IWYU pragma: keep
#include <future>
#include <iostream>
#include <mutex>
#include <print> // IWYU pragma: keep
#include <type_traits>
#include "core/util.hpp"
#include "network/server.hpp"
#include "network/client.hpp"

int main(int argc, char* argv[])
{
    tecm::thread_pool pool;
    boost::asio::io_context io{};

    auto const address = boost::asio::ip::make_address("0.0.0.0");
    unsigned short port = std::getenv("PORT") ? util::stoi32(std::string{std::getenv("PORT")}).value : 8080;

    tecmn::server server{io, boost::asio::ip::tcp::endpoint{address, port}};

    server.client_connected.connect([&](util::handle_val<tecmn::client_connection> client) {
        std::cout << "client connected\n";

        event::pconnect(client->message_recieved, client->disconnected, [client](tecmn::message msg) {
            std::cout << "received: " << msg.get_string() << "\n";
            client->send(std::string("echo: ") + msg.get_string());
        });

        client->disconnected.once([&, client](int32_t reason) {
            std::cout << "client disconnected, reason=" << reason << "\n";
        });
    });

    server.run();

    std::vector<std::function<void()>> tasks;
    const int32_t server_thread_count = 4;
    for (int i = 0; i < server_thread_count; i++) {
        tasks.push_back([&]{ io.run(); });
    }

    pool.block_for_many(tasks);
    return 0;
}
