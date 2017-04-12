// BigInt.cpp : Defines the entry point for the console application.
#include "stdafx.h"

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>

class BigIntNibble
{
	// Class invariant bytes.size() > 0 always
	std::vector <uint8_t> bytes;
	bool negative{ false };

	void fromString( std::string digitsString )
	{
		size_t charsInString = digitsString.size();
		if ( charsInString > 0 )
		{
			size_t lastIndex = 0;
			// negative sign
			if ( digitsString[ 0 ] == '-' )
			{
				negative = true;
				++lastIndex;
				if ( charsInString == lastIndex )
				{
					throw std::bad_cast("only negative sign is bad idea");
				}
			}

			while ( digitsString[ lastIndex ] == '0' ) // zero
			{
				if ( negative )
				{
					throw std::bad_cast( "zero after negative sign is bad idea" );
				}
				++lastIndex;
				if ( charsInString == lastIndex )
				{
					bytes.push_back( 0 );
					return;
				}
			};

			bytes.reserve(( charsInString - lastIndex ) >> 1 );

			size_t index = charsInString;
			uint8_t prevValue = 0;
			uint8_t originalChar;
			uint8_t counter = 0;
			do
			{
				--index;
				originalChar = digitsString[ index ] - '0';
				if ( originalChar > 9 )
				{
					throw std::bad_cast( "invalid number" );
				}
				if ( counter & 0x01 )
				{
					bytes.push_back( prevValue | ( originalChar << 4 ) );
				}
				else
				{
					prevValue = originalChar;
				}
				++counter;
			} while ( index > lastIndex );
			if ( counter & 0x01 )
			{
				bytes.push_back( prevValue );
			}
		}
		else
		{
			throw std::bad_cast();
		}
	}

	void fromIndeger( const int64_t number )
	{
		if ( number < 0 )
		{
			negative = true;
		}
		size_t index = 0;
		int64_t numberRemain = number, current;
		uint8_t currentDigit;
		do
		{
			if (( index & 0x01 ) == 0 )
			{
				bytes.push_back( 0 );
			}
			current = ( numberRemain / 10 );
			currentDigit = ( uint8_t)( numberRemain - ( current * 10 ) );
			numberRemain = current;
			if ( negative )
			{
				currentDigit ^= 0xFF;
				++currentDigit;
			}
			SetByIndex( index, currentDigit );
			++index;
		} while ( numberRemain != 0 );
	}

public:

	BigIntNibble()
	{
		bytes.push_back( 0 );
	}

	BigIntNibble( const std::string number )
	{
		fromString( number );
	}

	BigIntNibble( const int64_t number )
	{
		fromIndeger( number );
	}

	// In digits
	size_t GetSize() const
	{
		return ( bytes.size() << 1 );
	}

	// Get digit by index statring from less significant
	uint8_t GetByIndex( const size_t index ) const
	{
		size_t realIndex = index >> 1;
		if ( realIndex >= bytes.size() )
		{
			throw std::out_of_range( "bad index" );
		}
		if ( index & 0x01 )
		{
			return ( bytes[ realIndex ] >> 4 );
		}
		else
		{
			return ( bytes[ realIndex ] & 0x0F );
		}
	}

	// Set by index
	void SetByIndex( const size_t index, const uint8_t value )
	{
		size_t realIndex = index >> 1;
		if ( realIndex >= bytes.size() )
		{
			throw std::out_of_range( "bad index" );
		}
		if ( index & 0x01 )
		{
			bytes[ realIndex ] = (( bytes[ realIndex ] & 0x0F ) | ( value << 4 ));
		}
		else
		{
			bytes[ realIndex ] = (( bytes[ realIndex ] & 0xF0 ) | ( value & 0x0F ));
		}
	}

	void Print() const
	{
		std::cout << "number: " << ToString() << std::endl;
	}

	std::string ToString() const
	{
		std::string result;
		if ( negative )
		{
			// Negative
			result.push_back( '-' );
		}
		size_t index = GetSize();
		result.reserve( index );

		--index;
		uint8_t originalValue = GetByIndex( index );
		if ( originalValue != 0 )
		{
			result.push_back( originalValue + '0' );
		}
		while ( index > 0 )
		{
			--index;
			result.push_back( GetByIndex( index ) + '0' );
		};
		return result;
	}

	BigIntNibble& operator=( const std::string number )
	{
		bytes.clear();
		negative = false;
		fromString( number );
		bytes.shrink_to_fit();
	}

	BigIntNibble& operator+=( const BigIntNibble& value )
	{
		size_t currentSize = GetSize();
		size_t valueSize = value.GetSize();
		size_t maxDigits = currentSize < valueSize ? valueSize : currentSize;
		uint8_t temp;
		bool overload = false;
		size_t index;

		for ( index = 0; index < maxDigits; ++index )
		{
			temp = 0;
			if ( index < currentSize )
			{
				temp = GetByIndex( index );
			}
			if ( index < valueSize )
			{
				temp += value.GetByIndex( index );
			}
			if ( overload )
			{
				++temp;
			}

			if ( index >= GetSize() )
			{
				bytes.push_back( 0 );
			}

			if ( temp > 9 )
			{
				overload = true;
				SetByIndex( index, temp - 10 );
			}
			else
			{
				overload = false;
				SetByIndex( index, temp );
			}
		}

		if ( overload )
		{
			if ( index >= GetSize() )
			{
				bytes.push_back( 1 );
			}
			else
			{
				SetByIndex( index, 1 );
			}
		}

		return *this;
	}

