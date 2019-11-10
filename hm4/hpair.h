#ifndef CPAIR_H
#define CPAIR_H

#include "pair.h"

#include "stringhash.h"

namespace hm4{

	class HPair{
	public:
		using HKey	= PairConf::HLINE_INT;
		using SS	= StringHash<HKey>;

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
		int cmp(HKey hkey, std::string_view const key) const noexcept{
			if (key.empty())
				return Pair::CMP_NULLKEY;

			auto [ ok, result ] = SS::compare(hkey, this->hkey);

			if (ok)
				return result;

			return p->cmp(key);
		}

	private:
		HKey	hkey;
		Pair	*p;
	};

} // namespace hm4

#endif

