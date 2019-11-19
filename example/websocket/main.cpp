#include <iostream>
#include <savanna.hpp>

#include "cert.hpp"

using namespace m2d;
namespace beast = boost::beast;
namespace http = beast::http;

class get_localhost_endpoint_t : public savanna::get_endpoint_t
{
public:
	get_localhost_endpoint_t(std::map<std::string, std::string> params)
	    : get_endpoint_t(params){};

	virtual std::string scheme()
	{
		return savanna::url_scheme::http;
	}

	virtual std::string host()
	{
		return "localhost";
	}

	virtual std::string path()
	{
		return "/";
	}

	virtual int port()
	{
		return 8080;
	}
};

class post_localhost_endpoint_t : public savanna::post_endpoint_t
{
public:
	post_localhost_endpoint_t(std::map<std::string, std::string> params)
	    : post_endpoint_t(params){};

	virtual std::string scheme()
	{
		return savanna::url_scheme::http;
	}

	virtual std::string host()
	{
		return "localhost";
	}

	virtual std::string path()
	{
		return "/";
	}

	virtual int port()
	{
		return 8080;
	}
};

int main(int argc, char *argv[])
{
	std::once_flag once;
	std::call_once(once, savanna::load_root_cert, m2d::root_cert, *savanna::shared_ssl_ctx());

	std::map<std::string, std::string> params{
		{ "John", "1000" },
		{ "Tom", "1400" },
		{ "Harry", "800" }
	};
	auto endpoint = post_localhost_endpoint_t(params);
	savanna::request_t<post_localhost_endpoint_t> request(endpoint);
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
