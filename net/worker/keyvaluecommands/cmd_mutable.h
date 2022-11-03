#include "base.h"



namespace net::worker::commands::Mutable{



	template<class DBAdapter>
	struct SET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "set";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"set",		"SET"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return Result::error();

			auto const &key = p[1];

			if (key.empty())
				return Result::error();

			auto const &val = p[2];
			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			db.set(key, val, exp);

			return Result::ok();
		}
	};



	template<class DBAdapter>
	struct MSET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "mset";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"mset",		"MSET"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			// should be odd number arguments
			// mset a 5 b 6
			if (p.size() < 3 || p.size() % 2 == 0)
				return Result::error();

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2)
				if (const auto &key = *itk; key.empty())
					return Result::error();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;
				auto const &val = *std::next(itk);
				db.set(key, val);
			}

			return Result::ok();
		}
	};



	template<class DBAdapter>
	struct SETEX : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "setex";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"setex",	"SETEX"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() != 4)
				return Result::error();

			auto const &key = p[1];

			if (key.empty())
				return Result::error();

			auto const &val = p[3];
			auto const exp  = from_string<uint32_t>(p[2]);

			db.set(key, val, exp);

			return Result::ok();
		}
	};



	template<class DBAdapter>
	struct HSET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "hset";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"hset",		"HSET"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) final{
			if (p.size() != 4 && p.size() != 5)
				return Result::error();

			const auto &keyN = p[1];

			if (keyN.empty())
				return Result::error();

			const auto &subN = p[2];

			if (subN.empty())
				return Result::error();

			if (keyN.size() + subN.size() > MAX_KEY_SIZE)
				return Result::error();

			auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);
			auto const &val = p[3];
			auto const exp  = p.size() == 5 ? from_string<uint32_t>(p[4]) : 0;

			db.set(key, val, exp);

			return Result::ok();
		}
	};



	template<class DBAdapter>
	struct HMSET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "hmset";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"hmset",		"HMSET"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) final{
			// should be even number arguments
			// mset a sub1 5 sub2 6
			if (p.size() < 3 || p.size() % 2 != 0)
				return Result::error();

			const auto &keyN = p[1];

			if (keyN.empty())
				return Result::error();

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &subN = *itk;

				if (subN.empty())
					return Result::error();

				if (keyN.size() + subN.size() > MAX_KEY_SIZE)
					return Result::error();
			}

			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &subN = *itk;

				auto const &key  = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);
				auto const &val  = *std::next(itk);

				db.set(key, val);
			}

			return Result::ok();
		}
	};



	template<class DBAdapter>
	struct SETNX : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "setnx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"setnx",	"SETNX"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			if (auto it = db.find(key); it && it->isValid(std::true_type{})){
				return Result::ok(false);
			}else{
				// SET

				const auto &val = p[2];
				const auto exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

				db.set(key, val, exp);

				return Result::ok(true);
			}
		}
	};



	template<class DBAdapter>
	struct SETXX : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "setxx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"setxx",	"SETXX"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			if (auto it = db.find(key); it && it->isValid(std::true_type{})){
				// SET

				const auto &val = p[2];
				const auto exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

				db.setHint(& *it, val, exp);

				return Result::ok(true);
			}else{
				return Result::ok(false);
			}
		}
	};



	template<class DBAdapter>
	struct DEL : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "del";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"del",		"DEL"	,
			"unlink",	"UNLINK"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() < 2)
				return Result::error();

			auto const varg = 1;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (const auto &key = *itk; key.empty())
					return Result::error();

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &key = *itk;
				db.del(key);
			}

			return Result::ok_1();
		}
	};



	template<class DBAdapter>
	struct HDEL : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "hdel";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"hdel",		"HDEL"
		};

		constexpr static std::size_t MAX_KEY_SIZE = hm4::PairConf::MAX_KEY_SIZE
						- DBAdapter::SEPARATOR.size()
						- 16;

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) final{
			if (p.size() < 3)
				return Result::error();

			const auto &keyN = p[1];

			if (keyN.empty())
				return Result::error();

			auto const varg = 2;

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				if (subN.empty())
					return Result::error();

				if (keyN.size() + subN.size() > MAX_KEY_SIZE)
					return Result::error();
			}

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
				const auto &subN = *itk;

				auto const key = concatenateBuffer(blob.buffer_key, keyN, DBAdapter::SEPARATOR, subN);

				db.del(key);
			}

			return Result::ok_1();
		}
	};



	template<class DBAdapter>
	struct GETSET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "getset";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"getset",	"GETSET"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) final{
			if (p.size() != 3 && p.size() != 4)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			// because old_value may be overwritten,
			// we had to make a copy.

			auto it = db.find(key);

			if (it && it->isValid(std::true_type{})){
				blob.string = it->getVal();
			}else{
				blob.string = "";
			}

			// SET

			const auto &val = p[2];
			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			db.setHint(& *it, val, exp);

			// return

			return Result::ok(blob.string);
		}
	};



	template<class DBAdapter>
	struct GETDEL : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "getdel";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"getdel",	"GETDEL"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) final{
			if (p.size() != 2)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			if (auto it = db.find(key); it && it->isValid(std::true_type{})){

				// because old_value may be overwritten,
				// we had to make a copy.
				blob.string = it->getVal();

				// DEL
				db.delHint(& *it);

				// return

				return Result::ok(blob.string);
			}else{
				return Result::ok("");
			}
		}
	};



	template<class DBAdapter>
	struct EXPIRE : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "expire";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"expire",	"EXPIRE"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() != 3)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();



			if (auto it = db.find(key); it && it->isValid(std::true_type{})){

				// SET
				auto const exp  = from_string<uint32_t>(p[2]);

				db.expHint(& *it, exp);

				return Result::ok(true);
			}else{
				return Result::ok(false);
			}
		}
	};



	template<class DBAdapter>
	struct PERSIST : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "persist";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"persist",	"PERSIST"
		};

		Result process(ParamContainer const &p, DBAdapter &db, OutputBlob &) final{
			if (p.size() != 2)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			if (auto it = db.find(key); it && it->isValid(std::true_type{})){
				// SET

				if (it->getTTL() > 0){
					uint32_t const exp = 0;
					db.expHint(& *it, exp);
				}

				return Result::ok(true);
			}else{
				return Result::ok(false);
			}


		}
	};



	template<class DBAdapter, class RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mutable";

		static void load(RegisterPack &pack){
			return registerCommands<DBAdapter, RegisterPack,
				SET		,
				MSET		,
				SETEX		,
				HSET		,
				HMSET		,
				SETNX		,
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


