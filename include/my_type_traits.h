#ifndef MY_TYPE_TRAITS_H_
#define MY_TYPE_TRAITS_H_

template <typename T, typename ...Ts>
struct is_any : std::disjunction<std::is_same<T, Ts>...>{};

template <typename T, typename ...Ts>
constexpr bool is_any_v = is_any<T, Ts...>::value;

template<typename T, typename ...Ts>
constexpr bool is_any_of(T value, Ts... ts){
    static_assert(sizeof...(ts) > 0);

    return ((ts == value) || ...);
}

#endif
