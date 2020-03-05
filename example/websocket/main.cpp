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

	// savanna::url url("ws://echo.websocket.org");
	// auto raw_stream = savanna::websocket::raw(*savanna::shared_ws_ctx());
	// savanna::websocket::session<savanna::websocket::raw> session(std::move(stream), url);

	savanna::url url("wss://echo.websocket.org");
	auto stream = savanna::websocket::tls(*savanna::shared_ws_ctx(), *savanna::shared_ssl_ctx());
	savanna::websocket::session<savanna::websocket::tls> session(std::move(stream), url);

	session.on_message([](beast::flat_buffer buffer) {
		std::cout << "received: " << beast::make_printable(buffer.data()) << std::endl;
	});

	std::thread t([&]() {
		try {
			session.run();
		} catch (boost::wrapexcept<boost::system::system_error> e) {
			std::cout << "Error: " << e.what() << std::endl;
		}
	});
	t.detach();

	int retry_count = 3;
	while (session.current_state() != savanna::websocket::state::connected && retry_count > 0) {
		std::cout << "connecting..." << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		retry_count--;
		if (retry_count == 0) {
			return -1;
		}
	}

	while (true) {
		std::cout << "send: hello" << std::endl;
		auto ec = session.send("hello");
		if (ec) {
			std::cout << "error code: " << ec.value() << std::endl;
		}
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return 0;
}
