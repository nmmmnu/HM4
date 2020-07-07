#ifndef MY_INVOKE_CLASS_MEMBER_H_
#define MY_INVOKE_CLASS_MEMBER_H_

#include <type_traits>

template <class T, class F, class... Args>
constexpr auto invoke_class_member(T &&cl, F func, Args&&... args){
	if constexpr(std::is_member_pointer_v<F>)
		return (std::forward<T>(cl).*func)(std::forward<Args>(args)...);
	else
		return func(std::forward<Args>(args)...);
}

#endif

