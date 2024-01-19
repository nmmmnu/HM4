#ifndef MY_TYPE_TRAITS_H_
#define MY_TYPE_TRAITS_H_

template <typename T, typename... Ts>
struct is_any : std::disjunction<std::is_same<T, Ts>...>{};

template <typename T, typename... Ts>
constexpr bool is_any_v = is_any<T, Ts...>::value;

#endif