	friend BigIntNibble operator+( BigIntNibble left, const BigIntNibble& right )
	{
		left += right;
		return left;
	}
};

class BigInt
{
	std::vector <uint8_t> bytes;
	bool negative{ false };

	void fromString( std::string digitsString )
	{
		size_t charsInString = digitsString.size();
		if ( charsInString > 0 )
		{
			size_t lastIndex = 0;
			// negative sign
			if ( digitsString[ 0 ] == '-' )
			{
				negative = true;
				++lastIndex;
				if ( charsInString == lastIndex )
				{
					throw std::bad_cast( "only negative sign is bad idea" );
				}
			}

			while ( digitsString[ lastIndex ] == '0' ) // zero
			{
				if ( negative )
				{
					throw std::bad_cast( "zero after negative sign is bad idea" );
				}
				++lastIndex;
				if ( charsInString == lastIndex )
				{
					bytes.push_back( 0 );
					return;
				}
			};

			bytes.reserve(( charsInString - lastIndex ) >> 1 );

			size_t nibble;
			uint8_t originalChar;
			uint8_t digit;
			for ( size_t index = lastIndex; index < charsInString; ++index )
			{
				originalChar = digitsString[ index ] - '0';
				if ( originalChar > 9 )
				{
					throw std::bad_cast( "invalid number" );
				}
				for ( nibble = 0; nibble < ( bytes.size() << 1 ); ++nibble )
				{
					digit = GetDigit( nibble ) * 10 + originalChar;
					SetDigit( nibble, digit & 0x0F );
					originalChar = digit >> 4;
				}
				if ( originalChar != 0 )
				{
					bytes.push_back( originalChar );
				}
			}
			if ( bytes.size() == 0 )
			{
				bytes.push_back( 0 );
			}
		}
	}

	// total digits
	size_t GetDigits() const
	{
		return ( bytes.size() << 1 );
	}

	// Set digit by index
	void SetDigit( const size_t index, const uint8_t value )
	{
		if ( index & 0x01 )
		{
			bytes[ index >> 1 ] = ( ( bytes[ index >> 1 ] & 0x0F ) | ( value << 4 ) );
		}
		else
		{
			bytes[ index >> 1 ] = ( ( bytes[ index >> 1 ] & 0xF0 ) | ( value & 0x0F ) );
		}
	}

	// Get digit by index statring from less significant
	uint8_t GetDigit( const size_t index ) const
	{
		if ( index & 0x01 )
		{
			return ( bytes[ index >> 1 ] >> 4 );
		}
		else
		{
			return ( bytes[ index >> 1 ] & 0x0F );
		}
	}

	void fromUint64( const uint64_t number )
	{
		uint64_t absNumber = number;
		for ( uint8_t index = 0; index < 8; ++index )
		{
			bytes.push_back( absNumber & 0xFF );
			absNumber >>= 8;
		}
	}

public:

	BigInt()
	{
		bytes.push_back( 0 );
	}

	BigInt( const std::string number )
	{
		fromString( number );
	}

	BigInt( const int64_t number )
	{
		uint64_t absNumber = number;
		if ( number < 0 )
		{
			negative = true;
			absNumber = ( number & 0x7FFFFFFFFFFFFFFF ) + 1;
		}
		fromUint64( absNumber );
	}

	BigInt( const uint64_t number )
	{
		fromUint64( number );
	}

	void Print() const
	{
		std::cout << "number: " << ToString() << std::endl;
	}

	std::string ToString() const
	{
		std::string result;
		if ( negative )
		{
			// Negative
			result.push_back( '-' );
		}
		std::vector<uint8_t> digits;
		size_t nibble;
		uint8_t originalChar;
		uint8_t digit;
		digits.push_back( 0 );
		size_t index = ( bytes.size() << 1 );
		for ( ; index > 0; )
		{
			--index;
			originalChar = GetDigit( index );
			for ( nibble = 0; nibble < ( digits.size() << 1 ); ++nibble )
			{
				if ( nibble & 0x01 )
				{
					digit = ( digits[ nibble >> 1 ] >> 4 );
				}
				else
				{
					digit = ( digits[ nibble >> 1 ] & 0x0F );
				}

				digit = ( digit << 4 ) | originalChar;
				if ( digit > 9 )
				{
					originalChar = digit / 10;
					digit = digit - ( originalChar * 10 );
				}
				else
				{
					originalChar = 0;
				}
				if ( nibble & 0x01 )
				{
					digits[ nibble >> 1 ] = ( ( digits[ nibble >> 1 ] & 0x0F ) | ( digit << 4 ) );
				}
				else
				{
					digits[ nibble >> 1 ] = ( ( digits[ nibble >> 1 ] & 0xF0 ) | ( digit & 0x0F ) );
				}
			}
			if ( originalChar != 0 )
			{
				if ( originalChar > 9 )
				{
					digit = originalChar / 10;
					originalChar = originalChar - ( digit * 10 );
				}
				else
				{
					digit = 0;
				}
				digits.push_back((( digit << 4 ) | ( originalChar )));
			}
		}
		if ( digits.size() == 0 )
		{
			digits.push_back( 0 );
		}
		bool first = true;
		std::for_each( digits.rbegin(), digits.rend(), [ &result, &first ]( uint8_t v )
		{
			char val = ( v >> 4 ) + '0';
			if ( val != '0' || first == false )
			{
				result.push_back( val );
			}
			first = false;
			result.push_back( ( v & 0x0F ) + '0' );
		} );
		return result;
	}

