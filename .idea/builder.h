namespace FileBuilderConf{
	constexpr auto MODE = std::ios::out | std::ios::binary;
}

class FileDataBuilder{
public:
	FileDataBuilder(std::string_view const filename, bool const aligned) :
						file_data(filenameData(filename), FileBuilderConf::MODE),
						aligned(aligned){}

	void operator()(Pair const &pair){
		pair.fwrite(file_data);
		if (aligned)
			my_align::fwriteGap(file_data, pair.bytes(), PairConf::ALIGN);
	}

private:
	std::ofstream	file_data;

	bool		aligned;
};



class FileIndxBuilder{
public:
	FileIndxBuilder(std::string_view const filename, bool const aligned) :
						file_indx(filenameIndx(filename), FileBuilderConf::MODE),
						file_line(filenameLine(filename), FileBuilderConf::MODE),
						aligned(aligned){}

	void operator()(Pair const &pair){
		writeU64(file_indx, index);

		writeLine_(pair.getKey());

		if (aligned)
			index += my_align::calc(pair.bytes(), PairConf::ALIGN);
		else
			index += pair.bytes();
	}

private:
	void writeLine_(std::string_view const key){
		auto hkey = HPair::SS::createBE(key);

		if (hkey_ == hkey)
			return;

		// store new key
		hkey_ = hkey;

		writeStr(file_line, hkey_);
		writeU64(file_line, index);
	}

private:
	std::ofstream	file_indx;
	std::ofstream	file_line;

	bool		aligned;

	uint64_t	index		= 0;

	HPair::HKey	hkey_ = 0;
};



class FileMetaBuilder{
public:
	FileMetaBuilder(std::string_view const filename, bool const aligned) :
						file_meta(filenameMeta(filename), FileBuilderConf::MODE),
						aligned(aligned){}

	~FileMetaBuilder(){
		// write the header
		uint16_t const option_aligned = aligned ? FileMetaBlob::OPTIONS_ALIGNED : FileMetaBlob::OPTIONS_NONE;

		// write table header
		uint16_t const options =
					FileMetaBlob::OPTIONS_NONE	|
					FileMetaBlob::OPTIONS_SORTED	|
					option_aligned
		;

		using hm4::disk::FileMetaBlob;

		const FileMetaBlob blob = FileMetaBlob::create(
			options,
			count,
			tombstones,
			fixMin(minCreated),
			fixMax(maxCreated)
		);

		file_meta.write( (const char *) & blob, sizeof(FileMetaBlob));
	}

	void operator()(Pair const &pair){
		auto const created = pair.getCreated();

		if (created < minCreated)
			minCreated = created;

		if (created > maxCreated)
			maxCreated = created;

		if (pair.isTombstone())
			++tombstones;

		++count;
	}

private:
	std::ofstream	file_meta;

	bool		aligned;

	uint64_t	minCreated	= std::numeric_limits<uint64_t>::max();
	uint64_t	maxCreated	= std::numeric_limits<uint64_t>::min();

	uint64_t	count		= 0;
	uint64_t	tombstones	= 0;
};



class FileBuilder{
public:
	using value_type = Pair const;

	FileBuilder(std::string_view const filename, bool const aligned):
				meta(filename, aligned),
				indx(filename, aligned),
				data(filename, aligned){}

	void operator()(Pair const &pair){
		push_back(pair);
	}

	void push_back(Pair const &pair){
		meta(pair);
		indx(pair);
		data(pair);
	}

private:
	FileMetaBuilder meta;
	FileIndxBuilder indx;
	FileDataBuilder data;
};



