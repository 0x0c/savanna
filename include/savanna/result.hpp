#pragma once

#include <boost/optional.hpp>

namespace m2d
{
namespace savanna
{
	template <typename Response>
	class result
	{
	public:
		boost::optional<Response> response;
		boost::optional<boost::system::system_error> error;

		result(Response response)
		    : response(response)
		    , error(boost::none)
		{
		}

		result(boost::system::system_error error)
		    : response(boost::none)
		    , error(error)
		{
		}
	};
}
}
