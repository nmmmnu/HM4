#ifndef ACC_DATA_H_
#define ACC_DATA_H_

#include <cstdint>
#include <cmath>

#include <string_view>

#include "myendian.h"

class AccData{
	uint64_t	count_		= 0;
	double		first_		;
	double		last_		;
	double		min_		;
	double		max_		;
	double		sum_		;
	double		sumSquare_	;
	double		sumHarmonic_	;
	double		sumLog_		;

private:
	template<typename T>
	[[nodiscard]]
	constexpr static auto v(T const &key){
		return betoh(key);
	}

	template<typename T>
	constexpr static void v(T &key, T val){
		key = htobe(val);
	}

	template<typename T>
	constexpr static void i(T &key, T val = T{1}){
		key = htobe(betoh(key) + val);
	}

private:
	constexpr void init_(double x){
		v(count_	, uint64_t{1}			);
		v(first_	, x				);
		v(last_		, x				);
		v(min_		, x				);
		v(max_		, x				);
		v(sum_		, x				);
		v(sumSquare_	, x * x				);
		v(sumHarmonic_	, x != 0 ? 1 / x       : 0.	);
		v(sumLog_	, x > 0  ? std::log(x) : 0.	);
	}

	constexpr void accumulate_(double x){
		i(count_		);

		v(last_,  x		);

		if (x < v(min_))
			v(min_, x);

		if (x > v(max_))
			v(max_, x);

		{
			i(sum_,       x		);
			i(sumSquare_, x * x	);
		}

		if (x != 0)
			i(sumHarmonic_, + 1 / x	);

		if (x > 0)
			i(sumLog_, std::log(x)	);
	}

public:
	constexpr void clear(){
		count_ = 0;
	}

	constexpr void init(double x){
		if (std::isnan(x) || std::isinf(x))
			return;

		return init_(x);
	}

	constexpr void accumulate(double x){
		if (std::isnan(x) || std::isinf(x))
			return;

		if (!count_)
			return init_(x);
		else
			return accumulate_(x);
	}

	constexpr void operator()(double x){
		return accumulate(x);
	}

	constexpr void merge(AccData const &other, bool const older = true){
		// works even BE
		if (! other.count_)
			return;

		// works even BE
		if (!count_){
			*this = other;
			return;
		}

		if (older)
			first_	= other.first_;
		else
			last_	= other.last_;

		if (v(min_) > v(other.min_))
			min_ = other.min_;

		if (v(max_) < v(other.max_))
			max_ = other.max_;

		i(sum_		, v(other.sum_		));
		i(sumSquare_	, v(other.sumSquare_	));
		i(sumHarmonic_	, v(other.sumHarmonic_	));
		i(sumLog_ 	, v(other.sumLog_	));

		// update the count last
		i(count_	, v(other.count_	));
	}

public:
	constexpr double count() const{
		return static_cast<double>(v(count_));
	}

	constexpr double first() const{
		return v(first_);
	}

	constexpr double last() const{
		return v(last_);
	}

	constexpr double min() const{
		return v(min_);
	}

	constexpr double max() const{
		return v(max_);
	}

	constexpr double sum() const{
		return v(sum_);
	}

	// Sum of squares
	constexpr double sum2() const{
		return v(sumSquare_);
	}

	// Range
	constexpr double range() const{
		return v(max_) - v(min_);
	}

	// Change
	constexpr double change() const{
		return v(last_) - v(first_);
	}

	// Arithmetic Mean
	constexpr double avg() const{
		if (!count_)
			return 0.;
		else
			return v(sum_) / count();
	}

	// Harmonic Mean
	constexpr double harm() const{
		// works even BE, even for double, because is initialized to +0.0
		if (!count_ || sumHarmonic_ == 0.)
			return 0.;
		else
			return count() / v(sumHarmonic_);
	}

	// Geometric Mean
	constexpr double geom() const{
		// works even BE
		if (!count_)
			return 0.;
		else
			return std::exp(v(sumLog_) / count());
	}

	// Variance / Dispersion
	constexpr double vari() const{
		if (v(count_) <= 1)
			return 0.;

		auto const avg = this->avg();

		auto const r = v(sumSquare_) / count() - (avg * avg);

		if (r > 0.)
			return r;
		else
			return 0.;
	}

	// Standard Deviation / Volatility
	constexpr double sdev() const{
		if (auto const vari = this->vari(); vari > 0)
			return std::sqrt(vari);
		else
			return 0.;
	}

	// Root Mean Square
	constexpr double rms() const{
		// sumSquare_ can not be < 0
		// works even BE
		if (count_)
			return std::sqrt(v(sumSquare_) / count());
		else
			return 0.;
	}

	// Coefficient of Variation
	constexpr double cvar() const{
		if (auto const avg = this->avg(); avg != 0)
			return sdev() / avg;
		else
			return 0.;
	}

	constexpr double operator[](std::string_view s) const;
};



constexpr double AccData::operator[](std::string_view s) const{
	auto _ = [](std::string_view s){
		using T = uint64_t;

		switch(s.size()){
		case 3: return
				T(s[0]) << (0 * 8) |
				T(s[1]) << (1 * 8) |
				T(s[2]) << (2 * 8)
		;

		case 4: return
				T(s[0]) << (0 * 8) |
				T(s[1]) << (1 * 8) |
				T(s[2]) << (2 * 8) |
				T(s[3]) << (3 * 8)
		;

		case 5: return
				T(s[0]) << (0 * 8) |
				T(s[1]) << (1 * 8) |
				T(s[2]) << (2 * 8) |
				T(s[3]) << (3 * 8) |
				T(s[4]) << (4 * 8)
		;

		case 6: return
				T(s[0]) << (0 * 8) |
				T(s[1]) << (1 * 8) |
				T(s[2]) << (2 * 8) |
				T(s[3]) << (3 * 8) |
				T(s[4]) << (4 * 8) |
				T(s[5]) << (5 * 8)
		;

		default: return T(0);
		}
	};

	switch(_(s)){
	case _("count"	) : case _("COUNT"	) : return count	();
	case _("open"	) : case _("OPEN"	) :
	case _("first"	) : case _("FIRST"	) : return first	();
	case _("close"	) : case _("CLOSE"	) :
	case _("last"	) : case _("LAST"	) : return last		();
	case _("min"	) : case _("MIN"	) : return min		();
	case _("max"	) : case _("MAX"	) : return max		();
	case _("sum"	) : case _("SUM"	) : return sum		();
	case _("sumsq"	) : case _("SUMSQ"	) :
	case _("sum2"	) : case _("SUM2"	) : return sum2		();
	case _("range"	) : case _("RANGE"	) : return range	();
	case _("change"	) : case _("CHANGE"	) : return change	();
	case _("avg"	) : case _("AVG"	) : return avg		();
	case _("harm"	) : case _("HARM"	) : return harm		();
	case _("geom"	) : case _("GEOM"	) : return geom		();
	case _("vari"	) : case _("VARI"	) : return vari		();
	case _("sdev"	) : case _("SDEV"	) : return sdev		();
	case _("rms"	) : case _("RMS"	) : return rms		();
	case _("cvar"	) : case _("CVAR"	) : return cvar		();

	default: return 0.;
	}
}

#endif

