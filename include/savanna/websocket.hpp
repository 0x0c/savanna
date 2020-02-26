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
#include <boost/utility/string_view.hpp>

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
		using tls = beast::websocket::stream<beast::ssl_stream<tcp::socket>>;
		using raw = beast::websocket::stream<tcp::socket>;

		class session
		{
		public:
			enum state
			{
				unknown,
				closed,
				connected
			};

		private:
			beast::flat_buffer buffer_;
			savanna::url url_;
			std::string scheme_;
			boost::optional<std::function<void(beast::flat_buffer &)>> on_message_handler_;
			websocket::tls tls_stream_;
			websocket::raw raw_stream_;
			state current_state_ = unknown;

			void wait(websocket::raw &stream)
			{
				stream.async_read(buffer_, beast::bind_front_handler(&session::on_read, this));
			}

			void wait(websocket::tls &stream)
			{
				stream.async_read(buffer_, beast::bind_front_handler(&session::on_read, this));
			}

			void make_connection(websocket::raw &stream, tcp::resolver::results_type const &results, std::string path)
			{
				net::connect(stream.next_layer(), results.begin(), results.end());
				stream.set_option(beast::websocket::stream_base::decorator([](beast::websocket::request_type &req) {
					req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " savanna");
				}));

				stream.handshake(url_.host(), path);
			}

			void make_connection(websocket::tls &stream, tcp::resolver::results_type const &results, std::string path)
			{
				net::connect(stream.next_layer().next_layer(), results.begin(), results.end());
				stream.set_option(beast::websocket::stream_base::decorator([](beast::websocket::request_type &req) {
					req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " savanna");
				}));

				stream.next_layer().handshake(ssl::stream_base::client);
				stream.handshake(url_.host(), path);
			}

			void on_read(beast::error_code ec, std::size_t bytes_transferred)
			{
				boost::ignore_unused(bytes_transferred);

				if (ec) {
					throw beast::system_error { ec };
				}

				if (on_message_handler_) {
					auto handler = *on_message_handler_;
					handler(buffer_);
				}
			}

			void on_write(beast::error_code ec, std::size_t bytes_transferred)
			{
				boost::ignore_unused(bytes_transferred);
				if (ec) {
					throw beast::system_error { ec };
				}
			}

			void set_current_state(state s)
			{
				current_state_ = s;
				state_changed(s);
			}

		public:
			std::function<void(state)> state_changed = [](state s) {};
			session(savanna::url url)
			    : url_(url)
			    , scheme_(url_.scheme())
			    , tls_stream_(*shared_ws_ctx(), *shared_ssl_ctx())
			    , raw_stream_(*shared_ws_ctx())
			{
			}

			~session()
			{
				close();
			}

			state current_state()
			{
				return current_state_;
			}

			void run(std::string path = "/")
			{
				try {
					auto control_callback = [this](beast::websocket::frame_type kind, boost::string_view payload) {
						if (kind == beast::websocket::frame_type::close) {
							this->set_current_state(closed);
						}
						boost::ignore_unused(kind, payload);
					};

					auto resolver = shared_ws_resolver();
					auto const results = resolver->resolve(url_.host(), url_.port_str());
					if (scheme_ == url_scheme::wss) {
						make_connection(tls_stream_, results, path);
						tls_stream_.control_callback(control_callback);
					}
					else {
						make_connection(raw_stream_, results, path);
						raw_stream_.control_callback(control_callback);
					}
					set_current_state(connected);

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
				} catch (boost::wrapexcept<boost::system::system_error> e) {
					set_current_state(unknown);
					throw e;
				}
			}

			boost::system::error_code send(std::string data)
			{
				boost::system::error_code ec;
				if (scheme_ == url_scheme::wss) {
					tls_stream_.write(net::buffer(data), ec);
				}
				else {
					raw_stream_.write(net::buffer(data), ec);
				}
				return ec;
			}

			boost::system::error_code close()
			{
				boost::system::error_code ec;
				if (scheme_ == url_scheme::wss) {
					if (tls_stream_.is_open()) {
						tls_stream_.close(beast::websocket::close_code::normal, ec);
					}
				}
				else {
					if (raw_stream_.is_open()) {
						raw_stream_.close(beast::websocket::close_code::normal, ec);
					}
				}

				return ec;
			}

			void on_message(std::function<void(beast::flat_buffer &)> handler)
			{
				on_message_handler_ = handler;
			}
		};
	}
}
}
