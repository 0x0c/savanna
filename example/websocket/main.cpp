#include <chrono>
#include <iostream>
#include <savanna.hpp>

#include "cert.hpp"

using namespace m2d;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

int main(int argc, char *argv[])
{
	std::once_flag once;
	std::call_once(once, savanna::load_root_cert, m2d::root_cert, *savanna::shared_ssl_ctx());

//    savanna::url url("ws://echo.websocket.org");
    savanna::url url("wss://echo.websocket.org");
    savanna::websocket::session session(url);
    session.on_message([](beast::flat_buffer buffer) {
        std::cout << "received: " << beast::make_printable(buffer.data()) << std::endl;
    });
	try {
		session.connect();
		std::cout << "connected" << std::endl;

		while (true) {
			std::cout << "send: hello" << std::endl;
			session.send("hello");
			std::this_thread::sleep_for(std::chrono::seconds(1));
		}
	} catch (boost::system::system_error const &e) {
		std::cout << "Error: " << e.what() << std::endl;
	}

	return 0;
}
