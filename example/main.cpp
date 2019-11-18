#include <iostream>
#include <savanna.hpp>

#include "cert.hpp"

using namespace m2d;
namespace beast = boost::beast;
namespace http = beast::http;

class YahooEndpoint : public savanna::get_endpoint_t
{
public:
	YahooEndpoint()
	    : get_endpoint_t(boost::none) {};

	virtual std::string scheme()
	{
		return savanna::url_scheme::http;
	}

	virtual std::string host()
	{
		return "www.yahoo.co.jp";
	}

	virtual std::string path()
	{
		return "/";
	}
};

int main(int argc, char *argv[])
{
	std::once_flag once;
	std::call_once(once, savanna::url_session::load_root_cert, m2d::root_cert);

	savanna::request_t<YahooEndpoint>
	    request;
	request.follow_location = true;
	auto result = savanna::url_session::send<http::dynamic_body>(request);
	if (result.error) {
		auto e = *(result.error);
		std::cout << "Error: " << e.what() << std::endl;
		return -1;
	}

	auto response = *(result.response);
	std::cout << "Response: " << response << std::endl;

	return 0;
}
