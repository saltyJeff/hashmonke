#pragma once

#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

extern "C"
{
#include "../algo/md.h"
}

inline std::string to_hex(const char *data, size_t len)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i)
    {
        oss << std::setw(2) << (static_cast<unsigned int>(data[i]) & 0xFF);
    }
    return oss.str();
}

inline std::string to_hex(const uint8_t *data, size_t len)
{
    return to_hex(reinterpret_cast<const char *>(data), len);
}

inline std::vector<uint8_t> from_hex(const std::string &hex)
{
    std::vector<uint8_t> bytes;
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
    {
        unsigned int byte = 0;
        std::stringstream ss;
        ss << std::hex << hex.substr(i, 2);
        ss >> byte;
        bytes.push_back(static_cast<uint8_t>(byte));
    }
    return bytes;
}

inline std::string hash_string(struct hashmonke_md *md, const std::string &input)
{
    if (!md)
        return "";
    size_t dsize = hashmonke_md_digest_size(md);
    hashmonke_md_update_func(md, input.data(), input.size());
    const uint8_t *digest = hashmonke_md_final_func(md);
    std::string hex = to_hex(digest, dsize);
    free((void *)digest);
    return hex;
}