	BigInt& operator=( const std::string number )
	{
		bytes.clear();
		negative = false;
		fromString( number );
		bytes.shrink_to_fit();
		return *this;
	}

	BigInt& operator+=( const BigInt& value )
	{
		size_t currentSize = bytes.size();
		size_t valueSize = value.bytes.size();
		size_t maxBytes = currentSize < valueSize ? valueSize : currentSize;
		uint32_t temp;
		bool overload = false;
		size_t index;

		for ( index = 0; index < maxBytes; ++index )
		{
			temp = 0;
			if ( index < currentSize )
			{
				temp = bytes[ index ];
			}
			if ( index < valueSize )
			{
				temp += value.bytes[ index ];
			}
			if ( overload )
			{
				++temp;
			}

			if ( index >= bytes.size())
			{
				bytes.push_back( 0 );
			}

			if ( temp & 0xF00 )
			{
				overload = true;
				bytes[ index ] = uint8_t( temp & 0xFF );
			}
			else
			{
				overload = false;
				bytes[ index ] = uint8_t( temp );
			}
		}

		if ( overload )
		{
			if ( index >= bytes.size())
			{
				bytes.push_back( 1 );
			}
			else
			{
				bytes[ index ] = 1;
			}
		}

		return *this;
	}

	friend BigInt operator+( BigInt left, const BigInt& right )
	{
		left += right;
		return left;
	}

};

void benchamark()
{
	std::string number = "9383631516348246482763476237452647981701029719318243918438762575662864958543784563754762537642739007240127341624512436125939429482731979237492734162736451";
	std::chrono::high_resolution_clock::time_point startTime;
	std::chrono::high_resolution_clock::time_point endTime;
	std::chrono::milliseconds result;

	// **************
	// Create test
	// **************
	startTime = std::chrono::high_resolution_clock::now();

	for ( int i = 0; i < 100000; ++i )
	{
		BigInt bi( number );
	}

	result = std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::high_resolution_clock::now() - startTime );
	std::cout << "Create BigInt " << result.count() << "ms" << std::endl;
	startTime = std::chrono::high_resolution_clock::now();

	for ( int i = 0; i < 100000; ++i )
	{
		BigIntNibble bin( number );
	}

	result = std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::high_resolution_clock::now() - startTime );
	std::cout << "Create BigIntNibble " << result.count() << "ms" << std::endl;

	// **************
	// Math test
	// **************
	startTime = std::chrono::high_resolution_clock::now();

	BigInt bi( number );
	BigInt bi1( number );

	for ( int i = 0; i < 1000000; ++i )
	{
		bi += bi1;
	}

	result = std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::high_resolution_clock::now() - startTime );
	std::cout << "Math BigInt " << result.count() << "ms" << std::endl;
	startTime = std::chrono::high_resolution_clock::now();

	BigIntNibble bin( number );
	BigIntNibble bin1( number );

	for ( int i = 0; i < 1000000; ++i )
	{
		bin += bin1;
	}

	result = std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::high_resolution_clock::now() - startTime );
	std::cout << "Math BigIntNibble " << result.count() << "ms" << std::endl;
}

/*
parse and create:
BigIntNibble faster than BigInt x60 times

create and parse:
BigInt faster than BigIntNibble x3 times

BigIntNibbele use 2% more memory than BigInt
*/

int main( int argc, char* argv[] )
{
	//benchamark();

	if ( argc != 3 )
	{
		std::cout << "usage: bigit.exe 42 42" << std::endl;
		std::cout << "where 42 - any integer number" << std::endl;
		return -1;
	}

	BigInt firstNumber, secondNumber;
	try
	{
		firstNumber = argv[ 1 ];
	}
	catch (std::bad_cast& e)
	{
		std::cout << "bad number in first parametr: " << argv[ 1 ] << std::endl;
		std::cout << e.what() << std::endl;
		return -1;
	}

	try
	{
		secondNumber = argv[ 2 ];
	}
	catch ( std::bad_cast& e )
	{
		std::cout << "bad number in second parametr: " << argv[ 2 ] << std::endl;
		std::cout << e.what() << std::endl;
		return -1;
	}

	std::cout << firstNumber.ToString() << " + " << secondNumber.ToString() << " = ";
	firstNumber += secondNumber;
	std::cout << firstNumber.ToString();

	return 0;
}
