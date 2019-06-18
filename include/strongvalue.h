#ifndef STRONG_VALUE_H_
#define STRONG_VALUE_H_

template<typename T, typename TAG>
struct StrongValue{
	using type = T;

	T v;

	constexpr operator T() const{
		return v;
	}

	constexpr const T &operator()() const noexcept{
		return v;
	}

	T &operator()() noexcept{
		return v;
	}

	constexpr const T &get() const noexcept{
		return v;
	}

	T &get() noexcept{
		return v;
	}

	using SV = StrongValue;

	friend constexpr bool operator==(const SV &a, const SV &b){ return { a.v == b.v }; }
	friend constexpr bool operator!=(const SV &a, const SV &b){ return { a.v != b.v }; }
	friend constexpr bool operator< (const SV &a, const SV &b){ return { a.v <  b.v }; }
	friend constexpr bool operator> (const SV &a, const SV &b){ return { a.v >  b.v }; }
	friend constexpr bool operator<=(const SV &a, const SV &b){ return { a.v <= b.v }; }
	friend constexpr bool operator>=(const SV &a, const SV &b){ return { a.v >= b.v }; }

	friend constexpr SV   operator+ (const SV &a){ return { +a.v }; }
	friend constexpr SV   operator- (const SV &a){ return { -a.v }; }

	friend constexpr SV   operator+ (const SV &a, const SV &b){ return { a.v +  b.v }; }
	friend constexpr SV   operator- (const SV &a, const SV &b){ return { a.v -  b.v }; }
	friend constexpr SV   operator* (const SV &a, const SV &b){ return { a.v *  b.v }; }
	friend constexpr SV   operator/ (const SV &a, const SV &b){ return { a.v /  b.v }; }

	friend SV  &operator++(SV &a){ ++a.v; return a; }
	friend SV  &operator--(SV &a){ --a.v; return a; }

	friend SV   operator++(SV &a, int){ SV tmp = a; ++a.v; return tmp; }
	friend SV   operator--(SV &a, int){ SV tmp = a; --a.v; return tmp; }

	friend SV  &operator+=(SV &a, const SV &b){ a.v += b.v; return a; }
	friend SV  &operator-=(SV &a, const SV &b){ a.v -= b.v; return a; }
	friend SV  &operator*=(SV &a, const SV &b){ a.v *= b.v; return a; }
	friend SV  &operator/=(SV &a, const SV &b){ a.v /= b.v; return a; }
};

#endif


