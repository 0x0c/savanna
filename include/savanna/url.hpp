#pragma once

#include <regex>

namespace m2d
{
namespace savanna
{
	class url
	{
	private:
		std::string scheme_;
		std::string host_;
		std::string port_;
		std::string path_;
		std::string query_;
		std::string fragment_;

	public:
		url(std::string url)
		{
			std::regex ex("(http|https|ws|wss)://([^/ :]+):?([^/ ]*)(/?[^ #?]*)\\x3f?([^ #]*)#?([^ ]*)");
			std::cmatch what;
			if (regex_match(url.c_str(), what, ex)) {
				scheme_ = std::string(what[1].first, what[1].second);
				host_ = std::string(what[2].first, what[2].second);
				port_ = std::string(what[3].first, what[3].second);
				path_ = std::string(what[4].first, what[4].second);
				query_ = std::string(what[5].first, what[5].second);
				fragment_ = std::string(what[6].first, what[6].second);
			}
		}

		std::string to_string()
		{
			return scheme() + "://" + host() + ":" + path() + "?" + query();
		}

		std::string scheme()
		{
			return scheme_;
		}

		std::string host()
		{
			return host_;
		}

		int port()
		{
			return std::atoi(port_str().c_str());
		}

		std::string port_str()
		{
			if (port_ == "") {
				return "80";
			}

			return port_;
		}

		std::string path()
		{
			return path_;
		}

		std::string query()
		{
			return query_;
		}

		std::string fragment()
		{
			return fragment_;
		}
	};
}
}