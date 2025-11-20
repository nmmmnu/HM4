// we are in:
// namespace net::worker::commands::MIndex



namespace IXMRANGEFLEX_impl_{

	struct BaseToken{
		std::string_view	pkey;
		std::string_view	pval;

		std::string_view	index;
		std::string_view	sort;
		std::string_view	key;

		constexpr operator bool() const{
			return !index.empty();
		}

		constexpr void clearOthers(){
			index	=	"";
			sort	=	"";
			key	=	"";
		}
	};

	struct Token : BaseToken{
		size_t			count	= 0;

		Token &operator=(BaseToken const &bt){
			static_cast<BaseToken &>(*this) = bt;
			return *this;
		}

		constexpr static bool comp(Token const &a, Token const &b){
			return a.count > b.count;
		}

		constexpr static bool comp2(Token const &a, Token const &b){
			// first count >, then sort <, then key <

			if (a.count != b.count)
				return a.count > b.count;

			if (int comp = a.sort.compare(b.sort); a != 0)
				return comp < 0;

			return a.key < b.key;
		}
	};



	template<typename Iterator>
	class IteratorPair{
		Iterator		it_;
		Iterator		end_;
		char			separator_;
		bool			active_		= true;
		std::string_view	prefix_;

		BaseToken		token_;

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
			// first sort_, then key_

			if (int comp = a.token_.sort.compare(b.token_.sort); a != 0)
				return comp > 0;

			return a.token_.key  > b.token_.key;
		}

		constexpr auto const &getKey() const{
			return token_.key;
		}

		constexpr auto const &getPairKey() const{
			return token_.pkey;
		}

