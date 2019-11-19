#pragma once

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/http.hpp>

namespace m2d
{
namespace savanna
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	namespace ssl = boost::asio::ssl;
	using tcp = boost::asio::ip::tcp;

	namespace url_scheme
	{
		static const std::string https = "https://";
		static const std::string http = "http://";
		static const std::string wss = "wss://";
		static const std::string ws = "ws://";
	}

	static ssl::context *shared_ssl_ctx()
	{
		static ssl::context ctx(ssl::context::tlsv12_client);
		return &ctx;
	}

	static inline net::io_context *shared_ctx()
	{
		static net::io_context ioc;
		return &ioc;
	};

	static inline tcp::resolver *shared_resolver()
	{
		static tcp::resolver resolver(*shared_ctx());
		return &resolver;
	}

	static inline net::io_context *shared_ws_ctx()
	{
		static net::io_context ioc;
		return &ioc;
	};

	static inline tcp::resolver *shared_ws_resolver()
	{
		static tcp::resolver resolver(*shared_ws_ctx());
		return &resolver;
	}
}
}
