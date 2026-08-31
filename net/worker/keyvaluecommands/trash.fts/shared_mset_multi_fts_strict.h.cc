#include "shared_mset_multi_fts.h.cc"

namespace FTS::strict_impl_{

	using namespace FTS::impl_;



	template <class Iterator, size_t MaxTokens>
	class FTSIntersectorReducer{
		using IP		= IteratorPair<Iterator>;

		using Container		= StaticVector<IP, MaxTokens>;

		Container		container_;

		Token			tMax_;

		bool			active_ = true;

	public:
		bool push(Iterator begin, Iterator end, char separator, std::string_view prefix){
			if (!active_)
				return false;

			IP ip{ begin, end, separator, prefix };

			if (!ip){
				active_ = false;
				return false;
			}

			container_.push_back(std::move(ip));

			if (auto const &t = ip.getTokenRef(); !tMax_.valid || Token::greater(t, tMax_))
				tMax_ = t;

			return true;
		}

		// void clear_(){
		// 	container_.clear();
		// 	active_ = true;
		// }

		// size_t size() const{
		// 	return container_.size();
		// }

		bool empty() const{
			return !active_;
		}

		void print_() const{
			for(auto const &x : container_){
				auto const &t = x.getTokenRef();
				logger<Logger::DEBUG>() << "MSetMulti::FTSIntersector::print>>>" << t.index << t.key << t.getVal();
			}
		}

		template<typename F>
		Token operator()(F &f){
			if (!active_)
				return Token{};

		start: // label for goto

			for(auto &ip : container_){

				while(ip && Token::less(ip.getTokenRef(), tMax_)){
					if (f())
						return Token{};

					// auto const &t = ip.getTokenRef();
					// logger<Logger::DEBUG>() << "MSetMulti::FTSIntersector::walk" << "increase" << t.index << t.key << tMax_.key;

					++ip;
				}

				if (!ip){
					// auto const &t = ip.getTokenRef();
					// logger<Logger::DEBUG>() << "MSetMulti::FTSIntersector::walk" << "remove from the container" << t.index << t.key;

					active_ = false;
					return Token{};
				}

				if (auto const &t = ip.getTokenRef(); Token::greater(t, tMax_)){
					// logger<Logger::DEBUG>() << "MSetMulti::FTSIntersector::walk" << "restart" << t.index << t.key << tMax_.key;

					tMax_ = t;
					goto start;
				}
			}

			// seems like here all it point to max_
			// and all are valid.
			// increasing one it is sufficient.

			// make a copy because we increasing it in a moment.
			auto const t = container_.front().getTokenRef();

			// logger<Logger::DEBUG>() << "MSetMulti::FTSIntersector::walk" << "return" << t.index << t.key;

			++container_.front();

			return t;
		}

		Token operator()(std::true_type){
			auto f = [](){
				return false;
			};

			return operator()(f);
		}
	};



	template<typename DBAdapter, size_t MaxTokens>
	class FTS : public   FTSBase<FTSIntersectorReducer, DBAdapter, MaxTokens>{
		using Base = FTSBase<FTSIntersectorReducer, DBAdapter, MaxTokens>;
		// FTSMergerReducer also works fine, but I did breaking changes...

		using Base::reducer_		;
		using Base::resultCount_	;

		size_t tokensSize_ = 0;

	public:
		using SearchTokenContainer	= typename Base::SearchTokenContainer;

		FTS(DBAdapter &db, std::string_view keyN, uint32_t resultCount, std::string_view keyStart) :
							Base(db, keyN, resultCount, keyStart){}

		void addIPs(SearchTokenContainer const &tokens){
			tokensSize_ = tokens.size();

			Base::addIPs(tokens);
		}

		void process(OutputBlob::Container &container){
			container.clear();

			size_t results = 0;

			IterationCounter icounter;

			while(!reducer_.empty()){
				if (results >= resultCount_){
					logger<Logger::DEBUG>() << "MSetMulti::FTSStrict::walk" << "collected enough keys. break. iterations" << icounter.iterations();
					break;
				}

				auto const &t = reducer_(icounter);

				if (icounter.stop()){
					logger<Logger::DEBUG>() << "MSetMulti::FTSFlex::walk" << "too many iterations. break. iterations" << icounter.iterations();
					break;
				}

				logger<Logger::DEBUG>() << "MSetMulti::FTSStrict::walk" << (t.valid ? "Y" : "N") << t.key << t.getVal() << icounter.iterations();

				if (t.valid){

					if (t.isOK()){
						container.push_back(t.key);
						container.push_back(t.getVal());

						++results;
						logger<Logger::DEBUG>() << "MSetMulti::FTSStrict::walk" << "push" << t.key << t.getVal();
					}else{
						logger<Logger::DEBUG>() << "MSetMulti::FTSStrict::walk" << "skip tombstone / expired" << t.getKey();
					}

				}else{
					logger<Logger::DEBUG>() << "MSetMulti::FTSStrict::walk" << "input stream exhausted. break. iterations" << icounter.iterations();
					break;
				}
			}

			// logger<Logger::DEBUG>() << "MSetMulti::FTSStrict::walk" << "finished. iterations";

			// already sorted

			// already collected
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

} // namespace FTS::strict_impl_
