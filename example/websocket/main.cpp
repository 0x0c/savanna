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

	savanna::url url("ws://localhost:5001");
	savanna::websocket::session session(url);
	session.on_message([](beast::flat_buffer buffer) {
		std::cout << beast::make_printable(buffer.data()) << std::endl;
	});
	session.connect();

	while (true) {
		std::cout << "send" << std::endl;
		session.send("hello");
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return 0;
}
