#pragma once

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/version.hpp>
//#include <boost/bind.hpp>
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

	template <class Body>
	class delegate{
	public:
		virtual bool completion_callback(savanna::result<http::response<Body>>) = 0;
	};
	class interface: public std::enable_shared_from_this<interface>{
	protected:
		virtual void on_resolve_l(beast::error_code ec, tcp::resolver::results_type) = 0;
		virtual void on_connect_l(beast::error_code ec, tcp::resolver::results_type::endpoint_type) = 0;
		virtual void on_handshake_l(beast::error_code ec) = 0;
		virtual void on_write_l(beast::error_code ec, std::size_t bytes_transferred) = 0;
		virtual void on_read_l(beast::error_code ec, std::size_t bytes_transferred) = 0;
		virtual void on_shutdown_l(beast::error_code ec) = 0;
	public:
		virtual ~interface(){}
		virtual void error_callback(beast::error_code ec) = 0;
		void on_resolve(beast::error_code ec, tcp::resolver::results_type results){
			if (ec) {
				error_callback(ec);
				return;
			}
			on_resolve_l(ec, results);
		}
		void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type type){
			if (ec) {
				error_callback(ec);
				return;
			}
			on_connect_l(ec, type);
		}
		void on_handshake(beast::error_code ec){
			if (ec) {
				error_callback(ec);
				return;
			}
			on_handshake_l(ec);
		}
		void on_write(beast::error_code ec, std::size_t bytes_transferred){
			boost::ignore_unused(bytes_transferred);
			if (ec) {
				error_callback(ec);
				return;
			}
			on_write_l(ec, bytes_transferred);
		}
		void on_read(beast::error_code ec, std::size_t bytes_transferred){
			boost::ignore_unused(bytes_transferred);
			if (ec) {
				error_callback(ec);
				return;
			}
			on_read_l(ec, bytes_transferred);
		}
		void on_shutdown(beast::error_code ec){
			if (ec) {
				error_callback(ec);
				return;
			}
			on_shutdown_l(ec);
		}
		virtual void run(std::string host, std::string port_str, http::request<http::string_body> request) = 0;

	};
	template <class Body>
	class http_logic: public interface{
		delegate<Body>* delegate_;
		std::shared_ptr<beast::tcp_stream> tcp_stream_;
		std::shared_ptr<tcp::resolver> resolver_;
		std::chrono::nanoseconds timeout_interval_ = std::chrono::seconds(30);
		beast::flat_buffer buffer_;
		http::response<Body> response_;
		http::request<http::string_body> request_;
	protected:
		virtual void error_callback(beast::error_code ec) override {
			if(delegate_) delegate_->completion_callback(savanna::result<http::response<Body>>(beast::system_error { ec }));
		}
		virtual void on_resolve_l(beast::error_code ec, tcp::resolver::results_type results) override {
			std::cout << "on_resolve" << std::endl;
			tcp_stream_->expires_after(timeout_interval_);
			tcp_stream_->async_connect(results, beast::bind_front_handler(&http_logic<Body>::on_connect, this->shared_from_this()));
		}

		virtual void on_connect_l(beast::error_code ec, tcp::resolver::results_type::endpoint_type) override {
			std::cout << "on_connect" << std::endl;
			tcp_stream_->expires_after(timeout_interval_);
			http::async_write(*tcp_stream_, request_, beast::bind_front_handler(&http_logic<Body>::on_write, this->shared_from_this()));
		}
		virtual void on_handshake_l(beast::error_code ec) override { }

		virtual void on_write_l(beast::error_code ec, std::size_t bytes_transferred) override {
			std::cout << "on_write" << std::endl;
			http::async_read(*tcp_stream_, buffer_, response_, beast::bind_front_handler(&http_logic<Body>::on_read, this->shared_from_this()));
		}
		virtual void on_read_l(beast::error_code ec, std::size_t bytes_transferred) override {
			std::cout << "on_read" << std::endl;
			bool unfollow = true;
			if (ec && ec != beast::errc::not_connected) {
				error_callback(ec);
			}
			if(unfollow){
//				tcp_stream_->socket().shutdown(tcp::socket::shutdown_both, ec);
				if (ec && ec != beast::errc::not_connected) {
					//				completion_(savanna::result<http::response<Body>>(beast::system_error { ec }));
				}
				else {
					if(delegate_) unfollow = delegate_->completion_callback(savanna::result<http::response<Body>>(std::move(response_)));
				}
			}
		}

		virtual void on_shutdown_l(beast::error_code ec) override {}
	public:
		virtual ~http_logic(){
			tcp_stream_ = nullptr;
			resolver_ = nullptr;
		}
		http_logic(std::shared_ptr<beast::tcp_stream> tcp_stream, std::shared_ptr<tcp::resolver> resolver, delegate<Body>* delegate){
			resolver_ = resolver;
			tcp_stream_ = tcp_stream;
			delegate_ = delegate;
		}
		virtual void run(std::string host, std::string port_str, http::request<http::string_body> request) override {
			std::cout << "connect" << std::endl;
			request_ = request;
			resolver_->async_resolve(host, port_str, beast::bind_front_handler(&interface::on_resolve, this->shared_from_this()));
		}
	};
	template <class Body>
	class https_logic: public interface{
		delegate<Body>* delegate_;
		std::shared_ptr<beast::ssl_stream<beast::tcp_stream>> ssl_stream_;
		std::shared_ptr<tcp::resolver> resolver_;
		std::chrono::nanoseconds timeout_interval_ = std::chrono::seconds(30);
		beast::flat_buffer buffer_;
		http::response<Body> response_;
		http::request<http::string_body> request_;
		std::string host_;
	protected:
		virtual void error_callback(beast::error_code ec) override {
			if(delegate_) delegate_->completion_callback(savanna::result<http::response<Body>>(beast::system_error { ec }));
		}
		virtual void on_resolve_l(beast::error_code ec, tcp::resolver::results_type results) override {
			std::cout << "on_resolve" << std::endl;
			if (!SSL_set_tlsext_host_name(ssl_stream_->native_handle(), host_.c_str())) {
				beast::error_code ec { static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
				error_callback(ec);
				return;
			}
			// Set a timeout on the operation
			beast::get_lowest_layer(*ssl_stream_).expires_after(timeout_interval_);
			// Make the connection on the IP address we get from a lookup
			beast::get_lowest_layer(*ssl_stream_).async_connect(results, beast::bind_front_handler(&https_logic<Body>::on_connect, this->shared_from_this()));
		}

		virtual void on_connect_l(beast::error_code ec, tcp::resolver::results_type::endpoint_type) override {
			std::cout << "on_connect" << std::endl;
			ssl_stream_->async_handshake(
			    ssl::stream_base::client,
			    beast::bind_front_handler(
			        &https_logic<Body>::on_handshake,
			        this->shared_from_this()));
		}
		virtual void on_handshake_l(beast::error_code ec) override {
			// Set a timeout on the operation
			beast::get_lowest_layer(*ssl_stream_).expires_after(timeout_interval_);
			// Send the HTTP request to the remote host
			http::async_write(*ssl_stream_, request_, beast::bind_front_handler(&https_logic<Body>::on_write, this->shared_from_this()));
		}

		virtual void on_write_l(beast::error_code ec, std::size_t bytes_transferred) override {
			std::cout << "on_write" << std::endl;
			http::async_read(*ssl_stream_, buffer_, response_, beast::bind_front_handler(&https_logic<Body>::on_read, this->shared_from_this()));
		}
		virtual void on_read_l(beast::error_code ec, std::size_t bytes_transferred) override {
			std::cout << "on_read" << std::endl;
			bool unfollow = true;
			if (ec && ec != beast::errc::not_connected) {
				error_callback(ec);
			}
			else {
				if(delegate_) unfollow = delegate_->completion_callback(savanna::result<http::response<Body>>(std::move(response_)));
			}
			if(unfollow){
				ssl_stream_->async_shutdown(beast::bind_front_handler(&https_logic<Body>::on_shutdown,this->shared_from_this()));
			}
		}

		virtual void on_shutdown_l(beast::error_code ec) override {
			std::cout << "on_shutdown" << std::endl;
		}
	public:
		https_logic(std::shared_ptr<beast::ssl_stream<beast::tcp_stream>> ssl_stream, std::shared_ptr<tcp::resolver> resolver, delegate<Body>* delegate){
			resolver_ = resolver;
			ssl_stream_ = ssl_stream;
			delegate_ = delegate;
		}
		virtual void run(std::string host, std::string port_str, http::request<http::string_body> request) override {
			std::cout << "https_logic connect " << host << std::endl;
			host_ = host;
			request_ = request;

//			auto const results = resolver_->resolve(host, port_str);
//			std::cout << results->endpoint().address().to_string() << std::endl;
			resolver_->async_resolve(host, port_str, beast::bind_front_handler(&interface::on_resolve, this->shared_from_this()));
		}
	};
	template <class Body>
	class reuse_async_url_session : public delegate<Body>
	{
	private:
		std::shared_ptr<interface> logic = nullptr;
		std::shared_ptr<http_logic<Body>> http_logic_ = nullptr;
		std::shared_ptr<https_logic<Body>> https_logic_ = nullptr;
		bool already_run_ = false;
		std::function<void(savanna::result<http::response<Body>>)> completion_ = nullptr;
		http::request<http::string_body> request_;
		savanna::request2 original_request_;
		std::shared_ptr<net::io_context> ctx_;
		std::shared_ptr<tcp::resolver> resolver_;
		std::shared_ptr<beast::tcp_stream> tcp_stream_;
		std::shared_ptr<beast::ssl_stream<beast::tcp_stream>> ssl_stream_;

		bool follow_location_ = false;

		std::string current_scheme()
		{
			auto target = request_.target();
			auto url = savanna::url(std::string(target));
			return url.scheme();
		}

		void update_request(savanna::request2 original_request, std::string new_target, std::string new_host)
		{
			request_.target(new_target);
			request_.set(http::field::host, new_host);
			for (auto h = original_request.header_fields.begin(); h != original_request.header_fields.end(); ++h) {
				request_.set(h->first, h->second);
			}
			if (original_request.endpoint->method() != http::verb::get) {
				if (original_request.endpoint->params()) {
					auto p = *original_request.endpoint->params();
					request_.body() = original_request.body + savanna::param_str(p);
					request_.prepare_payload();
				}
				else {
					request_.body() = original_request.body;
					request_.prepare_payload();
				}
			}
			follow_location_ = original_request.follow_location;
		}

		void run(std::string host, std::string port_str)
		{
			select();
			logic->run(host, port_str, request_);
		}
		void select(){
			if (current_scheme() == url_scheme::https) {
				std::cout << "select https" << std::endl;
				logic = https_logic_;
			}else{
				std::cout << "select http" << std::endl;
				logic = http_logic_;
			}
		};

		virtual bool completion_callback(savanna::result<http::response<http::dynamic_body>> result) override {
			if (!result.error) {
				if ((result.response->result_int() >= 300) && result.response->result_int() < 400 && follow_location_) {
					auto location = result.response->base()["Location"].to_string();
					auto new_url = savanna::url(location);
					update_request(original_request_, new_url.to_string(), new_url.host());
					run(new_url.host(), new_url.port_str());
					return false;
				}
			}
			completion_(result);
			return true;
		}
	public:
		explicit reuse_async_url_session(ssl::context* ssl_ctx)
		{
			ctx_ = std::make_shared<net::io_context>();
			resolver_ = std::make_shared<tcp::resolver>(*ctx_),
			tcp_stream_ = std::make_shared<beast::tcp_stream>(*ctx_);
			ssl_stream_ = std::make_shared<beast::ssl_stream<beast::tcp_stream>>(*ctx_, *ssl_ctx);
			http_logic_ = std::make_shared<http_logic<Body>>(tcp_stream_, resolver_, this);
			https_logic_ = std::make_shared<https_logic<Body>>(ssl_stream_, resolver_, this);
		}


		bool send(savanna::request2 original_request, std::function<void(savanna::result<http::response<Body>>)> completion)
		{
			completion_ = completion;
			request_.version(original_request.version);
			request_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
			request_.method(original_request.endpoint->method());
			original_request_ = original_request;
			update_request(original_request, original_request.endpoint->build_url_str(), original_request.endpoint->host());

			auto url = original_request.endpoint->url();
			run(url.host(), url.port_str());
			ctx_->run();
			return true;
		}


		void cancel()
		{
			if (current_scheme() == url_scheme::https) {
				beast::get_lowest_layer(*ssl_stream_).cancel();
			}
			else {
				tcp_stream_->cancel();
			}
		}
	};
}
}