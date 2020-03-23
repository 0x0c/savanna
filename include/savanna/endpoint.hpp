#pragma once

#include <boost/beast/http.hpp>
#include <boost/optional.hpp>
#include <exception>

#include "savanna/url.hpp"

namespace m2d
{
namespace savanna
{
	namespace beast = boost::beast;
	namespace http = beast::http;

	class endpoint
	{
	public:
		virtual std::string scheme()
		{
			return url_scheme::https;
		}

		virtual std::string host() = 0;
		virtual http::verb method() = 0;
		virtual boost::optional<std::map<std::string, std::string>> params() = 0;
		virtual std::string path() = 0;

		virtual std::string build_path()
		{
			return path();
		}

		virtual int port()
		{
			return 80;
		}

		virtual std::string param_str()
		{
			if (params()) {
				std::string param_str;
				auto pp = *(params());
				for (auto p = pp.begin(); p != pp.end(); ++p) {
					param_str += p->first;
					param_str += "=";
					param_str += p->second;
					param_str += "&";
				}
				param_str.pop_back();
				return param_str;
			}

			return "";
		}

		virtual std::string build_url_str()
		{
			std::string new_url;
			new_url += scheme() + "://";
			new_url += host();
			if (port() != 80) {
				new_url += ":" + std::to_string(port());
			}
			new_url += build_path();

			return new_url;
		}

		savanna::url url()
		{
			return savanna::url(build_url_str());
		}
	};

	class post_endpoint : public endpoint
	{
	private:
		boost::optional<std::map<std::string, std::string>> params_;

	public:
		post_endpoint(boost::optional<std::map<std::string, std::string>> params)
		    : params_(params)
		{
		}

		boost::optional<std::map<std::string, std::string>> params()
		{
			return params_;
		}

		http::verb method()
		{
			return http::verb::post;
		}

		std::string build_path()
		{
			return path();
		}
	};

	class get_endpoint : public endpoint
	{
	private:
		boost::optional<std::map<std::string, std::string>> query_;

	public:
		get_endpoint(boost::optional<std::map<std::string, std::string>> query)
		    : query_(query)
		{
		}

		boost::optional<std::map<std::string, std::string>> params()
		{
			return query_;
		}

		http::verb method()
		{
			return http::verb::get;
		}

		std::string build_path()
		{
			std::string new_path;
			new_path += path();
			if (params()) {
				new_path += "?";
				new_path += param_str();
			}
			return new_path;
		}
	};

	class put_endpoint : public post_endpoint
	{
	private:
		boost::optional<std::map<std::string, std::string>> params_;

	public:
		put_endpoint(boost::optional<std::map<std::string, std::string>> params)
		    : post_endpoint(params)
		{
		}

		http::verb method()
		{
			return http::verb::put;
		}
	};

	class delete_endpoint : public post_endpoint
	{
	private:
		boost::optional<std::map<std::string, std::string>> params_;

	public:
		delete_endpoint(boost::optional<std::map<std::string, std::string>> params)
		    : post_endpoint(params)
		{
		}

		http::verb method()
		{
			return http::verb::delete_;
		}
	};
}
}
