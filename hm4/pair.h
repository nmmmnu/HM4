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

#include "stdallocator.h"

namespace hm4{
	namespace PairConf{
		constexpr size_t	ALIGN		= sizeof(void *);

		constexpr uint16_t	MAX_KEY_SIZE	= 0b0'0000'0000'0011'1111'1111;	// 1023, MySQL is 1000
		constexpr uint32_t	MAX_VAL_SIZE	= 0b0'1111'1111'1111'1111'1111; // 1048575, 1 MB
		constexpr uint16_t	MAX_VAL_MASK	= 0b0'0000'1111'0000'0000'0000;
		constexpr size_t	MAX_VAL_MASK_SH	= 4;

		constexpr uint16_t	MAX_KEY_MASK	= MAX_KEY_SIZE;

		constexpr uint16_t	MAX_KEY_MASK_BE	= htobe<uint16_t>(MAX_KEY_MASK);

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

	private:
		[[nodiscard]]
		static Pair *createInRawMemory__(Pair *pair,
				std::string_view key,
				std::string_view val,
				uint32_t expires, uint32_t created) noexcept;

		template<class Allocator>
		[[nodiscard]]
		static Pair *create__(
				Allocator &allocator,
				std::string_view const key,
				std::string_view const val,
				uint32_t const expires = 0, uint32_t const created = 0) noexcept{

			if ( ! check(key, val) )
				return nullptr;

			Pair *pair = static_cast<Pair *>(
					allocator.allocate( Pair::bytes(key.size(), val.size()) )
			);

			if (!pair)
				return nullptr;

			return createInRawMemory__(pair, key, val, expires, created);
		}

		template<class Allocator>
		[[nodiscard]]
		static Pair *clone__(Allocator &allocator, const Pair *src) noexcept{
			if (!src)
				return nullptr;

			void *pair = allocator.allocate(src->bytes());

			if (!pair)
				return nullptr;

			memcpy(pair, src, src->bytes());

			return static_cast<Pair *>(pair);
		}

		template<class Allocator>
		static void destroy__(Allocator &allocator, Pair *pair) noexcept{
			return allocator.deallocate(pair);
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

				return allocator.wrapInSmartPtr(
					create__(allocator, key, val, expires, created)
				);
			}

			template<class Allocator>
			[[nodiscard]]
			static auto clone(Allocator &allocator, const Pair *src) noexcept{
				return allocator.wrapInSmartPtr(
					clone__(allocator, src)
				);
			}

			template<class Allocator>
			[[nodiscard]]
			static auto clone(Allocator &allocator, const Pair &src) noexcept{
				return clone(allocator, & src);
			}

			template<class Allocator>
			using type = decltype( clone(std::declval<Allocator &>(), nullptr) );
		};

	public:
		[[nodiscard]]
		static auto create(
				std::string_view const key,
				std::string_view const val,
				uint32_t const expires = 0, uint32_t const created = 0) noexcept{

			MyAllocator::STDAllocator allocator;
			return smart_ptr::create(allocator, key, val, expires, created);
		}

		[[nodiscard]]
		static auto clone(const Pair *src) noexcept{
			MyAllocator::STDAllocator allocator;
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
			return !(keylen & PairConf::MAX_KEY_MASK_BE);
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
			return vallen == 0;
		}

		[[nodiscard]]
		uint64_t getCreated() const noexcept{
			return betoh<uint64_t>(created);
		}

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



	using OPair = std::unique_ptr<Pair>;

	inline void print(OPair const &pair){
		if (pair)
			pair->print();
		else
			printf("%s\n", PairConf::EMPTY_MESSAGE);
	}

} // namespace

#endif

