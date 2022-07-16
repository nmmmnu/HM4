#ifndef PAIR_H
#define PAIR_H

#include <cstdint>
#include <cstring>

#include <ostream>
#include <memory>
#include <string_view>

#include "myendian.h"
#include "mynarrow.h"
#include "myalign.h"
#include "mystring.h"
#include "comparator.h"

#include "baseallocator.h"

namespace hm4{
inline namespace version_2_00_00{

	namespace PairConf{
		constexpr uint32_t	VERSION		= 20000;

		constexpr size_t	ALIGN		= sizeof(void *);

		constexpr uint16_t	MAX_KEY_SIZE	= 0b0'0000'0000'0011'1111'1111;	// 1023, MySQL is 1000
		constexpr uint32_t	MAX_VAL_SIZE	= 0b0'1111'1111'1111'1111'1111; // 1048575, 1 MB
		constexpr uint16_t	MAX_VAL_MASK	= 0b0'0000'1111'0000'0000'0000;
		constexpr size_t	MAX_VAL_MASK_SH	= 4;

		constexpr uint16_t	MAX_KEY_MASK	= MAX_KEY_SIZE;

		constexpr const char	*EMPTY_MESSAGE	= "---pair-is-empty---";
	}



	struct Pair{
		uint64_t	created;	// 8
		uint32_t	expires;	// 4, 136 years, not that bad.
		uint16_t	keylen;		// 2, 4 bits are used for vallen. 2 bits are reserved for future versions.
		uint16_t	vallen;		// 2
		char		buffer[2];	// dynamic, these are the two \0 terminators.

	public:
		static constexpr int			CMP_NULLKEY	= -1;

		inline
		static constexpr std::string_view	TOMBSTONE	= "";

		enum class WriteOptions : bool{
			NONE	= false,
			ALIGNED	= true
		};

	public:
		Pair() = delete;
		Pair(Pair const &) = delete;
		Pair(Pair &&) = delete;
		Pair &operator=(Pair const &) = delete;
		Pair &operator=(Pair &&) = delete;

	public:
		[[nodiscard]]
		constexpr static bool check(
					std::string_view const key
				) noexcept{
			return key.size() > 0 && key.size() <= PairConf::MAX_KEY_SIZE;
		}

		[[nodiscard]]
		constexpr static bool check(
					std::string_view const key,
					std::string_view const val
				) noexcept{
			return check(key) && val.size() <= PairConf::MAX_VAL_SIZE;
		}

	public:
		static void createInRawMemory(Pair *pair,
				std::string_view key,
				std::string_view val,
				uint32_t expires, uint32_t created) noexcept;

		static void cloneInRawMemory(Pair *pair, const Pair &src) noexcept{
			memcpy((void *) pair, & src, src.bytes());
		}

	private:
		template<class Allocator>
		[[nodiscard]]
		static Pair *create__(
				Allocator &allocator,
				std::string_view const key,
				std::string_view const val,
				uint32_t const expires = 0, uint32_t const created = 0) noexcept{

			if ( ! check(key, val) )
				return nullptr;

			using namespace MyAllocator;

			Pair *pair = allocate<Pair>(
					allocator,
					Pair::bytes(key.size(), val.size())
			);

			if (!pair)
				return nullptr;

			createInRawMemory(pair, key, val, expires, created);

			return pair;
		}

		template<class Allocator>
		[[nodiscard]]
		static Pair *clone__(Allocator &allocator, const Pair &src) noexcept{
			using namespace MyAllocator;

			Pair *pair = allocate<Pair>(
					allocator,
					src.bytes()
			);

			if (!pair)
				return nullptr;

			cloneInRawMemory(pair, src);

			return pair;
		}

		template<class Allocator>
		[[nodiscard]]
		static Pair *clone__(Allocator &allocator, const Pair *src) noexcept{
			if (!src)
				return nullptr;

			return clone__(allocator, *src);
		}

		template<class Allocator>
		static void destroy__(Allocator &allocator, Pair *pair) noexcept{
			using namespace MyAllocator;

			return deallocate(allocator, pair);
		}

	public:
		struct smart_ptr{
			template<class Allocator>
			[[nodiscard]]
			static auto create(
					Allocator &allocator,
					std::string_view const key,
					std::string_view const val,
					uint32_t const expires = 0, uint32_t const created = 0) noexcept{

				using MyAllocator::wrapInSmartPtr;

				return wrapInSmartPtr(
					allocator,
					create__(allocator, key, val, expires, created)
				);
			}

			template<class Allocator>
			[[nodiscard]]
			static auto clone(Allocator &allocator, const Pair *src) noexcept{

				using MyAllocator::wrapInSmartPtr;

				return wrapInSmartPtr(
					allocator,
					clone__(allocator, src)
				);
			}

			template<class Allocator>
			[[nodiscard]]
			static auto clone(Allocator &allocator, const Pair &src) noexcept{

				using MyAllocator::wrapInSmartPtr;

				return wrapInSmartPtr(
					allocator,
					clone__(allocator, src)
				);
			}
		};

	public:
		[[nodiscard]]
		static auto create(
				std::string_view const key,
				std::string_view const val,
				uint32_t const expires = 0, uint32_t const created = 0) noexcept{

			std::nullptr_t allocator;
			return smart_ptr::create(allocator, key, val, expires, created);
		}

		[[nodiscard]]
		static auto clone(const Pair *src) noexcept{
			std::nullptr_t allocator;
			return smart_ptr::clone(allocator, src);
		}

		[[nodiscard]]
		static auto clone(const Pair &src) noexcept{
			return clone(& src);
		}

	public:
		[[nodiscard]]
		constexpr
		bool empty() const noexcept{
			if constexpr(1){
				constexpr uint16_t mask_be = htobe<uint16_t>(PairConf::MAX_KEY_MASK);

				return !(keylen & mask_be);
			}else{
				return !getKeyLen_();
			}
		}

	public:
		[[nodiscard]]
		std::string_view getKey() const noexcept{
			return { getKey_(), getKeyLen_() };
		}

		[[nodiscard]]
		std::string_view getVal() const noexcept{
			return { getVal_(), getValLen_() };
		}

		[[nodiscard]]
		bool isTombstone() const noexcept{
			if constexpr(1){
				constexpr uint16_t mask_be = htobe<uint16_t>(PairConf::MAX_VAL_MASK);

				return !(vallen | (keylen & mask_be));
			}else{
				return !getValLen_();
			}
		}

		[[nodiscard]]
		uint64_t getCreated() const noexcept{
			return betoh<uint64_t>(created);
		}

		[[nodiscard]]
		uint32_t getExpires() const noexcept{
			return betoh<uint32_t>(expires);
		}

		[[nodiscard]]
		uint64_t getTTL() const noexcept;

	public:
		[[nodiscard]]
		int cmp(std::string_view const key) const noexcept{
			return std::empty(key) ?
				CMP_NULLKEY :
				cmpX<0>(key);
		}

		[[nodiscard]]
		int cmp(Pair const &pair) const noexcept{
			return cmp(pair.getKey());
		}

		template<size_t start>
		[[nodiscard]]
		int cmpX(std::string_view const key) const noexcept{
			if constexpr(start == 0){
				return ::compare(getKey_(), getKeyLen_(), key.data(), key.size());
			}else{
			//	assert(key.size() >= start);
			//	assert(getKeyLen_() >= start);

				return ::compare(getKey_() + start, getKeyLen_() - start, key.data() + start, key.size() - start);
			}
		}

	public:
		[[nodiscard]]
		bool equals(std::string_view const key) const noexcept{
			return std::empty(key) ?
				false :
				equalsX<0>(key);
		}

		[[nodiscard]]
		int equals(Pair const &pair) const noexcept{
			return equals(pair.getKey());
		}

		template<size_t start>
		[[nodiscard]]
		bool equalsX(std::string_view const key) const noexcept{
			if constexpr(start == 0){
				return ::equals(getKey_(), getKeyLen_(), key.data(), key.size());
			}else{
			//	assert(key.size() >= start);
			//	assert(getKeyLen_() >= start);
				return ::equals(getKey_() + start, getKeyLen_() - start, key.data() + start, key.size() - start);
			}
		}

	public:
		[[nodiscard]]
		int cmpTime(Pair const &pair) const noexcept{
			return comparator::comp(
				getCreated(),
				pair.getCreated()
			);
		}

		template<bool B>
		[[nodiscard]]
		int cmpWithTime(Pair const &pair, std::bool_constant<B>) const noexcept{
			if (int const result = cmp(pair); result)
				return result;

			if constexpr(B)
				return + cmpTime(pair);
			else
				return - cmpTime(pair);
		}

		[[nodiscard]]
		int cmpWithTime(Pair const &pair) const noexcept{
			return cmpWithTime(pair, std::true_type{});
		}

	public:
		[[nodiscard]]
		bool isValid(std::false_type) const noexcept{
			// check if expired.
			if ( isExpired_() )
				return false;

			// finally all OK
			return true;
		}

		[[nodiscard]]
		bool isValid(std::true_type) const noexcept{
			// check if is tombstone
			if ( isTombstone() )
				return false;

			// chaining
			return isValid(std::false_type{});
		}

		template<bool B>
		[[nodiscard]]
		bool isValidForReplace(Pair const &other, std::bool_constant<B> tag) const noexcept{
			// if other is created after this,
			// then obviously this is not valid
			if (other.getCreated() > getCreated())
				return false;

			// chaining
			return isValid(tag);
		}

		// ==============================

		[[nodiscard]]
		bool isValid() const noexcept{
			return isValid(std::false_type{});
		}

		[[nodiscard]]
		bool isValidForReplace(const Pair &other) const noexcept{
			return isValidForReplace(other, std::false_type{});
		}

		// ==============================

		[[nodiscard]]
		size_t bytes() const noexcept{
			return bytes(getKeyLen_(), getValLen_());
		}

		// ==============================

		void print() const noexcept;

		void fwrite(std::ostream & os, WriteOptions const writeOptions) const{
			os.write((const char *) this, narrow<std::streamsize>( bytes() ) );

			if (writeOptions == WriteOptions::ALIGNED)
				my_align::fwriteGap(os, bytes(), PairConf::ALIGN);
		}

		// ==============================

		[[nodiscard]]
		constexpr
		static size_t bytes(size_t const keyLen, size_t const valLen) noexcept{
			return sizeof(Pair) + keyLen + valLen;
		}

		[[nodiscard]]
		constexpr
		static size_t bytes(std::string_view const key, std::string_view const val) noexcept{
			return bytes(key.size(), val.size());
		}

		[[nodiscard]]
		constexpr
		static size_t maxBytes() noexcept{
			return bytes(PairConf::MAX_KEY_SIZE, PairConf::MAX_VAL_SIZE);
		}

	private:
		[[nodiscard]]
		const char *getKey_() const noexcept{
			return buffer;
		}

		[[nodiscard]]
		const char *getVal_() const noexcept{
			return & buffer[ getKeyLen_() + 1 ];
		}

		[[nodiscard]]
		constexpr
		size_t getKeyLen_() const noexcept{
			return betoh<uint16_t>(keylen) & PairConf::MAX_KEY_MASK;
		}

		[[nodiscard]]
		constexpr
		size_t getValLen_() const noexcept{
			// if we do magic with betoh<>(PairConf::MAX_VAL_MASK),
			// we will not know how much to shift

			uint32_t const bits16 = betoh<uint16_t>(keylen) & PairConf::MAX_VAL_MASK;

			return bits16 << PairConf::MAX_VAL_MASK_SH | betoh<uint16_t>(vallen);
		}

	private:
		[[nodiscard]]
		bool isExpired_() const noexcept;

		static uint64_t getCreateTime__(uint32_t created) noexcept;

	} __attribute__((__packed__));

	static_assert(std::is_trivial<Pair>::value, "Pair must be POD type");

	[[nodiscard]]
	inline bool equals(const Pair *p1, const Pair *p2){
		return p1->equals(*p2);
	}

	inline void print(Pair const &pair){
		pair.print();
	}


	using OPair = MyAllocator::SmartPtrType<Pair, std::nullptr_t>;

	static_assert(std::is_same_v<OPair, std::unique_ptr<Pair> >, "OPair is not std::unique_ptr");

	inline void print(OPair const &pair){
		if (pair)
			pair->print();
		else
			printf("%s\n", PairConf::EMPTY_MESSAGE);
	}



} // anonymous namespace
} // namespace

#endif

