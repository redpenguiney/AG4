#pragma once
#include <bit>
#include <assert.hpp>

// can be used for short, unsigned short, word, unsigned word (2-byte types)
#define BYTESWAP16(n) (((n&0xFF00)>>8)|((n&0x00FF)<<8))

// can be used for int or unsigned int or float (4-byte types)
#define BYTESWAP32(n) ((BYTESWAP16((n&0xFFFF0000)>>16))|((BYTESWAP16(n&0x0000FFFF))<<16))

// can be used for unsigned long long or double (8-byte types)
#define BYTESWAP64(n) ((BYTESWAP32((n&0xFFFFFFFF00000000)>>32))|((BYTESWAP32(n&0x00000000FFFFFFFF))<<32))

template<typename T>
T Deserialize(uint8_t*& currentLocation, int& bytesRemaining) = delete;

template <>
int Deserialize(uint8_t*& currentLocation, int& bytesRemaining) {
	Assert(bytesRemaining >= sizeof(int));
	int out;
	memcpy(&out, currentLocation, sizeof(int));
	currentLocation += sizeof(int);
	bytesRemaining -= sizeof(int);
	if constexpr (std::endian::native == std::endian::little) {
		out = BYTESWAP32(out);
	}
	return out;
}

template <>
uint8_t Deserialize(uint8_t*& currentLocation, int& bytesRemaining) {
	uint8_t out = *currentLocation;
	currentLocation += sizeof(uint8_t);
	bytesRemaining -= sizeof(uint8_t);
	return out;
}