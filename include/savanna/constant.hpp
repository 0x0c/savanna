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
		static const std::string https = "https";
		static const std::string http = "http";
		static const std::string wss = "wss";
		static const std::string ws = "ws";
	}
}
}
