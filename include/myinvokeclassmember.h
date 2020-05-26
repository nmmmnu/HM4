#ifndef MY_INVOKE_CLASS_MEMBER_H_
#define MY_INVOKE_CLASS_MEMBER_H_

#include <type_traits>

namespace class_invoke_impl_{
	template <class T, class F, class... Args>
	constexpr auto invoke_class_member_(T &&cl, F func, std::true_type, Args&&... args){
		return (std::forward<T>(cl).*func)(std::forward<Args>(args)...);
	}

	template <class T, class F, class... Args>
	constexpr auto invoke_class_member_(T const &, F func, std::false_type, Args&&... args){
		return func(std::forward<Args>(args)...);
	}
}

template <class T, class F, class... Args>
constexpr auto invoke_class_member(T &&cl, F func, Args&&... args){
	using namespace class_invoke_impl_;

	return invoke_class_member_(std::forward<T>(cl), func, std::is_member_pointer<F>{}, std::forward<Args>(args)...);
}

#endif

