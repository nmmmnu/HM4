#ifndef SGN_H_
#define SGN_H_

template<typename T>
constexpr int sgn(const T &a, const T &b) noexcept{
	return (a > b) - (a < b);
}

template<typename T>
constexpr int sgn(const T &a) noexcept{
	return sgn(a, T(0));
}

#endif

