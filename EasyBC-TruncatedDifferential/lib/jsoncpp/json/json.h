

#ifndef JSON_AMALGAMATED_H_INCLUDED
#define JSON_AMALGAMATED_H_INCLUDED

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

#ifndef JSON_FEATURES_H_INCLUDED
#define JSON_FEATURES_H_INCLUDED

#if !defined(JSON_IS_AMALGAMATION)
#include "forwards.h"
#endif

#pragma pack(push)
#pragma pack()

namespace Json {

class JSON_API Features {
  public:
    static Features all();

    static Features strictMode();

    Features();

    bool allowComments_{true};

    bool strictRoot_{false};

    bool allowDroppedNullPlaceholders_{false};

    bool allowNumericKeys_{false};
};

}

#pragma pack(pop)

#endif

#ifndef JSON_VALUE_H_INCLUDED
#define JSON_VALUE_H_INCLUDED

#if !defined(JSON_IS_AMALGAMATION)
#include "forwards.h"
#endif

#if !defined(JSONCPP_NORETURN)
#if defined(_MSC_VER) && _MSC_VER == 1800
#define JSONCPP_NORETURN __declspec(noreturn)
#else
#define JSONCPP_NORETURN [[noreturn]]
#endif
#endif

#if !defined(JSONCPP_TEMPLATE_DELETE)
#if defined(__clang__) && defined(__apple_build_version__)
#if __apple_build_version__ <= 8000042
#define JSONCPP_TEMPLATE_DELETE
#endif
#elif defined(__clang__)
#if __clang_major__ == 3 && __clang_minor__ <= 8
#define JSONCPP_TEMPLATE_DELETE
#endif
#endif
#if !defined(JSONCPP_TEMPLATE_DELETE)
#define JSONCPP_TEMPLATE_DELETE = delete
#endif
#endif

#if __cplusplus >= 201703L
#define JSONCPP_HAS_STRING_VIEW 1
#endif

#include <array>
#include <exception>
#include <map>
#include <memory>
#include <string>
#include <vector>

#ifdef JSONCPP_HAS_STRING_VIEW
#include <string_view>
#endif

#if defined(JSONCPP_DISABLE_DLL_INTERFACE_WARNING)
#pragma warning(push)
#pragma warning(disable : 4251 4275)
#endif

#pragma pack(push)
#pragma pack()

namespace Json {

#if JSON_USE_EXCEPTION

class JSON_API Exception : public std::exception {
  public:
    Exception(String msg);
    ~Exception() noexcept override;
    char const *what() const noexcept override;

  protected:
    String msg_;
};

class JSON_API RuntimeError : public Exception {
  public:
    RuntimeError(String const &msg);
};

class JSON_API LogicError : public Exception {
  public:
    LogicError(String const &msg);
};
#endif

JSONCPP_NORETURN void throwRuntimeError(String const &msg);

JSONCPP_NORETURN void throwLogicError(String const &msg);

enum ValueType {
    nullValue = 0,
    intValue,
    uintValue,
    realValue,
    stringValue,
    booleanValue,
    arrayValue,
    objectValue
};

enum CommentPlacement {
    commentBefore = 0,
    commentAfterOnSameLine,
    commentAfter,

    numberOfCommentPlacement
};

enum PrecisionType {
    significantDigits = 0,
    decimalPlaces
};

class JSON_API StaticString {
  public:
    explicit StaticString(const char *czstring) : c_str_(czstring) {}

    operator const char *() const { return c_str_; }

    const char *c_str() const { return c_str_; }

  private:
    const char *c_str_;
};

class JSON_API Value {
    friend class ValueIteratorBase;

  public:
    using Members = std::vector<String>;
    using iterator = ValueIterator;
    using const_iterator = ValueConstIterator;
    using UInt = Json::UInt;
    using Int = Json::Int;
#if defined(JSON_HAS_INT64)
    using UInt64 = Json::UInt64;
    using Int64 = Json::Int64;
#endif
    using LargestInt = Json::LargestInt;
    using LargestUInt = Json::LargestUInt;
    using ArrayIndex = Json::ArrayIndex;

    using value_type = std::string;

#if JSON_USE_NULLREF

    static const Value &null;
    static const Value &nullRef;
#endif

    static Value const &nullSingleton();

    static constexpr LargestInt minLargestInt =
        LargestInt(~(LargestUInt(-1) / 2));

