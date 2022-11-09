#include "base.h"



namespace net::worker::commands::Mutable{



	template<class Protocol, class DBAdapter>
	struct SET : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "set";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"set",		"SET"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			auto const &key = p[1];

			if (key.empty())
				return;

			auto const &val = p[2];
			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			db.set(key, val, exp);

			return result.set();
		}
	};



	template<class Protocol, class DBAdapter>
	struct MSET : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "mset";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"mset",		"MSET"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			// should be odd number arguments
			// mset a 5 b 6
			if (p.size() < 3 || p.size() % 2 == 0)
				return;

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &key = *itk; key.empty())
					return;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;
				auto const &val = *std::next(itk);
				db.set(key, val);
			}

			return result.set();
		}
	};



	template<class Protocol, class DBAdapter>
	struct MSETNX : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "msetnx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"msetnx",		"MSETNX"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			// should be odd number arguments
			// mset a 5 b 6
			if (p.size() < 3 || p.size() % 2 == 0)
				return;

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &key = *itk; key.empty())
					return;

			// check if any key exists
			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;
				if (auto it = db.find(key); it && it->isValid(std::true_type{}))
					return result.set_0();
			}

			// set keys
			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;
				auto const &val = *std::next(itk);
				db.set(key, val);
			}

			return result.set_1();
		}
	};



	template<class Protocol, class DBAdapter>
	struct MSETXX : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "msetxx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"msetxx",		"MSETXX"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			// should be odd number arguments
			// mset a 5 b 6
			if (p.size() < 3 || p.size() % 2 == 0)
				return;

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &key = *itk; key.empty())
					return;

			buffer_.clear();
			buffer_.reserve(std::end(p) - std::begin(p) + varg);

			// check if any key NOT exists
			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;

				if (auto it = db.find(key); it && it->isValid(std::true_type{})){

					auto const &val = *std::next(itk);

					if (const auto *p = & *it; db.canUpdateWithHint(p))
						buffer_.emplace_back(p, key, val);
					else
						buffer_.emplace_back(nullptr, key, val);

				}else
					return result.set_0();
			}

			// set keys with hints
			for(auto &x : buffer_)
				if (x.p)
					db.setHint(x.p, x.val);

			// set keys without hints
			for(auto &x : buffer_)
				if (x.p == nullptr)
					db.set(x.key, x.val);

			return result.set_1();
		}

	private:
		struct msetxx_data{
			const hm4::Pair *p;
			std::string_view key;
			std::string_view val;

			constexpr msetxx_data(const hm4::Pair *p, std::string_view key, std::string_view val) :
							p(p), key(key), val(val){}
		};

		std::vector<msetxx_data> buffer_;
	};



	template<class Protocol, class DBAdapter>
	struct SETEX : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "setex";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"setex",	"SETEX"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 4)
				return;

			auto const &key = p[1];

			if (key.empty())
				return;

			auto const &val = p[3];
			auto const exp  = from_string<uint32_t>(p[2]);

			db.set(key, val, exp);

			return result.set();
		}
	};



	template<class Protocol, class DBAdapter>
	struct HSET : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "hset";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"hset",		"HSET"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() != 4 && p.size() != 5)
				return;

			const auto &keyN = p[1];

			if (keyN.empty())
				return;

			const auto &subN = p[2];

			if (subN.empty())
				return;

			if (keyN.size() + subN.size() > MAX_KEY_SIZE)
				return;

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);
			auto const &val = p[3];
			auto const exp  = p.size() == 5 ? from_string<uint32_t>(p[4]) : 0;

			db.set(key, val, exp);

			return result.set();
		}
	};



	template<class Protocol, class DBAdapter>
	struct HMSET : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "hmset";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"hmset",		"HMSET"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			// should be even number arguments
			// mset a sub1 5 sub2 6
			if (p.size() < 3 || p.size() % 2 != 0)
				return;

			const auto &keyN = p[1];

			if (keyN.empty())
				return;

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &subN = *itk;

				if (subN.empty())
					return;

				if (keyN.size() + subN.size() > MAX_KEY_SIZE)
					return;
			}

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &subN = *itk;

				auto const &key  = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);
				auto const &val  = *std::next(itk);

				db.set(key, val);
			}

			return result.set();
		}
	};



	template<class Protocol, class DBAdapter>
	struct SETNX : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "setnx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"setnx",	"SETNX"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			if (auto it = db.find(key); it && it->isValid(std::true_type{})){
				return result.set(false);
			}else{
				// SET

				const auto &val = p[2];
				const auto exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

				db.set(key, val, exp);

				return result.set(true);
			}
		}
	};



	template<class Protocol, class DBAdapter>
	struct SETXX : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "setxx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"setxx",	"SETXX"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			if (auto it = db.find(key); it && it->isValid(std::true_type{})){
				// SET

				const auto &val = p[2];
				const auto exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

				db.setHint(& *it, val, exp);

				return result.set(true);
			}else{
				return result.set(false);
			}
		}
	};



	template<class Protocol, class DBAdapter>
	struct DEL : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "del";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"del",		"DEL"	,
			"unlink",	"UNLINK"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() < 2)
				return;

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; key.empty())
					return;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &key = *itk;
				db.del(key);
			}

			return result.set_1();
		}
	};



	template<class Protocol, class DBAdapter>
	struct HDEL : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "hdel";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"hdel",		"HDEL"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &blob) final{
			if (p.size() < 3)
				return;

			const auto &keyN = p[1];

			if (keyN.empty())
				return;

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				if (subN.empty())
					return;

				if (keyN.size() + subN.size() > MAX_KEY_SIZE)
					return;
			}

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

				db.del(key);
			}

			return result.set_1();
		}
	};



	template<class Protocol, class DBAdapter>
	struct GETSET : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "getset";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"getset",	"GETSET"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			// because old_value may be overwritten,
			// we had to make a copy.

			auto it = db.find(key);

			if (it && it->isValid(std::true_type{})){
				// seamlessly send value to output buffer...
				result.set(it->getVal());
			}else{
				result.set("");
			}

			// SET

			const auto &val = p[2];
			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			db.setHint(& *it, val, exp);

			// return

			return;
		}
	};



	template<class Protocol, class DBAdapter>
	struct GETDEL : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "getdel";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"getdel",	"GETDEL"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			if (auto it = db.find(key); it && it->isValid(std::true_type{})){

				// seamlessly send value to output buffer...

				result.set(it->getVal());

				// DEL
				db.delHint(& *it);

				// return

				return;
			}else{
				return result.set("");
			}
		}
	};



	template<class Protocol, class DBAdapter>
	struct EXPIRE : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "expire";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"expire",	"EXPIRE"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;



			if (auto it = db.find(key); it && it->isValid(std::true_type{})){

				// SET
				auto const exp  = from_string<uint32_t>(p[2]);

				db.expHint(& *it, exp);

				return result.set(true);
			}else{
				return result.set(false);
			}
		}
	};



	template<class Protocol, class DBAdapter>
	struct PERSIST : Base<Protocol,DBAdapter>{
		constexpr inline static std::string_view name	= "persist";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"persist",	"PERSIST"
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			if (auto it = db.find(key); it && it->isValid(std::true_type{})){
				// SET

				if (it->getTTL() > 0){
					uint32_t const exp = 0;
					db.expHint(& *it, exp);
				}

				return result.set(true);
			}else{
				return result.set(false);
			}


		}
	};



	template<class Protocol, class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mutable";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, DBAdapter, RegisterPack,
				SET		,
				MSET		,
				SETEX		,
				HSET		,
				HMSET		,
				SETNX		,
				MSETNX		,
				MSETXX		,
				SETXX		,
				DEL		,
				HDEL		,
				GETSET		,
				GETDEL		,
				EXPIRE		,
				PERSIST
			>(pack);
		}
	};


} // namespace


