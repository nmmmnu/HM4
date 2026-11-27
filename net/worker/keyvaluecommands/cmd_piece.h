#include "base.h"

#include "mumps_piece.h"

#include <algorithm>

namespace net::worker::commands::Piece{

	namespace impl_{

		using MumpsPieceMany		= mumps_piece::MumpsPieceMany<OutputBlob::ContainerSize>;
		using MumpsPieceSingle		= mumps_piece::MumpsPieceSingle;
		using MumpsPieceAppend		= mumps_piece::MumpsPieceAppend<OutputBlob::ContainerSize>;

		inline std::string_view trimSV(std::string_view sv, char delimiter) noexcept{
			return mumps_piece::trimSV(sv, delimiter);
		}

		constexpr size_t clampIndex(std::string_view sv){
			auto const index = from_string<size_t>(sv);

			return std::clamp<size_t>(index, 0, OutputBlob::ContainerSize);
		}

		template <typename Protocol, typename MemoryManager, typename DBAdapter, typename Mutator, size_t N>
		void copyPieceInDB(DBAdapter &db, Result<Protocol,MemoryManager> &result, Mutator &mutator, std::string_view key, std::array<char, N> &buffer){
			auto const bytes = mutator.bytes();

			if (bytes > buffer.size())
				return result.set_error(ResultErrorMessages::INTERNAL_ERROR);

			char *data = buffer.data();

			mutator.copyToBuffer(data, bytes);

			auto const val = std::string_view{ data, bytes };

			hm4::insert(*db, key, val);

			return result.set_1();
		}

		template<typename MumpsPieceMutator, bool CP>
		struct PIECESET_Factory : hm4::PairFactory::IFactoryAction<CP,1, PIECESET_Factory<MumpsPieceMutator, CP> >{
			using Pair = hm4::Pair;
			using Base = hm4::PairFactory::IFactoryAction<CP,1, PIECESET_Factory<MumpsPieceMutator, CP> >;

			constexpr PIECESET_Factory(std::string_view const key, const Pair *pair, MumpsPieceMutator &mutator) :
							Base::IFactoryAction	(key, mutator.bytes(), pair),
							mutator			(mutator	){}

			void action(Pair *pair) const{
				mutator.copyToBuffer(pair->getValC());
			}

		private:
			MumpsPieceMutator &mutator;
		};

	} // namespace impl_



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PIECECOUNT: BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PIECECOUNT() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PIECECOUNT", std::begin(cmd__), std::end(cmd__)){}

		// piececount key delimiter
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			if (p.size() != 1 + 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const key		= p[1];
			auto const delimiter	= p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const val = hm4::getPairVal(*db, key);

			auto const count = 1 + std::count(std::begin(val), std::end(val), delimiter[0]);

			return result.set(count);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"piececount",		"PIECECOUNT",
			"piecesize",		"PIECESIZE"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PIECEGETALL: BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PIECEGETALL() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PIECEGETALL", std::begin(cmd__), std::end(cmd__)){}

		// piecegetall key delimiter
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() != 1 + 2)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

			auto const key		= p[1];
			auto const delimiter	= p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			using namespace impl_;

			auto const val = hm4::getPairVal(*db, key);

			auto const &mpm = blob.construct<MumpsPieceMany>(val, delimiter[0]);

			return result.set_container(mpm);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"piecegetall",		"PIECEGETALL"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PIECEGET: BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PIECEGET() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PIECEGET", std::begin(cmd__), std::end(cmd__)){}

		// pieceget key delimiter ID
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() != 1 + 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			using namespace impl_;

			auto const key		= p[1];
			auto const delimiter	= p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const index	= clampIndex(p[3]);

			auto const val = hm4::getPairVal(*db, key);

			// no need to be allocated, but for consistency
			auto const &mps = blob.construct<MumpsPieceSingle>(val, delimiter[0]);

