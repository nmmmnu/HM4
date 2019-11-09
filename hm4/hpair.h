#ifndef CPAIR_H
#define CPAIR_H

#include "pair.h"

#include "stringhash.h"

namespace hm4{

	class HPair{
		using uint	= uint64_t;
		using SS	= StringHash<uint>;

	public:
		HPair(Pair *p) noexcept :	hkey	(SS::create(p->getKey())	),
						p	(p				){}

	public:
		const Pair &operator *() const noexcept{
			return *p;
		}

		const Pair *operator ->() const noexcept{
			return p;
		}

		const Pair *get() const noexcept{
			return p;
		}

	public:
		int cmp(uint hkey, std::string_view const key) const noexcept{
			if (key.empty())
				return Pair::CMP_NULLKEY;

			auto [ ok, result ] = SS::compare(hkey, this->hkey);

			if (ok)
				return result;

			return p->cmp(key);
		}

	private:
		uint	hkey;
		Pair	*p;
	};

} // namespace hm4

#endif