    static constexpr LargestInt maxLargestInt = LargestInt(LargestUInt(-1) / 2);

    static constexpr LargestUInt maxLargestUInt = LargestUInt(-1);

    static constexpr Int minInt = Int(~(UInt(-1) / 2));

    static constexpr Int maxInt = Int(UInt(-1) / 2);

    static constexpr UInt maxUInt = UInt(-1);

#if defined(JSON_HAS_INT64)

    static constexpr Int64 minInt64 = Int64(~(UInt64(-1) / 2));

    static constexpr Int64 maxInt64 = Int64(UInt64(-1) / 2);

    static constexpr UInt64 maxUInt64 = UInt64(-1);
#endif

    static constexpr UInt defaultRealPrecision = 17;

    static constexpr double maxUInt64AsDouble = 18446744073709551615.0;

#ifdef __NVCC__
  public:
#else
  private:
#endif
#ifndef JSONCPP_DOC_EXCLUDE_IMPLEMENTATION
    class CZString {
      public:
        enum DuplicationPolicy { noDuplication = 0,
                                 duplicate,
                                 duplicateOnCopy };
        CZString(ArrayIndex index);
        CZString(char const *str, unsigned length, DuplicationPolicy allocate);
        CZString(CZString const &other);
        CZString(CZString &&other) noexcept;
        ~CZString();
        CZString &operator=(const CZString &other);
        CZString &operator=(CZString &&other) noexcept;

        bool operator<(CZString const &other) const;
        bool operator==(CZString const &other) const;
        ArrayIndex index() const;

        char const *data() const;
        unsigned length() const;
        bool isStaticString() const;

      private:
        void swap(CZString &other);

        struct StringStorage {
            unsigned policy_ : 2;
            unsigned length_ : 30;
        };

        char const *cstr_;
        union {
            ArrayIndex index_;
            StringStorage storage_;
        };
    };

  public:
    typedef std::map<CZString, Value> ObjectValues;
#endif

  public:
    Value(ValueType type = nullValue);
    Value(Int value);
    Value(UInt value);
#if defined(JSON_HAS_INT64)
    Value(Int64 value);
    Value(UInt64 value);
#endif
    Value(double value);
    Value(const char *value);
    Value(const char *begin, const char *end);

    Value(const StaticString &value);
    Value(const String &value);
#ifdef JSONCPP_HAS_STRING_VIEW
    Value(std::string_view value);
#endif
    Value(bool value);
    Value(std::nullptr_t ptr) = delete;
    Value(const Value &other);
    Value(Value &&other) noexcept;
    ~Value();

    Value &operator=(const Value &other);
    Value &operator=(Value &&other) noexcept;

    void swap(Value &other);

    void swapPayload(Value &other);

    void copy(const Value &other);

    void copyPayload(const Value &other);

    ValueType type() const;

    bool operator<(const Value &other) const;
    bool operator<=(const Value &other) const;
    bool operator>=(const Value &other) const;
    bool operator>(const Value &other) const;
    bool operator==(const Value &other) const;
    bool operator!=(const Value &other) const;
    int compare(const Value &other) const;

    const char *asCString() const;
#if JSONCPP_USE_SECURE_MEMORY
    unsigned getCStringLength() const;

#endif
    String asString() const;

    bool getString(char const **begin, char const **end) const;
#ifdef JSONCPP_HAS_STRING_VIEW

    bool getString(std::string_view *str) const;
#endif
    Int asInt() const;
    UInt asUInt() const;
#if defined(JSON_HAS_INT64)
    Int64 asInt64() const;
    UInt64 asUInt64() const;
#endif
    LargestInt asLargestInt() const;
    LargestUInt asLargestUInt() const;
    float asFloat() const;
    double asDouble() const;
    bool asBool() const;

    bool isNull() const;
    bool isBool() const;
    bool isInt() const;
    bool isInt64() const;
    bool isUInt() const;
    bool isUInt64() const;
    bool isIntegral() const;
    bool isDouble() const;
    bool isNumeric() const;
    bool isString() const;
    bool isArray() const;
    bool isObject() const;

    template <typename T>
    T as() const JSONCPP_TEMPLATE_DELETE;
    template <typename T>
    bool is() const JSONCPP_TEMPLATE_DELETE;

    bool isConvertibleTo(ValueType other) const;

    ArrayIndex size() const;

    bool empty() const;

    explicit operator bool() const;

    void clear();

    void resize(ArrayIndex newSize);

