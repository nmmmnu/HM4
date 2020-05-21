#ifndef List_DB_ADAPTER_H_
#define List_DB_ADAPTER_H_

#include "mystring.h"

#include <iostream>
#include <algorithm>

#include "fixedvector.h"

#include "version.h"

namespace accumulator_impl_{
	#if 0
	template<size_t>
	using GetXVector = std::vector<std::string_view>;
	#else
	template<size_t SIZE>
	using GetXVector = FixedVector<std::string_view, SIZE>;
	#endif

	inline bool samePrefix(std::string_view const p, std::string_view const s){
		if (p.size() > s.size())
			return false;

		return std::equal(std::begin(p), std::end(p), std::begin(s));
	}

	template<class Accumulator, class List, typename... Args>
	auto accumulate_(List const &list, std::string_view const key, uint16_t const maxResults, std::string_view const prefix, Args&&... args){
		auto it = key.empty() ? std::begin(list) : list.find(key, std::false_type{} );

		uint16_t count = 0;

		Accumulator accumulator(std::forward<Args>(args)...);

		for(; it != std::end(list); ++it){
			auto const &key = it->getKey();

			if (++count > maxResults)
				return accumulator.result(key);

			if (prefix.empty() == false && ! samePrefix(prefix, key))
				return accumulator.result();

			if (it->isValid(std::true_type{}))
				accumulator(*it);
		}

		return accumulator.result();
	}

	template<class Accumulator, class List, typename... Args>
	auto accumulatePair_(List const &list, std::string_view const key, uint16_t const maxResults, std::string_view const prefix, Args&&... args){
		auto const [ data, lastKey ] = accumulate_<Accumulator>(list, key, maxResults, prefix, std::forward<Args>(args)...);

		return std::array<std::string, 2>{
			std::to_string(data),
			{ lastKey.data(), lastKey.size() }
		};
	}

	// Accumulator implementations

	template<class Vector>
	struct AccumulatorVector{
		Vector &data;

		AccumulatorVector(Vector &data) : data(data){}

		void operator()(hm4::Pair const &pair){
			data.emplace_back(pair.getKey());

			if (pair.isValid(std::true_type{}))
				data.emplace_back(pair.getVal());
			else
				data.emplace_back();
		}

		constexpr static void result(std::string_view = ""){
		}
	};

	template<class Vector>
	struct AccumulatorVectorNew{
		Vector &data;

		AccumulatorVectorNew(Vector &data) : data(data){}

		void operator()(hm4::Pair const &pair){
			if (pair.isValid(std::true_type{})){
				data.emplace_back(pair.getKey());
				data.emplace_back(pair.getVal());
			}
		}

		void result(std::string_view key = ""){
			// emplace even empty
			data.emplace_back(key);
		}
	};

	template<class T>
	struct AccumulatorCount{
		T data = 0;

		void operator()(hm4::Pair const &){
			++data;
		}

		auto result(std::string_view key = "") const{
			return std::make_pair(data, key);
		}
	};

	template<class T>
	struct AccumulatorSum{
		T data = 0;

		void operator()(hm4::Pair const &pair){
			data += from_string<T>(pair.getVal());
		}

		auto result(std::string_view key = "") const{
			return std::make_pair(data, key);
		}
	};

} // accumulator_impl_

template<class List, class COMMAND=std::nullptr_t>
class ListDBAdapter{
public:
	constexpr static uint16_t DEFAULT_RESULTS = 10;
	constexpr static uint16_t MAXIMUM_RESULTS = 1000;

public:
	constexpr static bool MUTABLE = ! std::is_const_v<List>;

public:
	ListDBAdapter(List &list, COMMAND &cmd) :
				list_(list),
				cmd_(& cmd){}

	ListDBAdapter(List &list) :
				ListDBAdapter(list, nullptr){}

public:
	// Immutable Methods

