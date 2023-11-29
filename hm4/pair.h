#ifndef PAIR_H
#define PAIR_H

#include <cstdint>
#include <cstring>
#include <cassert>

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
inline namespace version_4_00_00{

	namespace PairConf{
		constexpr uint32_t	VERSION			= 4'00'00;

		constexpr size_t	ALIGN			= sizeof(void *);

		constexpr uint16_t	MAX_KEY_SIZE		= 0b0'0000'0000'0000'0000'0000'0011'1111'1111;	// 1023, MySQL is 1000
		constexpr uint32_t	MAX_VAL_SIZE		= 0b0'0000'1111'1111'1111'1111'1111'1111'1111;	// 256 MB

		// Note, in net/protocol/redisprotocol.cc,
		// there is seemingly unrelated value MAX_PARAM_SIZE
		// You need to set it manually to MAX_VAL_SIZE or larger

		constexpr uint16_t	MAX_KEY_MASK		= MAX_KEY_SIZE;
		constexpr uint32_t	MAX_VAL_MASK		= MAX_VAL_SIZE;

		constexpr uint32_t	EXPIRES_TOMBSTONE	= 0xFFFF'FFFF;
		constexpr uint32_t	EXPIRES_MAX		= EXPIRES_TOMBSTONE - 1;

		constexpr const char	*EMPTY_MESSAGE		= "---pair-is-empty---";
	}



	struct Pair{
		uint64_t	created;	// 8
		uint32_t	expires;	// 4, 136 years, not that bad. but beware for problem 2038-01-19
		uint32_t	vallen;		// 4, 28 bits used
		uint16_t	keylen;		// 2, 10 bits used
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
		static uint64_t prepareCreateTime(uint32_t created) noexcept;
		static uint64_t prepareCreateTimeSimulate(uint32_t created) noexcept;

	public:
		template<bool copy_key, bool copy_val,bool make_key, bool make_val>
		static void createInRawMemory(Pair *pair, std::string_view const key, std::string_view const val, uint32_t const expires, uint32_t const created){
			static_assert(
				(make_key == 0			) ||
				(make_key == 1 && make_val == 1	) ||
				false
			);

			static_assert(
				(copy_key == 0			) ||
				(copy_key == 1 && make_key == 1	) ||
				false
			);

			static_assert(
				(copy_val == 0			) ||
				(copy_val == 1 && make_val == 1	) ||
				false
			);

			pair->created	= htobe<uint64_t>(prepareCreateTime(created));
			pair->expires	= htobe<uint32_t>(expires); // std::min(expires, PairConf::EXPIRES_MAX)

			if constexpr(copy_key || make_key){
				uint16_t const keylen = static_cast<uint16_t>(key.size() & PairConf::MAX_KEY_MASK);
				pair->keylen	= htobe<uint16_t>(keylen);
			}

			if constexpr(copy_val || make_val){
				uint32_t const vallen = static_cast<uint32_t>(val.size() & PairConf::MAX_VAL_MASK);
				pair->vallen	= htobe<uint32_t>(vallen);
			}

			#pragma GCC diagnostic push
			#pragma GCC diagnostic ignored "-Warray-bounds"
			#pragma GCC diagnostic ignored "-Wstringop-overflow"

			if constexpr(copy_key){
				memcpy(& pair->buffer[0],		key.data(), key.size());
			}

			if constexpr(copy_key || make_key){
				pair->buffer[key.size()] = '\0';
			}


			if constexpr(copy_val){
				// this is safe with NULL pointer.
				memcpy(& pair->buffer[key.size() + 1],	val.data(), val.size());
			}

			if constexpr(copy_val || make_val){
				pair->buffer[key.size() + 1 + val.size()] = '\0';
			}

			#pragma GCC diagnostic pop
		}

		template<bool copy_key, bool copy_val,bool make_key, bool make_val>
		static void createInRawMemory(Pair *pair, std::string_view const key, size_t const val_size, uint32_t const expires, uint32_t const created){
			static_assert(copy_val == false, "When you pass null value, don't break the rules!");

			// now break the rules
			const char *val_str = nullptr;
			std::string_view const val{ val_str, val_size };

			return createInRawMemory<copy_key, copy_val, make_key, make_val>(pair, key, val, expires, created);
		}

		static void cloneInRawMemory(Pair *pair, const Pair &src) noexcept{
			memcpy(static_cast<void *>(pair), & src, src.bytes());
		}

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

	private:
		template<class Allocator, class Factory>
		[[nodiscard]]
		static Pair *create__(Allocator &allocator, Factory &factory) noexcept{
			if ( !factory.valid() )
				return nullptr;

			// if valid, this is also OK
			//if ( factory.bytes() > maxBytes() )
			//	return nullptr;

			using namespace MyAllocator;

			Pair *pair = allocate<Pair>(
					allocator,
					factory.bytes()
			);

			if (!pair)
				return nullptr;

			factory.create(pair);

			return pair;
		}

		template<class Allocator>
		static void destroy__(Allocator &allocator, Pair *pair) noexcept{
			using namespace MyAllocator;

			return deallocate(allocator, pair);
		}

	public:
		struct smart_ptr{
			template<class Allocator, class PairFactory>
			[[nodiscard]]
			static auto create(Allocator &allocator, PairFactory &factory) noexcept{
				using MyAllocator::wrapInSmartPtr;

				return wrapInSmartPtr(
					allocator,
					create__(allocator, factory)
				);
			}

			template<class PairFactory, class Allocator, typename ...Args>
			[[nodiscard]]
			static auto createF(Allocator &allocator, Args &&...args) noexcept{
				PairFactory factory{ std::forward<Args>(args)... };
				return create(allocator, factory);
			}

			template<class Allocator>
			[[nodiscard]]
			static MyAllocator::SmartPtrType<Pair, Allocator> create(
					Allocator &allocator,
					std::string_view const key,
					std::string_view const val,
					uint32_t const expires = 0, uint32_t const created = 0) noexcept;

			template<class Allocator>
			[[nodiscard]]
			static MyAllocator::SmartPtrType<Pair, Allocator> clone(Allocator &allocator, const Pair *src) noexcept;

			template<class Allocator>
			[[nodiscard]]
			static auto clone(Allocator &allocator, const Pair &src) noexcept{
				return clone(allocator, & src);
			}
		};

	public:
		template<class Allocator, class PairFactory>
		[[nodiscard]]
		static auto create(PairFactory &factory) noexcept{
			std::nullptr_t allocator;
			return smart_ptr::create(allocator, factory);
		}

	public:
		[[nodiscard]]
		constexpr
		bool isKeyEmpty() const noexcept{
			// big endian 0
			return !(keylen & htobe<uint16_t>(PairConf::MAX_KEY_MASK));
		}

	public:
		[[nodiscard]]
		constexpr std::string_view getKey() const noexcept{
			return { getKey_(), getKeyLen_() };
		}

		[[nodiscard]]
		constexpr std::string_view getVal() const noexcept{
			return { getVal_(), getValLen_() };
		}

		[[nodiscard]]
		constexpr const char *getValC() const noexcept{
			return getVal_();
		}

		[[nodiscard]]
		constexpr char *getValC() noexcept{
			return getVal_();
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
		bool isExpiresSet() const noexcept{
			return expires;
		}

		[[nodiscard]]
		uint32_t getTTL() const noexcept;

		[[nodiscard]]
		uint32_t getExpiresAt() const noexcept;

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
		bool equals(Pair const &pair) const noexcept{
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
		// beware problem 2038 !!!
		[[nodiscard]]
		bool isExpired() const noexcept{
			if (expires == PairConf::EXPIRES_TOMBSTONE)
				return true;
			else
				return isExpired_();
		}

		[[nodiscard]]
		bool isTombstone() const noexcept{
			// big endian 0
			return (vallen & htobe<uint32_t>(PairConf::MAX_VAL_MASK)) == 0;
		}

		[[nodiscard]]
		bool isTombstone(std::true_type) const noexcept{
			if (expires == PairConf::EXPIRES_TOMBSTONE)
				return true;
			else
				return isTombstone();
		}

		[[nodiscard]]
		bool isOK() const noexcept{
			// check if is tombstone
			if ( isTombstone(std::true_type{}) )
				return false;

			// check if is expired
			if ( isExpired_() )
				return false;

			return true;
		}

		// ==============================

		[[nodiscard]]
		constexpr size_t bytes() const noexcept{
			return bytes_(getKeyLen_(), getValLen_());
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
			assert(isKeyValid(keyLen));
			assert(isValValid(valLen));
			return bytes_(keyLen, valLen);
		}

		[[nodiscard]]
		constexpr
		static size_t bytes(std::string_view const key, std::string_view const val) noexcept{
			assert(isKeyValid(key));
			assert(isValValid(val));
			return bytes_(key.size(), val.size());
		}

		[[nodiscard]]
		constexpr
		static size_t maxBytes() noexcept{
			return bytes_(PairConf::MAX_KEY_SIZE, PairConf::MAX_VAL_SIZE);
		}

		[[nodiscard]]
		constexpr
		static size_t maxBytesTombstone() noexcept{
			return bytes_(PairConf::MAX_KEY_SIZE, 0);
		}

		[[nodiscard]]
		constexpr
		static bool isKeyValid(size_t size) noexcept{
			return size > 0 && size <= PairConf::MAX_KEY_SIZE;
		}

		[[nodiscard]]
		constexpr
		static bool isKeyValid(std::string_view key) noexcept{
			return isKeyValid(key.size());
		}

		[[nodiscard]]
		constexpr
		static bool isValValid(size_t size) noexcept{
			return	size <= PairConf::MAX_VAL_SIZE;
		}

		[[nodiscard]]
		constexpr
		static bool isValValid(std::string_view val) noexcept{
			return	isValValid(val.size());
		}

	private:
		[[nodiscard]]
		constexpr
		static size_t bytes_(size_t const keyLen, size_t const valLen) noexcept{
			return sizeof(Pair) + keyLen + valLen;
		}

		[[nodiscard]]
		constexpr const char *getKey_() const noexcept{
			return buffer;
		}

		[[nodiscard]]
		constexpr const char *getVal_() const noexcept{
			return & buffer[ getKeyLen_() + 1 ];
		}

		[[nodiscard]]
		constexpr char *getVal_() noexcept{
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
			return betoh<uint32_t>(vallen) & PairConf::MAX_VAL_MASK;
		}

		[[nodiscard]]
		bool isExpired_() const noexcept;

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



	namespace PairFactory{
		struct MutableNotifyMessage{
			size_t bytes_old = 0;
			size_t bytes_new = 0;
		};

		struct Normal{
			std::string_view key;
			std::string_view val;
			uint32_t expires;
			uint32_t created;

			constexpr Normal(
				std::string_view const key,
				std::string_view const val,
				uint32_t const expires = 0, uint32_t const created = 0) :
					key	(key		),
					val	(val		),
					expires	(expires	),
					created	(created	){}

			[[nodiscard]]
			constexpr std::string_view getKey() const noexcept{
				return key;
			}

			[[nodiscard]]
			constexpr uint32_t getCreated() const noexcept{
				return created;
			}

			[[nodiscard]]
			constexpr size_t bytes() const{
				return Pair::bytes(key.size(), val.size());
			}

			[[nodiscard]]
			constexpr bool valid() const{
				return
					Pair::isKeyValid(key) &&
					Pair::isValValid(val)
				;
			}

			void createHint(Pair *pair) const{
				Pair::createInRawMemory<0,1,0,1>(pair, key, val, expires, created);
			}

			void create(Pair *pair) const{
				Pair::createInRawMemory<1,1,1,1>(pair, key, val, expires, created);
			}
		};

		struct Expires{
			std::string_view key;
			std::string_view val;
			uint32_t expires;
			uint32_t created;

			constexpr Expires(
				std::string_view const key,
				std::string_view const val,
				uint32_t const expires = 0, uint32_t const created = 0) :
					key	(key		),
					val	(val		),
					expires	(expires	),
					created	(created	){}

			[[nodiscard]]
			constexpr std::string_view getKey() const noexcept{
				return key;
			}

			[[nodiscard]]
			constexpr uint32_t getCreated() const noexcept{
				return created;
			}

			[[nodiscard]]
			constexpr size_t bytes() const{
				return Pair::bytes(key.size(), val.size());
			}

			[[nodiscard]]
			constexpr bool valid() const{
				return
					Pair::isKeyValid(key) &&
					Pair::isValValid(val)
				;
			}

			void createHint(Pair *pair) const{
				Pair::createInRawMemory<0,0,0,0>(pair, key, val, expires, created);
			}

			void create(Pair *pair) const{
				Pair::createInRawMemory<1,1,1,1>(pair, key, val, expires, created);
			}
		};

		struct Tombstone{
			std::string_view key;

			constexpr Tombstone(std::string_view const key) :
					key(key){}

			[[nodiscard]]
			constexpr std::string_view getKey() const noexcept{
				return key;
			}

			[[nodiscard]]
			constexpr uint32_t getCreated() const noexcept{
				return 0;
			}

			[[nodiscard]]
			constexpr size_t bytes() const{
				return Pair::bytes(key.size(), Pair::TOMBSTONE.size());
			}

			[[nodiscard]]
			constexpr bool valid() const{
				return Pair::isKeyValid(key);
			}

			void createHint(Pair *pair) const{
				if constexpr(false){
					Pair::createInRawMemory<0,1,0,1>(pair, key, Pair::TOMBSTONE, 0, 0);
				}else{
					Pair::createInRawMemory<0,0,0,0>(pair, key, "", PairConf::EXPIRES_TOMBSTONE, 0);
				}
			}

			void create(Pair *pair) const{
				Pair::createInRawMemory<1,1,1,1>(pair, key, Pair::TOMBSTONE, 0, 0);
			}
		};

		struct Clone{
			const Pair *src;

			constexpr Clone(const Pair *src) :
					src(src){}

			constexpr Clone(const Pair &src) :
					Clone(&src){}

			[[nodiscard]]
			constexpr std::string_view getKey() const noexcept{
				return src->getKey();
			}

			[[nodiscard]]
			uint32_t getCreated() const noexcept{
				return (uint32_t) src->getCreated();
			}

			[[nodiscard]]
			size_t bytes() const{
				return src->bytes();
			}

			[[nodiscard]]
			constexpr static bool valid(){
				return true;
			}

			void createHint(Pair *pair) const{
				Pair::cloneInRawMemory(pair, *src);
			}

			void create(Pair *pair) const{
				Pair::cloneInRawMemory(pair, *src);
			}
		};

		struct IFactory{
			virtual ~IFactory() = default;

			virtual std::string_view getKey() const = 0;
			virtual uint32_t getCreated() const = 0;
			virtual size_t bytes() const = 0;
			virtual bool valid() const = 0;

			virtual void createHint(Pair *pair) = 0;
			virtual void create(Pair *pair) = 0;
		};

	} // namespace PairFactory



	template<class Allocator>
	[[nodiscard]]
	MyAllocator::SmartPtrType<Pair, Allocator> Pair::smart_ptr::create(
			Allocator &allocator,
			std::string_view const key,
			std::string_view const val,
			uint32_t const expires, uint32_t const created) noexcept{

		using namespace PairFactory;
		return smart_ptr::createF<Normal>(allocator, key, val, expires, created);
	}

	template<class Allocator>
	[[nodiscard]]
	MyAllocator::SmartPtrType<Pair, Allocator> Pair::smart_ptr::clone(
			Allocator &allocator,
			const Pair *src) noexcept{

		using namespace PairFactory;
		return smart_ptr::createF<Clone>(allocator, src);
	}



} // anonymous namespace
} // namespace

#endif