    Value &operator[](ArrayIndex index);
    Value &operator[](int index);

    const Value &operator[](ArrayIndex index) const;
    const Value &operator[](int index) const;

    Value get(ArrayIndex index, const Value &defaultValue) const;

    bool isValidIndex(ArrayIndex index) const;

    Value &append(const Value &value);
    Value &append(Value &&value);

    bool insert(ArrayIndex index, const Value &newValue);
    bool insert(ArrayIndex index, Value &&newValue);

#ifdef JSONCPP_HAS_STRING_VIEW

    Value &operator[](std::string_view key);

    const Value &operator[](std::string_view key) const;
#else

    Value &operator[](const char *key);

    const Value &operator[](const char *key) const;

    Value &operator[](const String &key);

    const Value &operator[](const String &key) const;
#endif

    Value &operator[](const StaticString &key);
#ifdef JSONCPP_HAS_STRING_VIEW

    Value get(std::string_view key, const Value &defaultValue) const;
#else

    Value get(const char *key, const Value &defaultValue) const;

    Value get(const String &key, const Value &defaultValue) const;
#endif

    Value get(const char *begin, const char *end,
              const Value &defaultValue) const;

    Value const *find(char const *begin, char const *end) const;

    Value const *find(const String &key) const;

    template <typename T, bool (T::*TMemFn)() const>
    Value const *findValue(const String &key) const {
        Value const *found = find(key);
        if (!found || !(found->*TMemFn)())
            return nullptr;
        return found;
    }

    Value const *findNull(const String &key) const;
    Value const *findBool(const String &key) const;
    Value const *findInt(const String &key) const;
    Value const *findInt64(const String &key) const;
    Value const *findUInt(const String &key) const;
    Value const *findUInt64(const String &key) const;
    Value const *findIntegral(const String &key) const;
    Value const *findDouble(const String &key) const;
    Value const *findNumeric(const String &key) const;
    Value const *findString(const String &key) const;
    Value const *findArray(const String &key) const;
    Value const *findObject(const String &key) const;

    Value *demand(char const *begin, char const *end);

#if JSONCPP_HAS_STRING_VIEW
    void removeMember(std::string_view key);
#else
    void removeMember(const char *key);

    void removeMember(const String &key);
#endif

#if JSONCPP_HAS_STRING_VIEW
    bool removeMember(std::string_view key, Value *removed);
#else
    bool removeMember(String const &key, Value *removed);

    bool removeMember(const char *key, Value *removed);
#endif

    bool removeMember(const char *begin, const char *end, Value *removed);

    bool removeIndex(ArrayIndex index, Value *removed);

#ifdef JSONCPP_HAS_STRING_VIEW

    bool isMember(std::string_view key) const;
#else

    bool isMember(const char *key) const;

    bool isMember(const String &key) const;
#endif

    bool isMember(const char *begin, const char *end) const;

    Members getMemberNames() const;

    JSONCPP_DEPRECATED("Use setComment(String const&) instead.")
    void setComment(const char *comment, CommentPlacement placement) {
        setComment(String(comment, strlen(comment)), placement);
    }

    void setComment(const char *comment, size_t len, CommentPlacement placement) {
        setComment(String(comment, len), placement);
    }

    void setComment(String comment, CommentPlacement placement);
    bool hasComment(CommentPlacement placement) const;

    String getComment(CommentPlacement placement) const;

    String toStyledString() const;

    const_iterator begin() const;
    const_iterator end() const;

    iterator begin();
    iterator end();

    const Value &front() const;

    Value &front();

    const Value &back() const;

    Value &back();

    void setOffsetStart(ptrdiff_t start);
    void setOffsetLimit(ptrdiff_t limit);
    ptrdiff_t getOffsetStart() const;
    ptrdiff_t getOffsetLimit() const;

  private:
    void setType(ValueType v) {
        bits_.value_type_ = static_cast<unsigned char>(v);
    }
    bool isAllocated() const { return bits_.allocated_; }
    void setIsAllocated(bool v) { bits_.allocated_ = v; }

    void initBasic(ValueType type, bool allocated = false);
    void dupPayload(const Value &other);
    void releasePayload();
    void dupMeta(const Value &other);

    Value &resolveReference(const char *key);
    Value &resolveReference(const char *key, const char *end);

    union ValueHolder {
        LargestInt int_;
        LargestUInt uint_;
        double real_;
        bool bool_;
        char *string_;
        ObjectValues *map_;
    } value_;

