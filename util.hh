//
// Created by herlight on 2019/11/11.
//

#ifndef __MC_UTIL_HH__
#define __MC_UTIL_HH__

#include "config.hh"
#include <iostream>
#include <type_traits>
#include <climits>
#include <random>

namespace mc {

namespace details {

template<typename ...Args, typename = typename std::enable_if<sizeof...(Args) == 0>::type>
inline std::ostream &__va_output(std::ostream &os, Args &&...args) {
    return os << std::endl;
}
template<typename T, typename ...Args>
inline std::ostream &__va_output(std::ostream &os, T &&t, Args &&...args) {
    os << t;
    return __va_output(os, std::forward<Args>(args)...);
}
template<typename ...Args>
inline std::ostream &__log_impl(std::ostream &os, const char *file, int line, Args &&...args) {
    os << file << ":" << line << ": ";
    return __va_output(os, std::forward<Args>(args)...);
}

};

#define log(os, args...) details::__log_impl(os, __FILE__, __LINE__, args)

#ifdef MC_DEBUG

#define STR_(x) #x
#define STR(x) STR_(x)

#define debug(args...) log(std::cerr, args)
#define debugf(args...) (fprintf(stderr, __FILE__":" STR(__LINE__) ": " args), fprintf(stderr, "\n"))

#else

#define debug(args...) (void)0
#define debugf(args...) (void)0

#endif

#define debugv(v) debug(#v, ": ", v)

namespace details {

template<typename T>
struct reversion_wrapper {
    T &iterable;
    auto begin() -> decltype(std::declval<T>().rbegin()) { return iterable.rbegin(); }
    auto end() -> decltype(std::declval<T>().rend()) { return iterable.rend(); }
};

};

template<typename T>
details::reversion_wrapper<T> reverse(T &&iterable) {
    return {iterable};
}

static inline int MaxInt() {
    return std::numeric_limits<int>::max();
}

#define when(cond, act) case (cond): act break;

};

#endif //__MC_UTIL_HH__
