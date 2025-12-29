#ifndef SHARED_MSET_MULTI_FTS_H_
#define SHARED_MSET_MULTI_FTS_H_

namespace FTS::impl_{

	struct Token{
		bool			valid	= false;

		const hm4::Pair		*pair	= nullptr;

		std::string_view	index;
		std::string_view	sort;
		std::string_view	key;



		auto getVal() const{
			return pair ? pair->getVal() : "";
		}

		auto getKey() const{
			return pair ? pair->getKey() : "";
		}

		auto isOK() const{
			return pair ? pair->isOK() : false;
		}



		template<typename Iterator>
		void updatePair(Iterator it, char separator){
			valid     = true;

			pair = & *it;

			if (it->isOK()){
				// get data
				update_(it->getKey(), separator);
			}else{
				// tombstone
				clearOthers_();
			}
		}



		static constexpr bool greater(Token const &a, Token const &b){
			// first sort_, then key_

			if (int const comp = a.sort.compare(b.sort); comp != 0)
				return comp > 0;

			return a.key  > b.key;
		}

		static constexpr bool less(Token const &a, Token const &b){
			return greater(b, a);
		}

	private:
		constexpr void clearOthers_(){
			index	=	"";
			sort	=	"";
			key	=	"";
		}

		void update_(std::string_view s, char separator){
			StringTokenizer const tok{ s, separator };
			auto _ = getForwardTokenizer(tok);

			// a~index~sort~key
					_(); // 0 = a (keyN)
			index	=	_(); // 1 = index
			sort	=	_(); // 2 = sort
			key	=	_(); // 3 = key (keySub)
		}
	};



	template<typename Iterator>
	class IteratorPair{
		Iterator		it_;
		Iterator		end_;
		char			separator_;
		bool			active_		= true;
		std::string_view	prefix_;

		Token			token_;

	public:
		constexpr IteratorPair(Iterator it, Iterator end, char separator, std::string_view prefix) :
								it_		(it		),
								end_		(end		),
								separator_	(separator	),
								prefix_		(prefix		){
			updateIndex();
		}

		constexpr operator bool() const{
			return active_;
		}

		static constexpr bool comp(IteratorPair const &a, IteratorPair const &b){
			return Token::greater(a.token_, b.token_);
		}

		constexpr auto const &getTokenRef() const{
			return token_;
		}

		void operator ++(){
			++it_;

			updateIndex();
		}

	private:
		constexpr void updateIndex(){
			if (it_ == end_){
				active_ = false;
				return;
			}

			using shared::stop_predicate::StopPrefixPredicate;

			if (StopPrefixPredicate stop{ prefix_ }; stop(it_->getKey())){
				active_ = false;
				return;
			}

			token_.updatePair(it_, separator_);
		}
	};



	template<template<typename, size_t> typename FTSReducer, typename DBAdapter, size_t MaxTokens>
	class FTSBase{
	protected:
		using IT_			= typename DBAdapter::List::iterator;
		using MyFTSReducer		= FTSReducer<IT_, MaxTokens>;

		using PairBufferKeyVector	= StaticVector<hm4::PairBufferKey, MaxTokens>;

		DBAdapter	&db_;

	public:
		using SearchTokenContainer	= StaticVector<std::string_view,   MaxTokens>;

	private:
		std::string_view	keyN_;
		std::string_view	keyStart_;

	protected:
		uint32_t		resultCount_;

		MyFTSReducer		reducer_;

	private:
		PairBufferKeyVector	bufferKeyVector_;

	protected:
		FTSBase(DBAdapter &db, std::string_view keyN, uint32_t resultCount, std::string_view keyStart) :
							db_		(db				),
							keyN_		(keyN				),
							keyStart_	(calcKeyStart__(keyStart)	),
							resultCount_	(resultCount			){}

	private:
		static std::string_view calcKeyStart__(std::string_view s){
			if (s.empty())
				return "";

			return extractNth_(2, DBAdapter::SEPARATOR[0], s);
		}