    struct {

        unsigned int value_type_ : 8;

        unsigned int allocated_ : 1;
    } bits_;

    class Comments {
      public:
        Comments() = default;
        Comments(const Comments &that);
        Comments(Comments &&that) noexcept;
        Comments &operator=(const Comments &that);
        Comments &operator=(Comments &&that) noexcept;
        bool has(CommentPlacement slot) const;
        String get(CommentPlacement slot) const;
        void set(CommentPlacement slot, String comment);

      private:
        using Array = std::array<String, numberOfCommentPlacement>;
        std::unique_ptr<Array> ptr_;
    };
    Comments comments_;

    ptrdiff_t start_;
    ptrdiff_t limit_;
};

template <>
inline bool Value::as<bool>() const { return asBool(); }
template <>
inline bool Value::is<bool>() const { return isBool(); }

template <>
inline Int Value::as<Int>() const { return asInt(); }
template <>
inline bool Value::is<Int>() const { return isInt(); }

template <>
inline UInt Value::as<UInt>() const { return asUInt(); }
template <>
inline bool Value::is<UInt>() const { return isUInt(); }

#if defined(JSON_HAS_INT64)
template <>
inline Int64 Value::as<Int64>() const { return asInt64(); }
template <>
inline bool Value::is<Int64>() const { return isInt64(); }

template <>
inline UInt64 Value::as<UInt64>() const { return asUInt64(); }
template <>
inline bool Value::is<UInt64>() const { return isUInt64(); }
#endif

template <>
inline double Value::as<double>() const { return asDouble(); }
template <>
inline bool Value::is<double>() const { return isDouble(); }

template <>
inline String Value::as<String>() const { return asString(); }
template <>
inline bool Value::is<String>() const { return isString(); }

template <>
inline float Value::as<float>() const { return asFloat(); }
template <>
inline const char *Value::as<const char *>() const {
    return asCString();
}

class JSON_API PathArgument {
  public:
    friend class Path;

    PathArgument();
    PathArgument(ArrayIndex index);
    PathArgument(const char *key);
    PathArgument(String key);

  private:
    enum Kind { kindNone = 0,
                kindIndex,
                kindKey };
    String key_;
    ArrayIndex index_{};
    Kind kind_{kindNone};
};

class JSON_API Path {
  public:
    Path(const String &path, const PathArgument &a1 = PathArgument(),
         const PathArgument &a2 = PathArgument(),
         const PathArgument &a3 = PathArgument(),
         const PathArgument &a4 = PathArgument(),
         const PathArgument &a5 = PathArgument());

    const Value &resolve(const Value &root) const;
    Value resolve(const Value &root, const Value &defaultValue) const;

    Value &make(Value &root) const;

  private:
    using InArgs = std::vector<const PathArgument *>;
    using Args = std::vector<PathArgument>;

    void makePath(const String &path, const InArgs &in);
    void addPathInArg(const String &path, const InArgs &in,
                      InArgs::const_iterator &itInArg, PathArgument::Kind kind);
    static void invalidPath(const String &path, int location);

    Args args_;
};

class JSON_API ValueIteratorBase {
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using size_t = unsigned int;
    using difference_type = int;
    using SelfType = ValueIteratorBase;

    bool operator==(const SelfType &other) const { return isEqual(other); }

    bool operator!=(const SelfType &other) const { return !isEqual(other); }

    difference_type operator-(const SelfType &other) const {
        return other.computeDistance(*this);
    }

    Value key() const;

    UInt index() const;

    String name() const;

    JSONCPP_DEPRECATED("Use `key = name();` instead.")
    char const *memberName() const;

    char const *memberName(char const **end) const;

  protected:
    const Value &deref() const;
    Value &deref();

    void increment();

    void decrement();

    difference_type computeDistance(const SelfType &other) const;

    bool isEqual(const SelfType &other) const;

    void copy(const SelfType &other);

  private:
    Value::ObjectValues::iterator current_;

    bool isNull_{true};

  public:
    ValueIteratorBase();
    explicit ValueIteratorBase(const Value::ObjectValues::iterator &current);
};

class JSON_API ValueConstIterator : public ValueIteratorBase {
    friend class Value;

  public:
    using value_type = const Value;

    using reference = const Value &;
    using pointer = const Value *;
    using SelfType = ValueConstIterator;

