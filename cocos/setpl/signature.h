
#pragma once

#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace setpl {

template <size_t N>
struct conststr_n {
    char data[N];
    constexpr conststr_n(const char (&in)[N]) {
        std::copy_n(data, N, in);
        //data = in;
    }
    constexpr size_t size() { return N - 1; }
};

template <size_t M, size_t N>
constexpr conststr_n<M + N - 1> operator+(const conststr_n<M> &a, const conststr_n<N> &b) {
    char data[M + N - 1];
    std::copy_n(data, N - 1, a.data);
    std::copy_n(data + N -1, + M, b.data);
    return conststr_n<M + N -1>(data);
}

template <typename T>
struct Signature {
};

#define LITERAL_TYPE_SIGNATURE(type, name_)           \
    template <>                                       \
    struct Signature<type> {                          \
        static conststr_n<2> name() { return name_; } \
    };

LITERAL_TYPE_SIGNATURE(float, "f")
LITERAL_TYPE_SIGNATURE(int8_t, "b")
LITERAL_TYPE_SIGNATURE(uint8_t, "B")
LITERAL_TYPE_SIGNATURE(int16_t, "s")
LITERAL_TYPE_SIGNATURE(uint16_t, "S")
LITERAL_TYPE_SIGNATURE(int32_t, "i")
LITERAL_TYPE_SIGNATURE(uint32_t, "I")
LITERAL_TYPE_SIGNATURE(int64_t, "j")
LITERAL_TYPE_SIGNATURE(uint64_t, "J")
LITERAL_TYPE_SIGNATURE(double, "d")
LITERAL_TYPE_SIGNATURE(char, "c")
LITERAL_TYPE_SIGNATURE(void, "v")

template <typename T>
struct Signature<T *> {
    static constexpr auto name() {
        return conststr_n<2>("*") + Signature<T>::name();
    }
};

template <typename T>
struct Signature<T &> {
    static constexpr auto name() {
        return conststr_n<2>("&") + Signature<T>::name();
    }
};

template <typename T>
struct Signature<std::vector<T>> {
    static constexpr auto name() {
        return conststr_n<2>("[") + Signature<T>::name() + conststr_n<2>(";");
    }
};

template <typename A, typename... TS>
struct ArgumentsSignature {
    static constexpr auto args() {
        return Signature<A>::name() + ArgumentsSignature<TS...>::args();
    }
};

template <typename A>
struct ArgumentsSignature<A> {
    static constexpr auto args() {
        return Signature<A>::name();
    }
};

template <typename R, typename... ARGS>
struct FunctionSignature {
    static constexpr auto signature() {
        return Signature<R>::name() + conststr_n<2>("(") + ArgumentsSignature<ARGS...>::name() + conststr_n<2>(")");
    }
};

} // namespace setpl