#pragma once
#include <bit>
#include <assert.hpp>
#include <vector>
#include <glm/vec3.hpp>
#include <glm/gtx/quaternion.hpp>

// can be used for short, unsigned short, word, unsigned word (2-byte types)
#define BYTESWAP16(n) (((n&0xFF00)>>8)|((n&0x00FF)<<8))

// can be used for int or unsigned int or float (4-byte types)
#define BYTESWAP32(n) ((BYTESWAP16((n&0xFFFF0000)>>16))|((BYTESWAP16(n&0x0000FFFF))<<16))

// can be used for unsigned long long or double (8-byte types)
#define BYTESWAP64(n) ((BYTESWAP32((n&0xFFFFFFFF00000000)>>32))|((BYTESWAP32(n&0x00000000FFFFFFFF))<<32))

template <class T>
struct is_vector : std::false_type {};

template <class T>
struct is_vector<std::vector<T>> : std::true_type {};

template<typename T>
T Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) = delete;

template <>
inline size_t Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
	if (bytesRemaining < sizeof(size_t)) throw std::runtime_error("not enough data to read!");
	size_t out;
	memcpy(&out, currentLocation, sizeof(size_t));
	currentLocation += sizeof(size_t);
	bytesRemaining -= sizeof(size_t);
	if constexpr (std::endian::native == std::endian::little) {
		out = BYTESWAP64(out);
	}
	return out;
}

template <>
inline int Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
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
inline unsigned int Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
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
inline uint8_t Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
	if (bytesRemaining < sizeof(uint8_t)) throw std::runtime_error("not enough data to read!");
	uint8_t out = *currentLocation;
	currentLocation += sizeof(uint8_t);
	bytesRemaining -= sizeof(uint8_t);
	return out;
}

template <>
inline bool Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
	if (bytesRemaining < sizeof(uint8_t)) throw std::runtime_error("not enough data to read!");
	uint8_t out = *currentLocation;
	currentLocation += sizeof(uint8_t);
	bytesRemaining -= sizeof(uint8_t);
	if (out == 0) return false; else return true;
}

template <>
inline std::string Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
	if (bytesRemaining < sizeof(size_t)) throw std::runtime_error("not enough data to read!");
	size_t length = Deserialize<size_t>(currentLocation, bytesRemaining);
	if (bytesRemaining < length) throw std::runtime_error("not enough data to read!");
	std::string out(reinterpret_cast<char*>(currentLocation), length);
	currentLocation += length;
	bytesRemaining -= length;
	return out;
}

template <>
inline float Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
	if (bytesRemaining < sizeof(float)) throw std::runtime_error("not enough data to read!");
	float out;
	memcpy(&out, currentLocation, sizeof(float));
	currentLocation += sizeof(float);
	bytesRemaining -= sizeof(float);
	if constexpr (std::endian::native == std::endian::little) {
		out = std::bit_cast<float>(BYTESWAP32(std::bit_cast<uint32_t>(out)));
	}
	return out;
}

template <>
inline double Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
	if (bytesRemaining < sizeof(double)) throw std::runtime_error("not enough data to read!");
	double out;
	memcpy(&out, currentLocation, sizeof(double));
	currentLocation += sizeof(double);
	bytesRemaining -= sizeof(double);
	if constexpr (std::endian::native == std::endian::little) {
		out = std::bit_cast<double>(BYTESWAP64(std::bit_cast<uint64_t>(out)));
	}
	return out;
}

template <>
inline glm::vec3 Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
	glm::vec3 out;
	out.x = Deserialize<float>(currentLocation, bytesRemaining);
	out.y = Deserialize<float>(currentLocation, bytesRemaining);
	out.z = Deserialize<float>(currentLocation, bytesRemaining);
	return out;
}

template <>
inline glm::dvec3 Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
	glm::dvec3 out;
	out.x = Deserialize<double>(currentLocation, bytesRemaining);
	out.y = Deserialize<double>(currentLocation, bytesRemaining);
	out.z = Deserialize<double>(currentLocation, bytesRemaining);
	return out;
}

