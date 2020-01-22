Prior the test you need to generate data files:

(
   bin/db_builder    TEST_DATA/data.001.txt TEST_DATA.BIN/data.001.db 128
   bin/db_builder    TEST_DATA/data.002.txt TEST_DATA.BIN/data.002.db 128
   bin/db_builder    TEST_DATA/data.003.txt TEST_DATA.BIN/data.003.db 128
   bin/db_builder    TEST_DATA/data.004.txt TEST_DATA.BIN/data.004.db 128

   bin/db_builder    TEST_DATA/unsorted.0.txt TEST_DATA.BIN/unsorted.0.db 128
)

Then you can do tests such:

bin/db_merge - TEST_DATA.BIN/all.db TEST_DATA.BIN/data.*.db

bin/db_file l TEST_DATA.BIN/data.'*'.db -
bin/db_file l TEST_DATA.BIN/data.'*'.db Bos
bin/db_file r TEST_DATA.BIN/data.'*'.db Boston
bin/db_file r TEST_DATA.BIN/data.'*'.db Muenchen

bin/db_file l TEST_DATA.BIN/unsorted.'*'.db -

