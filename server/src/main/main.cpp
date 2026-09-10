#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include "boost/asio/buffer.hpp"
#include "boost/asio/ip/udp.hpp"
#include "boost/asio/write.hpp"
#include "boost/system/detail/error_code.hpp"
#include "core/threadpool.hpp"
#include <functional>
#include <memory>
#include <print>
#include <array>
#include <iostream>

class io_user_base {
public:
    boost::asio::io_context& io_;
    io_user_base(boost::asio::io_context& io_ref) : io_(io_ref) {}
};

std::string make_daytime_string()
{
  std::time_t now = std::time(0);
  return std::ctime(&now);
}

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::print("need 2 args");
        return 1;
    }
    boost::asio::io_context io;
    boost::asio::ip::udp::resolver resolver{io};
    auto recieverendpoint = *resolver.resolve(boost::asio::ip::udp::v4(), argv[1], "daytime").begin();

    boost::asio::ip::udp::socket socket{io};
    socket.open(boost::asio::ip::udp::v4());

    std::array<int8_t, 128> buff;
    boost::asio::ip::udp::endpoint senderendpoint;

    size_t len = socket.receive_from(boost::asio::buffer(buff), senderendpoint);
    std::cout.write(reinterpret_cast<const char*>(buff.data()), len);

    io.run();

    return 0;
}
