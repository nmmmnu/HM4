
//we are in namespace net::worker::shared::msetmulti{

template<typename MyMDecoder, typename DBAdapter, typename Result>
void cmdProcessAdd(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &blob){
	if (p.size() != 7)
		return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_6);

	auto const keyN		= p[1];
	auto const keySub	= p[2];
	auto const delimiter	= p[3];
	auto const tokens	= p[4];
	auto const keySort	= p[5];
	auto const value	= p[6];

	if (delimiter.size() != 1)
		return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

	if (!valid(keyN, keySub, keySort))
		return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

	if (!hm4::Pair::isValValid(value.size()))
		return result.set_error(ResultErrorMessages::EMPTY_VAL);

	if (!MyMDecoder::checkSize(tokens.size()))
		return result.set_error(ResultErrorMessages::EMPTY_VAL);

	auto &containerNew	= blob.construct<OutputBlob::Container>();
	auto &containerOld	= blob.construct<OutputBlob::Container>();
	auto &buferVal		= blob.allocate<hm4::PairBufferVal>();

	bool const b = add<MyMDecoder>(db, keyN, keySub, tokens, delimiter[0], keySort, value,
						containerNew, containerOld, buferVal);

	if (!b)
		return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

	return result.set_1();
}

template<typename DBAdapter, typename Result>
void cmdProcessGet(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &){
	if (p.size() != 3)
		return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

	auto const keyN   = p[1];
	auto const keySub = p[2];

	if (!valid(keyN, keySub))
		return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

	using MyMDecoder = shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	return result.set(
		get<MyMDecoder>(db, keyN, keySub)
	);
}

template<typename DBAdapter, typename Result>
void cmdProcessMGet(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &blob){
	if (p.size() < 3)
		return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

	const auto keyN = p[1];

	if (keyN.empty())
		return result.set_error(ResultErrorMessages::EMPTY_KEY);

	auto const varg = 2;

	for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk)
		if (auto const keySub = *itk; !valid(keyN, keySub))
			return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

	auto &container = blob.construct<OutputBlob::Container>();

	using MyMDecoder = shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
		auto const &keySub = *itk;

		container.emplace_back(
			get<MyMDecoder>(db, keyN, keySub)
		);
	}

	return result.set_container(container);
}

template<typename DBAdapter, typename Result>
void cmdProcessExists(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &){
	// EXISTS key subkey0

	if (p.size() != 3)
		return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

	auto const keyN   = p[1];
	auto const keySub = p[2];

	if (keyN.empty() || keySub.empty())
		return result.set_error(ResultErrorMessages::EMPTY_KEY);

	if (!valid(keyN, keySub))
		return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);

	return result.set(
		exists(db, keyN, keySub)
	);
}

template<typename DBAdapter, typename Result>
void cmdProcessGetIndexes(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &blob){
	if (p.size() != 3)
		return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_2);

	auto const keyN   = p[1];
	auto const keySub = p[2];

	if (!valid(keyN, keySub))
		return result.set_error(ResultErrorMessages::EMPTY_KEY);

	auto &container = blob.construct<OutputBlob::Container>();

	using MyMDecoder = shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	[[maybe_unused]]
	bool const b = getIndexes<MyMDecoder>(db, keyN, keySub, container);

	return result.set_container(container);
}

template<typename DBAdapter, typename Result>
void cmdProcessRem(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &blob){
	// REM key subkey0 subkey1 ...

	if (p.size() < 3)
		return result.set_error(ResultErrorMessages::NEED_GROUP_PARAMS_3);

	const auto &keyN = p[1];

	if (keyN.empty())
		return result.set_error(ResultErrorMessages::EMPTY_KEY);

	auto const varg = 2;

	for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
		if (auto const &keySub = *itk; !valid(keyN, keySub))
			return result.set_error(ResultErrorMessages::INVALID_KEY_SIZE);
	}

	auto &container = blob.construct<OutputBlob::Container>();
	auto &buferVal  = blob.allocate<hm4::PairBufferVal>();

	using MyMDecoder = shared::msetmulti::FTS::BaseMDecoder<DBAdapter>;

	for(auto itk = std::begin(p) + varg; itk != std::end(p); ++itk){
		auto const &keySub = *itk;

		container.clear();

		rem<MyMDecoder>(db, keyN, keySub, container, buferVal);
	}

	return result.set_1();
}


template<typename MyMDecoder, typename MyFTS, typename DBAdapter, typename Result>
void cmdProcessRangeMulti(ParamContainer const &p, DBAdapter &db, Result &result, OutputBlob &blob){
	using namespace shared::accumulate_results;

	if (p.size() != 6)
		return result.set_error(ResultErrorMessages::NEED_EXACT_PARAMS_5);

	auto const keyN		= p[1];
	auto const delimiter	= p[2];
	auto const tokens	= p[3];

	if (delimiter.size() != 1)
		return result.set_error(ResultErrorMessages::INVALID_PARAMETERS);

	if (keyN.empty() || tokens.empty())
		return result.set_error(ResultErrorMessages::EMPTY_KEY);

	auto const count	= myClamp<uint32_t>(p[4], ITERATIONS_RESULTS_MIN, ITERATIONS_RESULTS_MAX);
	auto const keyStart	= p[5];

	return processFTS<MyMDecoder, MyFTS>(keyN, delimiter[0], tokens, count, keyStart,
							db, result, blob);
}