	std::string_view get(std::string_view const key) const{
		assert(!key.empty());

		return getVal_( list_.find(key, std::true_type{} ) );
	}

	auto getall(std::string_view const key, uint16_t const resultsCount, std::string_view const prefix) const{
		auto const maxResults = std::clamp(resultsCount,  DEFAULT_RESULTS, MAXIMUM_RESULTS);

		using namespace accumulator_impl_;

		using MyVector = GetXVector<2 * MAXIMUM_RESULTS>;

		MyVector result;

		result.reserve(maxResults * 2);

		using Accumulator = AccumulatorVector<MyVector>;

		accumulate_<Accumulator>(list_, key, maxResults, prefix, result);

		return result;
	}

	auto getx(std::string_view const key, uint16_t const resultsCount, std::string_view const prefix) const{
		auto const maxResults = std::clamp(resultsCount,  DEFAULT_RESULTS, MAXIMUM_RESULTS);

		using namespace accumulator_impl_;

		using MyVector = GetXVector<2 * MAXIMUM_RESULTS + 1>;

		MyVector result;

		result.reserve(maxResults * 2 + 1);

		using Accumulator = AccumulatorVectorNew<MyVector>;

		accumulate_<Accumulator>(list_, key, maxResults, prefix, result);

		return result;
	}

	auto count(std::string_view const key, uint16_t const resultsCount, std::string_view const prefix) const{
		auto const maxResults  = std::clamp(resultsCount,  DEFAULT_RESULTS, MAXIMUM_RESULTS);

		using namespace accumulator_impl_;

		using Accumulator = AccumulatorCount<uint16_t>;

		return accumulatePair_<Accumulator>(list_, key, maxResults, prefix);
	}

	auto sum(std::string_view const key, uint16_t const resultsCount, std::string_view const prefix) const{
		auto const maxResults  = std::clamp(resultsCount,  DEFAULT_RESULTS, MAXIMUM_RESULTS);

		using namespace accumulator_impl_;

		using Accumulator = AccumulatorSum<int64_t>;

		return accumulatePair_<Accumulator>(list_, key, maxResults, prefix);
	}

	std::string info() const{
		to_string_buffer_t buffer[2];

		return concatenate(
			"Version          : ", hm4::version::str,			"\n",
			"Keys (estimated) : ", to_string(list_.size(),  buffer[0]),	"\n",
			"Size             : ", to_string(list_.bytes(), buffer[1]),	"\n",
			"Mutable          : ", MUTABLE ? "Yes" : "No",			"\n"
		);
	}

	bool refresh(bool const completeRefresh){
		return refresh__(completeRefresh, cmd_);
	}

private:
	template<class T>
	static bool refresh__(bool const completeRefresh, T *cmd){
		return cmd && cmd->command(completeRefresh);
	}

	constexpr
	static bool refresh__(bool const /* completeRefresh */, std::nullptr_t *){
		return false;
	}

public:
	// Mutable Methods

	void set(std::string_view const key, std::string_view const val, std::string_view const exp = {} ){
		assert(!key.empty());

		auto const expires = from_string<uint32_t>(exp);

		list_.insert(key, val, expires);
	}

	bool del(std::string_view const key){
		assert(!key.empty());

		return list_.erase(key);
	}

	std::string_view incr(std::string_view const key, int64_t const val){
		assert(!key.empty());

		const auto p = list_.find(key, std::true_type{} );

		int64_t n = val;

		if (p != std::end(list_) && p->isValid(std::true_type{}))
			n += from_string<int64_t>(p->getVal());

		to_string_buffer_t buffer;

		std::string_view const val_n = to_string(n, buffer);

		return getVal_( list_.insert(key, val_n) );
	}

private:
	std::string_view getVal_(typename List::iterator const &it) const{
		if (it != std::end(list_) && it->isValid(std::true_type{}))
			return it->getVal();
		else
			return {};
	}

private:
	List		&list_;
	COMMAND		*cmd_		= nullptr;
};

#endif

