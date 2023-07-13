#pragma once

#include <boost/any.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/optional.hpp>
#include <boost/utility/string_view.hpp>

#include <functional>
#include <memory>
#include <thread>

#include "url.hpp"

namespace m2d
{
namespace savanna
{
	namespace beast = boost::beast;
	namespace http = beast::http;
	namespace net = boost::asio;
	namespace ssl = boost::asio::ssl;
	using tcp = boost::asio::ip::tcp;

	namespace async_websocket
	{
		typedef beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>> tls_stream;
		typedef beast::websocket::stream<beast::tcp_stream> raw_stream;
		typedef std::shared_ptr<tls_stream> tls_stream_ptr;
		typedef std::shared_ptr<raw_stream> raw_stream_ptr;

		enum state
		{
			unknown,
			closed,
			connected
		};

		class delegate{
		public:
			virtual void on_ready() = 0;
			virtual void on_read(beast::error_code ec, beast::flat_buffer buffer, std::size_t bytes_transferred) = 0;
			virtual void on_write(beast::error_code ec, std::size_t bytes_transferred) = 0;
			virtual void on_error(beast::error_code ec) = 0;
			virtual void control_callback(beast::websocket::frame_type kind, boost::string_view payload) = 0;
		};
		class interface: public std::enable_shared_from_this<interface>{
		protected:
			virtual void on_resolve_l(beast::error_code ec, tcp::resolver::results_type) = 0;
			virtual void on_connect_l(beast::error_code ec, tcp::resolver::results_type::endpoint_type) = 0;
			virtual void on_handshake_l(beast::error_code ec) = 0;
			virtual void on_ssl_handshake_l(beast::error_code ec) = 0;
			virtual void on_write_l(beast::error_code ec, std::size_t bytes_transferred) = 0;
			virtual void on_read_l(beast::error_code ec, std::size_t bytes_transferred) = 0;
			virtual void on_shutdown_l(beast::error_code ec) = 0;
			virtual void on_close_l(beast::error_code ec) = 0;
		public:
			virtual ~interface()= default;
			virtual void error_callback(beast::error_code ec) = 0;
			void on_resolve(beast::error_code ec, tcp::resolver::results_type results){
				if (ec) {
					error_callback(ec);
					return;
				}
				on_resolve_l(ec, std::move(results));
			}
			void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type type){
				if (ec) {
					error_callback(ec);
					return;
				}
				on_connect_l(ec, std::move(type));
			}
			void on_ssl_handshake(beast::error_code ec){
				if (ec) {
					error_callback(ec);
					return;
				}
				on_ssl_handshake_l(ec);
			}
			void on_handshake(beast::error_code ec){
				if (ec) {
					error_callback(ec);
					return;
				}
				on_handshake_l(ec);
			}
			void on_write(beast::error_code ec, std::size_t bytes_transferred){
				on_write_l(ec, bytes_transferred);
			}
			void on_read(beast::error_code ec, std::size_t bytes_transferred){
				on_read_l(ec, bytes_transferred);
			}
			void on_shutdown(beast::error_code ec){
				if (ec == net::error::eof || ec == net::ssl::error::stream_truncated) {
					// Rationale:
					// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
					ec = {};
				}
				if (ec) {
					error_callback(ec);
					return;
				}
				on_shutdown_l(ec);
			}
			void on_close(beast::error_code ec){
				on_close_l(ec);
			}
			virtual void run(std::string host, std::string port, std::string path) = 0;
			virtual void close() = 0;
			virtual void write(std::string data) = 0;
			virtual void read() = 0;
		};

