#pragma once

#include "network/client.hpp"
#include "network/server.hpp"
#include "core/threadpool.hpp"
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace tecmn {

struct application_context;

class network_application {
public:
    application_context& context;
    boost::asio::io_context io{};
    thread::thread_pool pool{};
    int8_t thread_count = 8;
    tecmn::server server = [&]{
        auto const address = boost::asio::ip::make_address("0.0.0.0");
        const char* port_env = std::getenv("PORT");
        unsigned short port = port_env ? util::stoi32(std::string{port_env}).value : 8080;
        return tecmn::server{io, tecmn::endpoint_t{address, port}};
    }();
    network_application(application_context& context);

    template<typename t>
    void set_client_connected(t&& callback) {
        server.client_connected.connect(std::forward<t>(callback));
    }

    template <typename func>
    requires (std::is_invocable_v<func, util::handle_val<tecmn::client_connection>>)
    auto for_each_client(func&& fn) {
        return server.for_each_client(std::forward<func>(fn));
    }

    template <typename func>
    requires (std::is_invocable_v<func, util::handle_val<tecmn::client_connection>>)
    auto for_each_client_without_lock(func&& fn) {
        return server.for_each_client_without_lock(std::forward<func>(fn));
    }

    void run();
};

}
