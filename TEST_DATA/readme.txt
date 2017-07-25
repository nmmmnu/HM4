Prior the test you need to generate data files:

(
   ./bin/test_file w s TEST_DATA/data.001.txt TEST_DATA.BIN/data.001.db
   ./bin/test_file w s TEST_DATA/data.002.txt TEST_DATA.BIN/data.002.db
   ./bin/test_file w s TEST_DATA/data.003.txt TEST_DATA.BIN/data.003.db
   ./bin/test_file w s TEST_DATA/data.004.txt TEST_DATA.BIN/data.004.db
)

Then you can do tests such:

./bin/db_merge - TEST_DATA.BIN/all.db TEST_DATA.BIN/data.*.db

./bin/db_collection l TEST_DATA.BIN/'*'.db -
./bin/db_collection l TEST_DATA.BIN/'*'.db Bos
./bin/db_collection r TEST_DATA.BIN/'*'.db Boston


