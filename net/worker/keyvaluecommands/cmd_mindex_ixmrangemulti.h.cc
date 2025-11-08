// we are in:
// namespace net::worker::commands::MIndex



namespace ixmrangemulti_impl_{

	struct Token{
		std::string_view	index;
		std::string_view	sort;
		std::string_view	key;
		std::string_view	val;
		size_t			count	= 0;

		constexpr operator bool() const{
			return !index.empty();
		}

		static constexpr bool comp2(Token const &a, Token const &b){
			if (a.count == b.count){
				int const comp = a.sort.compare(b.sort);

				if (comp == 0)
					return a.key < b.key;

				return comp < 0;
			}

			return a.count > b.count;
		}

		static constexpr bool comp(Token const &a, Token const &b){
			return a.count > b.count;
		}
	};



	template<typename Iterator>
	class IteratorPair{
		Iterator		it_;
		Iterator		end_;
		char			separator_;
		bool			active_		= true;
		std::string_view	prefix_;

		std::string_view	index_;
		std::string_view	key_;
		std::string_view	sort_; // suppose to be better for the cache-line ;)
		std::string_view	val_;

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
			return a.key_ > b.key_;
		}

		constexpr auto const &getKey() const{
			return key_;
		}

		constexpr void getToken(Token &t) const{
				t.index	= index_	;
				t.sort	= sort_		;
				t.key	= key_		;
				t.val	= val_		;
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

			StringTokenizer const tok{ it_->getKey(), separator_ };
			auto _ = getForwardTokenizer(tok);

			// a~index~sort~key

					_(); // 0 = a (keyN)
			index_	=	_(); // 1 = index
			sort_	=	_(); // 2 = sort
			key_	=	_(); // 3 = key (keySub)

			val_	=	it_->getVal();
		}
	};



	template <class Iterator, size_t MaxTokens>
	class Walker{
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

		operator bool(){
			return size();
		}

		void print_() const{
			for(auto &x : container_){
				Token t;
				x.getToken(t);
				logger<Logger::DEBUG>() << "Walker::print>>>" << t.index << t.key << t.val << t.count;
			}
		}

		Token operator()(){
			Token t;

			while(!container_.empty()){
				++t.count;

				std::pop_heap(std::begin(container_), std::end(container_), & IP::comp);

				auto &ip = container_.back();

				ip.getToken(t);

				++ip;

				if (ip){
					std::push_heap(std::begin(container_), std::end(container_), & IP::comp);
				}else{
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
	class FTS{
		using IT_			= typename DBAdapter::List::iterator;
		using MyWalker			= Walker<IT_, MaxTokens>;

		using PairBufferKeyVector	= StaticVector<hm4::PairBufferKey, MaxTokens>;
		using TokenContainer		= StaticVector<std::string_view,   MaxTokens>;

		using OutputHeap		= StaticVector<Token, net::worker::shared::config::ITERATIONS_RESULTS_MAX>;

		DBAdapter		&db;
		std::string_view	keyN;
		uint32_t		resultCount;

		MyWalker		walker;
		PairBufferKeyVector	bufferKeyVector;
		OutputHeap		heap;

	public:
		FTS(DBAdapter &db, std::string_view keyN, uint32_t resultCount) :
							db		(db		),
							keyN		(keyN		),
							resultCount	(resultCount	){}

		constexpr bool empty() const{
			return walker.empty();
		}

		void addIPs(TokenContainer const &tokens){
			namespace PN = mindex_impl_;

			for(auto const &index : tokens){
				bufferKeyVector.push_back();
				auto const prefix = PN::makeKeyDataSearch(bufferKeyVector.back(), DBAdapter::SEPARATOR, keyN, index);
				auto const key = prefix;

				auto begin = db->find(key);
				auto end = std::end(*db);

				[[maybe_unused]]
				bool const b = walker.push(begin, end, DBAdapter::SEPARATOR[0], prefix);

				logger<Logger::DEBUG>() << "IXMRANGEMULTI::addIPs" << "index" << index << "key" << key << "push accepted" << (b ? "Y" : "N");
			}
		}

		void walk(){
			while(walker){
				// assuming the output list (heap) is full,
				//
				// if front.count is 4 and we have 4 lists,
				// if front.count is 3 and we have 3 lists,
				// ...
				// if front.count is N and we have N lists,
				//
				// then is not possible new elements to come
				// so we are done

				if (heap.size() >= resultCount && heap.front().count >= walker.size()){
					// logger<Logger::DEBUG>() << "IXMRANGEMULTI::walk" << "collected enough keys. break";
					break;
				}

				auto const &t = walker();

				if (!t)
					continue;

				if (heap.size() < resultCount){
					heap.push_back(t);
					std::push_heap(std::begin(heap), std::end(heap), & Token::comp);

					// logger<Logger::DEBUG>() << "IXMRANGEMULTI::walk" << "initial push" << t.key << t.val << t.count;

					continue;
				}

				if (!Token::comp(t, heap.front()))
					continue;

				// remove smallest
				std::pop_heap(std::begin(heap), std::end(heap), & Token::comp);
				heap.pop_back();

				// insert bigger
				heap.push_back(t);
				std::push_heap(std::begin(heap), std::end(heap), & Token::comp);

				// logger<Logger::DEBUG>() << "IXMRANGEMULTI::walk" << "push" << t.index << t.key << t.val << t.count;
			}

			//std::sort_heap(std::begin(heap), std::end(heap), & Token::comp);

			// using different comparator
			std::sort(std::begin(heap), std::end(heap), & Token::comp2);
		}

		void collect(OutputBlob::Container &container) const{
			for(auto &t : heap){
				container.push_back(t.key);
				container.push_back(t.val);

				// logger<Logger::DEBUG>() << "IXMRANGEMULTI::collect" << t.index << t.key << t.val << t.count;
			}
		}
	};

} // namespace ixmrangemulti_impl_



// we are in:
// namespace net::worker::commands::MIndex

template<class Protocol, class DBAdapter>
void IXMRANGEMULTI<Protocol, DBAdapter>::processMulti__(std::string_view keyN, TokenContainer &tokens, uint32_t resultCount, std::string_view keyStart,
			DBAdapter &db, Result<Protocol> &result, OutputBlob &blob){

	(void)keyStart;

	using namespace ixmrangemulti_impl_;

	using MyFTS = FTS<DBAdapter, MaxTokens>;

	auto fts = blob.construct<MyFTS>(db, keyN, resultCount);

	fts.addIPs(tokens);

	if (fts.empty()){
		std::array<std::string_view, 1> container;
		return result.set_container(container);
	}

	fts.walk();

	auto &container = blob.construct<OutputBlob::Container>();

	fts.collect(container);

	container.push_back(""); // tail

	return result.set_container(container);
}

// we are in:
// namespace net::worker::commands::MIndex