		class raw_stream_logic: public interface{
			std::shared_ptr<beast::websocket::stream<beast::tcp_stream>> raw_stream_;
			std::shared_ptr<tcp::resolver> resolver_;
			delegate* delegate_;
			std::string host_;
			std::string port_;
			std::string path_;
			beast::flat_buffer buffer_;
		protected:
			void error_callback(beast::error_code ec) override {
				if(delegate_) delegate_->on_error(ec);
			}
			void on_resolve_l( beast::error_code ec, tcp::resolver::results_type results) override {
				beast::get_lowest_layer(*raw_stream_).expires_after(std::chrono::seconds(120));
				beast::get_lowest_layer(*raw_stream_).async_connect(results, beast::bind_front_handler(&interface::on_connect, this->shared_from_this()));
			}

			void on_connect_l(beast::error_code ec, tcp::resolver::results_type::endpoint_type) override {
				beast::get_lowest_layer(*raw_stream_).expires_never();
				raw_stream_->set_option(beast::websocket::stream_base::timeout::suggested(beast::role_type::client));

				raw_stream_->set_option(beast::websocket::stream_base::decorator([](beast::websocket::request_type &req) {
					req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " savanna");
				}));
				raw_stream_->async_handshake(host_, path_, beast::bind_front_handler(&interface::on_handshake,this->shared_from_this()));
			}

			void on_ssl_handshake_l(beast::error_code ec) override {

			}
			void on_handshake_l(beast::error_code ec) override {
				//success
				if(delegate_) {
					auto control_callback = [this](beast::websocket::frame_type kind, boost::string_view payload) {
						delegate_->control_callback(kind, payload);
					};
					raw_stream_->control_callback(control_callback);
					delegate_->on_ready();
				}
			}

			void on_write_l(beast::error_code ec, std::size_t bytes_transferred) override {
				if(delegate_) delegate_->on_write(ec, bytes_transferred);
			}
			void on_read_l(beast::error_code ec, std::size_t bytes_transferred) override {
				if(delegate_) delegate_->on_read(ec, buffer_, bytes_transferred);
			}

			void on_shutdown_l(beast::error_code ec) override {}
			void on_close_l(beast::error_code ec) override {}
		public:
			~raw_stream_logic() override{
				raw_stream_ = nullptr;
				resolver_ = nullptr;
			}
			raw_stream_logic(raw_stream_ptr tcp_stream, delegate* delegate, std::shared_ptr<tcp::resolver> resolver){
				raw_stream_ = tcp_stream;
				resolver_ = resolver;
				delegate_ = delegate;
			}
			void run(std::string host, std::string port, std::string path) override {
				host_ = host;
				port_ = port;
				path_ = path;
				resolver_->async_resolve(host_, port_, beast::bind_front_handler(&interface::on_resolve, this->shared_from_this()));
			}
			void read() override {
				buffer_.consume(buffer_.size());
				raw_stream_->async_read(
				    buffer_,
				    beast::bind_front_handler(
				        &interface::on_read,
				        shared_from_this()));
			}
			void write(std::string data) override {
				raw_stream_->async_write(
				    net::buffer(data),
				    beast::bind_front_handler(
				        &interface::on_write,
				        shared_from_this()));
			}
			void close() override {
				raw_stream_->async_close(
				    beast::websocket::close_code::normal,
				    beast::bind_front_handler(
				        &interface::on_close,
				        shared_from_this()));
			}
		};


		class ssl_stream_logic: public interface{
			std::shared_ptr<beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>>> ssl_stream_;
			std::shared_ptr<tcp::resolver> resolver_;
			delegate* delegate_;
			std::string host_;
			std::string port_;
			std::string path_;
			beast::flat_buffer buffer_;

			std::map<std::string, std::shared_ptr<SSL_SESSION>>* ssl_cache_;
			std::string ipaddress_;
		protected:
			void error_callback(beast::error_code ec) override {
				if(delegate_) delegate_->on_error(ec);
			}
			void on_resolve_l( beast::error_code ec, tcp::resolver::results_type results) override {
				beast::get_lowest_layer(*ssl_stream_).expires_after(std::chrono::seconds(120));
				beast::get_lowest_layer(*ssl_stream_).async_connect(results, beast::bind_front_handler(&interface::on_connect, this->shared_from_this()));
			}

