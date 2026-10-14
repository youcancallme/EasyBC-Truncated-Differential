

#ifndef JSON_FORWARD_AMALGAMATED_H_INCLUDED
#define JSON_FORWARD_AMALGAMATED_H_INCLUDED

#define JSON_IS_AMALGAMATION

#ifndef JSON_VERSION_H_INCLUDED
#define JSON_VERSION_H_INCLUDED

#define JSONCPP_VERSION_STRING "1.9.7"
#define JSONCPP_VERSION_MAJOR 1
#define JSONCPP_VERSION_MINOR 9
#define JSONCPP_VERSION_PATCH 7
#define JSONCPP_VERSION_QUALIFIER
#define JSONCPP_VERSION_HEXA                                         \
    ((JSONCPP_VERSION_MAJOR << 24) | (JSONCPP_VERSION_MINOR << 16) | \
     (JSONCPP_VERSION_PATCH << 8))

#if !defined(JSONCPP_USE_SECURE_MEMORY)
#define JSONCPP_USE_SECURE_MEMORY 0
#endif

#endif

#ifndef JSON_ALLOCATOR_H_INCLUDED
#define JSON_ALLOCATOR_H_INCLUDED

#include <algorithm>
#include <cstring>
#include <memory>

#pragma pack(push)
#pragma pack()

namespace Json {
template <typename T>
class SecureAllocator {
  public:
    using value_type = T;
    using pointer = T *;
    using const_pointer = const T *;
    using reference = T &;
    using const_reference = const T &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    pointer allocate(size_type n) {

        return static_cast<pointer>(::operator new(n * sizeof(T)));
    }

    void deallocate(pointer p, size_type n) {

#if defined(HAVE_MEMSET_S)
        memset_s(p, n * sizeof(T), 0, n * sizeof(T));
#elif defined(_WIN32)
        RtlSecureZeroMemory(p, n * sizeof(T));
#else
        std::fill_n(reinterpret_cast<volatile unsigned char *>(p), n, 0);
#endif

        ::operator delete(p);
    }

    template <typename... Args>
    void construct(pointer p, Args &&...args) {

        ::new (static_cast<void *>(p)) T(std::forward<Args>(args)...);
    }

    size_type max_size() const { return size_t(-1) / sizeof(T); }

    pointer address(reference x) const { return std::addressof(x); }

    const_pointer address(const_reference x) const { return std::addressof(x); }

    void destroy(pointer p) {

        p->~T();
    }

    SecureAllocator() {}
    template <typename U>
    SecureAllocator(const SecureAllocator<U> &) {}
    template <typename U>
    struct rebind {
        using other = SecureAllocator<U>;
    };
};

template <typename T, typename U>
bool operator==(const SecureAllocator<T> &, const SecureAllocator<U> &) {
    return true;
}

template <typename T, typename U>
bool operator!=(const SecureAllocator<T> &, const SecureAllocator<U> &) {
    return false;
}

}

#pragma pack(pop)

#endif

#ifndef JSON_CONFIG_H_INCLUDED
#define JSON_CONFIG_H_INCLUDED
#include <cstddef>
#include <cstdint>
#include <istream>
#include <memory>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>

#ifndef JSON_USE_EXCEPTION
#define JSON_USE_EXCEPTION 1
#endif

#ifndef JSON_USE_NULLREF
#define JSON_USE_NULLREF 1
#endif

#if defined(JSON_DLL_BUILD)
#if defined(_MSC_VER) || defined(__MINGW32__)
#define JSON_API __declspec(dllexport)
#define JSONCPP_DISABLE_DLL_INTERFACE_WARNING
#elif defined(__GNUC__) || defined(__clang__)
#define JSON_API __attribute__((visibility("default")))
#endif

#elif defined(JSON_DLL)
#if defined(_MSC_VER) || defined(__MINGW32__)
#define JSON_API __declspec(dllimport)
#define JSONCPP_DISABLE_DLL_INTERFACE_WARNING
#endif
#endif

#if !defined(JSON_API)
#define JSON_API
#endif

#if defined(_MSC_VER) && _MSC_VER < 1800
#error \
    "ERROR:  Visual Studio 12 (2013) with _MSC_VER=1800 is the oldest supported compiler with sufficient C++11 capabilities"
#endif

#if defined(_MSC_VER) && _MSC_VER < 1900

extern JSON_API int msvc_pre1900_c99_snprintf(char *outBuf, size_t size,
                                              const char *format, ...);
#define jsoncpp_snprintf msvc_pre1900_c99_snprintf
#else
#define jsoncpp_snprintf std::snprintf
#endif

#define JSONCPP_OVERRIDE override

#ifdef __clang__
#if __has_extension(attribute_deprecated_with_message)
#define JSONCPP_DEPRECATED(message) __attribute__((deprecated(message)))
#endif
#elif defined(__GNUC__)
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#define JSONCPP_DEPRECATED(message) __attribute__((deprecated(message)))
#elif (__GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1))
#define JSONCPP_DEPRECATED(message) __attribute__((__deprecated__))
#endif
#elif defined(_MSC_VER)

#define JSONCPP_DEPRECATED(message) __declspec(deprecated(message))
#endif

#if !defined(JSONCPP_DEPRECATED)
#define JSONCPP_DEPRECATED(message)
#endif

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >= 6))
#define JSON_USE_INT64_DOUBLE_CONVERSION 1
#endif

#if !defined(JSON_IS_AMALGAMATION)

#include "allocator.h"
#include "version.h"

#endif

namespace Json {
using Int = int;
using UInt = unsigned int;
#if defined(JSON_NO_INT64)
using LargestInt = int;
using LargestUInt = unsigned int;
#undef JSON_HAS_INT64
#else

#if defined(_MSC_VER)
using Int64 = __int64;
using UInt64 = unsigned __int64;
#else
using Int64 = int64_t;
using UInt64 = uint64_t;
#endif
using LargestInt = Int64;
using LargestUInt = UInt64;
#define JSON_HAS_INT64
#endif

template <typename T>
using Allocator =
    typename std::conditional<JSONCPP_USE_SECURE_MEMORY, SecureAllocator<T>,
                              std::allocator<T>>::type;
using String = std::basic_string<char, std::char_traits<char>, Allocator<char>>;
using IStringStream =
    std::basic_istringstream<String::value_type, String::traits_type,
                             String::allocator_type>;
using OStringStream =
    std::basic_ostringstream<String::value_type, String::traits_type,
                             String::allocator_type>;
using IStream = std::istream;
using OStream = std::ostream;
}

using JSONCPP_STRING = Json::String;
using JSONCPP_ISTRINGSTREAM = Json::IStringStream;
using JSONCPP_OSTRINGSTREAM = Json::OStringStream;
using JSONCPP_ISTREAM = Json::IStream;
using JSONCPP_OSTREAM = Json::OStream;

#endif

#ifndef JSON_FORWARDS_H_INCLUDED
#define JSON_FORWARDS_H_INCLUDED

#if !defined(JSON_IS_AMALGAMATION)
#include "config.h"
#endif

namespace Json {

class StreamWriter;
class StreamWriterBuilder;
class Writer;
class FastWriter;
class StyledWriter;
class StyledStreamWriter;

class Reader;
class CharReader;
class CharReaderBuilder;

class Features;

using ArrayIndex = unsigned int;
class StaticString;
class Path;
class PathArgument;
class Value;
class ValueIteratorBase;
class ValueIterator;
class ValueConstIterator;

}

#endif

#endif
