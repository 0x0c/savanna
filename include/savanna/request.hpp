#pragma once

#include <boost/optional.hpp>
#include <chrono>

namespace m2d
{
namespace savanna
{
	template <typename Endpoint>
	class request_t
	{
	public:
		bool follow_location = false;
		Endpoint endpoint;
		unsigned int version = 11;
		std::string body;
		std::map<std::string, std::string> header_fields;
		std::chrono::milliseconds timeout_interval = std::chrono::seconds(30);

		request_t(Endpoint e)
		    : endpoint(e)
		{
			static_assert(std::is_base_of<endpoint_t, Endpoint>::value, "Endpoint not derived from endpoint_t");
		}

		request_t &set_http_version(unsigned int version)
		{
			this->version = version;
			return &this;
		}
	};
}
}
