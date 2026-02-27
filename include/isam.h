#ifndef ISAM_H_
#define ISAM_H_

#include "stringtokenizer.h"
#include "mystring.h"
#include "staticvector.h"

#include <limits>
#include <algorithm>
#include <cassert>
#include <cstdio>


namespace ISAM_impl_{
	namespace config{
		// 3:id,10:name,2:country

		constexpr char		DELIMITER1	= ',';
		constexpr char		DELIMITER2	= ':';

		constexpr char		PADDING		= ' ';

		constexpr size_t	CONTAINER_SIZE	= 128;

		constexpr std::string_view EMPTY_FIELD_NAME = "__FIELD_NAME__";
	} // namespace ISAM_config



	constexpr std::string_view trim(std::string_view s){
		while (!s.empty() && s.back() == config::PADDING)
			s.remove_suffix(1);

		return s;
	}



	struct Field{
		size_t			off;
		size_t			size;
		std::string_view	name;

		constexpr Field(size_t off, size_t size, std::string_view name) :
								off	(off	),
								size	(size	),
								name	(name	){

			assert(off < std::numeric_limits<size_t>::max() - size);
		}

		void print() const{
			printf("%8zu %8zu %.*s\n",
						off, size,
						static_cast<int>(name.size()), name.data()
			);
		}

		void print(const char *storage) const{
			auto const value = load(storage);

			printf("%8zu %8zu [%.*s] => %.*s\n",
						off, size,
						static_cast<int>(name.size()),  name.data(),
						static_cast<int>(value.size()), value.data()
			);
		}

		bool store(char *storage, std::string_view value) const{
			char *pos = storage + off;

			auto const size_value = std::min(size, value.size());
			auto const size_pad   = size - size_value;

			memcpy(pos, value.data(), size_value);
			pos += size_value;

			memset(pos, config::PADDING, size_pad);
		//	pos += size_pad;

			return true;
		}

		[[nodiscard]]
		std::string_view load(const char *storage) const{
			std::string_view value{ storage + off, size };

			return trim(value);
		}

		[[nodiscard]]
		size_t bytes() const{
			return off + size;
		}
	};



	struct ISAM{
		template<typename T>
		using Container			= StaticVector<T, config::CONTAINER_SIZE>;

		using FieldContainer		= Container<Field>;

		constexpr static char PADDING	= config::PADDING;

	private:
		FieldContainer container;

	public:
		class IndexSearcherByName;
		class LinearSearcherByName;
		class PrecomputedSearcherByName;

	private:
		ISAM() = default;

	public:
		explicit ISAM(std::string_view schema){
			auto f = [this](auto const &field){
				if (container.full()){
					container.clear();
					return true;
				}

				container.push_back(field);

				return false;
			};

			iterateSchema(schema, f);
		}


		struct ResultCreateAndSearchByName;

		static ResultCreateAndSearchByName createAndSearchByName(std::string_view schema, std::string_view name);

	public:
		[[nodiscard]]
		size_t size() const{
			return container.size();
		}

		[[nodiscard]]
		size_t bytes() const{
			if (container.empty())
				return 0;

			return container.back().bytes();
		}

		auto getName(size_t id) const{
			return container[id].name;
		}

	public:
		IndexSearcherByName	getIndexSearcherByName()  const;
		LinearSearcherByName	getLinearSearcherByName() const;

	public:
		bool store(char *storage, size_t id, std::string_view value) const{
			return container[id].store(storage, value);
		}

		template<typename SearcherByName, typename ...Args>
		bool store(char *storage, std::string_view value, SearcherByName const &searcher, Args &&...args) const{
			static_assert(
				std::is_same_v<SearcherByName, IndexSearcherByName		> ||
				std::is_same_v<SearcherByName, LinearSearcherByName		> ||
				std::is_same_v<SearcherByName, PrecomputedSearcherByName	>
			);

			if (const auto *field = searcher(std::forward<Args>(args)...); field)
				return field->store(storage, value);

			return false;
		}

	public:
		[[nodiscard]]
		std::string_view load(const char *storage, size_t id) const{
			return container[id].load(storage);
		}

