#ifndef MY_BIT_BUFFER_H_
#define MY_BIT_BUFFER_H_

#include <cstdint>
#include <cassert>

#include <string_view>

namespace MyBuffer{

	struct BitBufferView{
		using size_type		= std::size_t;
		using value_type	= const void;

	private:
		value_type	*data_	= nullptr;
		size_type	bits_	= 0;

	public:
		constexpr BitBufferView()	= default;

		constexpr BitBufferView(value_type *data, size_type bits) :
					data_(data),
					bits_(bits){}

		constexpr BitBufferView(std::string_view s, size_type bits) :
					BitBufferView(s.data(), bits){

			assert(s.size() >= bytes());
		}

	public:
		constexpr operator bool() const noexcept{
			return data_;
		}

		constexpr const value_type *data() const noexcept{
			return data_;
		}

		bool operator[](size_type const index) const noexcept{
			const auto *p = static_cast<const uint8_t *>(data_);

			auto    const byte  = index / 8;
			auto    const bit   = index % 8;
			uint8_t const value = p[byte];

			return (value >> bit) & 1;
		}

		constexpr static std::size_t bytes(size_type size){
			return (size + 7) / 8;
		}

		constexpr std::size_t bytes() const noexcept{
			return bytes(bits_);
		}

		constexpr size_type bits() const noexcept{
			return bits_;
		}

		constexpr std::string_view toSV() const noexcept{
			return {
				static_cast<const char*>(data()),
				bytes()
			};
		}
	};

} // namespace MyBuffer

#endif

