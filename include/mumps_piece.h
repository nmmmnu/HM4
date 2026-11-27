#ifndef MUMPS_PIECE_H_
#define MUMPS_PIECE_H_

#include "stringtokenizer.h"
#include "staticvector.h"
#include "bufferpusher.h"

#include <cassert>

namespace mumps_piece{
	namespace impl_{

		template<size_t Size>
		void copyToBuffer(BufferPusher &buffer, bool first, StaticVector<std::string_view, Size> const &vector, char delimiter, size_t bytes = 0) noexcept{
			for(auto x : vector){
				if (first){
					first = false;
				}else{
					buffer.push(delimiter);
				}

				buffer.push(x);
			}

			if (bytes){
				assert(buffer.size() == bytes);
			}
		}

		template<size_t Size>
		void copyToBuffer(StaticVector<std::string_view, Size> const &vector, char delimiter, char *ptr, size_t bytes = 0) noexcept{
			BufferPusher buffer{ ptr };
			copyToBuffer(buffer, true, vector, delimiter, bytes);
		}

		template<size_t Size>
		void trim(StaticVector<std::string_view, Size> &vector){
			while(!vector.empty() && vector.back().empty())
				vector.pop_back();
		}

	} // namespace impl_



	inline std::string_view trimSV(std::string_view sv, char delimiter) noexcept{
		while(!sv.empty() && sv.back() == delimiter)
			sv.remove_suffix(1);

		return sv;
	}



	template<size_t Size>
	struct MumpsPieceMany{
		using Vector = StaticVector<std::string_view, Size>;

		constexpr MumpsPieceMany(char delimiter) noexcept :
						delimiter_	(delimiter		){}

		MumpsPieceMany(std::string_view sv, char delimiter) noexcept :
						vector_		(load__(sv, delimiter)	),
						delimiter_	(delimiter		){}

	public:
		constexpr auto operator[](size_t id) const noexcept{
			return id < size() ? vector_[id] : "";
		}

		constexpr auto begin() const noexcept{
			return std::begin(vector_);
		}

		constexpr auto end() const noexcept{
			return std::end(vector_);
		}

		constexpr const auto *data() const noexcept{
			return vector_.data();
		}

		constexpr size_t size() const noexcept{
			return vector_.size();
		}

		constexpr size_t capacity() const noexcept{
			return vector_.capacity();
		}

		constexpr size_t bytes() const noexcept{
			size_t size = 0;

			for(auto x : vector_)
				size += x.size() + 1;

			return size ? size - 1 : size;
		}

		constexpr char delimiter() const noexcept{
			return delimiter_;
		}

	public:
		bool set(size_t pos, std::string_view val) noexcept{
			if (pos >= capacity())
				return false;

			// no resize
			while(vector_.size() <= pos)
				vector_.push_back();

			vector_[pos] = val;

			impl_::trim(vector_);

			return true;
		}

		bool set(std::string_view val) noexcept{
			if (val.empty())
				return true;

			if (vector_.full())
				return false;

			vector_.push_back(val);

			return true;
		}

		void copyToBuffer(char *data, size_t bytes = 0) const noexcept{
			impl_::copyToBuffer(vector_, delimiter_, data, bytes);
		}

	private:
		static Vector load__(std::string_view sv, char delimiter) noexcept{
			Vector vector;

			StringTokenizer tok{ sv, delimiter };

			for(auto x : tok){
				if (vector.full())
					break;

				vector.push_back(x);
			}

			impl_::trim(vector);

			return vector;
		}

	private:
		Vector	vector_;
		char	delimiter_;
	};





	template<size_t Size>
	struct MumpsPieceAppend{
		using Vector = StaticVector<std::string_view, Size>;

		template<typename It>
		MumpsPieceAppend(std::string_view data, char delimiter, It begin, It end) noexcept :
								sizeFront_(calcSizeFront__(data, delimiter)	),
								delimiter_(delimiter				){

			assert(static_cast<size_t>(std::distance(begin, end)) <= Size);

			for(auto it = begin; it != end; ++it)
				if (!it->empty())
					vector_.push_back(*it);
		}

		constexpr size_t bytes() const noexcept{
			size_t size = sizeFront_;

			for(auto x : vector_)
				size += x.size() + 1;

			return sizeFront_ == 0 && size ? size - 1 : size;
		}

		void copyToBuffer(char *data, size_t bytes = 0) const noexcept{
			BufferPusher buffer{ data };

			buffer.skip(sizeFront_);

			bool const first = sizeFront_ == 0;

			impl_::copyToBuffer(buffer, first, vector_, delimiter_, bytes);
		}

