#pragma once

#include <initializer_list>

class Flag
{
	int i;
public:
	constexpr inline Flag( int ai ) noexcept : i( ai ) {}
	constexpr inline operator int() const noexcept { return i; }
};

template< typename Enum >
class Flags
{
	static_assert( ( sizeof( Enum ) <= sizeof( int ) ),
				   "Flags uses an int as storage, so an enum with underlying "
				   "long long will overflow." );
	static_assert( ( std::is_enum< Enum >::value ), "Flags is only usable on enumeration types." );

	struct Private;
	typedef int( Private::*Zero );

public:
#ifdef __DOXYGEN__
	// the definition below is too complex for __DOXYGEN__
	typedef int Int;
#else
	typedef typename std::conditional<
		std::is_unsigned<typename std::underlying_type< Enum >::type >::value,
		unsigned int,
		signed int
	>::type Int;
#endif
	typedef Enum enum_type;
#ifdef __DOXYGEN__
	constexpr inline Flags( const Flags &other );
	constexpr inline Flags &operator=( const Flags &other );
#endif
	constexpr inline Flags( Enum f ) noexcept : i( Int( f ) ) {}
	constexpr inline Flags( Zero = nullptr ) noexcept : i( 0 ) {}
	constexpr inline Flags( int f ) noexcept : i( f ) {}

	constexpr inline Flags( std::initializer_list< Enum > flags ) noexcept
		: i( initializer_list_helper( flags.begin(), flags.end() ) ) {}

	constexpr inline Flags &operator&=( int mask ) noexcept { i &= mask; return *this; }
	constexpr inline Flags &operator&=( uint mask ) noexcept { i &= mask; return *this; }
	constexpr inline Flags &operator&=( Enum mask ) noexcept { i &= Int( mask ); return *this; }
	constexpr inline Flags &operator|=( Flags f ) noexcept { i |= f.i; return *this; }
	constexpr inline Flags &operator|=( Enum f ) noexcept { i |= Int( f ); return *this; }
	constexpr inline Flags &operator^=( Flags f ) noexcept { i ^= f.i; return *this; }
	constexpr inline Flags &operator^=( Enum f ) noexcept { i ^= Int( f ); return *this; }

	constexpr inline operator Int() const noexcept { return i; }

	constexpr inline Flags operator|( Flags f ) const noexcept { return Flags( Flag( i | f.i ) ); }
	constexpr inline Flags operator|( Enum f ) const noexcept { return Flags( Flag( i | Int( f ) ) ); }
	constexpr inline Flags operator^( Flags f ) const noexcept { return Flags( Flag( i ^ f.i ) ); }
	constexpr inline Flags operator^( Enum f ) const noexcept { return Flags( Flag( i ^ Int( f ) ) ); }
	constexpr inline Flags operator&( int mask ) const noexcept { return Flags( Flag( i & mask ) ); }
	constexpr inline Flags operator&( uint mask ) const noexcept { return Flags( Flag( i & mask ) ); }
	constexpr inline Flags operator&( Enum f ) const noexcept { return Flags( Flag( i & Int( f ) ) ); }
	constexpr inline Flags operator~() const noexcept { return Flags( Flag( ~i ) ); }

	constexpr inline bool operator!() const noexcept { return !i; }

	constexpr inline bool testFlag( Enum f ) const noexcept { return ( i & Int( f ) ) == Int( f ) && ( Int( f ) != 0 || i == Int( f ) ); }
	constexpr inline Flags &setFlag( Enum f, bool on = true ) noexcept
	{
		return on ? ( *this |= f ) : ( *this &= ~f );
	}

private:
	constexpr static inline Int initializer_list_helper( typename std::initializer_list< Enum >::const_iterator it,
														 typename std::initializer_list< Enum >::const_iterator end ) noexcept
	{
		return ( it == end ? Int( 0 ) : ( Int( *it ) | initializer_list_helper( it + 1, end ) ) );
	}

	Int i;
};