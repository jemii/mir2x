/*
 * =====================================================================================
 *
 *       Filename: totype.hpp
 *        Created: 03/07/2020 17:07:46
 *    Description: 
 *
 *        Version: 1.0
 *       Revision: none
 *       Compiler: gcc
 *
 *         Author: ANHONG
 *          Email: anhonghe@gmail.com
 *   Organization: USTC
 *
 * =====================================================================================
 */

#pragma once
#include <string>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string_view>

#define to_d(x) static_cast<int>(x)
#define to_u(x) static_cast<unsigned int>(x)
#define to_f(x) static_cast<float>(x)

#define to_lld(x) static_cast<         long long>(x)
#define to_llu(x) static_cast<unsigned long long>(x)

#define to_uz( x) static_cast<size_t  >(x)
#define to_u8( x) static_cast<uint8_t >(x)
#define to_u16(x) static_cast<uint16_t>(x)
#define to_u32(x) static_cast<uint32_t>(x)
#define to_u64(x) static_cast<uint64_t>(x)

#define to_cvptr(x) static_cast<const void *>(x)

template<typename T> T as_type(const void *buf)
{
    static_assert(std::is_trivially_copyable_v<T>);

    T t;
    std::memcpy(&t, buf, sizeof(t));
    return t;
}

inline uint16_t as_u16(const void *buf) { return as_type<uint16_t>(buf); }
inline uint32_t as_u32(const void *buf) { return as_type<uint32_t>(buf); }
inline uint64_t as_u64(const void *buf) { return as_type<uint64_t>(buf); }

inline const char * to_cstr(const char *s)
{
    if(s == nullptr){
        return "(null)";
    }
    else if(s[0] == '\0'){
        return "(empty)";
    }
    else{
        return s;
    }
}

inline const char *to_cstr(const unsigned char *s)
{
    return reinterpret_cast<const char *>(s);
}

inline const char *to_cstr(const char8_t *s)
{
    return reinterpret_cast<const char *>(s);
}

inline const char *to_cstr(const std::string &s)
{
    return to_cstr(s.c_str());
}

inline const char *to_cstr(const std::u8string &s)
{
    return to_cstr(s.c_str());
}

// cast char buf to char8_t buf
// this may break the strict-aliasing rule

inline const char8_t * to_u8cstr(const char8_t *s)
{
    if(s == nullptr){
        return u8"(null)";
    }
    else if(s[0] == '\0'){
        return u8"(empty)";
    }
    else{
        return s;
    }
}

inline const char8_t *to_u8cstr(const unsigned char *s)
{
    return reinterpret_cast<const char8_t *>(s);
}

inline const char8_t *to_u8cstr(const char *s)
{
    return reinterpret_cast<const char8_t *>(s);
}

inline const char8_t *to_u8cstr(const std::u8string &s)
{
    return to_u8cstr(s.c_str());
}

inline const char8_t *to_u8cstr(const std::string &s)
{
    return to_u8cstr(s.c_str());
}

inline const char *to_boolcstr(bool b)
{
    return b ? "true" : "false";
}

inline const char8_t *to_boolu8cstr(bool b)
{
    return b ? u8"true" : u8"false";
}

template<typename T> auto to_sv(const T &t)
{
    return std::string_view(to_cstr(t));
}

template<typename T> auto to_u8sv(const T &t)
{
    return std::u8string_view(to_u8cstr(t));
}

template<typename T, typename F> static T check_cast(F from)
{
    auto to = static_cast<T>(from);
    if(static_cast<F>(to) != from){
        throw std::runtime_error("cast fails to preserve original value");
    }
    return to;
}
