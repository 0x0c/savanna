#pragma once

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
#include <functional>
#include <memory>

#include "endpoint.hpp"
#include "request.hpp"
#include "result.hpp"
#include "url.hpp"
#include "util.hpp"

namespace m2d
{
namespace savanna
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	namespace ssl = net::ssl;
	using tcp = net::ip::tcp;

	template <typename Body, typename Endpoint>
	class async_request_executor : public std::enable_shared_from_this<async_request_executor<Body, Endpoint>>
	{
	private:
        bool already_run_ = false;
		std::function<void(savanna::result<http::response<Body>>)> completion_;
		savanna::request<Endpoint> original_request_;
		http::request<http::string_body> request_;
		http::response<Body> response_;
		beast::flat_buffer buffer_;
		std::shared_ptr<net::io_context> ctx_;
		std::shared_ptr<tcp::resolver> resolver_;
		std::shared_ptr<beast::tcp_stream> tcp_stream_;
		std::shared_ptr<beast::ssl_stream<beast::tcp_stream>> ssl_stream_;
        
        std::string current_scheme() {
            auto target = request_.target();
            auto url = savanna::url(std::string(target));
            return url.scheme();
        }

		void on_resolve(beast::error_code ec, tcp::resolver::results_type results)
		{
			if (ec) {
				completion_(savanna::result<http::response<Body>>(beast::system_error { ec }));
				return;
			}
            
            if (current_scheme() == url_scheme::https) {
                auto host = original_request_.endpoint.host();
                char const *host_c_str = host.c_str();
                if (!SSL_set_tlsext_host_name(ssl_stream_->native_handle(), host_c_str)) {
                    beast::error_code ec { static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
                    completion_(savanna::result<http::response<Body>>(beast::system_error { ec }));
                    return;
                }
                // Set a timeout on the operation
                beast::get_lowest_layer(*ssl_stream_).expires_after(original_request_.timeout_interval);
                // Make the connection on the IP address we get from a lookup
                beast::get_lowest_layer(*ssl_stream_).async_connect(results, beast::bind_front_handler(&async_request_executor::on_connect, this->shared_from_this()));
            }
            else {
                tcp_stream_->expires_after(original_request_.timeout_interval);
                tcp_stream_->async_connect(results, beast::bind_front_handler(&async_request_executor::on_connect, this->shared_from_this()));
            }
		}

		void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
		{
			if (ec) {
				completion_(savanna::result<http::response<Body>>(beast::system_error { ec }));
				return;
			}

            if (current_scheme() == url_scheme::https) {
                // Perform the SSL handshake
                ssl_stream_->async_handshake(
                    ssl::stream_base::client,
                    beast::bind_front_handler(
                        &async_request_executor::on_handshake,
                        this->shared_from_this()));
            }
            else {
                tcp_stream_->expires_after(original_request_.timeout_interval);
                http::async_write(*tcp_stream_, request_,
                    beast::bind_front_handler(
                                              &async_request_executor::on_write,
                                              this->shared_from_this()));
            }
		}

		void on_handshake(beast::error_code ec)
		{
			if (ec) {
				completion_(savanna::result<http::response<Body>>(beast::system_error { ec }));
				return;
			}
            if (current_scheme() == url_scheme::https) {
                // Set a timeout on the operation
                beast::get_lowest_layer(*ssl_stream_).expires_after(original_request_.timeout_interval);
                // Send the HTTP request to the remote host
                http::async_write(*ssl_stream_, request_, beast::bind_front_handler(&async_request_executor::on_write, this->shared_from_this()));
            }
		}

		void on_write(beast::error_code ec, std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);

			if (ec) {
				completion_(savanna::result<http::response<Body>>(beast::system_error { ec }));
				return;
			}

            if (current_scheme() == url_scheme::https) {
                // Receive the HTTP response
                http::async_read(*ssl_stream_, buffer_, response_, beast::bind_front_handler(&async_request_executor::on_read, this->shared_from_this()));
            }
            else {
                http::async_read(*tcp_stream_, buffer_, response_, beast::bind_front_handler(&async_request_executor::on_read, this->shared_from_this()));
            }
		}