			void on_connect_l(beast::error_code ec, tcp::resolver::results_type::endpoint_type type) override {
				// add session to the cache after a successful connection
				ipaddress_ = type.address().to_string();
				if(ssl_cache_) {
					auto cached_session = ssl_cache_->find(ipaddress_);
					if (cached_session != ssl_cache_->end()) {
						SSL_set_session(ssl_stream_->next_layer().native_handle(), cached_session->second.get());
					}
				}
				ssl_stream_->next_layer().async_handshake(
				    ssl::stream_base::client,
				    beast::bind_front_handler(
				        &interface::on_ssl_handshake,
				        this->shared_from_this()));
			}

			void on_ssl_handshake_l(beast::error_code ec) override {
				beast::get_lowest_layer(*ssl_stream_).expires_never();
				ssl_stream_->set_option(beast::websocket::stream_base::decorator([](beast::websocket::request_type &req) {
					req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " savanna");
				}));

				// after a connection can check if ssl-session was reused
				if(ssl_cache_) {
					auto session = std::shared_ptr<SSL_SESSION>(SSL_get1_session(ssl_stream_->next_layer().native_handle()), SSL_SESSION_free);
					if (SSL_session_reused(ssl_stream_->next_layer().native_handle())) {
						//	std::cout << "session reused" << std::endl;
					}
					else {
						//	std::cout << "session new negotiated" << std::endl;
						(*ssl_cache_)[ipaddress_] = session;
					}
				}
				ssl_stream_->async_handshake(host_, path_, beast::bind_front_handler(&interface::on_handshake,this->shared_from_this()));
			}
			void on_handshake_l(beast::error_code ec) override {
				//success
				if(delegate_) {
					auto control_callback = [this](beast::websocket::frame_type kind, boost::string_view payload) {
						delegate_->control_callback(kind, payload);
					};
					ssl_stream_->control_callback(control_callback);
					delegate_->on_ready();
				}
			}

			void on_write_l(beast::error_code ec, std::size_t bytes_transferred) override {
				if(delegate_) delegate_->on_write(ec, bytes_transferred);
			}
			void on_read_l(beast::error_code ec, std::size_t bytes_transferred) override {
				if(delegate_) delegate_->on_read(ec, buffer_, bytes_transferred);
			}

