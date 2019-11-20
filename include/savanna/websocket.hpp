#pragma once

#include <boost/any.hpp>
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

		class session
		{
		private:
			beast::flat_buffer buffer_;
			savanna::url url_;
			std::string scheme_;
			boost::optional<std::function<void(beast::flat_buffer &)>> on_message_handler_;
			websocket::tls tls_stream_;
			websocket::raw raw_stream_;
			// boost::asio::io_context::work work_;

			void wait(websocket::raw &stream)
			{
				stream.async_read(buffer_, beast::bind_front_handler(&session::on_read, this));
			}

			void wait(websocket::tls &stream)
			{
				stream.async_read(buffer_, beast::bind_front_handler(&session::on_read, this));
			}

			void make_connection(websocket::raw &stream, tcp::resolver::results_type const &results)
			{
				net::connect(stream.next_layer(), results.begin(), results.end());
				stream.set_option(beast::websocket::stream_base::decorator([](beast::websocket::request_type &req) {
					req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " savanna");
				}));

				stream.handshake(url_.host(), "/");
			}

			void make_connection(websocket::tls &stream, tcp::resolver::results_type const &results)
			{
				net::connect(stream.next_layer().next_layer(), results.begin(), results.end());
				stream.set_option(beast::websocket::stream_base::decorator([](beast::websocket::request_type &req) {
					req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " savanna");
				}));

				stream.handshake(url_.host(), "/");
			}

			void on_read(beast::error_code ec, std::size_t bytes_transferred)
			{
				std::cout << "on_read" << std::endl;
				boost::ignore_unused(bytes_transferred);

				if (ec) {
					throw beast::system_error{ ec };
				}

				if (on_message_handler_) {
					auto handler = *on_message_handler_;
					handler(buffer_);
				}

				// if (scheme_ == url_scheme::wss) {
				// 	wait(tls_stream_);
				// }
				// else {
				// 	wait(raw_stream_);
				// }
			}

			void on_write(beast::error_code ec, std::size_t bytes_transferred)
			{
				boost::ignore_unused(bytes_transferred);
				std::cout << "on_write" << std::endl;
				if (ec) {
					throw beast::system_error{ ec };
				}
			}

		public:
			session(savanna::url url)
			    : url_(url)
			    , scheme_(url_.scheme() + "://")
			    , tls_stream_(*shared_ws_ctx(), *shared_ssl_ctx())
			    , raw_stream_(*shared_ws_ctx())
			// , work_(*shared_ws_ctx())
			{
			}

			~session()
			{
				close();
			}

			void connect()
			{
				auto resolver = shared_ws_resolver();
				auto const results = resolver->resolve(url_.host(), url_.port_str());
				if (scheme_ == url_scheme::wss) {
					make_connection(tls_stream_, results);
				}
				else {
					make_connection(raw_stream_, results);
				}

				std::thread t([&]() {
					while (true) {
						beast::flat_buffer buffer;
						if (scheme_ == url_scheme::wss) {
							tls_stream_.read(buffer);
						}
						else {
							// wait(raw_stream_);
							raw_stream_.read(buffer);
						}
						if (on_message_handler_) {
							auto handler = *on_message_handler_;
							handler(buffer);
						}
					}
				});
				t.detach();
			}

			void send(std::string data)
			{
				if (scheme_ == url_scheme::wss) {
					tls_stream_.write(net::buffer(data));
				}
				else {
					// raw_stream_.async_write(net::buffer(data), beast::bind_front_handler(&session::on_write, this));
					raw_stream_.write(net::buffer(data));
				}
				// (*shared_ws_ctx()).run();
			}

			void close()
			{
				if (scheme_ == url_scheme::wss) {
					tls_stream_.close(beast::websocket::close_code::normal);
				}
				else {
					raw_stream_.close(beast::websocket::close_code::normal);
				}
			}

			void on_message(std::function<void(beast::flat_buffer &)> handler)
			{
				on_message_handler_ = handler;
			}
		};
	}
}
}
