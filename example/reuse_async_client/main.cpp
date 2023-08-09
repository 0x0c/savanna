#include <chrono>
#include <iostream>
#include <savanna.hpp>
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

int main(int argc, char *argv[])
{
	std::map<std::string, std::string> params {
		{ "John", "1000" },
		{ "Tom", "1400" },
		{ "Harry", "800" }
	};
	auto endpoint = std::make_shared<get_yahoo_endpoint>(params);
	savanna::ssl_reuse::request request(endpoint);
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

	auto async_session = std::make_shared<savanna::ssl_reuse::reuse_async_url_session>();
	std::cout << "Wait..." << std::endl;

	std::vector<std::thread> th;
	for(int i=0; i<3; i++){
		th.emplace_back([&] {
			ssl::context ssl_ctx(ssl::context::tlsv12_client);
			std::once_flag once;
			std::call_once(once, savanna::load_root_cert, m2d::root_cert(), ssl_ctx);
			auto executor = async_session->prepare<http::dynamic_body>(std::move(ssl_ctx));
			executor->send(request, [&](savanna::result<http::response<http::dynamic_body>> result) {
				if (result.error) {
					auto e = *(result.error);
					std::cout << "Error: " << e.what() << ", code: " << e.code() << std::endl;
					return;
				}

				auto response = *(result.response);
				//		std::cout << "Got response: " << response << std::endl;
				std::cout << "Got response" << std::endl;
			});
			std::cout << "Done" << std::endl << std::endl;
		});
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
	for(auto &t : th){
		t.join();
	}
	std::cout << "Finish" << std::endl;
	for(const auto &item : async_session->ssl_cache()){
		std::cout << "host: " << item.first << ", session_ptr: " << item.second << std::endl;
	}
	return 0;
}
