#include "network/networkapplication.hpp"

tecmn::network_application::network_application(application_context& context) : context(context) {}

void tecmn::network_application::run() {
    server.run();
    std::vector<std::function<void()>> tasks;
    for (int i = 0; i < thread_count; i++) {
        tasks.push_back([&]{ io.run(); });
    }
    pool.block_for_many(tasks);
}