	private:
		static size_t calcSizeFront__(std::string_view data, char delimiter) noexcept{
			// trim data as a string
			auto const sv = trimSV(data, delimiter);
			return sv.size();
		}

	private:
		Vector			vector_;
		size_t			sizeFront_;

		char			delimiter_;
	};





	struct MumpsPieceSingle{
		struct Mutator;

	public:
		constexpr MumpsPieceSingle(std::string_view data, char delimiter) noexcept :
						data_		(data		),
						delimiter_	(delimiter	){}

		std::string_view operator[](size_t id) const noexcept{
			size_t start = 0;

			while (id--){
				auto const found = data_.find(delimiter_, start);
				if (found == std::string_view::npos)
					return {};

				start = found + 1;
			}

			auto const end = data_.find(delimiter_, start);

			return data_.substr(start, end - start);
		}

		Mutator mutate(size_t pos, std::string_view val) const noexcept;
		Mutator mutate(std::string_view val) const noexcept;

	private:
		std::string_view	data_;
		char			delimiter_;
	};

	struct MumpsPieceSingle::Mutator{
		constexpr Mutator(
				std::string_view val, char delimiter,
				std::string_view prefix, std::string_view suffix, size_t delimiters
						) noexcept :	val		(val		),
								prefix		(prefix		),
								suffix		(suffix		),
								delimiters	(delimiters	),
								delimiter	(delimiter	){}

		constexpr size_t bytes() const noexcept{
			return prefix.size() + delimiters + val.size() + suffix.size();
		}

		auto copyToBuffer(char *data, size_t bytes = 0) const noexcept{
			BufferPusher buffer{ data };

			buffer.push(prefix);

			for(size_t i = 0; i < delimiters; ++i)
				buffer.push(delimiter);

			buffer.push(val);
			buffer.push(suffix);

			if (bytes){
				assert(buffer.size() == bytes);
			}
		}

		auto updateBufferInPlace(char *data) const noexcept{
			BufferPusher buffer{ data };

			buffer.skip(prefix);
			buffer.push(val);
		//	buffer.skip(suffix);
		}

	private:
		std::string_view	val		;
		std::string_view	prefix		;
		std::string_view	suffix		;

		size_t			delimiters	;

		char			delimiter	;
	};

	inline auto MumpsPieceSingle::mutate(size_t pos, std::string_view val) const noexcept -> Mutator{
		size_t start = 0;
		size_t count = 0;

		while (count < pos){
			auto const found = data_.find(delimiter_, start);

			if (found == std::string_view::npos)
				break;

			start = found + 1;
			++count;
		}

		if (count == pos){
			// case 1 : index exists
			auto const end		= data_.find(delimiter_, start);
			auto const sliceEnd	= end == std::string_view::npos ? data_.size() : end;

			auto prefix = data_.substr(0, start);
			auto suffix = data_.substr(sliceEnd);

			suffix = trimSV(suffix, delimiter_);

			if (val.empty() && suffix.empty())
				prefix = trimSV(prefix, delimiter_);

			return Mutator{
				/* val		*/ val					,
				/* delimiter	*/ delimiter_				,

				/* prefix	*/ prefix				,
				/* suffix	*/ suffix				,
				/* delimiters	*/ 0
			};

		}else if (val.empty()){
			// case 2a : index is after the string end and val is empty

			return Mutator{
				/* val		*/ ""					,
				/* delimiter	*/ delimiter_				,

				/* prefix	*/ trimSV(data_, delimiter_)	,
				/* suffix	*/ ""					,
				/* delimiters	*/ 0
			};
		}else{
			// case 2b : index is after the string end

			return Mutator{
				/* val		*/ val					,
				/* delimiter	*/ delimiter_				,

				/* prefix	*/ data_				,
				/* suffix	*/ ""					,
				/* delimiters	*/ pos - count
			};
		}
	}

	inline auto MumpsPieceSingle::mutate(std::string_view val) const noexcept -> Mutator{

		auto const prefix = trimSV(data_, delimiter_);

		return Mutator{
			/* val		*/ val				,
			/* delimiter	*/ delimiter_			,

			/* prefix	*/ prefix			,
			/* suffix	*/ ""				,
			/* delimiters	*/ val.empty() || prefix.empty() ? 0u : 1u
		};
	}

} // namespace mumps_piece

#endif


