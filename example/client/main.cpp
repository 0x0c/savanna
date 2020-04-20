#include <iostream>
#include <savanna.hpp>

#include "../cert.hpp"

using namespace m2d;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;

class get_yahoo_endpoint : public savanna::get_endpoint
{
public:
	get_yahoo_endpoint(std::map<std::string, std::string> params)
	    : get_endpoint(params) {};

	virtual std::string scheme()
	{
		return savanna::url_scheme::http;
	}

	virtual std::string host()
	{
		return "yahoo.co.jp";
	}

	virtual std::string path()
	{
		return "/";
	}
};

int main(int argc, char *argv[])
{
	std::map<std::string, std::string> params {
		{ "John", "1000" },
		{ "Tom", "1400" },
		{ "Harry", "800" }
	};
	auto endpoint = get_yahoo_endpoint(params);
	savanna::request<get_yahoo_endpoint> request(std::move(endpoint));
	request.body = "BODY";
	request.follow_location = true;
	
	// see existing http header fields
	// https://www.boost.org/doc/libs/1_72_0/boost/beast/http/field.hpp
	request.header_fields = {
		{ "A", "a" },
		{ "B", "b" },
		{ "C", "c" }
	};

	ssl::context ssl_ctx(ssl::context::tlsv12_client);
	std::once_flag once;
	std::call_once(once, savanna::load_root_cert, m2d::root_cert, ssl_ctx);

	net::io_context ctx;
	tcp::resolver resolver(ctx);
	beast::tcp_stream tcp_stream(ctx);
	beast::ssl_stream<beast::tcp_stream> ssl_stream(ctx, ssl_ctx);
	auto session = savanna::url_session(std::move(tcp_stream), std::move(ssl_stream), std::move(resolver));

	std::istream::char_type ch;
	while ((ch = std::cin.get()) != 'q') {
		auto result = session.send<http::dynamic_body>(request);
		if (result.error) {
			auto e = *(result.error);
			std::cout << "Error: " << e.what() << ", code: " << e.code() << std::endl;
			return -1;
		}

		auto response = *(result.response);
		// std::cout << "Got response: " << response << std::endl;
		std::cout << "Got response" << std::endl;
	}

	return 0;
}
