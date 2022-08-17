#include "base.h"



namespace net::worker::commands::Mutable{

	namespace impl_{
		template<class DBAdapter>
		auto exists(DBAdapter &db, std::string_view key){
			auto it = db.find(key);

			return it && it->isValid(std::true_type{});
		}
	}



	template<class DBAdapter>
	struct SET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "set";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"set",		"SET"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
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
	struct SETEX : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "setex";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"setex",	"SETEX"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
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
	struct SETNX : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "setnx";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"setnx",	"SETNX"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() != 3 && p.size() != 4)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			if (impl_::exists(db, key)){
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
	struct DEL : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "del";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"del",		"DEL"	,
			"unlink",	"UNLINK"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() != 2)
				return Result::error();

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			bool const result = db.del(key);

			return Result::ok(result);
		}
	};



	template<class DBAdapter>
	struct GETSET : Base<DBAdapter>{
		constexpr inline static std::string_view name	= "getset";
		constexpr inline static bool mut		= true;
		constexpr inline static std::string_view cmd[]	= {
			"getset",	"GETSET"
		};

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() != 3 && p.size() != 4)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			// because old_value may be overwritten,
			// we had to make a copy.

			if (auto it = db.find(key);
				it && it->isValid(std::true_type{})){

				blob.string = it->getVal();
			}else{
				blob.string = "";
			}

			// SET

			const auto &val = p[2];
			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			db.set(key, val, exp);

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

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &blob) const final{
			if (p.size() != 2)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			if (auto it = db.find(key);
				it && it->isValid(std::true_type{})){

				// because old_value may be overwritten,
				// we had to make a copy.
				blob.string = it->getVal();

				// DEL

				db.del(key);

				// return

				return Result::ok(blob.string);
			}else{
				blob.string = "";

				return Result::ok(blob.string);
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

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() != 3)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();



			if (auto it = db.find(key);
				it && it->isValid(std::true_type{})){

				// SET
				auto const exp  = from_string<uint32_t>(p[2]);

				db.set(key, it->getVal(), exp);

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

		Result operator()(ParamContainer const &p, DBAdapter &db, OutputBlob &) const final{
			if (p.size() != 2)
				return Result::error();

			// GET

			const auto &key = p[1];

			if (key.empty())
				return Result::error();

			if (auto it = db.find(key);
				it && it->isValid(std::true_type{})){

				// SET

				if (it->getTTL() > 0)
					db.set(key, it->getVal(), 0);

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
				SETEX		,
				SETNX		,
				DEL		,
				GETSET		,
				GETDEL		,
				EXPIRE		,
				PERSIST
			>(pack);
		}
	};


} // namespace


