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

	class endpoint_t
	{
	public:
		virtual std::string scheme()
		{
			return "https://";
		}

		virtual std::string host() = 0;
		virtual http::verb method() = 0;
		virtual boost::optional<std::map<std::string, std::string>> params() = 0;
		virtual std::string build_path() = 0;

		virtual int port()
		{
			return 80;
		}

		virtual std::string path()
		{
			return build_path();
		}

		virtual std::string build_url_str()
		{
			std::string new_url;
			new_url += scheme();
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

	class get_endpoint_t : public endpoint_t
	{
	private:
		boost::optional<std::map<std::string, std::string>> query_;

	public:
		get_endpoint_t(boost::optional<std::map<std::string, std::string>> query)
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

		virtual std::string query_str()
		{
			if (params()) {
				std::string new_query_str = "?";
				for (auto p = params()->begin(); p != params()->end(); ++p) {
					new_query_str += p->first;
					new_query_str += "=";
					new_query_str += p->second;
					new_query_str += "&";
				}
				new_query_str.pop_back();
				return new_query_str;
			}

			return "";
		}

		std::string build_path()
		{
			std::string new_path;
			new_path += path();
			new_path += query_str();
			return new_path;
		}
	};
}
}