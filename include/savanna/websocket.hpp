#pragma once

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/optional.hpp>

#include <functional>
#include <memory>
#include <thread>

#include "url.hpp"

namespace m2d
{
namespace savanna
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	namespace ssl = boost::asio::ssl;
	using tcp = boost::asio::ip::tcp;

	namespace websocket
	{
		void fail(beast::error_code ec, char const *what)
		{
			std::cerr << what << ": " << ec.message() << "\n";
		}

		using tls = beast::websocket::stream<beast::ssl_stream<tcp::socket>>;
		using raw = beast::websocket::stream<tcp::socket>;

		// class stream
		// {
		// };

		// template <typename T>
		class session
		{
		private:
			beast::flat_buffer buffer_;
			savanna::url url_;
			boost::optional<std::function<void(beast::flat_buffer &)>> on_message_handler;
			savanna::websocket::raw *stream_ = nullptr;

			void wait()
			{
				if (stream_) {
					beast::flat_buffer buffer;
					stream_->read(buffer);
					if (on_message_handler) {
						auto handler = *(on_message_handler);
						handler(buffer);
					}
				}
			}

			void
			on_read(
			    beast::error_code ec,
			    std::size_t bytes_transferred)
			{
				boost::ignore_unused(bytes_transferred);

				if (ec)
					return fail(ec, "read");

				std::cout << beast::make_printable(buffer_.data()) << std::endl;

				// Close the WebSocket connection
				stream_->async_read(
				    buffer_,
				    beast::bind_front_handler(
				        &session::on_read,
				        this));
			}

		public:
			session(savanna::url url)
			    : url_(url)
			{
			}

			~session()
			{
				close();
				delete stream_;
			}

			void run()
			{
				auto resolver = shared_ws_resolver();
				auto const results = resolver->resolve(url_.host(), url_.port_str());

				// auto scheme = url_.scheme() + "://";
				// if (scheme == url_scheme::wss) {
				// use tls
				// stream_ = new websocket::tls{ *shared_ws_ctx(), *shared_ssl_ctx() };
				// }
				// else {
				stream_ = new websocket::raw{ *shared_ws_ctx() };
				// }

				net::connect(stream_->next_layer(), results.begin(), results.end());

				stream_->set_option(beast::websocket::stream_base::decorator(
				    [](beast::websocket::request_type &req) {
					    req.set(http::field::user_agent,
					        std::string(BOOST_BEAST_VERSION_STRING) + " savanna");
				    }));

				stream_->handshake(url_.host(), "/");
				stream_->async_read(
				    buffer_,
				    beast::bind_front_handler(
				        &session::on_read,
				        this));
				(*shared_ws_ctx()).run();
			}

			void close()
			{
				stream_->close(beast::websocket::close_code::normal);
			}

			void on_message(std::function<void(beast::flat_buffer &)> handler)
			{
				on_message_handler = handler;
			}
		};
	}
}
}