		template<typename SearcherByName, typename ...Args>
		[[nodiscard]]
		std::string_view load(const char *storage, SearcherByName const &searcher, Args &&...args) const{
			static_assert(
				std::is_same_v<SearcherByName, IndexSearcherByName		> ||
				std::is_same_v<SearcherByName, LinearSearcherByName		> ||
				std::is_same_v<SearcherByName, PrecomputedSearcherByName	>
			);

			if (const auto *field = searcher(std::forward<Args>(args)...); field)
				return field->load(storage);

			return {};
		}

	public:
		void print() const{
			for(auto &x : container)
				x.print();
		}

		void print(const char *storage) const{
			for(auto &x : container)
				x.print(storage);
		}

	public:
		template<typename F>
		static bool iterateSchema(std::string_view schema, F &f);

	}; // class ISAM



	class ISAM::IndexSearcherByName{
		friend struct ISAM;

		Container<const Field *> ix;

		IndexSearcherByName(FieldContainer const &container){
			for(auto const &field : container)
				ix.push_back(&field);

			auto cmp = [](const Field *a, const Field *b){
				return a->name < b->name;
			};

			std::sort(ix.begin(), ix.end(), cmp);
		}

		const Field *operator()(std::string_view name) const{
			auto cmp = [](const Field *a, std::string_view b){
				return a->name < b;
			};

			auto it = std::lower_bound(ix.begin(), ix.end(), name, cmp);

			if (it != ix.end() && (*it)->name == name)
				return *it;

			return nullptr;
		}
	};

	class ISAM::LinearSearcherByName{
		friend struct ISAM;

		const FieldContainer *container;

		constexpr LinearSearcherByName(FieldContainer const &container) : container(&container){}

		const Field *operator()(std::string_view name) const{
			for(auto const &field : *container)
				if (field.name == name)
					return &field;

			return nullptr;
		}
	};

	class ISAM::PrecomputedSearcherByName{
		friend struct ISAM;

		constexpr static size_t MAX = std::numeric_limits<size_t>::max();

		const FieldContainer	*container;
		size_t			id		= MAX;

		constexpr PrecomputedSearcherByName(FieldContainer const &container) : container(&container){}

		const Field *operator()() const{
			if (id >= container->size())
				return nullptr;

			auto &item = (*container)[id];

			return & item;
		}

		const Field *operator()(std::string_view name) const{
			if (id >= container->size())
				return nullptr;

			auto &item = (*container)[id];

			if (item.name != name)
				return nullptr;

			return & item;
		}

		constexpr bool empty() const{
			return id == MAX;
		}

		void reset(){
			id = MAX;
		}
	};

	inline auto ISAM::getIndexSearcherByName() const -> IndexSearcherByName{
		return IndexSearcherByName{ container };
	}

	inline auto ISAM::getLinearSearcherByName() const -> LinearSearcherByName{
		return LinearSearcherByName{ container };
	}

	struct ISAM::ResultCreateAndSearchByName{
		ISAM				isam;
		PrecomputedSearcherByName	searcher{ isam.container };

	public:
		ResultCreateAndSearchByName(std::string_view schema, std::string_view name){
			auto &container = isam.container;

			auto f = [&](auto const &field){
				if (container.full()){
					container.clear();
					searcher.reset();

					return true;
				}

				container.push_back(field);

				if (searcher.empty() && field.name == name)
					searcher.id = container.size() - 1;

				return false;
			};

			iterateSchema(schema, f);
		}
	};

	inline auto ISAM::createAndSearchByName(std::string_view schema, std::string_view name) -> ResultCreateAndSearchByName{
		return ResultCreateAndSearchByName{ schema, name };
	}

	template<typename F>
	bool ISAM::iterateSchema(std::string_view schema, F &f){
		// 3:id,10:name,2:country

		using T = size_t;

		StringTokenizer tokenizer{ schema, config::DELIMITER1 };

		size_t off = 0;

		for(std::string_view s : tokenizer){
			StringTokenizer tok{ s, config::DELIMITER2 };
			auto _          = getForwardTokenizer(tok);

			auto const size = from_string<T>(_());

			if (size == 0)
				return false;

			if (off >= std::numeric_limits<T>::max() - size)
				return false;

			auto const name = _();

			Field field{
				off	,
				size	,
				!name.empty() ? name : config::EMPTY_FIELD_NAME
			};

			if (f(field))
				return true;

			off += size;
		}

		return true;
	}

} // namespace ISAM_impl_

using ISAM = ISAM_impl_::ISAM;

#endif

