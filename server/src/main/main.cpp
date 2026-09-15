#include "core/util.hpp"
#include "network/networkapplication.hpp"
#include "core/file.hpp"
#include <print>

namespace tecmn {
struct application_context {
    std::vector<file::json::tree> messages;
};
}
int main(int argc, char* argv[])
{
    tecmn::application_context context;
    tecmn::network_application app{context};
    app.set_client_connected([&](util::handle_val<tecmn::client_connection> client) {
        event::pconnect(client->message_recieved, client->disconnected, [&app, client](tecmn::message msg) {
            if (msg.is_text) {
                auto json = file::json::parse(msg.get_string());
                if (json.root.is_object() and json.root.has("intent")) {
                    std::string& intent = json.root["intent"];
                    if (intent == "test") {
                        std::string& text = json.root["text"];
                        std::println("{}", text);
                        return;
                    }
                }
            }
        });

        client->disconnected.once([&, client](int32_t reason) {
            std::cout << "client disconnected, reason=" << reason << "\n";
        });
    });
    app.run();
    return 0;
}
