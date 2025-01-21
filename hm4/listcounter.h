#ifndef LIST_COUNTER_H_
#define LIST_COUNTER_H_

#include "ilist.h"

namespace hm4{

	namespace list_counter_impl_{

		struct ListCounterCount{
			using size_type = config::size_type;

			constexpr auto operator()() const{
				return value;
			}

			constexpr void clr(){
				value = 0;
			}

			constexpr void inc(size_t const){
				++value;
			}

			constexpr void dec(size_t const){
				--value;
			}

			constexpr void upd(size_t const, size_t const) const{
			}

		private:
			size_type	value	= 0;
		};



		struct ListCounterBytes{
			constexpr auto operator()() const{
				return value;
			}

			constexpr void clr(){
				value = 0;
			}

			constexpr void inc(size_t const s){
				value += s;
			}

			constexpr void dec(size_t const s){
				value -= s;
			}

			constexpr void upd(size_t const so, size_t const sn){
				dec(so);
				inc(sn);
			}

		private:
			size_t	value	= 0;
		};



		struct ListCounterAlignedBytes{
			constexpr auto operator()() const{
				return value;
			}

			constexpr void clr(){
				value = 0;
			}

			constexpr void inc(size_t const s){
				value += calc__(s);
			}

			constexpr void dec(size_t const s){
				value -= calc__(s);
			}

			constexpr void upd(size_t const so, size_t const sn){
				dec(so);
				inc(sn);
			}

		private:
			constexpr static size_t calc__(size_t const s){
				return my_align::calc(s, PairConf::ALIGN);
			}

		private:
			size_t	value = 0;
		};

	} // namespace list_counter_impl_



	struct ListCounter{
		constexpr auto size() const{
			return count_();
		}

		constexpr auto bytes() const{
			return bytes_();
		}

	public:
		constexpr void clr(){
			count_	.clr();
			bytes_	.clr();
		}

		constexpr void inc(size_t const s){
			count_	.inc(s);
			bytes_	.inc(s);
		}

		constexpr void dec(size_t const s){
			count_	.dec(s);
			bytes_	.dec(s);
		}

		constexpr void upd(size_t const so, size_t const sn){
			count_	.upd(so, sn);
			bytes_	.upd(so, sn);
		}

	private:
		list_counter_impl_::ListCounterCount	count_;
		list_counter_impl_::ListCounterBytes	bytes_;
	};

} // namespace

#endif