    ValueConstIterator();
    ValueConstIterator(ValueIterator const &other);

  private:
    explicit ValueConstIterator(const Value::ObjectValues::iterator &current);

  public:
    SelfType &operator=(const ValueIteratorBase &other);

    SelfType operator++(int) {
        SelfType temp(*this);
        ++*this;
        return temp;
    }

    SelfType operator--(int) {
        SelfType temp(*this);
        --*this;
        return temp;
    }

    SelfType &operator--() {
        decrement();
        return *this;
    }

    SelfType &operator++() {
        increment();
        return *this;
    }

    reference operator*() const { return deref(); }

    pointer operator->() const { return &deref(); }
};

class JSON_API ValueIterator : public ValueIteratorBase {
    friend class Value;

  public:
    using value_type = Value;
    using size_t = unsigned int;
    using difference_type = int;
    using reference = Value &;
    using pointer = Value *;
    using SelfType = ValueIterator;

    ValueIterator();
    explicit ValueIterator(const ValueConstIterator &other);
    ValueIterator(const ValueIterator &other);

  private:
    explicit ValueIterator(const Value::ObjectValues::iterator &current);

  public:
    SelfType &operator=(const SelfType &other);

    SelfType operator++(int) {
        SelfType temp(*this);
        ++*this;
        return temp;
    }

    SelfType operator--(int) {
        SelfType temp(*this);
        --*this;
        return temp;
    }

    SelfType &operator--() {
        decrement();
        return *this;
    }

    SelfType &operator++() {
        increment();
        return *this;
    }

    reference operator*() const { return const_cast<reference>(deref()); }
    pointer operator->() const { return const_cast<pointer>(&deref()); }
};

inline void swap(Value &a, Value &b) { a.swap(b); }

inline const Value &Value::front() const { return *begin(); }

inline Value &Value::front() { return *begin(); }

inline const Value &Value::back() const { return *(--end()); }

inline Value &Value::back() { return *(--end()); }

}

#pragma pack(pop)

#if defined(JSONCPP_DISABLE_DLL_INTERFACE_WARNING)
#pragma warning(pop)
#endif

#endif

#ifndef JSON_READER_H_INCLUDED
#define JSON_READER_H_INCLUDED

#if !defined(JSON_IS_AMALGAMATION)
#include "json_features.h"
#include "value.h"
#endif
#include <deque>
#include <iosfwd>
#include <istream>
#include <stack>
#include <string>

#if defined(JSONCPP_DISABLE_DLL_INTERFACE_WARNING)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

#pragma pack(push)
#pragma pack()

namespace Json {

class JSON_API Reader {
  public:
    using Char = char;
    using Location = const Char *;

    struct StructuredError {
        ptrdiff_t offset_start;
        ptrdiff_t offset_limit;
        String message;
    };

    Reader();

    Reader(const Features &features);

    bool parse(const std::string &document, Value &root,
               bool collectComments = true);

    bool parse(const char *beginDoc, const char *endDoc, Value &root,
               bool collectComments = true);

    bool parse(IStream &is, Value &root, bool collectComments = true);

    JSONCPP_DEPRECATED("Use getFormattedErrorMessages() instead.")
    String getFormatedErrorMessages() const;

    String getFormattedErrorMessages() const;

    std::vector<StructuredError> getStructuredErrors() const;

    bool pushError(const Value &value, const String &message);

    bool pushError(const Value &value, const String &message, const Value &extra);

    bool good() const;

  private:
    enum TokenType {
        tokenEndOfStream = 0,
        tokenObjectBegin,
        tokenObjectEnd,
        tokenArrayBegin,
        tokenArrayEnd,
        tokenString,
        tokenNumber,
        tokenTrue,
        tokenFalse,
        tokenNull,
        tokenArraySeparator,
        tokenMemberSeparator,
        tokenComment,
        tokenError
    };

    class Token {
      public:
        TokenType type_;
        Location start_;
        Location end_;
    };

    class ErrorInfo {
      public:
        Token token_;
        String message_;
        Location extra_;
    };

    using Errors = std::deque<ErrorInfo>;