		constexpr auto const &getToken() const{
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

			token_.pkey = it_->getKey();
			token_.pval = it_->getVal();

			if (it_->isOK()){
				StringTokenizer const tok{ it_->getKey(), separator_ };
				auto _ = getForwardTokenizer(tok);

				// a~index~sort~key
							_(); // 0 = a (keyN)
				token_.index	=	_(); // 1 = index
				token_.sort	=	_(); // 2 = sort
				token_.key	=	_(); // 3 = key (keySub)
			}else{
				// tombstone

				token_.clearOthers();
			}
		}
	};



	template <class Iterator, size_t MaxTokens>
	class FTSMerger{
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

		void reset(){
			container_.reset();
		}

		auto size() const{
			return container_.size();
		}

		auto empty() const{
			return !size();
		}

		operator bool() const{
			return size();
		}

		void print_() const{
			for(auto &x : container_){
				Token t = x.getToken();
				logger<Logger::DEBUG>() << "IXMRANGE*::Merger::print>>>" << t.index << t.key << t.pval << t.count;
			}
		}

		Token operator()(){
			Token t;

			while(!container_.empty()){
				++t.count;

				std::pop_heap(std::begin(container_), std::end(container_), & IP::comp);

				auto &ip = container_.back();

				t = ip.getToken();

			//	logger<Logger::DEBUG>() << "IXMRANGE*::Merger::walk" << "----->" << t.pkey << t.count;

				++ip;

				if (ip){
					std::push_heap(std::begin(container_), std::end(container_), & IP::comp);
				}else{
					logger<Logger::DEBUG>() << "IXMRANGE*::Merger::walk" << "remove from the container" << t.index << t.key;

					container_.pop_back();

					if (container_.empty()){
						return t;
					}
				}

				if ( t.key != container_.front().getKey() ){
					// pair index not match
					return t;
				}
			}

			return Token{};
		}
	};



	template<typename DBAdapter, size_t MaxTokens>
	class FTSBase{
	protected:
		using IT_			= typename DBAdapter::List::iterator;
		using MyFTSMerger		= FTSMerger<IT_, MaxTokens>;

		using PairBufferKeyVector	= StaticVector<hm4::PairBufferKey, MaxTokens>;
		using TokenContainer		= StaticVector<std::string_view,   MaxTokens>;

		DBAdapter		&db_;

	private:
		std::string_view	keyN_;
		std::string_view	keyStart_;

	protected:
		uint32_t		resultCount_;

		MyFTSMerger		merger_;

	private:
		PairBufferKeyVector	bufferKeyVector_;

	protected:
		FTSBase(DBAdapter &db, std::string_view keyN, uint32_t resultCount, std::string_view keyStart) :
							db_		(db				),
							keyN_		(keyN				),
							keyStart_	(calcKeyStart__(keyStart)	),
							resultCount_	(resultCount			){}

		constexpr size_t getIterationsMax() const{
			auto const max = net::worker::shared::config::ITERATIONS_LOOPS_MAX;
			auto const cnt = 3;

			static_assert(cnt > 0);

			if (merger_.size() <= cnt)
				return max;
			else
				return max / (merger_.size() - cnt);
		}

	private:
		static std::string_view calcKeyStart__(std::string_view s){
			if (s.empty())
				return "";

			namespace PN = mindex_impl_;

			return PN::extractNth_(2, DBAdapter::SEPARATOR[0], s);
		}

		constexpr std::string_view calcStart_(hm4::PairBufferKey &bufferKey, std::string_view prefix) const{
			if (keyStart_.empty())
				return prefix;

			return concatenateBuffer(bufferKey, prefix, keyStart_);
		}

	public:
		constexpr bool empty() const{
			return merger_.empty();
		}

		constexpr auto size() const{
			return merger_.size();
		}

		void addIPs(TokenContainer const &tokens){
			namespace PN = mindex_impl_;

			for(auto const &index : tokens){
				bufferKeyVector_.push_back();
				auto const prefix = PN::makeKeyDataSearch(bufferKeyVector_.back(), DBAdapter::SEPARATOR, keyN_, index);

				hm4::PairBufferKey bufferKey;	// the key needs to be alive until db->find(key)

				auto const key = calcStart_(bufferKey, prefix);

				auto begin = db_->find(key);	// key does not need to be alive any longer
				auto end = std::end(*db_);

				[[maybe_unused]]
				bool const b = merger_.push(begin, end, DBAdapter::SEPARATOR[0], prefix);

				logger<Logger::DEBUG>() << "IXMRANGE*::addIPs" << "index" << index << "prefix" << prefix << "key" << key << "accepted" << (b ? "Y" : "N");
			}
		}
	};


	template<typename DBAdapter, size_t MaxTokens>
	class FTSFlex : public FTSBase<DBAdapter, MaxTokens>{
		using Base = FTSBase<DBAdapter, MaxTokens>;

		using Base::merger_		;
		using Base::resultCount_	;

		using OutputHeap	= StaticVector<Token, net::worker::shared::config::ITERATIONS_RESULTS_MAX>;
		OutputHeap		heap_;

	public:
		FTSFlex(DBAdapter &db, std::string_view keyN, uint32_t resultCount, std::string_view keyStart) :
							Base(db, keyN, resultCount, keyStart){}

		void process(OutputBlob::Container &container){
			size_t iterations = 0;

			auto const iterationsMax = this->getIterationsMax();

			while(merger_){
				if (++iterations > iterationsMax){
					logger<Logger::DEBUG>() << "IXMRANGEFLEX::walk" << "too many iterations. break";
					break;
				}

				// assuming the output list (heap) is full,
				//
				// if front.count is 4 and we have 4 lists,
				// if front.count is 3 and we have 3 lists,
				// ...
				// if front.count is N and we have N lists,
				//
				// then is not possible new elements to come
				// so we are done

				if (heap_.size() >= resultCount_ && heap_.front().count >= merger_.size()){
					logger<Logger::DEBUG>() << "IXMRANGEFLEX::walk" << "collected enough keys. break. iterations" << iterations;
					break;
				}

				auto const &t = merger_();

				if (!t)
					continue;

				if (heap_.size() < resultCount_){
					heap_.push_back(t);
					std::push_heap(std::begin(heap_), std::end(heap_), & Token::comp);

				//	logger<Logger::DEBUG>() << "IXMRANGEFLEX::walk" << "initial push" << t.key << t.val << t.count;

					continue;
				}

				if (!Token::comp(t, heap_.front()))
					continue;

				// remove smallest
				std::pop_heap(std::begin(heap_), std::end(heap_), & Token::comp);
				heap_.pop_back();

				// insert bigger
				heap_.push_back(t);
				std::push_heap(std::begin(heap_), std::end(heap_), & Token::comp);

			//	logger<Logger::DEBUG>() << "IXMRANGEFLEX::walk" << "push" << t.index << t.key << t.val << t.count;
			}

			logger<Logger::DEBUG>() << "IXMRANGEFLEX::walk" << "finished. iterations" << iterations;

			//std::sort_heap(std::begin(heap), std::end(heap), & Token::comp);

			// using different comparator
			std::sort(std::begin(heap_), std::end(heap_), & Token::comp2);

			// collecting the data
			container.clear();

			for(auto const &t : heap_){
				container.push_back(t.key);
				container.push_back(t.pval);

				// logger<Logger::DEBUG>() << "IXMRANGEFLEX::collect" << t.index << t.key << t.val << t.count;
			}
		}

		std::string_view tail(){
			if (!merger_){
				// merger is inactive
				return "";
			}

			auto const &t = merger_();

			return t.pkey;
		}
	};



	template<typename DBAdapter, size_t MaxTokens>
	class FTSStrict : public FTSBase<DBAdapter, MaxTokens>{
		using Base = FTSBase<DBAdapter, MaxTokens>;

		using Base::merger_		;
		using Base::resultCount_	;

		size_t tokensSize_;

	public:
		FTSStrict(DBAdapter &db, std::string_view keyN, uint32_t resultCount, size_t tokensSize, std::string_view keyStart) :
							Base(db, keyN, resultCount, keyStart),
								tokensSize_(tokensSize){}

		void process(OutputBlob::Container &container){
			container.clear();

			size_t results = 0;

			size_t iterations = 0;

			auto const iterationsMax = this->getIterationsMax();

			while(merger_){
				if (++iterations > iterationsMax){
					logger<Logger::DEBUG>() << "IXMRANGESTRICT::walk" << "too many iterations. break";
					break;
				}

				if (results >= resultCount_){
					logger<Logger::DEBUG>() << "IXMRANGESTRICT::walk" << "collected enough keys. break. iterations" << iterations;
					break;
				}

				if (merger_.size() < tokensSize_){
					logger<Logger::DEBUG>() << "IXMRANGESTRICT::walk" << "input stream exhausted. break. iterations" << iterations;
					break;
				}

				auto const &t = merger_();

				if (t && t.count == tokensSize_){
					container.push_back(t.key);
					container.push_back(t.pval);

					++results;
				//	logger<Logger::DEBUG>() << "IXMRANGESTRICT::walk" << "push" << t.key << t.val << t.count;
				}
			}

			logger<Logger::DEBUG>() << "IXMRANGESTRICT::walk" << "finished. iterations" << iterations;

			// already sorted

			// already collected
		}

		std::string_view tail(){
			if (!merger_){
				// merger is inactive
				return "";
			}

			if (merger_.size() < tokensSize_){
				// input stream exhausted.
				return "";
			}

			auto const &t = merger_();

			return t.pkey;
		}
	};



} // namespace IXMRANGEFLEX_impl_



