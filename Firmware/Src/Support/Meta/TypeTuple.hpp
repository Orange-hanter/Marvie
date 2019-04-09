#pragma once

#include <type_traits>

namespace Meta
{
	template< typename... Types >
	struct TypeTuple;

	template< typename FirstType, typename... Types >
	struct TypeTuple< FirstType, Types... > : public TypeTuple< Types... >
	{
		using Type = FirstType;
	};

	template<>
	struct TypeTuple<>
	{
		using Type = void;
	};

	template< std::size_t Index, class... Types >
	struct TypeTupleElement;

	template< std::size_t Index, class FirstType, class... Types >
	struct TypeTupleElement< Index, FirstType, Types... > : public TypeTupleElement< Index - 1, Types... >
	{
	};

	template< class FirstType, class... Types >
	struct TypeTupleElement< 0, FirstType, Types... >
	{
		using Tuple = TypeTuple< FirstType, Types... >;
	};

	template<>
	struct TypeTupleElement< 0 >
	{
		using Tuple = TypeTuple<>;
	};
} // namespace Meta