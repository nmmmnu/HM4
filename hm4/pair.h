#ifndef PAIR_H
#define PAIR_H

#include <cstdint>
#include <cstring>

#include <ostream>
#include <memory>
#include <string_view>

#include "myendian.h"
#include "mynarrow.h"
#include "mystring.h"
#include "comparator.h"

#include "stdallocator.h"

namespace hm4{
	namespace PairConf{
		constexpr size_t	ALIGN		= sizeof(void *);
		constexpr uint16_t	MAX_KEY_SIZE	=      1024;	// MySQL is 1000
		constexpr uint16_t	MAX_VAL_SIZE	= 16 * 1024;

		constexpr const char	*EMPTY_MESSAGE	= "---pair-is-empty---";
	}

	namespace impl_{
		template<typename T>
		constexpr auto neg(T const &a, std::true_type){
			return +a;
		}

		template<typename T>
		constexpr auto neg(T const &a, std::false_type){
			return -a;
		}
	}



	struct Pair{
		uint64_t	created;	// 8
		uint32_t	expires;	// 4, 136 years, not that bad.
		uint16_t	keylen;		// 2
		uint16_t	vallen;		// 2
		char		buffer[2];	// dynamic, these are the two \0 terminators.

	public:
		static constexpr int			CMP_NULLKEY	= -1;

		inline
		static constexpr std::string_view	TOMBSTONE	= "";

	public:
		Pair() = delete;
		Pair(Pair const &) = delete;
		Pair(Pair &&) = delete;
		Pair &operator=(Pair const &) = delete;
		Pair &operator=(Pair &&) = delete;

	public:
		constexpr static bool check(
					std::string_view const key
				) noexcept{
			return key.size() > 0 && key.size() <= PairConf::MAX_KEY_SIZE;
		}

		constexpr static bool check(
					std::string_view const key,
					std::string_view const val
				) noexcept{
			return check(key) && val.size() <= PairConf::MAX_VAL_SIZE;
		}

	public:
		struct ptr {
			template<class Allocator>
			[[nodiscard]]
			static Pair *create(
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

				return copy_(pair, key, val, expires, created);
			}

			template<class Allocator>
			[[nodiscard]]
			static Pair *clone(Allocator &allocator, const Pair *src) noexcept{
				if (!src)
					return nullptr;

				Pair *pair = static_cast<Pair *>(
						allocator.allocate(src->bytes())
				);

				if (!pair)
					return nullptr;

				memcpy(pair, src, src->bytes());

				return pair;
			}

			template<class Allocator>
			static void destroy(Allocator &allocator, Pair *pair) noexcept{
				return allocator.deallocate(pair);
			}
		};

	public:
		struct smart_ptr{
		public:
			template<class Allocator>
			static auto create(
					Allocator &allocator,
					std::string_view const key,
					std::string_view const val,
					uint32_t const expires = 0, uint32_t const created = 0) noexcept{

				return allocator.wrapInSmartPtr(
					ptr::create(allocator, key, val, expires, created)
				);
			}

			template<class Allocator>
			static auto clone(Allocator &allocator, const Pair *src) noexcept{
				return allocator.wrapInSmartPtr(
					ptr::clone(allocator, src)
				);
			}

			template<class Allocator>
			static auto clone(Allocator &allocator, const Pair &src) noexcept{
				return clone(allocator, & src);
			}

			template<class Allocator>
			using type = decltype( clone(std::declval<Allocator &>(), nullptr) );
		};

	public:
		static auto create(
				std::string_view const key,
				std::string_view const val,
				uint32_t const expires = 0, uint32_t const created = 0) noexcept{

			MyAllocator::STDAllocator allocator;
			return smart_ptr::create(allocator, key, val, expires, created);
		}

		static auto clone(const Pair *src) noexcept{
			MyAllocator::STDAllocator allocator;
			return smart_ptr::clone(allocator, src);
		}

		static auto clone(const Pair &src) noexcept{
			return clone(& src);
		}

	private:
		static Pair *copy_(Pair *pair,
				std::string_view key,
				std::string_view val,
				uint32_t expires, uint32_t created) noexcept;

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
				cmpX<0>(key);
		}

		int cmp(Pair const &pair) const noexcept{
			return cmp(pair.getKey());
		}

		template<size_t start>
		int cmpX(std::string_view const key) const noexcept{
		//	assert(key.size() >= start);
		//	assert(getKeyLen_() >= start);

			return ::compare(getKey_() + start, getKeyLen_() - start, key.data() + start, key.size() - start);
		}

		template<>
		int cmpX<0>(std::string_view const key) const noexcept{
			return ::compare(getKey_(), getKeyLen_(), key.data(), key.size());
		}

	public:
		bool equals(std::string_view const key) const noexcept{
			return std::empty(key) ?
				false :
				equalsX<0>(key);
		}

		int equals(Pair const &pair) const noexcept{
			return equals(pair.getKey());
		}

		template<size_t start>
		bool equalsX(std::string_view const key) const noexcept{
		//	assert(key.size() >= start);
		//	assert(getKeyLen_() >= start);
			return ::equals(getKey_() + start, getKeyLen_() - start, key.data() + start, key.size() - start);
		}

		template<>
		bool equalsX<0>(std::string_view const key) const noexcept{
			return ::equals(getKey_(), getKeyLen_(), key.data(), key.size());
		}

	public:
		int cmpTime(Pair const &pair) const noexcept{
			return comparator::comp(
				getCreated(),
				pair.getCreated()
			);
		}

		template<bool B>
		int cmpWithTime(Pair const &pair, std::bool_constant<B> tag) const noexcept{
			if (int const result = cmp(pair); result)
				return result;

			return impl_::neg(cmpTime(pair), tag);
		}

		int cmpWithTime(Pair const &pair) const noexcept{
			return cmpWithTime(pair, std::true_type{});
		}

	public:
		bool isValid(std::false_type) const noexcept{
			// check if expired.
			if ( isExpired_() )
				return false;

			// finally all OK
			return true;
		}

		bool isValid(std::true_type) const noexcept{
			// check if is tombstone
			if ( isTombstone() )
				return false;

			// chaining
			return isValid(std::false_type{});
		}

		template<bool B>
		bool isValidForReplace(Pair const &other, std::bool_constant<B> tag) const noexcept{
			// if other is created after this,
			// then obviously this is not valid
			if (other.getCreated() > getCreated())
				return false;

			// chaining
			return isValid(tag);
		}

		// ==============================

		bool isValid() const noexcept{
			return isValid(std::false_type{});
		}

		bool isValidForReplace(const Pair &other) const noexcept{
			return isValidForReplace(other, std::false_type{});
		}

		// ==============================

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
		static size_t bytes(std::string_view const key, std::string_view const val) noexcept{
			return bytes(key.size(), val.size());
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
			return betoh<uint16_t>(vallen);
		}

	private:
		bool isExpired_() const noexcept;

		static uint64_t getCreateTime__(uint32_t created) noexcept;

	} __attribute__((__packed__));

	static_assert(std::is_trivial<Pair>::value, "Pair must be POD type");

	// not used
	inline bool less____(const Pair *p1, const Pair *p2){
		return p1->cmp(*p2) < 0;
	}

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

