#pragma once

#include "TypeTuple.hpp"

namespace Meta
{
	template< std::size_t _Index, typename _Type >
	struct ArgInfo
	{
		using Type = _Type;
		static constexpr std::size_t Index = _Index;
	};

	template< std::size_t Value, typename Function, typename... Args >
	struct _ArgsCheck
	{
		static constexpr int check( Function function, TypeTuple< Args... > typeTuple )
		{
			function( ArgInfo< sizeof...( Args ) - Value, typename TypeTupleElement< sizeof...( Args ) - Value, Args... >::Tuple::Type >{} );
			return _ArgsCheck< Value - 1, Function, Args... >::check( function, typeTuple );
		}
	};

	template< typename Function, typename... Args >
	struct _ArgsCheck< 0, Function, Args... >
	{
		static constexpr int check( Function function, TypeTuple< Args... > typeTuple )
		{
			return 0;
		}
	};

	template< typename _ClassType, typename _ResultType, typename... _Args >
	struct _FunctionInfo
	{
		using ClassType = _ClassType;
		using ResultType = _ResultType;
		using ArgsTypeTuple = TypeTuple< _Args... >;

		static constexpr std::size_t ArgsCount = sizeof...( _Args );

		template< std::size_t Index >
		using ArgType = typename TypeTupleElement< Index, _Args... >::Tuple::Type;

		template< typename Function >
		static constexpr int argsCheck( Function function )
		{
			return _ArgsCheck< ArgsCount, Function, _Args... >::check( function, ArgsTypeTuple{} );
		}
	};

	template< typename ClassType, typename ResultType, typename... Args >
	constexpr auto _getFunctionInfo( ResultType ( ClassType::* )( Args... ) )
	{
		return _FunctionInfo< ClassType, ResultType, Args... >{};
	}

	template< typename ResultType, typename... Args >
	constexpr auto _getFunctionInfo( ResultType ( * )( Args... ) )
	{
		return _FunctionInfo< void, ResultType, Args... >{};
	}

	template< typename Function >
	struct FunctionInfo
	{
		using _Info = decltype( _getFunctionInfo( Function() ) );

		using ClassType = typename _Info::ClassType;
		using ResultType = typename _Info::ResultType;
		using ArgsTypeTuple = typename _Info::ArgsTypeTuple;

		static constexpr std::size_t ArgsCount = _Info::ArgsCount;

		template< std::size_t Index >
		using ArgType = typename _Info::template ArgType< Index >;

		template< typename CheckFunction >
		static constexpr int argsCheck( CheckFunction function )
		{
			return _Info::argsCheck( function );
		}
	};

} // namespace Meta