		constexpr std::string_view calcStart_(hm4::PairBufferKey &bufferKey, std::string_view prefix) const{
			if (keyStart_.empty())
				return prefix;

			return concatenateBuffer(bufferKey, prefix, keyStart_);
		}

	public:
		constexpr bool empty() const{
			return reducer_.empty();
		}

		constexpr auto size() const{
			return reducer_.size();
		}

		void addIPs(SearchTokenContainer const &tokens){
			for(auto const &index : tokens){
				bufferKeyVector_.push_back();
				auto const prefix = makeKeyDataSearch(bufferKeyVector_.back(), DBAdapter::SEPARATOR, keyN_, index);

				hm4::PairBufferKey bufferKey;	// the key needs to be alive until db->find(key)

				auto const key = calcStart_(bufferKey, prefix);

				auto begin = db_->find(key);	// key does not need to be alive any longer
				auto end   = std::end(*db_);

				[[maybe_unused]]
				bool const b = reducer_.push(begin, end, DBAdapter::SEPARATOR[0], prefix);

				logger<Logger::DEBUG>() << "MSetMulti::FTSBase::addIPs" << "index" << index << "prefix" << prefix << "key" << key << "accepted" << (b ? "Y" : "N");
			}
		}
	};



	struct IterationCounter{
		bool operator()(){
			++iterations_;

			stop_ = iterations_ > iterationsMax__;

			return stop_;
		}

		constexpr auto stop() const{
			return stop_;
		}

		constexpr auto iterations() const{
			return iterations_;
		}

	private:
		size_t	iterations_	= 0;
		bool	stop_		= false;

		constexpr static size_t iterationsMax__	= net::worker::shared::config::ITERATIONS_LOOPS_MAX;
	};

} // namespace FTS::impl_

template<typename MDecoder, typename MyFTS, typename DBAdapter, typename Result>
void processFTS(std::string_view keyN, char delimiter, std::string_view tokens, uint32_t resultCount, std::string_view keyStart,
			DBAdapter &db, Result &result, OutputBlob &blob){

	using SearchTokenContainer = typename MyFTS::SearchTokenContainer;

	auto &tokensContainer = blob.construct<SearchTokenContainer>();

	MDecoder decoder;

	if (!decoder.indexesFind(tokens, delimiter, tokensContainer))
		return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

	if (tokensContainer.size() == 1){
		auto const &index = tokensContainer[0];

		return rangeSingle(keyN, index, resultCount, keyStart,
							db, result, blob);
	}else{
		auto fts = blob.construct<MyFTS>(db, keyN, resultCount, keyStart);

		fts.addIPs(tokensContainer);

		if (fts.empty()){
			std::array<std::string_view, 1> container;
			return result.set_container(container);
		}

		auto &container = blob.construct<OutputBlob::Container>();

		fts.process(container);

		container.push_back(fts.tail()); // tail

		return result.set_container(container);
	}
}

namespace FTS{

	template<typename DBAdapter>
	struct BaseMDecoder{
		struct Result{
			std::string_view txt;
			std::string_view sort;
		};

		// mindex and sindex are the same
		static Result value(std::string_view value){
			StringTokenizer const tok{ value, DBAdapter::SEPARATOR[0] };

			auto f = getForwardTokenizer(tok);
			auto l = getBackwardTokenizer(tok);

			auto const txt  = f();
			auto const txt2 = f(); // check that the list is long at least 2 words
			auto const sort = l();

			if (!txt.size() || !txt2.size() || !sort.size())
				return { "", "" };

			return { txt, sort };
		}

		// mindex and sindex are the same
		template<typename Container>
		static bool indexesStored(std::string_view value, Container &container){
			StringTokenizer const tok{ value, DBAdapter::SEPARATOR[0] };

			bool lastElement = false;

			for(auto const &x : tok){
				// sort key is inside
				if (container.full())
					return false;

				if (lastElement || x.empty())
					return false;

				// last element is sort and may not be sorted
				if (!container.empty() && container.back() >= x)
					lastElement = true;

				container.push_back(x);
			}

			// inside must be: at least one token + sort key
			return container.size() >= 2;
		}
	};

} // namespace FTS

#endif

