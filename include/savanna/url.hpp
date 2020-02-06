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

		std::string to_string(bool append_query = true, bool append_fragment = true)
		{
			std::string str = scheme() + "//" + host() + ":" + path();
			if (append_query) {
				str += "?" + query();
			}
			if (append_fragment) {
				str += "#" + fragment();
			}
			return str;
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
				if (scheme() == savanna::url_scheme::https || scheme() == savanna::url_scheme::wss) {
					return "443";
				}
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
