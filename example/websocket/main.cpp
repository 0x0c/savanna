#include <chrono>
#include <iostream>
#include <savanna.hpp>

#include "../cert.hpp"

using namespace m2d;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

int main(int argc, char *argv[])
{
	ssl::context ssl_ctx(ssl::context::tlsv12_client);
	std::once_flag once;
	std::call_once(once, savanna::load_root_cert, m2d::root_cert, ssl_ctx);

	net::io_context ctx;
	tcp::resolver resolver(ctx);
	
  	// savanna::url url("ws://echo.websocket.org");
	// auto stream = savanna::websocket::raw_stream(ctx);
	// savanna::websocket::session<savanna::websocket::raw_stream> session(std::move(resolver), std::move(stream), url);

	savanna::url url("wss://echo.websocket.org");
	auto stream = savanna::websocket::tls_stream(ctx, ssl_ctx);
	savanna::websocket::session<savanna::websocket::tls_stream> session(std::move(resolver), std::move(stream), url);

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
