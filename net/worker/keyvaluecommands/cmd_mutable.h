#include "base.h"



namespace net::worker::commands::Mutable{



	template<class Protocol, class DBAdapter>
	struct SET : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			auto const &key = p[1];

			if (key.empty())
				return;

			auto const &val = p[2];
			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			hm4::insert(*db, key, val, exp);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"set",		"SET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MSET : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

				hm4::insert(*db, key, val);
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"mset",		"MSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MSETNX : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

				if ( hm4::getPairOK(*db, key) )
					return result.set_0();
			}

			// set keys
			for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
				auto const &key = *itk;
				auto const &val = *std::next(itk);

				hm4::insert(*db, key, val);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"msetnx",		"MSETNX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct MSETXX : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

				if (auto *it = hm4::getPairPtr(*db, key); it){
					auto const &val = *std::next(itk);

				// TODO HINT

				//	if (const auto *p = & *it; db.canUpdateWithHint(p))
				//		buffer_.emplace_back(p, key, val);
				//	else

					buffer_.emplace_back(nullptr, key, val);

				}else
					return result.set_0();
			}

			// set keys with hints
			// for(auto &x : buffer_)
			// 	if (x.p)
			// 		db.setHint(x.p, x.val);

			// set keys without hints
			for(auto &x : buffer_)
			//	if (x.p == nullptr)
				hm4::insert(*db, x.key, x.val);

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

	private:
		constexpr inline static std::string_view cmd[]	= {
			"msetxx",		"MSETXX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SETEX : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 4)
				return;

			auto const &key = p[1];

			if (key.empty())
				return;

			auto const &val = p[3];
			auto const exp  = from_string<uint32_t>(p[2]);

			hm4::insert(*db, key, val, exp);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"setex",	"SETEX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HSET : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

			hm4::insert(*db, key, val, exp);

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hset",		"HSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HMSET : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

				hm4::insert(*db, key, val);
			}

			return result.set();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hmset",		"HMSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SETNX : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;

			if ( hm4::getPairOK(*db, key) ){
				return result.set(false);
			}else{
				// SET

				const auto &val = p[2];
				const auto exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

				hm4::insert(*db, key, val, exp);

				return result.set(true);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"setnx",	"SETNX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct SETXX : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3 && p.size() != 4)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;



			if (auto *it = hm4::getPairPtr(*db, key); it){
				// SET

				const auto &val = p[2];
				const auto exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			// TODO HINT
			//	db.setHint(& *it, val, exp);

				hm4::insert(*db, key, val, exp);

				return result.set(true);
			}else{
				return result.set(false);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"setxx",	"SETXX"
		};
	};



	template<class Protocol, class DBAdapter>
	struct DEL : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

				hm4::erase(*db, key);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"del",		"DEL"	,
			"unlink",	"UNLINK"
		};
	};



	template<class Protocol, class DBAdapter>
	struct HDEL : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

				hm4::erase(*db, key);
			}

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"hdel",		"HDEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GETSET : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
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

			auto it = db->find(key, std::true_type{});

			if (it != std::end(*db) && it->isValid(std::true_type{})){
				// seamlessly send value to output buffer...
				result.set(it->getVal());
			}else{
				result.set("");
			}

			// SET

			const auto &val = p[2];
			auto const exp  = p.size() == 4 ? from_string<uint32_t>(p[3]) : 0;

			// TODO HINT
			// db.setHint(& *it, val, exp);

			hm4::insert(*db, key, val, exp);

			// return

			return;
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"getset",	"GETSET"
		};
	};



	template<class Protocol, class DBAdapter>
	struct GETDEL : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;



			if (auto *it = hm4::getPairPtr(*db, key); it){
				// seamlessly send value to output buffer...

				result.set(it->getVal());

				// DEL
				// TODO HINT
				// db.delHint(& *it);
				hm4::erase(*db, key);

				// return

				return;
			}else{
				return result.set("");
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"getdel",	"GETDEL"
		};
	};



	template<class Protocol, class DBAdapter>
	struct EXPIRE : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 3)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;



			if (auto *it = hm4::getPairPtr(*db, key); it){
				// SET
				auto const exp  = from_string<uint32_t>(p[2]);

				// TODO HINT
				// db.expHint(& *it, exp);
				hm4::insert(*db, key, it->getVal(), exp);

				return result.set(true);
			}else{
				return result.set(false);
			}
		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"expire",	"EXPIRE"
		};
	};



	template<class Protocol, class DBAdapter>
	struct PERSIST : MBase<Protocol,DBAdapter>{
		const std::string_view *begin() const final{
			return std::begin(cmd);
		};

		const std::string_view *end()   const final{
			return std::end(cmd);
		};

		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol> &result, OutputBlob &) final{
			if (p.size() != 2)
				return;

			// GET

			const auto &key = p[1];

			if (key.empty())
				return;



			if (auto *it = hm4::getPairPtr(*db, key); it){
				// SET

				if (it->getTTL() > 0){
					uint32_t const exp = 0;

					// TODO HINT
					// db.expHint(& *it, exp);
					hm4::insert(*db, key, it->getVal(), exp);
				}

				return result.set(true);
			}else{
				return result.set(false);
			}


		}

	private:
		constexpr inline static std::string_view cmd[]	= {
			"persist",	"PERSIST"
		};
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


