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
	std::call_once(once, savanna::load_root_cert, m2d::root_cert(), ssl_ctx);

	auto ctx = std::make_shared<net::io_context>();

	// savanna::url url("ws://echo.websocket.org");
	// auto stream = savanna::websocket::raw_stream(ctx);
	// savanna::websocket::session<savanna::websocket::raw_stream> session(std::move(resolver), std::move(stream), url);

	savanna::url url("ws://localhost:80");
	auto stream = std::make_shared<savanna::async_websocket::raw_stream>(*ctx);
	savanna::async_websocket::async_session session;

	auto executor = session.prepare(ctx, stream, url);

	executor->on_message([](beast::flat_buffer buffer) {
		std::cout << "received: " << beast::make_printable(buffer.data()) << std::endl;
	});

	std::thread t([&]() {
		try {
			executor->run();
		} catch (boost::system::system_error  e) {
			std::cout << "Error: " << e.what() << std::endl;
		}
	});
	t.detach();

	int retry_count = 3;
	while (executor->current_state() != savanna::async_websocket::state::connected && retry_count > 0) {
		std::cout << "connecting..." << std::endl;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		retry_count--;
		if (retry_count == 0) {
			return -1;
		}
	}

	while (executor->current_state() == savanna::async_websocket::state::connected) {
		std::cout << "send: hello" << std::endl;
		executor->send("hello");
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return 0;
}
