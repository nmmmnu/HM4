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



namespace hm4{
	namespace PairConf{
		using HLINE_INT = uint64_t;

		constexpr size_t	ALIGN		= sizeof(void *);
		constexpr uint16_t	MAX_KEY_SIZE	=      1024;	// MySQL is 1000
		constexpr uint32_t	MAX_VAL_SIZE	= 16 * 1024;

		constexpr const char	*EMPTY_MESSAGE	= "---pair-is-empty---";
	}



	struct Pair{
		uint64_t	created;	// 8
		uint32_t	expires;	// 4, 136 years, not that bad.
		uint32_t	vallen;		// 4
		uint16_t	keylen;		// 2
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

	private:
		struct unique_ptr_allocator{
			static void *allocate(std::size_t const size) noexcept{
				return ::operator new(size, std::nothrow);
			}
		};

	public:
		struct up{
		private:
			template<class Allocator, typename T>
			static auto wrap__(Allocator &allocator, T *p) noexcept{
				auto deleter = [&allocator](void *p){
					allocator.deallocate(p);
				};

				return std::unique_ptr<T, decltype(deleter)>{
					p,
					deleter
				};
			};

			template<typename T>
			static auto wrap__(unique_ptr_allocator &, T *p) noexcept{
				return std::unique_ptr<T>{ p };
			};

		public:
			template<class Allocator>
			static auto create(
					Allocator &allocator,
					std::string_view const key,
					std::string_view const val,
					uint32_t const expires = 0, uint32_t const created = 0) noexcept{

				return wrap__(
					allocator,
					ptr::create(allocator, key, val, expires, created)
				);
			}

			template<class Allocator>
			static auto clone(Allocator &allocator, const Pair *src) noexcept{
				return wrap__(
					allocator,
					ptr::clone(allocator, src)
				);
			}

			template<class Allocator>
			static auto clone(Allocator &allocator, const Pair &src) noexcept{
				return clone(allocator, & src);
			}
		};

	public:
		static auto create(
				std::string_view const key,
				std::string_view const val,
				uint32_t const expires = 0, uint32_t const created = 0) noexcept{

			unique_ptr_allocator allocator;
			return up::create(allocator, key, val, expires, created);
		}

		static auto clone(const Pair *src) noexcept{
			unique_ptr_allocator allocator;
			return up::clone(allocator, src);
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
			return betoh<uint32_t>(vallen);
		}

	private:
		bool isExpired_() const noexcept;

		static uint64_t getCreateTime__(uint32_t created) noexcept;

	} __attribute__((__packed__));

	static_assert(std::is_trivial<Pair>::value, "Pair must be POD type");



	using OPair = std::unique_ptr<Pair>;

	inline void print(Pair const &pair){
		pair.print();
	}

	inline void print(OPair const &pair){
		if (pair)
			pair->print();
		else
			printf("%s\n", PairConf::EMPTY_MESSAGE);
	}

} // namespace

#endif