    bool readToken(Token &token);
    bool readTokenSkippingComments(Token &token);
    void skipSpaces();
    bool match(const Char *pattern, int patternLength);
    bool readComment();
    bool readCStyleComment();
    bool readCppStyleComment();
    bool readString();
    void readNumber();
    bool readValue();
    bool readObject(Token &token);
    bool readArray(Token &token);
    bool decodeNumber(Token &token);
    bool decodeNumber(Token &token, Value &decoded);
    bool decodeString(Token &token);
    bool decodeString(Token &token, String &decoded);
    bool decodeDouble(Token &token);
    bool decodeDouble(Token &token, Value &decoded);
    bool decodeUnicodeCodePoint(Token &token, Location &current, Location end,
                                unsigned int &unicode);
    bool decodeUnicodeEscapeSequence(Token &token, Location &current,
                                     Location end, unsigned int &unicode);
    bool addError(const String &message, Token &token, Location extra = nullptr);
    bool recoverFromError(TokenType skipUntilToken);
    bool addErrorAndRecover(const String &message, Token &token,
                            TokenType skipUntilToken);
    void skipUntilSpace();
    Value &currentValue();
    Char getNextChar();
    void getLocationLineAndColumn(Location location, int &line,
                                  int &column) const;
    String getLocationLineAndColumn(Location location) const;
    void addComment(Location begin, Location end, CommentPlacement placement);

    static bool containsNewLine(Location begin, Location end);
    static String normalizeEOL(Location begin, Location end);

    using Nodes = std::stack<Value *>;
    Nodes nodes_;
    Errors errors_;
    String document_;
    Location begin_{};
    Location end_{};
    Location current_{};
    Location lastValueEnd_{};
    Value *lastValue_{};
    String commentsBefore_;
    Features features_;
    bool collectComments_{};
};

class JSON_API CharReader {
  public:
    struct JSON_API StructuredError {
        ptrdiff_t offset_start;
        ptrdiff_t offset_limit;
        String message;
    };

    virtual ~CharReader() = default;

    virtual bool parse(char const *beginDoc, char const *endDoc, Value *root,
                       String *errs);

    std::vector<StructuredError> getStructuredErrors() const;

    class JSON_API Factory {
      public:
        virtual ~Factory() = default;

        virtual CharReader *newCharReader() const = 0;
    };

  protected:
    class Impl {
      public:
        virtual ~Impl() = default;
        virtual bool parse(char const *beginDoc, char const *endDoc, Value *root,
                           String *errs) = 0;
        virtual std::vector<StructuredError> getStructuredErrors() const = 0;
    };

    explicit CharReader(std::unique_ptr<Impl> impl) : _impl(std::move(impl)) {}

  private:
    std::unique_ptr<Impl> _impl;
};

class JSON_API CharReaderBuilder : public CharReader::Factory {
  public:
    Json::Value settings_;

    CharReaderBuilder();
    ~CharReaderBuilder() override;

    CharReader *newCharReader() const override;

    bool validate(Json::Value *invalid) const;

    Value &operator[](const String &key);

    static void setDefaults(Json::Value *settings);

    static void strictMode(Json::Value *settings);

    static void ecma404Mode(Json::Value *settings);
};

bool JSON_API parseFromStream(CharReader::Factory const &, IStream &, Value *root,
                              String *errs);

JSON_API IStream &operator>>(IStream &, Value &);

}

#pragma pack(pop)

#if defined(JSONCPP_DISABLE_DLL_INTERFACE_WARNING)
#pragma warning(pop)
#endif

#endif

#ifndef JSON_WRITER_H_INCLUDED
#define JSON_WRITER_H_INCLUDED

#if !defined(JSON_IS_AMALGAMATION)
#include "value.h"
#endif
#include <ostream>
#include <string>
#include <vector>

#if defined(JSONCPP_DISABLE_DLL_INTERFACE_WARNING) && defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4251)
#endif

#pragma pack(push)
#pragma pack()

namespace Json {

class Value;

class JSON_API StreamWriter {
  protected:
    OStream *sout_;

  public:
    StreamWriter();
    virtual ~StreamWriter();

    virtual int write(Value const &root, OStream *sout) = 0;

    class JSON_API Factory {
      public:
        virtual ~Factory();

        virtual StreamWriter *newStreamWriter() const = 0;
    };
};

String JSON_API writeString(StreamWriter::Factory const &factory,
                            Value const &root);

class JSON_API StreamWriterBuilder : public StreamWriter::Factory {
  public:
    Json::Value settings_;

    StreamWriterBuilder();
    ~StreamWriterBuilder() override;

    StreamWriter *newStreamWriter() const override;

    bool validate(Json::Value *invalid) const;

    Value &operator[](const String &key);

