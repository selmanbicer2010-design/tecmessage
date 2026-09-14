#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "core/event.hpp"
#include "core/file.hpp"
#include "core/threadpool.hpp"
#include <cstdint> // IWYU pragma: keep
#include <print> // IWYU pragma: keep
#include "core/util.hpp"
#include "network/server.hpp"
#include "network/client.hpp"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

//clean up .gitignore and create dockerfile for render

int main(int argc, char* argv[])
{
    tecm::thread_pool pool;
    boost::asio::io_context io{};

    auto const address = boost::asio::ip::make_address("0.0.0.0");
    const char* port_env = std::getenv("PORT");
    unsigned short port = port_env ? util::stoi32(std::string{port_env}).value : 8080;

    tecmn::server server{io, boost::asio::ip::tcp::endpoint{address, port}};

    server.client_connected.connect([&](util::handle_val<tecmn::client_connection> client) {
        //std::cout << "client connected\n";

        event::pconnect(client->message_recieved, client->disconnected, [client](tecmn::message msg) {
            auto jsonstr = R"json(
                {
                  "type": "message",
                  "user": "selman",
                  "content": "Hello, WebSocket!",
                  "id": 42
                }
            )json";
            client->send(std::string("echo: ") + file::json::serialize(file::json::parse(jsonstr)));
        });

        client->disconnected.once([&, client](int32_t reason) {
            //std::cout << "client disconnected, reason=" << reason << "\n";
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
