#include <iostream>
#include <savanna.hpp>
#include <chrono>
#include <thread>

#include "../cert.hpp"

using namespace m2d;
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ssl = net::ssl;
using tcp = net::ip::tcp;
using std::this_thread::sleep_for;

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

class post_localhost_endpoint : public savanna::post_endpoint
{
public:
	post_localhost_endpoint(std::map<std::string, std::string> params)
	    : post_endpoint(params) {};

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
	std::map<std::string, std::string> params {
		{ "John", "1000" },
		{ "Tom", "1400" },
		{ "Harry", "800" }
	};
	auto endpoint = get_yahoo_endpoint(params);
	savanna::request<get_yahoo_endpoint> request(std::move(endpoint));
	//    auto endpoint = post_localhost_endpoint(params);
	//    savanna::request<post_localhost_endpoint> request(std::move(endpoint));
	request.body = "BODY";
	request.follow_location = true;

	// see also
	// https://www.boost.org/doc/libs/1_72_0/boost/beast/http/field.hpp
	request.header_fields = {
		{ "A", "a" },
		{ "B", "b" },
		{ "C", "c" },
		{ savanna::to_string(http::field::content_type), "text/plain" }
	};

	ssl::context ssl_ctx(ssl::context::tlsv12_client);
	std::once_flag once;
	std::call_once(once, savanna::load_root_cert, m2d::root_cert(), ssl_ctx);
	auto async_session = savanna::async_url_session(std::move(ssl_ctx));

	std::cout << "Start async request..." << std::endl;
	auto executor = async_session.prepare<http::dynamic_body>(request, [&](savanna::result<http::response<http::dynamic_body>> result) {
		if (result.error) {
			auto e = *(result.error);
			std::cout << "Error: " << e.what() << ", code: " << e.code() << std::endl;
			return;
		}

		auto response = *(result.response);
//		std::cout << "Got response: " << response << std::endl;
        std::cout << "Got response" << std::endl;
	});
    std::cout << "Wait..." << std::endl;
    std::thread t([&]{
        executor->send();
    });
    t.join();
	std::cout << "Done" << std::endl;

	return 0;
}