		void on_read(beast::error_code ec, std::size_t bytes_transferred)
		{
			boost::ignore_unused(bytes_transferred);

			if (ec) {
				completion_(savanna::result<http::response<Body>>(beast::system_error { ec }));
				return;
			}

			// Write the message to standard out
			//			std::cout << res_ << std::endl;
            if (current_scheme() == url_scheme::https) {
                // Set a timeout on the operation
                beast::get_lowest_layer(*ssl_stream_).expires_after(original_request_.timeout_interval);

                if ((response_.result_int() >= 300) && response_.result_int() < 400 && original_request_.follow_location) {
                    auto location = response_.base()["Location"].to_string();
                    auto new_url = savanna::url(location);
                    update_request(new_url.to_string(), new_url.host());
                    run(new_url.host(), new_url.port_str());
                    return;
                }
                // Gracefully close the stream
                ssl_stream_->async_shutdown(
                    beast::bind_front_handler(
                        &async_request_executor::on_shutdown,
                        this->shared_from_this()));
            }
            else {
                tcp_stream_->expires_after(original_request_.timeout_interval);
                if ((response_.result_int() >= 300) && response_.result_int() < 400 && original_request_.follow_location) {
                    auto location = response_.base()["Location"].to_string();
                    auto new_url = savanna::url(location);
                    update_request(new_url.to_string(), new_url.host());
                    run(new_url.host(), new_url.port_str());
                    return;
                }
                // Gracefully close the stream
                tcp_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
                if(ec && ec != beast::errc::not_connected) {
                    completion_(savanna::result<http::response<Body>>(beast::system_error { ec }));
                }
            }
		}

		void on_shutdown(beast::error_code ec)
		{
			if (ec == net::error::eof) {
				// Rationale:
				// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
				ec = {};
			}
			if (ec) {
				completion_(savanna::result<http::response<Body>>(beast::system_error { ec }));
				return;
			}

			// If we get here then the connection is closed gracefully
			completion_(savanna::result<http::response<Body>>(std::move(response_)));
		}
        
        void update_request(std::string new_target, std::string new_host) {
            request_.target(new_target);
            request_.set(http::field::host, new_host);
            for (auto h = original_request_.header_fields.begin(); h != original_request_.header_fields.end(); ++h) {
                request_.set(h->first, h->second);
            }
            if (original_request_.endpoint.method() != http::verb::get) {
                if (original_request_.endpoint.params()) {
                    auto p = *original_request_.endpoint.params();
                    request_.body() = original_request_.body + savanna::param_str(p);
                    request_.prepare_payload();
                }
                else {
                    request_.body() = original_request_.body;
                    request_.prepare_payload();
                }
            }
        }
        
        void run(std::string host, std::string port_str) {
            resolver_->async_resolve(host, port_str, beast::bind_front_handler(&async_request_executor::on_resolve, this->shared_from_this()));
        }

	public:
		explicit async_request_executor(savanna::request<Endpoint> request, ssl::context &ssl_ctx, std::function<void(savanna::result<http::response<Body>>)> completion)
		    : completion_(completion)
		    , original_request_(request)
		{
			ctx_ = std::make_shared<net::io_context>();
			resolver_ = std::make_shared<tcp::resolver>(*ctx_),
			tcp_stream_ = std::make_shared<beast::tcp_stream>(*ctx_),
			ssl_stream_ = std::make_shared<beast::ssl_stream<beast::tcp_stream>>(*ctx_, ssl_ctx);
            
            request_.version(original_request_.version);
            request_.method(original_request_.endpoint.method());
            request_.target(original_request_.endpoint.build_url_str());
            request_.set(http::field::host, original_request_.endpoint.host());
            request_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

            for (auto h = original_request_.header_fields.begin(); h != original_request_.header_fields.end(); ++h) {
                request_.set(h->first, h->second);
            }

            if (original_request_.endpoint.method() != http::verb::get) {
                if (original_request_.endpoint.params()) {
                    auto p = *original_request_.endpoint.params();
                    request_.body() = original_request_.body + savanna::param_str(p);
                    request_.prepare_payload();
                }
                else {
                    request_.body() = original_request_.body;
                    request_.prepare_payload();
                }
            }
		}

		void run()
		{
            if (already_run_ == false) {
                auto url = original_request_.endpoint.url();
                run(url.host(), url.port_str());
                ctx_->run();
            }
		}
	};

	class async_url_session
	{
	private:
		ssl::context ssl_ctx_;

	public:
		explicit async_url_session(ssl::context ssl_ctx)
		    : ssl_ctx_(std::move(ssl_ctx))
		{
		}

		template <typename Body, typename Endpoint>
		std::shared_ptr<savanna::async_request_executor<Body, Endpoint>> send(savanna::request<Endpoint> request, std::function<void(savanna::result<http::response<Body>>)> completion)
		{
			static_assert(std::is_base_of<savanna::endpoint, Endpoint>::value, "Endpoint not derived from endpoint");
			 auto executor = std::make_shared<savanna::async_request_executor<Body, Endpoint>>(std::move(request), ssl_ctx_, std::move(completion));
			 executor->run();
			 return executor;
		}
	};
}
}
