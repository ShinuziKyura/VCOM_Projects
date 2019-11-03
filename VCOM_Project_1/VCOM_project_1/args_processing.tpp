#include <system_error>
#include <charconv>

template <class Value_Type>
bool process_option(std::unordered_map<std::string, std::string> const& in_options_map, char const* in_option, char const* in_option_ex, Value_Type& out_value)
{
	auto const it_end = in_options_map.end();
	auto it = in_options_map.find(in_option);

	if (it == it_end)
	{
		it = in_options_map.find(in_option_ex);

		if (it == it_end)
		{
			return true;
		}
	}

	auto const& str = it->second;
	auto result = std::from_chars(&str.front(), &str.back(), out_value);

	return result.ec != std::errc::invalid_argument;
}