template <>
inline glm::quat Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
	glm::quat out;
	out.x = Deserialize<float>(currentLocation, bytesRemaining);
	out.y = Deserialize<float>(currentLocation, bytesRemaining);
	out.z = Deserialize<float>(currentLocation, bytesRemaining);
	out.w = Deserialize<float>(currentLocation, bytesRemaining);
	return out;
}



template <typename T> requires is_vector<T>::value
inline T Deserialize(uint8_t*& currentLocation, size_t& bytesRemaining) {
	size_t length = Deserialize<size_t>(currentLocation, bytesRemaining);
	T out;
	out.reserve(length);
	for (size_t i = 0; i < length; i++) {
		out.push_back(Deserialize<typename T::value_type>(currentLocation, bytesRemaining));
	}
	return out;
}

template <typename T>
inline size_t SerializedSize(const T& v) {
	return sizeof(v);
}

template <>
inline size_t SerializedSize(const std::string& v) {
	return sizeof(size_t) + v.length();
}

template <typename T>
size_t SerializedSize(const std::vector<T>& v) {
	return sizeof(size_t) + v.size() * sizeof(T);
}

// it is assumed that there is enough space to copy in because you should have allocated the buffer currentLocation resides in using a SerializedSize() fold expression or something
template <typename T>
void Serialize(const T& valToSerialize, uint8_t*& currentLocation) = delete;

template <>
inline void Serialize(const size_t& val, uint8_t*& currentLocation) {
	size_t in = val;
	if constexpr (std::endian::native == std::endian::little) {
		in = BYTESWAP64(in);
	}
	memcpy(currentLocation, &in, sizeof(in));
	currentLocation += sizeof(in);
}

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

template <>
inline void Serialize(const bool& val, uint8_t*& currentLocation) {
	uint8_t in = val ? 1 : 0;
	memcpy(currentLocation, &in, sizeof(in));
	currentLocation += sizeof(in);
}

template <>
inline void Serialize(const std::string& val, uint8_t*& currentLocation) {
	Serialize<size_t>(val.length(), currentLocation);
	memcpy(currentLocation, val.data(), val.length());
	currentLocation += val.length();
}

template <>
inline void Serialize(const float& val, uint8_t*& currentLocation) {
	float in = val;
	if constexpr (std::endian::native == std::endian::little) {
		in = std::bit_cast<float>(BYTESWAP32(std::bit_cast<uint32_t>(in)));
	}
	memcpy(currentLocation, &in, sizeof(in));
	currentLocation += sizeof(in);
}

template <>
inline void Serialize(const double& val, uint8_t*& currentLocation) {
	double in = val;
	if constexpr (std::endian::native == std::endian::little) {
		in = std::bit_cast<double>(BYTESWAP64(std::bit_cast<uint64_t>(in)));
	}
	memcpy(currentLocation, &in, sizeof(in));
	currentLocation += sizeof(in);
}

template <>
inline void Serialize(const glm::vec3& val, uint8_t*& currentLocation) {
	Serialize(val.x, currentLocation);
	Serialize(val.y, currentLocation);
	Serialize(val.z, currentLocation);
}

template <>
inline void Serialize(const glm::dvec3& val, uint8_t*& currentLocation) {
	Serialize(val.x, currentLocation);
	Serialize(val.y, currentLocation);
	Serialize(val.z, currentLocation);
}

template <>
inline void Serialize(const glm::quat& val, uint8_t*& currentLocation) {
	Serialize(val.x, currentLocation);
	Serialize(val.y, currentLocation);
	Serialize(val.z, currentLocation);
	Serialize(val.w, currentLocation);
}

template <typename T> requires is_vector<T>::value
inline void Serialize(const T& val, uint8_t*& currentLocation) {
	Serialize<size_t>(val.size(), currentLocation);
	for (size_t i = 0; i < val.size(); i++)
		Serialize<typename T::value_type>(val[i], currentLocation);
}