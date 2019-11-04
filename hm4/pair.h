#ifndef PAIR_H
#define PAIR_H

#include <cstdint>
#include <cstddef>
#include <ostream>
#include <string_view>

#include "myendian.h"
#include "mynarrow.h"
#include "mystring.h"
#include "comparator.h"

#define log__(...) /* nada */



namespace hm4{
	namespace PairConf{
		constexpr size_t	ALIGN		= sizeof(uint64_t);

		constexpr uint16_t	MAX_KEY_SIZE	=      1024;	// MySQL is 1000
		constexpr uint32_t	MAX_VAL_SIZE	= 16 * 1024;
	}



	struct Pair{
		uint64_t	created;	// 8
		uint32_t	expires;	// 4, 136 years, not that bad.
		uint32_t	vallen;		// 4
		uint16_t	keylen;		// 2
		char		buffer[2];	// dynamic, these are the two \0 terminators.

	public:
		static constexpr int		CMP_NULLKEY	= -1;

	public:
		Pair() = delete;
		Pair(Pair const &) = delete;
		Pair(Pair &&) = delete;
		Pair &operator=(Pair const &) = delete;
		Pair &operator=(Pair &&) = delete;

	public:
		template<class Allocator>
		[[nodiscard]]
		static Pair *createPtr(
				Allocator &allocator,
				std::string_view const key,
				std::string_view const val,
				uint32_t expires = 0, uint32_t created = 0){

			if (	key.size() == 0				||
				key.size() > PairConf::MAX_KEY_SIZE	||
				val.size() > PairConf::MAX_VAL_SIZE	)
				return nullptr;

			Pair *pair = allocator. template allocate<Pair>(Pair::bytes(key.size(), val.size()));

			if (!pair)
				return nullptr;

			return copy_(pair, key, val, expires, created);
		}

		template<class Allocator>
		[[nodiscard]]
		static Pair *tombstonePrt(
				Allocator &allocator,
				std::string_view const key){

			using namespace std::literals;
			return createPtr(allocator, key, ""sv);
		}

		template<class Allocator>
		[[nodiscard]]
		static Pair *clonePtr(Allocator &allocator, const Pair *src){
			if (!src)
				return nullptr;

			Pair *pair = allocator. template allocate<Pair>(src->bytes());

			if (!pair)
				return nullptr;

			memcpy(pair, src, src->bytes());

			return pair;
		}

		template<class Allocator>
		[[nodiscard]]
		static Pair *clonePtr(Allocator &allocator, const Pair &src){
			return clonePtr(allocator, & src);
		}

		template<class Allocator>
		static void destroyPtr(Allocator &allocator, Pair *pair){
			return allocator.deallocate(pair);
		}

	public:
		template<class Allocator>
		static /* std::unique_ptr */ auto create(
				Allocator &allocator,
				std::string_view const key,
				std::string_view const val,
				uint32_t expires = 0, uint32_t created = 0){

			return allocator.getUP(
				createPtr(allocator, key, val, expires, created)
			);
		}

		template<class Allocator>
		static /* std::unique_ptr */ auto tombstone(
				Allocator &allocator,
				std::string_view const key){

			return allocator.getUP( tombstonePrt(allocator, key) );
		}

		template<class Allocator>
		static /* std::unique_ptr */ auto clone(Allocator &allocator, const Pair *src){
			return allocator.getUP( clonePtr(allocator, src) );
		}

		template<class Allocator>
		static /* std::unique_ptr */ auto clone(Allocator &allocator, const Pair &src){
			return allocator.getUP( clonePtr(allocator, src) );
		}

	private:
		static Pair *copy_(Pair *pair,
				std::string_view key,
				std::string_view val,
				uint32_t expires, uint32_t created);

	public:
		constexpr
		bool empty() const noexcept{
			return keylen;
		}

	public:
		std::string_view  getKey() const noexcept{
			return { getKey_(), getKeyLen_() };
		}

		std::string_view  getVal() const noexcept{
			return { getVal_(), getValLen_() };
		}

		bool isTombstone() const noexcept{
			return vallen == 0;
		}

		uint64_t getCreated() const noexcept{
			return betoh<uint64_t>(created);
		}

	public:
		int cmp(std::string_view const key) const noexcept{
			return std::empty(key) ?
				CMP_NULLKEY :
				::compare(getKey_(), getKeyLen_(), key.data(), key.size());
		}

	public:
		bool equals(std::string_view const key) const noexcept{
			return std::empty(key) ?
				false :
				::equals(getKey_(), getKeyLen_(), key.data(), key.size());
		}

	public:
		int cmpTime(const Pair &pair) const noexcept{
			return comparator::comp(
				getCreated(),
				pair.getCreated()
			);
		}

		int cmpTime(const Pair *pair) const noexcept{
			return cmpTime(*pair);
		}

	public:
		bool isValid(bool const tombstoneCheck = false) const noexcept{
			if ( tombstoneCheck && isTombstone() )
				return false;

			if ( isExpired_() )
				return false;

			// finally all OK
			return true;
		}

		bool isValid(const Pair &, bool const tombstoneCheck = false) const noexcept{
			return isValid(tombstoneCheck);
		}

		bool isValid(const Pair *pair, bool const tombstoneCheck = false) const noexcept{
			return isValid(*pair, tombstoneCheck);
		}

		size_t bytes() const noexcept{
			return bytes(getKeyLen_(), getValLen_());
		}

		// ==============================

		void print() const noexcept;

		void fwrite(std::ostream & os) const{
			os.write((const char *) this, narrow<std::streamsize>( bytes() ) );
		}

		// ==============================

		constexpr
		static size_t bytes(size_t const keyLen, size_t const valLen) noexcept{
			return sizeof(Pair) + keyLen + valLen;
		}

		constexpr
		static size_t maxBytes() noexcept{
			return bytes(PairConf::MAX_KEY_SIZE, PairConf::MAX_VAL_SIZE);
		}

	private:
		const char *getKey_() const noexcept{
			return buffer;
		}

		const char *getVal_() const noexcept{
			return & buffer[ getKeyLen_() + 1 ];
		}

		size_t getKeyLen_() const noexcept{
			return betoh<uint16_t>(keylen);
		}

		size_t getValLen_() const noexcept{
			return betoh<uint32_t>(vallen);
		}

	private:
		bool isExpired_() const noexcept;

		static uint64_t getCreateTime__(uint32_t created) noexcept;

	} __attribute__((__packed__));

	static_assert(std::is_trivial<Pair>::value, "Pair must be POD type");



	inline void print(Pair const &pair){
		pair.print();
	}

} // namespace

#endif

