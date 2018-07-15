/* sha1.h

Copyright (c) 2005 Michael D. Leonhard

http://tamale.net/

This file is licensed under the terms described in the
accompanying LICENSE file.
*/

#ifndef SHA1_HEADER
typedef unsigned int Uint32;

class SHA1
{
	private:
		// fields
		Uint32 H0, H1, H2, H3, H4;
		Uint32 W[80];
		unsigned char bytes[64];
		int unprocessedBytes;
		Uint32 size;
		void process();
	public:
		SHA1();
		~SHA1();
		void addBytes( const char* data, int num );
		struct Digest
		{
			Digest();
			bool operator == ( const Digest& d );
			bool operator != ( const Digest& d );

			Uint32 h0, h1, h2, h3, h4;
		};
		Digest result();
		void reset();
		// utility methods
		static inline Uint32 lrot( Uint32 x, int bits );
		static inline Uint32 toBigEndian( Uint32 num );
};

#define SHA1_HEADER
#endif
