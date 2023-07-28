Changelog
=========

---

### 1.2.15
-	Release

### 1.2.16

-	make more user-friendly behavour in GETX and accumulators,
	by changing clamp-ing of the input value -
	now it will still clamp to higher value if you specify very big number like 100'000.
-	bug fix expiration in GETSET
-	bug fix tombstone in DEL
-	new command GETDEL
-	new command PERSIST
-	new command COPY
-	new command RENAME
-	new command VERSION
-	new command TYPE
-	new command UNLINK (alias to DEL)
-	new command PING, ECHO
-	new command SADD / SPOP (atomic queues)
-	new command COPYNX, RENAMENX
-	new command SELECT
-	implement file inode based reload
-	implement db_net automatic-reload
-	new tool db_compact - perform "smart" merge

### 1.2.17

-	automatic binlog replay when db_net starts
-	special seprator (currently \~) is added after SADD / SPOP keys,
	to avoid mixing with user keys.
-	new command HGETALL ... in getx module
-	new command HGET, HEXISTS ... in immutable module
-	new command HSET, HDEL ... in mutable module
-	new command PFADD, PFCOUNT, PFINTERSECT, PFMERGE, PFBITS
-	new command TOUCH
-	implement update in place
-	new command DELX
-	improve INFO - allocator information is shown now.
-	addressing "problem 2038' in mytime
-	implement update in place for EXPIRE / PERSIST / GETDEL / DELX
-	new command STRLEN (useful for debugging HLL and BIT modules)
-	new command BITSET, BITGET, BITCOUNT, BITMAX
-	new command MGET with multiple keys
-	improve DEL, HDEL, PFADD, PFCOUNT, PFMERGE with multiple keys
-	new command HMGET with multiple keys
-	new command MSET
-	new command SETXX
-	new command CAS, CAD, compatible with Alibaba Tair Cloud.
-	Release

### 1.2.18

-	if configured,
	INCR / INCRBY / DECR / DECRBY may return a string (convertible to zero),
	if previous value is some string (convertible to zero)
-	implement "no copy buffer" commands such GETSET and GETDEL
	via separate input and output buffers
-	new command RESET
-	new command DBSIZE
-	new command MSETNX, MSETXX
-	increase default value of "timeout" in the ini file to 5 min
-	implement SparePool of allocated buffer,
	similar to Apache httpd MinSpareServers / MaxSpareServers
-	increase max MAX_PARAM_SIZE in RedisProtocol parser
-	implement "no copy buffer" when read() from socket
-	bug fix, prevent crash if db file exists, but other files does not.
-	implement SparePool as MinMaxHeap,
-	use 3 way quicksort in UnsortedList for db_builder and log replay speedup of 30%

### 1.2.19

-	fix potential bug in INCR / INCRBY / DECR / DECRBY
-	refactor all commands
-	split GETX module into IMMUTABLEX and MUTABLEX
-	new command PERSISTX, EXPIREX
-	remove allocation in MSETXX
-	new command HGETKEYS, HGETVALS, HLEN
-	optimized overwrite in lists

### 1.3.0

-	refactor pair to support values up to 64 GB
-	refactor BITSET / BITGET
-	new commands BITMSET, BITMGET
-	new commands APPEND, GETRANGE
-	command INFO shows pair limits (Max Key Size / Max Val Size)
-	new commands MAXKEYSIZE, MAXVALSIZE
-	new commands MURMUR / MURMURHASH64A
-	new module Bloom Filter, new commands BFADD, BFRESERVE, BFEXISTS, BFMEXISTS
-	new commands PFRESERVE / HLLRESERVE
-	increase minimal Arena Allocator size to 4 * Max Size of the pair

### 1.3.1

-	refactor insertion, so double buffer copy is avoid as much as possible
-	fix bug with system commands EXIT, SHUTDOWN
-	fix bug with command INFO - shows pair limits correctly

### 1.3.2

-	fix potential buffer overflow with inserting hints in BITSET
-	refactor insertion, to have simple code
-	array functions - CVPUSH, CVPOP, CVSET, CVGET, CVMGET, CVGETRANGE, CVLEN, CVMAX

### 1.3.3

-	refactor IFactoryAction to use CRPT

### 1.3.4

-	new commands HDELALL, HPERSISTALL, HEXPIREALL
-	fsync binloglist with crontab
-	implement tombstone via expire (0x0'FFFF'FFFF), refactor hints
-	new commands PERSISTDELETED / PERSISTEXPIRED, DUMP
-	new tool db_compact_tombstone - propose tombstone removal merge - still in test stage
-	migrate to new version of format library FMT
-	new log system
-	new config options crontab_server_info and log_level
-	command INFO now show connected clients

### 1.3.4.1

-	fix bug with comparing 8 bytes strings

### 1.3.4.2

-	array functions not compiled by default
-	geohash functions - GEOADD, GEOREM, GEOGET, GEOMGET, GEORADIUS, GEODIST, GEOENCODE, GEODECODE
-	new config option map_memlist_arena for map all virtual memory pages to phisical memory used from AllocatorArena.
	This affect only Linux systems with vm.overcommit_memory turned on.
-	fix bug with calculating List::bytes() in List::mutable_notify()

