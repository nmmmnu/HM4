#ifndef MUMPS_PIECE_H_
#define MUMPS_PIECE_H_

#include "stringtokenizer.h"
#include "staticvector.h"
#include "bufferpusher.h"

#include <cassert>

namespace mumps_piece{

	template<size_t Size>
	struct MumpsPieceMany{
		using Vector = StaticVector<std::string_view, Size>;

		constexpr MumpsPieceMany(char delimiter) noexcept :
						delimiter_	(delimiter			){}

		MumpsPieceMany(std::string_view sv, char delimiter) noexcept :
						vector_		(decompose__(sv, delimiter)	),
						delimiter_	(delimiter			){}

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

			trim__();

			return true;
		}

		bool set(std::string_view val) noexcept{
			trim__();

			if (val.empty())
				return true;

			if (vector_.full())
				return false;

			vector_.push_back(val);

			return true;
		}

		auto copyToBuffer(char *data, size_t bytes = 0) const noexcept{
			BufferPusher buffer{ data };

			bool first = true;

			for(auto x : vector_){
				if (first){
					first = false;
				}else{
					buffer.push(delimiter_);
				}

				buffer.push(x);
			}

			if (bytes){
				assert(buffer.size() == bytes);
			}
		}

	private:
		void trim__(){
			while(!vector_.empty() && vector_.back().empty())
				vector_.pop_back();
		}

		static Vector decompose__(std::string_view sv, char delimiter) noexcept{
			Vector vector;

			StringTokenizer tok{ sv, delimiter };

			for(auto x : tok){
				if (vector.full())
					break;

				vector.push_back(x);
			}

			return vector;
		}

	private:
		Vector	vector_;
		char	delimiter_;
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
		static std::string_view trim_(std::string_view sv, char delimiter) noexcept{
			while(!sv.empty() && sv.back() == delimiter)
				sv.remove_suffix(1);

			return sv;
		}

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

			suffix = trim_(suffix, delimiter_);

			if (val.empty() && suffix.empty())
				prefix = trim_(prefix, delimiter_);

			return Mutator{
				/* val		*/ val				,
				/* delimiter	*/ delimiter_			,

				/* prefix	*/ prefix			,
				/* suffix	*/ suffix			,
				/* delimiters	*/ 0
			};

		}else if (val.empty()){
			// case 2a : index is after the string end and val is empty

			return Mutator{
				/* val		*/ ""				,
				/* delimiter	*/ delimiter_			,

				/* prefix	*/ trim_(data_, delimiter_)	,
				/* suffix	*/ ""				,
				/* delimiters	*/ 0
			};
		}else{
			// case 2b : index is after the string end

			return Mutator{
				/* val		*/ val				,
				/* delimiter	*/ delimiter_			,

				/* prefix	*/ data_			,
				/* suffix	*/ ""				,
				/* delimiters	*/ pos - count
			};
		}
	}

	inline auto MumpsPieceSingle::mutate(std::string_view val) const noexcept -> Mutator{

		auto const prefix = trim_(data_, delimiter_);

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


