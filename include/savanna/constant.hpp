#pragma once

#include <boost/beast/http.hpp>

namespace m2d
{
namespace savanna
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;

	static inline net::io_context *shared_ctx()
	{
		static net::io_context ioc;
		return &ioc;
	};
}
}