// we are in:
// namespace net::worker::commands::MIndex

template<class Protocol, class DBAdapter>
void IXMRANGEFLEX<Protocol, DBAdapter>::process__(std::string_view keyN, TokenContainer &tokens, uint32_t resultCount, std::string_view keyStart,
			DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){

	using namespace IXMRANGEFLEX_impl_;

	using MyFTS = FTSFlex<DBAdapter, MaxTokens>;

	auto fts = blob.construct<MyFTS>(db, keyN, resultCount, keyStart);

	fts.addIPs(tokens);

	if (fts.empty()){
		std::array<std::string_view, 1> container;
		return result.set_container(container);
	}

	auto &container = blob.construct<OutputBlob::Container>();

	fts.process(container);

	container.push_back(fts.tail()); // tail

	return result.set_container(container);
}

template<class Protocol, class DBAdapter>
void IXMRANGESTRICT<Protocol, DBAdapter>::process__(std::string_view keyN, TokenContainer &tokens, uint32_t resultCount, std::string_view keyStart,
			DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){

	using namespace IXMRANGEFLEX_impl_;

	using MyFTS = FTSStrict<DBAdapter, MaxTokens>;

	auto fts = blob.construct<MyFTS>(db, keyN, resultCount, tokens.size(), keyStart);

	fts.addIPs(tokens);

	if (fts.size() != tokens.size()){
		std::array<std::string_view, 1> container;
		return result.set_container(container);
	}

	auto &container = blob.construct<OutputBlob::Container>();

	fts.process(container);

	container.push_back(fts.tail()); // tail

	return result.set_container(container);
}

// we are in:
// namespace net::worker::commands::MIndex

