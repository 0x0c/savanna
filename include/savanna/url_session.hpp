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
		template <typename Stream, typename Body, typename Endpoint>
		http::response<Body> send_request(Stream &stream, savanna::url url, savanna::request<Endpoint> request)
		{
			std::string path = url.to_string();

			http::request<http::string_body> req(request.endpoint.method(), path, request.version);
			req.set(http::field::host, url.host());
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			for (auto h = request.header_fields.begin(); h != request.header_fields.end(); ++h) {
				req.set(h->first, h->second);
			}

			if (request.endpoint.method() != http::verb::get) {
				if (request.endpoint.params()) {
					auto p = *request.endpoint.params();
					req.body() = request.body + param_str(p);
					req.prepare_payload();
				}
				else {
					req.body() = request.body;
					req.prepare_payload();
				}
			}
			http::write(stream, req);

			beast::flat_buffer buffer;
			http::response<Body> response;
			http::read(stream, buffer, response);

			if ((response.result_int() >= 3) & 00 && response.result_int() < 400 && request.follow_location) {
				auto location = response.base()["Location"].to_string();
				auto new_url = savanna::url(location);
				response = send_request<Stream, Body>(stream, std::move(new_url), std::move(request));
			}

			return response;
		}

		template <typename Body, typename Endpoint>
		http::response<Body> send_request(savanna::request<Endpoint> request)
		{
			auto url = request.endpoint.url();
			auto const results = resolver_.resolve(url.host(), url.port_str());
			if (url.scheme() == url_scheme::https) {
				if (!SSL_set_tlsext_host_name(ssl_stream_.native_handle(), url.host().c_str())) {
					beast::error_code ec { static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
					if (ec != net::ssl::error::stream_truncated) {
						throw beast::system_error { ec };
					}
				}

				beast::get_lowest_layer(ssl_stream_).expires_after(request.timeout_interval);
				beast::get_lowest_layer(ssl_stream_).connect(results);

				// Perform the SSL handshake
				ssl_stream_.handshake(ssl::stream_base::client);

				auto response = send_request<beast::ssl_stream<beast::tcp_stream>, Body, Endpoint>(ssl_stream_, std::move(url), std::move(request));

				beast::error_code ec;
				ssl_stream_.shutdown(ec);

				if (ec == net::error::eof || ec == net::ssl::error::stream_truncated) {
					// Rationale:
					// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
					// https://github.com/boostorg/beast/blob/master/example/http/server/async-ssl/http_server_async_ssl.cpp#L216
					ec = {};
				}

				if (ec && ec != beast::errc::not_connected) {
					// not_connected happens sometimes
					// so don't bother reporting it.
					throw beast::system_error { ec };
				}

				return response;
			}
			else {
				tcp_stream_.connect(results);
				auto response = send_request<beast::tcp_stream, Body, Endpoint>(tcp_stream_, url, std::move(request));

				beast::error_code ec;
				tcp_stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

				if (ec && ec != beast::errc::not_connected) {
					// not_connected happens sometimes
					// so don't bother reporting it.
					throw beast::system_error { ec };
				}
				return response;
			}
		}

	private:
		tcp::resolver resolver_;
		beast::tcp_stream tcp_stream_;
		beast::ssl_stream<beast::tcp_stream> ssl_stream_;

	public:
		url_session(beast::tcp_stream tcp_stream, beast::ssl_stream<beast::tcp_stream> ssl_stream, tcp::resolver resolver)
		    : resolver_(std::move(resolver))
		    , tcp_stream_(std::move(tcp_stream))
		    , ssl_stream_(std::move(ssl_stream))
		{
		}

		template <typename Body, typename Endpoint>
		savanna::result<http::response<Body>> send(savanna::request<Endpoint> request)
		{
			static_assert(std::is_base_of<endpoint, Endpoint>::value, "Endpoint not derived from endpoint");
			try {
				http::response<Body> response = send_request<Body, Endpoint>(std::move(request));
				return result<http::response<Body>>(std::move(response));
			} catch (boost::system::system_error const &e) {
				return result<http::response<Body>>(e);
			}
		}
	};
}
}