    static void setDefaults(Json::Value *settings);
};

class JSON_API Writer {
  public:
    virtual ~Writer();

    virtual String write(const Value &root) = 0;
};

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
class JSON_API FastWriter : public Writer {
  public:
    FastWriter();
    ~FastWriter() override = default;

    void enableYAMLCompatibility();

    void dropNullPlaceholders();

    void omitEndingLineFeed();

  public:
    String write(const Value &root) override;

  private:
    void writeValue(const Value &value);

    String document_;
    bool yamlCompatibilityEnabled_{false};
    bool dropNullPlaceholders_{false};
    bool omitEndingLineFeed_{false};
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
class JSON_API StyledWriter : public Writer {
  public:
    StyledWriter();
    ~StyledWriter() override = default;

  public:
    String write(const Value &root) override;

  private:
    void writeValue(const Value &value);
    void writeArrayValue(const Value &value);
    bool isMultilineArray(const Value &value);
    void pushValue(const String &value);
    void writeIndent();
    void writeWithIndent(const String &value);
    void indent();
    void unindent();
    void writeCommentBeforeValue(const Value &root);
    void writeCommentAfterValueOnSameLine(const Value &root);
    static bool hasCommentForValue(const Value &value);
    static String normalizeEOL(const String &text);

    using ChildValues = std::vector<String>;

    ChildValues childValues_;
    String document_;
    String indentString_;
    unsigned int rightMargin_{74};
    unsigned int indentSize_{3};
    bool addChildValues_{false};
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
class JSON_API StyledStreamWriter {
  public:
    StyledStreamWriter(String indentation = "\t");
    ~StyledStreamWriter() = default;

  public:
    void write(OStream &out, const Value &root);

  private:
    void writeValue(const Value &value);
    void writeArrayValue(const Value &value);
    bool isMultilineArray(const Value &value);
    void pushValue(const String &value);
    void writeIndent();
    void writeWithIndent(const String &value);
    void indent();
    void unindent();
    void writeCommentBeforeValue(const Value &root);
    void writeCommentAfterValueOnSameLine(const Value &root);
    static bool hasCommentForValue(const Value &value);
    static String normalizeEOL(const String &text);

    using ChildValues = std::vector<String>;

    ChildValues childValues_;
    OStream *document_;
    String indentString_;
    unsigned int rightMargin_{74};
    String indentation_;
    bool addChildValues_ : 1;
    bool indented_ : 1;
};
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(JSON_HAS_INT64)
String JSON_API valueToString(Int value);
String JSON_API valueToString(UInt value);
#endif
String JSON_API valueToString(LargestInt value);
String JSON_API valueToString(LargestUInt value);
String JSON_API valueToString(
    double value, unsigned int precision = Value::defaultRealPrecision,
    PrecisionType precisionType = PrecisionType::significantDigits);
String JSON_API valueToString(bool value);
String JSON_API valueToQuotedString(const char *value);
String JSON_API valueToQuotedString(const char *value, size_t length);

JSON_API OStream &operator<<(OStream &, const Value &root);

}

#pragma pack(pop)

#if defined(JSONCPP_DISABLE_DLL_INTERFACE_WARNING)
#pragma warning(pop)
#endif

#endif

#ifndef JSON_ASSERTIONS_H_INCLUDED
#define JSON_ASSERTIONS_H_INCLUDED

#include <cstdlib>
#include <sstream>

#if !defined(JSON_IS_AMALGAMATION)
#include "config.h"
#endif

#if JSON_USE_EXCEPTION

#define JSON_ASSERT(condition)                           \
    do {                                                 \
        if (!(condition)) {                              \
            Json::throwLogicError("assert json failed"); \
        }                                                \
    } while (0)

#define JSON_FAIL_MESSAGE(message)        \
    do {                                  \
        OStringStream oss;                \
        oss << message;                   \
        Json::throwLogicError(oss.str()); \
        abort();                          \
    } while (0)

#else

#define JSON_ASSERT(condition) assert(condition)

#define JSON_FAIL_MESSAGE(message)          \
    {                                       \
        OStringStream oss;                  \
        oss << message;                     \
        assert(false && oss.str().c_str()); \
        abort();                            \
    }

#endif

#define JSON_ASSERT_MESSAGE(condition, message) \
    do {                                        \
        if (!(condition)) {                     \
            JSON_FAIL_MESSAGE(message);         \
        }                                       \
    } while (0)

#endif

#endif
