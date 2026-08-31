#include "shared_mset_multi_fts.h.cc"

namespace FTS::flex_impl_{

	using namespace FTS::impl_;



	struct MergerCountToken : Token{
		size_t			count	= 0;

		MergerCountToken &operator=(Token const &bt){
			static_cast<Token &>(*this) = bt;
			return *this;
		}

		constexpr static bool comp(MergerCountToken const &a, MergerCountToken const &b){
			return a.count > b.count;
		}

		constexpr static bool comp2(MergerCountToken const &a, MergerCountToken const &b){
			// first count >, then sort <, then key <

			if (a.count != b.count)
				return a.count > b.count;

			return less(a, b);
		}
	};



	template <class Iterator, size_t MaxTokens>
	class FTSMergerReducer{
		using IP		= IteratorPair<Iterator>;

		using Container		= StaticVector<IP, MaxTokens>;

		Container		container_;

	public:
		bool push(Iterator begin, Iterator end, char separator, std::string_view prefix){
			IP ip{ begin, end, separator, prefix };

			if (!ip)
				return false;

			container_.push_back(std::move(ip));
			std::push_heap(std::begin(container_), std::end(container_), & IP::comp);

			return true;
		}

		// void clear(){
		// 	container_.clear();
		// }

		auto size() const{
			return container_.size();
		}

		auto empty() const{
			return !size();
		}

		void print_() const{
			for(auto const &x : container_){
				auto const &t = x.getTokenRef();
				logger<Logger::DEBUG>() << "MSetMulti::FTSMerger::print>>>" << t.index << t.key << t.getVal();
			}
		}

		template<typename F>
		MergerCountToken operator()(F &f){
			MergerCountToken t;

			while(!container_.empty()){
				if (f())
					return MergerCountToken{};

				++t.count;

				std::pop_heap(std::begin(container_), std::end(container_), & IP::comp);

				auto &ip = container_.back();

				t = ip.getTokenRef();

				// logger<Logger::DEBUG>() << "MSetMulti::FTSMerger::walk" << "----->" << t.pairKey << t.count;

				++ip;

				if (ip){
					std::push_heap(std::begin(container_), std::end(container_), & IP::comp);
				}else{
					logger<Logger::DEBUG>() << "MSetMulti::FTSMerger::walk" << "remove from the container" << t.index << t.key;

					container_.pop_back();

					if (container_.empty())
						return t;
				}

				if ( t.key != container_.front().getTokenRef().key ){
					// pair index not match
					return t;
				}
			}

			return MergerCountToken{};
		}

		MergerCountToken operator()(std::true_type){
			auto f = [](){
				return false;
			};

			return operator()(f);
		}
	};



	template<typename DBAdapter, size_t MaxTokens>
	class FTS : public FTSBase<flex_impl_::FTSMergerReducer, DBAdapter, MaxTokens>{
		using Base =   FTSBase<flex_impl_::FTSMergerReducer, DBAdapter, MaxTokens>;

		using Base::reducer_		;
		using Base::resultCount_	;

		using OutputHeap		= StaticVector<flex_impl_::MergerCountToken, net::worker::shared::config::ITERATIONS_RESULTS_MAX>;
		OutputHeap			heap_;

	public:
		using SearchTokenContainer	= typename Base::SearchTokenContainer;

		FTS(DBAdapter &db, std::string_view keyN, uint32_t resultCount, std::string_view keyStart) :
							Base(db, keyN, resultCount, keyStart){}

		void process(OutputBlob::Container &container){
			IterationCounter icounter;

			// net::worker::shared::config::ITERATIONS_LOOPS_MAX

			using flex_impl_::MergerCountToken;

			auto comp      = & MergerCountToken::comp;
			auto compFinal = & MergerCountToken::comp2;

			while(!reducer_.empty()){
				// assuming the output list (heap) is full,
				//
				// if front.count is 4 and we have 4 lists,
				// if front.count is 3 and we have 3 lists,
				// ...
				// if front.count is N and we have N lists,
				//
				// then is not possible new elements to come
				// so we are done

				if (heap_.size() >= resultCount_ && heap_.front().count >= reducer_.size()){
					logger<Logger::DEBUG>() << "MSetMulti::FTSFlex::walk" << "collected enough keys. break. iterations" << icounter.iterations();
					break;
				}

				auto const &t = reducer_(icounter);

				if (icounter.stop()){
					logger<Logger::DEBUG>() << "MSetMulti::FTSFlex::walk" << "too many iterations. break.";
					break;
				}

				if (!t.valid)
					continue;

				if (!t.isOK()){
					logger<Logger::DEBUG>() << "MSetMulti::FTSFlex::walk" << "skip tombstone / expired" << t.getKey();

					continue;
				}

				if (heap_.size() < resultCount_){
					heap_.push_back(t);
					std::push_heap(std::begin(heap_), std::end(heap_), comp);

					logger<Logger::DEBUG>() << "MSetMulti::FTSFlex::walk" << "initial push" << t.key << t.getVal() << t.count;

					continue;
				}

				if (!comp(t, heap_.front()))
					continue;

				// remove smallest
				std::pop_heap(std::begin(heap_), std::end(heap_), comp);
				heap_.pop_back();

				// insert bigger
				heap_.push_back(t);
				std::push_heap(std::begin(heap_), std::end(heap_), comp);

				logger<Logger::DEBUG>() << "MSetMulti::FTSFlex::walk" << "push" << t.index << t.key << t.getVal() << t.count;
			}

			logger<Logger::DEBUG>() << "MSetMulti::FTSFlex::walk" << "finished. iterations" << icounter.iterations();

			//std::sort_heap(std::begin(heap), std::end(heap), comp);

			// using different comparator
			std::sort(std::begin(heap_), std::end(heap_), compFinal);

			// collecting the data
			container.clear();

			for(auto const &t : heap_){
				container.push_back(t.key);
				container.push_back(t.getVal());

				// logger<Logger::DEBUG>() << "MSetMulti::FTSFlex::collect" << t.index << t.key << t.getVal() << t.count;
			}
		}

		std::string_view tail(){
			if (reducer_.empty()){
				// merger is inactive
				return "";
			}

			auto const &t = reducer_(std::true_type{});

			return t.getKey();
		}
	};

} // namespace FTS::flex_impl_
