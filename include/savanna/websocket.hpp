// #pragma once

// #include <boost/asio/connect.hpp>
// #include <boost/asio/ip/tcp.hpp>
// #include <boost/asio/ssl/stream.hpp>
// #include <boost/beast/core.hpp>
// #include <boost/beast/ssl.hpp>
// #include <boost/beast/websocket.hpp>
// #include <boost/beast/websocket/ssl.hpp>
// #include <boost/optional.hpp>

// #include <functional>

// #include "url.hpp"

// namespace m2d
// {
// namespace savanna
// {
// 	namespace beast = boost::beast;
// 	namespace http = beast::http;
// 	namespace websocket = beast::websocket;
// 	namespace net = boost::asio;
// 	namespace ssl = boost::asio::ssl;
// 	using tcp = boost::asio::ip::tcp;

// 	namespace websocket
// 	{
// 		using tls = websocket::stream<beast::ssl_stream<tcp::socket>>;
// 		using raw = websocket::stream<tcp::socket>;

// 		template <typename T>
// 		class session : public std::enable_shared_from_this<session>
// 		{
// 		private:
// 			boost::optional<connection<T>> connection;
// 			savanna::url url_;
// 			beast::flat_buffer buffer_;
// 			boost::optional<std::function<void(beast::flat_buffer)>> on_message;

// 			void on_read(beast::error_code ec, std::size_t bytes_transferred)
// 			{
// 				boost::ignore_unused(bytes_transferred);

// 				if (ec)
// 					return fail(ec, "read");

// 				if (on_message) {
// 					auto handler = *(on_message);
// 					handler(buffer_);
// 				}

// 				stream_.async_read(buffer_, beast::bind_front_handler(&connection::on_read, shared_from_this()));
// 			}

// 		public:
// 			session(savanna::url url)
// 			    : url_(url)
// 			{
// 			}

// 			void connect()
// 			{
// 				// Look up the domain name
// 				auto resolver = shared_ws_resolver();
// 				auto const results = resolver.resolve(url_.host(), url_.port_str());

// 				net::connect(stream_.next_layer(), results.begin(), results.end());

// 				stream_.set_option(websocket::stream_base::decorator(
// 				    [](websocket::request_type &req) {
// 					    req.set(http::field::user_agent,
// 					        std::string(BOOST_BEAST_VERSION_STRING) + " savanna");
// 				    }));

// 				stream_.handshake(host, url_.path());

// 				stream_.async_read(buffer_, beast::bind_front_handler(&session::on_read, shared_from_this()));

// 				// connection = connection<T>();
// 				// connection.on_message = on_message;
// 				// connection.run();

// 				// stream.close(websocket::close_code::normal);
// 			}

// 			void close()
// 			{
// 				stream.close(websocket::close_code::normal);
// 			}

// 			savanna::websocket::session &on_message(std::function<void(beast::flat_buffer)> handler)
// 			{
// 				on_message = handler;
// 				return &this;
// 			}
// 		};
// 	}
// }
// }