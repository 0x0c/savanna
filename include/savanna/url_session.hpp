#pragma once

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>

namespace m2d
{
namespace savanna
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	namespace ssl = net::ssl;
	using tcp = net::ip::tcp;

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

	class url_session
	{
	private:
		static ssl::context *shared_ssl_ctx()
		{
			static ssl::context ctx(ssl::context::tlsv12_client);
			return &ctx;
		}

		static tcp::resolver *shared_resolver()
		{
			static tcp::resolver resolver(*shared_ctx());
			return &resolver;
		}

		template <typename Stream, typename Body>
		static http::response<Body> send_request(Stream &stream, savanna::url url, http::verb method, boost::optional<std::map<std::string, std::string>> params, bool follow_location, int version)
		{
			std::string path = url.path();
			if (url.query().size() > 0) {
				path += "?" + url.query();
			}
			if (url.fragment().size() > 0) {
				path += "#" + url.fragment();
			}
			http::request<http::string_body> req(method, path, version);
			req.set(http::field::host, url.host());
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
			if (method != http::verb::get && params) {
				auto p = *params;
				req.body() = param_str(p);
				req.prepare_payload();
			}
			http::write(stream, req);

			beast::flat_buffer buffer;
			http::response<Body> response;
			http::read(stream, buffer, response);

			if (response.result_int() >= 300 && response.result_int() < 400 && follow_location) {
				auto location = response.base()["Location"].to_string();
				response = send_request<Body>(savanna::url(location), method, params, follow_location, version);
			}

			return response;
		}

		template <typename Body>
		static http::response<Body> send_request(savanna::url url, http::verb method, boost::optional<std::map<std::string, std::string>> params, bool follow_location, int version)
		{
			auto const results = shared_resolver()->resolve(url.host(), url.port_str());
			auto scheme = url.scheme() + "://";
			if (scheme == url_scheme::https) {
				auto stream = beast::ssl_stream<beast::tcp_stream>(*shared_ctx(), *shared_ssl_ctx());

				if (!SSL_set_tlsext_host_name(stream.native_handle(), url.host().c_str())) {
					beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
					throw beast::system_error{ ec };
				}

				beast::get_lowest_layer(stream).connect(results);

				// Perform the SSL handshake
				stream.handshake(ssl::stream_base::client);

				auto response = send_request<beast::ssl_stream<beast::tcp_stream>, Body>(stream, url, method, params, follow_location, version);

				beast::error_code ec;
				stream.shutdown(ec);

				if (ec == net::error::eof) {
					// Rationale:
					// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
					ec = {};
				}
				if (ec && ec != beast::errc::not_connected) {
					// not_connected happens sometimes
					// so don't bother reporting it.
					throw beast::system_error{ ec };
				}

				return response;
			}
			else {
				auto stream = beast::tcp_stream(*shared_ctx());
				stream.connect(results);
				auto response = send_request<beast::tcp_stream, Body>(stream, url, method, params, follow_location, version);

				beast::error_code ec;
				stream.socket().shutdown(tcp::socket::shutdown_both, ec);

				if (ec && ec != beast::errc::not_connected) {
					// not_connected happens sometimes
					// so don't bother reporting it.
					throw beast::system_error{ ec };
				}
				return response;
			}
		}

		template <typename Body, typename Endpoint>
		static http::response<Body> send_request(Endpoint endpoint, bool follow_location, int version)
		{
			static_assert(std::is_base_of<endpoint_t, Endpoint>::value, "Endpoint not derived from endpoint_t");
			http::response<Body> response = send_request<Body>(endpoint.url(), endpoint.method(), endpoint.params(), follow_location, version);
			return response;
		}

	public:
		static void load_root_cert(std::string cert)
		{
			auto ctx = shared_ssl_ctx();

			boost::system::error_code ec;
			ctx->add_certificate_authority(boost::asio::buffer(cert.data(), cert.size()), ec);
			if (ec) {
				throw beast::system_error{ ec };
			}

			ctx->set_verify_mode(ssl::verify_peer);
		}

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