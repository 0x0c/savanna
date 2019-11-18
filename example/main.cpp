#include <iostream>
#include <savanna.hpp>

using namespace m2d;
namespace beast = boost::beast;
namespace http = beast::http;

class YahooEndpoint : public savanna::get_endpoint_t
{
public:
	YahooEndpoint()
	    : get_endpoint_t(boost::none) {};

	virtual std::string host()
	{
		return "www.yahoo.com";
	}

	virtual http::verb method()
	{
		return http::verb::get;
	}

	virtual std::string path()
	{
		return "/";
	}
};

int main(int argc, char *argv[])
{
	savanna::request_t<YahooEndpoint> request;
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
