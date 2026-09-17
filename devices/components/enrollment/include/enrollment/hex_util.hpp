#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <span>

namespace dev {

inline std::string hex_encode(std::span<const uint8_t> data) {
	static const char* digits = "0123456789abcdef";
	std::string out;
	out.reserve(data.size() * 2);
	for (uint8_t b : data) {
		out.push_back(digits[b >> 4]);
		out.push_back(digits[b & 0x0F]);
	}

	return out;
}

inline std::vector<uint8_t> hex_decode(const std::string& hex) {
	std::vector<uint8_t> out;
	out.reserve(hex.size() / 2);
	for (size_t i = 0; i + 1 < hex.size(); i += 2) {
		uint8_t byte = static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16));
		out.push_back(byte);
	}

	return out;
}

} // namespace dev
