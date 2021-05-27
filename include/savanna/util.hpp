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

	static inline std::string param_str(std::map<std::string, std::string> &params)
	{
		std::string param_str;
		for (auto p = params.begin(); p != params.end(); ++p) {
			param_str += p->first;
			param_str += "=";
			param_str += p->second;
			param_str += "&";
		}
		param_str.pop_back();
		return param_str;
	}
}
}
