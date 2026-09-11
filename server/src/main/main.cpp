#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "boost/asio/buffer.hpp"
#include "boost/asio/ip/udp.hpp"
#include "boost/asio/placeholders.hpp"
#include "boost/asio/write.hpp"
#include "boost/system/detail/error_code.hpp"
#include "core/threadpool.hpp"
#include <cstdint>
#include <print>
#include <array>
#include "core/util.hpp"

class io_user_base {
public:
    boost::asio::io_context& io_;
    io_user_base(boost::asio::io_context& io_ref) : io_(io_ref) {}
};

std::string make_daytime_string() {
  std::time_t now = std::time(0);
  return std::ctime(&now);
}

class tcp_connection : public io_user_base {
public:
    using io_user_base::io_user_base;
    util::sequential_unordered_map<tcp_connection>* map;
    uint32_t key;
    std::string message;
    boost::asio::ip::tcp::socket socket{io_};

    void start() {
        message = make_daytime_string();
        boost::asio::async_write(socket, boost::asio::buffer(message),
            std::bind(&tcp_connection::handle_write, this));
    }

    void handle_write() {

        map->erase(key);
    }

};

class tcp_server : public io_user_base {
public:
    using io_user_base::io_user_base;
    boost::asio::ip::tcp::acceptor acceptor{io_, boost::asio::ip::tcp::endpoint{boost::asio::ip::tcp::v4(), 13}};
    util::sequential_unordered_map<tcp_connection> connections;

    void start_accept() {
        int32_t key = connections.push({io_});
        tcp_connection& connection = *connections.find(key);
        connection.key = key;
        connection.map = &connections;
        acceptor.async_accept(connection.socket,
            std::bind(&tcp_server::handle_accept, this,
                key, boost::asio::placeholders::error)
        );
    }

    void handle_accept(int32_t key, const boost::system::error_code& error) {
        tcp_connection& connection = *connections.find(key);
        connection.start();
        start_accept();
    }
};

class udp_server : public io_user_base {
public:
    using io_user_base::io_user_base;
    boost::asio::ip::udp::socket socket{io_, boost::asio::ip::udp::endpoint{boost::asio::ip::udp::v4(), 13}};
    boost::asio::ip::udp::endpoint remote_endpoint;
    std::array<char, 1> recv_buffer;

    void start_recieve() {
        socket.async_receive_from(
            boost::asio::buffer(recv_buffer), remote_endpoint,
            std::bind(
                &udp_server::handle_recieve, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred)
        );
    }

    void handle_recieve(const boost::system::error_code& error, std::size_t bytestransferred) {
        auto buff = make_daytime_string();
        socket.async_send_to(boost::asio::buffer(buff), remote_endpoint,
                  std::bind(&udp_server::handle_send, this, buff,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
    }

    void handle_send(std::string message, const boost::system::error_code& error, std::size_t bytes_transferred) {

    }
};

int main(int argc, char* argv[])
{
    tecm::thread_pool pool;

    pool.block_for_many({
        [&]{
            boost::asio::io_context io;
            tcp_server server1{io};
            server1.start_accept();
            io.run();
        },
        [&]{
            boost::asio::io_context io;
            udp_server server0{io};
            server0.start_recieve();
            io.run();
        },
        // [&]{
        //     boost::asio::io_context io; // the os abstraction to the system io
        //     boost::asio::ip::udp::socket socket{io, boost::asio::ip::udp::endpoint{boost::asio::ip::udp::v4(), 13}}; // declares an endpoint for the server using ipv4 and port 13
        //     for (;;)
        //     {
        //         std::array<int8_t, 1> recievebuff; // buffer to catch a value; the "send_buf" on the client; just tells us the client connected
        //         boost::asio::ip::udp::endpoint remoteendpoint; // declare the endpoint that the client will be at
        //         socket.receive_from(boost::asio::buffer(recievebuff), remoteendpoint); // blocking call - waits for client and fills buffer and their endpoint
        //         std::string message = make_daytime_string();
        //         boost::system::error_code err;
        //         socket.send_to(boost::asio::buffer(message), remoteendpoint, 0, err); // use the client endpoint to send back the daytime
        //     }
        // },
        // [&]{
        //     if (argc != 2)
        //     {
        //         std::print("need 2 args");
        //         return;
        //     }
        //     boost::asio::io_context io;
        //     boost::asio::ip::udp::resolver resolver{io}; // resolves string dns addresses -> ip and ports
        //     auto remoteendpoint = *resolver.resolve(boost::asio::ip::udp::v4(), argv[1], "daytime").begin(); // here, localhost and 13; so we connect to ourselves in the first lambda

        //     boost::asio::ip::udp::socket socket{io}; // our way of communicating to the server
        //     socket.open(boost::asio::ip::udp::v4()); // use ipv4

        //     std::array<int8_t, 1> send_buf  = {{ 0 }}; // send data
        //     socket.send_to(boost::asio::buffer(send_buf), remoteendpoint); // send it to the server

        //     std::array<int8_t, 128> buff;
        //     boost::asio::ip::udp::endpoint senderendpoint; // declare the endpoint
        //     size_t len = socket.receive_from(boost::asio::buffer(buff), senderendpoint); // blocking call - wait for server to send back the daytime (im kinda confused here - how is sendendpoint the server? its only declared here. is that actually the client?)
        //     std::cout.write(reinterpret_cast<const char*>(buff.data()), len); // print
        // },
    });

    return 0;
}