			void on_shutdown_l(beast::error_code ec) override {}
			void on_close_l(beast::error_code ec) override {}
		public:
			~ssl_stream_logic() override{
				ssl_stream_ = nullptr;
				resolver_ = nullptr;
			}
			ssl_stream_logic(tls_stream_ptr ssl_stream, delegate* delegate, std::shared_ptr<tcp::resolver> resolver, std::map<std::string, std::shared_ptr<SSL_SESSION>>* ssl_cache){
				ssl_stream_ = ssl_stream;
				resolver_ = resolver;
				delegate_ = delegate;
				ssl_cache_ = ssl_cache;
			}
			void run(std::string host, std::string port, std::string path) override {
				host_ = host;
				port_ = port;
				path_ = path;
				resolver_->async_resolve(host_, port_, beast::bind_front_handler(&interface::on_resolve, this->shared_from_this()));
			}
			void read() override {
				buffer_.consume(buffer_.size());
				ssl_stream_->async_read(
				    buffer_,
				    beast::bind_front_handler(
				        &interface::on_read,
				        shared_from_this()));
			}
			void write(std::string data) override {
				ssl_stream_->async_write(
				    net::buffer(data),
				    beast::bind_front_handler(
				        &interface::on_write,
				        shared_from_this()));
			}
			void close() override {
				ssl_stream_->async_close(
				    beast::websocket::close_code::normal,
				    beast::bind_front_handler(
				        &interface::on_close,
				        shared_from_this()));
			}
		};

		template <typename T>
		class reuse_websocket_executor: public delegate
		{
		private:
			std::shared_ptr<net::io_context> ctx_ = nullptr;
			std::shared_ptr<tcp::resolver> resolver_ = nullptr;
			std::shared_ptr<interface> interface_ = nullptr;

			savanna::url url_;
			std::string scheme_;
			boost::optional<std::function<void(beast::flat_buffer &)>> on_message_handler_;
			T stream_;
			state current_state_ = unknown;

			std::map<std::string, std::shared_ptr<SSL_SESSION>> *ssl_cache_;

			void make_connection(raw_stream_ptr stream, std::string path, std::shared_ptr<net::io_context> ctx)
			{
				resolver_ = std::make_shared<tcp::resolver>(*ctx);
				interface_ = std::make_shared<raw_stream_logic>(stream, this, resolver_);
			}

			void make_connection(tls_stream_ptr stream, std::string path, std::shared_ptr<net::io_context> ctx)
			{
				resolver_ = std::make_shared<tcp::resolver>(*ctx);
				interface_ = std::make_shared<ssl_stream_logic>(stream, this, resolver_, ssl_cache_);
			}

			void on_write(beast::error_code ec, std::size_t bytes_transferred) override
			{
				boost::ignore_unused(bytes_transferred);
				if (ec) {
					on_error(ec);
					return;
				}
			}

			void on_read(beast::error_code ec, beast::flat_buffer buffer, std::size_t bytes_transferred) override
			{
				boost::ignore_unused(bytes_transferred);

				if (ec) {
					on_error(ec);
					return;
				}

				if (on_message_handler_) {
					auto handler = *on_message_handler_;
					handler(buffer);
				}

				interface_->read();
			}
			void on_ready() override {
				set_current_state(connected);
				interface_->read();
			}
			void on_error(beast::error_code ec) override {
				set_current_state(unknown);
				throw beast::system_error { ec };
			}

			void control_callback(beast::websocket::frame_type kind, boost::string_view payload) override {
				if (kind == beast::websocket::frame_type::close) {
					this->set_current_state(closed);
				}
				boost::ignore_unused(kind, payload);
			};

			void set_current_state(state s)
			{
				current_state_ = s;
				state_changed(s);
			}

		public:
			std::function<void(state)> state_changed = [](state s) {};
			reuse_websocket_executor(std::shared_ptr<net::io_context> ctx, T stream, savanna::url url, std::map<std::string, std::shared_ptr<SSL_SESSION>>* ssl_cache)
			    : url_(url)
			    , ctx_(ctx)
			    , scheme_(url_.scheme())
			    , stream_(std::move(stream)){
				ssl_cache_ = ssl_cache;
			}

			~reuse_websocket_executor()
			{
				close();
			}

			state current_state()
			{
				return current_state_;
			}

			void run(std::string path = "/")
			{
				make_connection(stream_, path, ctx_);
				interface_->run(url_.host(), url_.port_str(), path);
				ctx_->run();
			}

			void send(std::string data)
			{
				interface_->write(data);
			}

			bool close()
			{
				interface_->close();
			}

			void on_message(std::function<void(beast::flat_buffer &)> handler)
			{
				on_message_handler_ = handler;
			}
		};

		class async_session{
			std::map<std::string, std::shared_ptr<SSL_SESSION>> ssl_cache_ = {};
		public:

			template <class T>
			std::shared_ptr<reuse_websocket_executor<T>> prepare(std::shared_ptr<net::io_context> ctx, T stream, savanna::url url){
				auto executor = std::make_shared<reuse_websocket_executor<T>>(ctx, stream, url, &ssl_cache_);
				return executor;
			}

			std::map<std::string, std::shared_ptr<SSL_SESSION>> ssl_cache(){
				return ssl_cache_;
			}
			void ssl_cache(std::map<std::string, std::shared_ptr<SSL_SESSION>> ssl_cache){
				ssl_cache_ = ssl_cache;
			}
		};
	}
}
}
