#include "isam.h"


namespace{



} // anonymous namespace

size_t const storage_size = 1024;
char storage[storage_size];

template<typename SearcherByName, typename ...Args>
void searchTest(ISAM const &isam, SearcherByName const &searcher, Args &&...args){
	auto const value  = isam.load(storage, searcher, std::forward<Args>(args)...);
	printf("---->>%.*s<<\n", static_cast<int>(value.size()), value.data());
};

int main(int argc, const char **argv){
	if (argc < 3){
		printf("Usage: %s text param param ...\n", argv[0]);
		printf("\tFor example\n");
		printf("\t%s 3:id,20:name,2:country,3   5 'New York' BGX 123\n", argv[0]);
		printf("\t\t(make sure to have field called 'name')\n");
		printf("\n");

		return 2;
	}

	std::string_view schema =  argv[1];

	{
		ISAM isam{ schema };

		if (isam.bytes() > storage_size){
			printf("Storage too small\n");
			return 3;
		}

		if (isam.size() != static_cast<size_t>(argc) - 2){
			printf("Incorrect number of parameters or overflow\n");
			return 3;
		}

		for(size_t i = 0; i < isam.size(); ++i)
			isam.store(storage, i, argv[i + 2]);

		printf(">>%.*s<<\n", static_cast<int>(isam.bytes()), storage);

		for(size_t i = 0; i < isam.size(); ++i){
			auto const value  = isam.load(storage, i);
			printf("====>>%.*s<<\n", static_cast<int>(value.size()), value.data());
		}

		isam.store(storage, "London", isam.getLinearSearcherByName(), "name");

		searchTest(isam, isam.getIndexSearcherByName(),  "name");
		searchTest(isam, isam.getLinearSearcherByName(), "name");

	//	isam.print(storage);
	}

	// storage still valid and populated...

	{
		ISAM isam;

		auto searcher = isam.parseAndSearchByName(schema, "name");

		isam.store(storage, "Sofia", searcher);

		searchTest(isam, searcher);

	//	isam.print(storage);
	}
}


