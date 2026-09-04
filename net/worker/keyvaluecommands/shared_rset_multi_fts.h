#ifndef SHARED_RSET_MULTI_FTS_H_
#define SHARED_RSET_MULTI_FTS_H_

#include <string_view>

#include "staticvector.h"

#include "shared_stoppredicate.h"
#include "shared_extractnth.h"

namespace net::worker::shared::rsetmulti::fts{

	template<typename Iterator>
	class IteratorPair{
		Iterator		it_		;
		Iterator		end_		;
		char			separator_	;
		bool			valid		= true;
		std::string_view	key_		;

	public:
		constexpr IteratorPair(char separator, Iterator it, Iterator end) :
						it_		(it		),
						end_		(end		),
						separator_	(separator	){
			updateKey_();
		}

		constexpr operator bool() const{
			return it_ != end_ && valid;
		}

		constexpr std::string_view operator*() const{
			return key_;
		}

		constexpr std::string_view getKey() const{
			return it_->getKey();
		}

		constexpr bool isOK() const{
			return it_->isOK();
		}

		constexpr void invalidate(){
			// does not work because some iterator can not be assigned
			// it_ = end_;

			valid = false;
		}

		constexpr IteratorPair &operator++(){
			++it_;
			updateKey_();
			return *this;
		}

	private:
		constexpr void updateKey_(){
			if (it_ == end_)
				return;

			key_ = shared::extractnth::extractNth(3 - 1, separator_, it_->getKey());
		}
	};



	template<typename Iterator>
	class TombstoneIteratorPair{
		using StopPrefixPredicate	= shared::stop_predicate::StopPrefixPredicate;
		using StopIterationPredicate	= shared::stop_predicate::StopIterationPredicate;

	private:
		IteratorPair<Iterator>	cursor_;
		StopPrefixPredicate	pstop_;
		StopIterationPredicate	&cstop_;

	public:
		constexpr TombstoneIteratorPair(StopPrefixPredicate pstop, StopIterationPredicate &cstop,
								char separator, Iterator it, Iterator end) :
					cursor_	(separator, it, end	),
					pstop_	(pstop			),
					cstop_	(cstop			){

			// we do not know, if prefix is OK
			if (cursor_ && pstop_(cursor_.getKey()))
				cursor_.invalidate();

			skipTombstones_();
		}

		constexpr operator bool() const{
			return cursor_;
		}

		constexpr std::string_view operator *() const{
			return *cursor_;
		}

		constexpr TombstoneIteratorPair &operator++(){
			++cursor_;
			skipTombstones_();
			return *this;
		}

		constexpr int seek(std::string_view target){
			while(cursor_ && *cursor_ < target){
				if (stop_()){
					cursor_.invalidate();
					return -1;
				}

				operator++();
			}

			if (!cursor_)
				return -1;

			return *cursor_ == target ? 0 : +1;
		}

	private:
		constexpr bool stop_(){
			return pstop_(cursor_.getKey()) || cstop_(*cursor_);
		}

		constexpr bool skipTombstones_(){
			while(cursor_ && !cursor_.isOK()){
				if (stop_()){
					cursor_.invalidate();
					return false;
				}

				++cursor_;
			}

			return cursor_;
		}
	};






	template <class Iterator, size_t MaxTokens>
	class FTSIntersector{
		using IP			= TombstoneIteratorPair<Iterator>;
		using Container			= StaticVector<IP, MaxTokens>;
		using StopIterationPredicate	= shared::stop_predicate::StopIterationPredicate;

	private:
		Container		container_;
		bool			active_		= true;
		StopIterationPredicate	cstop_		= { shared::config::ITERATIONS_LOOPS_MAX };

	public:
		constexpr FTSIntersector() = default;

		constexpr FTSIntersector(size_t maxIterations) :
						cstop_(maxIterations){}

		bool push(std::string_view prefix, char separator, Iterator begin, Iterator end){
			if (!active_)
				return false;

			IP ip{ prefix, cstop_, separator, begin, end };

			if (!ip){
				active_ = false;
				return false;
			}

			container_.push_back(std::move(ip));

			return true;
		}

		constexpr size_t getIterations() const{
			return cstop_.getIterations();
		}

		constexpr operator bool() const{
			return active_ && !container_.empty();
		}

		std::string_view operator()(){
			if (!operator bool())
				return {};

			if (!container_.front()){
				active_ = false;
				return {};
			}

			auto max = *container_.front();

		start: // label for goto

			for(auto &ip : container_){
				int res = ip.seek(max);

				if (res < 0){
					active_ = false;
					return {};
				}

				if (res > 0){
					max = *ip;
					goto start;
				}
			}

			// all IP's points to max and all are valid...
			for(auto &it : container_)
				++it;

			// for completness...
			cstop_.advance(container_.size());

			return max;
		}

		static auto fixItem(std::string_view s, char separator){
			return shared::extractnth::extractNth(1, separator, s);
		}
	};

} // namespace net::worker::shared::rsetmulti::fts

#endif

