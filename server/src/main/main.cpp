#include "network/networkapplication.hpp"
#include "core/file.hpp"

namespace tecmn {
struct application_context {
    std::vector<std::string> messages;
};
}
int main(int argc, char* argv[])
{
    tecmn::application_context context;
    tecmn::network_application app{context};
    app.set_client_connected([&](util::handle_val<tecmn::client_connection> client) {
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
            std::cout << "client disconnected, reason=" << reason << "\n";
        });
    });
    app.run();
    return 0;
}
