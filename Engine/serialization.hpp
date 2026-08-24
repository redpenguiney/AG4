#pragma once
#include <bit>
#include <assert.hpp>
#include <vector>

// can be used for short, unsigned short, word, unsigned word (2-byte types)
#define BYTESWAP16(n) (((n&0xFF00)>>8)|((n&0x00FF)<<8))

// can be used for int or unsigned int or float (4-byte types)
#define BYTESWAP32(n) ((BYTESWAP16((n&0xFFFF0000)>>16))|((BYTESWAP16(n&0x0000FFFF))<<16))

// can be used for unsigned long long or double (8-byte types)
#define BYTESWAP64(n) ((BYTESWAP32((n&0xFFFFFFFF00000000)>>32))|((BYTESWAP32(n&0x00000000FFFFFFFF))<<32))

template<typename T>
T Deserialize(uint8_t*& currentLocation, int& bytesRemaining) = delete;

template <>
inline int Deserialize(uint8_t*& currentLocation, int& bytesRemaining) {
	if (bytesRemaining < sizeof(int)) throw std::runtime_error("not enough data to read!");
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
inline unsigned int Deserialize(uint8_t*& currentLocation, int& bytesRemaining) {
	if (bytesRemaining < sizeof(unsigned int)) throw std::runtime_error("not enough data to read!");
	unsigned int out;
	memcpy(&out, currentLocation, sizeof(unsigned int));
	currentLocation += sizeof(unsigned int);
	bytesRemaining -= sizeof(unsigned int);
	if constexpr (std::endian::native == std::endian::little) {
		out = BYTESWAP32(out);
	}
	return out;
}

template <>
inline uint8_t Deserialize(uint8_t*& currentLocation, int& bytesRemaining) {
	if (bytesRemaining < sizeof(uint8_t)) throw std::runtime_error("not enough data to read!");
	uint8_t out = *currentLocation;
	currentLocation += sizeof(uint8_t);
	bytesRemaining -= sizeof(uint8_t);
	return out;
}

template <typename T>
size_t SerializedSize(const T& v) {
	return sizeof(v);
}

template <typename T>
size_t SerializedSize(const std::vector<T>& v) {
	return sizeof(size_t) + v.size() * sizeof(T);
}

// it is assumed that there is enough space to copy in because you should have allocated the buffer currentLocation resides in using a SerializedSize() fold expression or something
template <typename T>
void Serialize(const T& valToSerialize, uint8_t*& currentLocation) = delete;

template <>
inline void Serialize(const int& val, uint8_t*& currentLocation) {
	int in = val;
	if constexpr (std::endian::native == std::endian::little) {
		in = BYTESWAP32(in);
	}
	memcpy(currentLocation, &in, sizeof(in));
	currentLocation += sizeof(in);
}

template <>
inline void Serialize(const unsigned int& val, uint8_t*& currentLocation) {
	unsigned int in = val;
	if constexpr (std::endian::native == std::endian::little) {
		in = BYTESWAP32(in);
	}
	memcpy(currentLocation, &in, sizeof(in));
	currentLocation += sizeof(in);
}

template <>
inline void Serialize(const uint8_t& val, uint8_t*& currentLocation) {
	uint8_t in = val;
	memcpy(currentLocation, &in, sizeof(in));
	currentLocation += sizeof(in);
}