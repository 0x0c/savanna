#pragma once

#include <boost/optional.hpp>
#include <chrono>
#include "endpoint.hpp"

namespace m2d
{
namespace savanna
{
	static std::string to_string(http::field field)
	{
		return std::string(http::to_string(field));
	}

	template <typename Endpoint>
	class request
	{
	public:
		bool follow_location = false;
		Endpoint endpoint;
		unsigned int version = 11;
		std::string body;
		std::map<std::string, std::string> header_fields;
		std::chrono::nanoseconds timeout_interval = std::chrono::seconds(30);

		request(Endpoint e)
		    : endpoint(e)
		{
			static_assert(std::is_base_of<savanna::endpoint, Endpoint>::value, "Endpoint not derived from endpoint_t");
		}

		request &set_http_version(unsigned int version)
		{
			this->version = version;
			return &this;
		}
	};
	namespace ssl_reuse
	{
		class request
		{
		public:
			typedef savanna::endpoint Endpoint;
			bool follow_location = false;
			std::shared_ptr<Endpoint> endpoint;
			unsigned int version = 11;
			std::string body;
			std::map<std::string, std::string> header_fields;
			std::chrono::nanoseconds timeout_interval = std::chrono::seconds(30);

			request() { }
			request(std::shared_ptr<Endpoint> e)
			    : endpoint(e)
			{
			}
			virtual ~request()
			{
				endpoint = nullptr;
			}

			request &&set_http_version(unsigned int version)
			{
				this->version = version;
				return std::move(*this);
			}
		};
	}
}
}
