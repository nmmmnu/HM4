#ifndef MY_MATRIX_H_
#define MY_MATRIX_H_

template<typename T>
struct Matrix{
public:
	constexpr Matrix(size_t x, size_t y) : xs(x), ys(y){}

public:
	constexpr size_t getX() const{
		return xs;
	}

	constexpr size_t getY() const{
		return ys;
	}

public:
	constexpr size_t size() const{
		return xs * ys;
	}

	constexpr size_t bytes() const{
		return xs * ys * sizeof(T);
	}

public:
	constexpr auto const &operator()(const T *data, size_t x, size_t y) const{
		return data[ calc_(x, y) ];
	}

	constexpr auto &operator()(T *data, size_t x, size_t y) const{
		return data[ calc_(x, y) ];
	}

	constexpr auto const &operator()(const char *data_, size_t x, size_t y) const{
		const auto *data = reinterpret_cast<const T *>(data_);

		return data[ calc_(x, y) ];
	}

	constexpr auto &operator()(char *data_, size_t x, size_t y) const{
		auto *data = reinterpret_cast<T *>(data_);

		return data[ calc_(x, y) ];
	}

private:
	constexpr size_t calc_(size_t x, size_t y) const{
		return x + y * xs;
	}

private:
	size_t xs;
	size_t ys;
};

#endif

