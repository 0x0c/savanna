#pragma once

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/optional.hpp>
#include <exception>
#include <regex>

#include "savanna/endpoint.hpp"
#include "savanna/url.hpp"

namespace m2d
{
namespace savanna
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	using tcp = net::ip::tcp;

	static inline net::io_context *shared_ctx()
	{
		static net::io_context ioc;
		return &ioc;
	};

	template <typename Endpoint>
	class request_t
	{
	public:
		bool follow_location = false;
		Endpoint endpoint;
		unsigned int version = 11;

		request_t()
		    : endpoint(Endpoint())
		{
			static_assert(std::is_base_of<endpoint_t, Endpoint>::value, "Endpoint not derived from endpoint_t");
		}

		request_t &set_http_version(unsigned int version)
		{
			this->version = version;
			return &this;
		}
	};

	template <typename Response>
	class result_t
	{
	public:
		boost::optional<Response> response;
		boost::optional<std::exception> error;

		result_t(Response response)
		    : response(response)
		    , error(boost::none)
		{
		}

		result_t(std::exception error)
		    : response(boost::none)
		    , error(error)
		{
		}
	};

	class url_session
	{
	private:
		template <typename Body>
		static http::response<Body> send_request(savanna::url url, http::verb method, boost::optional<std::map<std::string, std::string>> params, bool follow_location, int version)
		{
			tcp::resolver resolver(*shared_ctx());
			beast::tcp_stream stream(*shared_ctx());
			auto const results = resolver.resolve(url.host(), url.port_str());

			stream.connect(results);

			http::request<http::string_body> req(method, url.path(), version);
			req.set(http::field::host, url.host());
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			// Send the HTTP request to the remote host
			http::write(stream, req);

			// This buffer is used for reading and must be persisted
			beast::flat_buffer buffer;

			// Declare a container to hold the response
			http::response<Body> response;

			// Receive the HTTP response
			http::read(stream, buffer, response);

			if (response.result_int() >= 300 && response.result_int() < 400 && follow_location) {
				auto location = response.base()["Location"].to_string();
				auto new_url = savanna::url(location);
				response = send_request<Body>(new_url, method, params, follow_location, version);
			}

			// Gracefully close the socket
			beast::error_code ec;
			stream.socket().shutdown(tcp::socket::shutdown_both, ec);

			// not_connected happens sometimes
			// so don't bother reporting it.
			if (ec && ec != beast::errc::not_connected) {
				throw beast::system_error { ec };
			}
			return response;
		}

		template <typename Body, typename Endpoint>
		static http::response<Body> send_request(Endpoint endpoint, bool follow_location, int version)
		{
			static_assert(std::is_base_of<endpoint_t, Endpoint>::value, "Endpoint not derived from endpoint_t");
			http::response<Body> response = send_request<Body>(endpoint.url(), endpoint.method(), endpoint.params(), follow_location, version);
			return response;
		}

	public:
		url_session() = default;
		template <typename Body, typename Endpoint>
		static savanna::result_t<http::response<Body>> send(savanna::request_t<Endpoint> request)
		{
			static_assert(std::is_base_of<endpoint_t, Endpoint>::value, "Endpoint not derived from endpoint_t");
			try {
				http::response<Body> response = send_request<Body, Endpoint>(request.endpoint, request.follow_location, request.version);
				return result_t<http::response<Body>>(response);
			} catch (std::exception const &e) {
				return result_t<http::response<Body>>(e);
			}
		}
	};
}
}