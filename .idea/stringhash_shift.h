
constexpr uint64_t getHash(std::string_view const s){
	auto _ = [](auto s, uint8_t i){
		return static_cast<uint64_t>(s[i]) << (8 * (7 - i));
	};

	switch(s.size()){
	case 0:  return 0;
	case 1:  return 0 | _(s,0);
	case 2:  return 0 | _(s,0) | _(s,1);
	case 3:  return 0 | _(s,0) | _(s,1) | _(s,2);
	case 4:  return 0 | _(s,0) | _(s,1) | _(s,2) | _(s,3);
	case 5:  return 0 | _(s,0) | _(s,1) | _(s,2) | _(s,3) | _(s,4);
	case 6:  return 0 | _(s,0) | _(s,1) | _(s,2) | _(s,3) | _(s,4) | _(s,5);
	case 7:  return 0 | _(s,0) | _(s,1) | _(s,2) | _(s,3) | _(s,4) | _(s,5) | _(s,6);
	default:
	case 8:  return 0 | _(s,0) | _(s,1) | _(s,2) | _(s,3) | _(s,4) | _(s,5) | _(s,6) | _(s,7);
	}
}


