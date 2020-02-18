#pragma once

#include <boost/optional.hpp>

namespace m2d
{
namespace savanna
{
	template <typename Response>
	class result_t
	{
	public:
		boost::optional<Response> response;
		boost::optional<boost::system::system_error> error;

		result_t(Response response)
		    : response(response)
		    , error(boost::none)
		{
		}

		result_t(boost::system::system_error error)
		    : response(boost::none)
		    , error(error)
		{
		}
	};
}
}