			return result.set(mps[index]);
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pieceget",		"PIECEGET"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PIECEEXISTS: BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PIECEEXISTS() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PIECEEXISTS", std::begin(cmd__), std::end(cmd__)){}

		// pieceexists key delimiter ID
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() != 1 + 3)
				return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_3);

			using namespace impl_;

			auto const key		= p[1];
			auto const delimiter	= p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto const index	= clampIndex(p[3]);

			auto const val = hm4::getPairVal(*db, key);

			// no need to be allocated, but for consistency
			auto const &mps = blob.construct<MumpsPieceSingle>(val, delimiter[0]);

			return result.set(!mps[index].empty());
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pieceexists",		"PIECEEXISTS"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PIECEMGET: BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PIECEMGET() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PIECEMGET", std::begin(cmd__), std::end(cmd__)){}

		// piecemget key delimiter ID ID...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() < 1 + 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			auto const varg = 3;

			using namespace impl_;

			const auto key		= p[1];
			const auto delimiter	= p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			const auto val = hm4::getPairVal(*db, key);

			if (std::distance(std::begin(p) + varg, std::end(p)) == 1){
				// no need to be allocated, but for consistency
				auto const &mps = blob.construct<MumpsPieceSingle>(val, delimiter[0]);

				auto const index = clampIndex(p[3]);

				return result.set_containerN(mps[index]);
			}else{
				auto const &mpm = blob.construct<MumpsPieceMany>(val, delimiter[0]);

				auto &container = blob.construct<OutputBlob::SmallContainer>();

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					auto const index = clampIndex(*itk);

					container.emplace_back(mpm[index]);
				}

				return result.set_container(container);
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"piecemget",		"PIECEMGET"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PIECEMEXISTS: BaseCommandRO<Protocol,MemoryManager,DBAdapter>{

		PIECEMEXISTS() : BaseCommandRO<Protocol,MemoryManager,DBAdapter>("PIECEMEXISTS", std::begin(cmd__), std::end(cmd__)){}

		// piecemget key delimiter ID ID...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() < 1 + 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			auto const varg = 3;

			using namespace impl_;

			const auto key		= p[1];
			const auto delimiter	= p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			const auto val = hm4::getPairVal(*db, key);

			auto toBool = [](std::string_view s) -> std::string_view{
				return s.empty() ? "0" : "1";
			};

			if (std::distance(std::begin(p) + varg, std::end(p)) == 1){
				// no need to be allocated, but for consistency
				auto const &mps = blob.construct<MumpsPieceSingle>(val, delimiter[0]);

				auto const index = clampIndex(p[3]);

				return result.set_containerN(toBool(mps[index]));
			}else{
				auto const &mpm = blob.construct<MumpsPieceMany>(val, delimiter[0]);

				auto &container = blob.construct<OutputBlob::SmallContainer>();

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					auto const index = clampIndex(*itk);

					container.emplace_back(toBool(mpm[index]));
				}

				return result.set_container(container);
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"piecemexists",		"PIECEMEXISTS"
		};

	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PIECESETALL: BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		PIECESETALL() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("PIECESETALL", std::begin(cmd__), std::end(cmd__)){}

		// piecesetall key delimiter val val...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() < 1 + 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			auto const varg = 3;

			using namespace impl_;

			auto const key		= p[1];
			auto const delimiter	= p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			auto &mpm = blob.construct<MumpsPieceMany>(delimiter[0]);

			size_t id = 0;

			for(auto it = std::begin(p) + varg; it != std::end(p); ++it)
				mpm.set(id++, *it);

			auto &mutator = mpm;

			using Factory = PIECESET_Factory<MumpsPieceMany, 0>;

			hm4::insertV<Factory>(*db, key, nullptr, mutator);

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"piecesetall",		"PIECESETALL"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PIECESET: BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		PIECESET() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("PIECESET", std::begin(cmd__), std::end(cmd__)){}

		// pieceset key delimiter id val id val...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			auto const varg = 3;

			if (p.size() < 1 + 4 || (p.size() - varg) % 2 != 0)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_4);

			using namespace impl_;

			auto const key		= p[1];
			auto const delimiter	= p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			const auto pair = hm4::getPairPtr(*db, key);
			const auto val  = pair ? trimSV(pair->getVal(), delimiter[0]) : "";

			if (std::distance(std::begin(p) + varg, std::end(p)) == 2){
				// no need to be allocated, but for consistency
				auto const &mps = blob.construct<MumpsPieceSingle>(val, delimiter[0]);

				auto const index = clampIndex(p[3]);
				auto const item  = p[4];

				auto mutator = mps.mutate(index, item);

				if (val.empty()){
					logger<Logger::DEBUG>() << "PIECESET" << "single, no copy";

					using Factory = PIECESET_Factory<MumpsPieceSingle::Mutator, 0>;

					hm4::insertV<Factory>(*db, key, nullptr, mutator);

					return result.set_1();
				}else if (mutator.bytes() == val.size()){
					logger<Logger::DEBUG>() << "PIECESET" << "single, update in place";

					using Factory = PIECESET_Factory<MumpsPieceSingle::Mutator, 1>;

					Factory factory{ key, pair, mutator };

					insertHintVFactory(*db, pair, factory); // has hint

					return result.set_1();
				}else{
					logger<Logger::DEBUG>() << "PIECESET" << "single, safe double copy";

					auto &bufferVal  = blob.allocate<hm4::PairBufferVal>();

					return copyPieceInDB(db, result, mutator, key, bufferVal);
				}

			}else{
				auto &mpm = blob.construct<MumpsPieceMany>(val, delimiter[0]);

				for(auto itk = std::begin(p) + varg; itk != std::end(p); itk += 2){
					auto const index = clampIndex(*(itk + 0));
					auto const item  = *(itk + 1);

					if (!mpm.set(index, item))
						return result.set_error(ResultErrorMessages::INTERNAL_ERROR);
				}

				auto &mutator = mpm;

				if (val.empty()){
					logger<Logger::DEBUG>() << "PIECESET" << "many, no copy";

					using Factory = PIECESET_Factory<MumpsPieceMany, 0>;

					hm4::insertV<Factory>(*db, key, nullptr, mutator);

					return result.set_1();
				}else{
					logger<Logger::DEBUG>() << "PIECESET" << "many, safe double copy";

					auto &bufferVal  = blob.allocate<hm4::PairBufferVal>();

					return copyPieceInDB(db, result, mutator, key, bufferVal);
				}
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pieceset",		"PIECESET"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PIECEDEL: BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		PIECEDEL() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("PIECEDEL", std::begin(cmd__), std::end(cmd__)){}

		// piecedel key delimiter id id...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob) final{
			return process__(p, db, result, blob);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &blob){
			if (p.size() < 1 + 3)
				return result.set_error(ResultErrorMessages::NEED_MORE_PARAMS_3);

			auto const varg = 3;

			using namespace impl_;

			auto const key		= p[1];
			auto const delimiter	= p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			const auto val = trimSV(hm4::getPairVal(*db, key), delimiter[0]);

			if (val.empty()){
				// done !
				logger<Logger::DEBUG>() << "PIECEDEL:" << "no op";

				return result.set_1();
			}

			if (std::distance(std::begin(p) + varg, std::end(p)) == 1){
				// no need to be allocated, but for consistency
				auto const &mps = blob.construct<MumpsPieceSingle>(val, delimiter[0]);

				auto const index = clampIndex(p[3]);
				auto const item  = std::string_view{};

				auto mutator = mps.mutate(index, item);

				if (mutator.bytes() == val.size()){
					logger<Logger::DEBUG>() << "PIECEDEL:" << "single, no op (update in place)";

					return result.set_1();
				}else{
					logger<Logger::DEBUG>() << "PIECEDEL:" << "single, safe double copy";

					auto &bufferVal  = blob.allocate<hm4::PairBufferVal>();

					return copyPieceInDB(db, result, mutator, key, bufferVal);
				}
			}else{
				auto &mpm = blob.construct<MumpsPieceMany>(val, delimiter[0]);

				for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
					auto const index = clampIndex(*itk);
					auto const item  = std::string_view{};

					if (!mpm.set(index, item))
						return result.set_error(ResultErrorMessages::INTERNAL_ERROR);
				}

				auto &mutator = mpm;

				if (mutator.bytes() == val.size()){
					logger<Logger::DEBUG>() << "PIECEDEL:" << "many, no op";

					return result.set_1();
				}else{
					logger<Logger::DEBUG>() << "PIECEDEL:" << "many, safe double copy";

					auto &bufferVal  = blob.allocate<hm4::PairBufferVal>();

					return copyPieceInDB(db, result, mutator, key, bufferVal);
				}
			}
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"piecedel",		"PIECEDEL"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter>
	struct PIECEAPPEND: BaseCommandRW<Protocol,MemoryManager,DBAdapter>{

		PIECEAPPEND() : BaseCommandRW<Protocol,MemoryManager,DBAdapter>("PIECEAPPEND", std::begin(cmd__), std::end(cmd__)){}

		// pieceappend key delimiter val val...
		void process(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result, OutputBlob &) final{
			return process__(p, db, result);
		}

	private:
		static void process__(ParamContainer const &p, DBAdapter &db, Result<Protocol,MemoryManager> &result){
			auto const varg = 3;

			if (p.size() < 1 + 3)
				return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

			using namespace impl_;

			auto const key		= p[1];
			auto const delimiter	= p[2];

			if (!hm4::Pair::isKeyValid(key))
				return result.set_error(ResultErrorMessages::EMPTY_KEY);

			if (delimiter.size() != 1)
				return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
				if (itk->empty())
					return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

			const auto *pair = hm4::getPairPtr(*db, key);

			MumpsPieceAppend mutator{
				pair ? pair->getVal() : ""	,
				delimiter[0]			,
				std::begin(p) + varg		,
				std::end(p)
			};

			using Factory = PIECESET_Factory<MumpsPieceAppend, 1>;

			Factory factory{ key, pair, mutator };

			insertHintVFactory(*db, pair, factory); // has hint

			return result.set_1();
		}

	private:
		constexpr inline static std::string_view cmd__[] = {
			"pieceappend",		"PIECEAPPEND"
		};
	};



	template<typename Protocol, typename MemoryManager, typename DBAdapter, typename RegisterPack>
	struct RegisterModule{
		constexpr inline static std::string_view name	= "mumps_piece";

		static void load(RegisterPack &pack){
			return registerCommands<Protocol, MemoryManager, DBAdapter, RegisterPack,
				PIECECOUNT	,
				PIECEGETALL	,
				PIECEGET	,
				PIECEMGET	,
				PIECEEXISTS	,
				PIECEMEXISTS	,
				PIECESETALL	,
				PIECESET	,
				PIECEDEL	,
				PIECEAPPEND
			>(pack);
		}
	};

} // namespace


