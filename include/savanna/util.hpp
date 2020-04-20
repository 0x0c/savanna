#pragma once

#include <boost/beast/ssl.hpp>

namespace m2d
{
namespace savanna
{
	namespace ssl = boost::asio::ssl;

	static void load_root_cert(std::string const &cert, ssl::context &ctx)
	{
		boost::system::error_code ec;
		ctx.add_certificate_authority(boost::asio::buffer(cert.data(), cert.size()), ec);
		if (ec) {
			throw beast::system_error { ec };
		}

		ctx.set_verify_mode(ssl::verify_peer);
	}
}
}