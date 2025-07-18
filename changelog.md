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
-	Release

### 1.3.4.3

-	introduce new test data structures - unrolled link list and unrolled skip list, but not using them in production
-	MacOS / M1 compilation
-	introduce new test data structure - AVL list, based on non recursive AVL Tree with balance and parent pointer.
-	make AVL list default data structure in db_net and db_builder_concurrent

### 1.3.4.4

-	Make AVL list intrusive, e.g. store the pair directly inside the node.
	This allows to save 8 bytes per node + another 8 bytes from hkey.
	Unfortunately the 7 bytes gap is still inside the node.
	This structure have memory footprint only 2% more than skip list and
	in same time is much faster than AVL list with pointer to the pair.

### 1.3.4.5

-	Release

### 1.3.4.6

-	Fix bug with binlog reload
-	Tool db_logger is removed
-	Make logger accept two iterators to log array-like objects
-	Possibility to log commands, currently compile time setting
-	Fix bug with AVL root update
-	Make AVL iterator bidirectional
-	Show memlist type in INFO command
-	Show built time in help screeens

### 1.3.5

-	Release

### 1.3.5.1

-	If Allocator needs deallocation, AVLList deallocate memory in non-recursive way.
	This does not affect ArenaAllocator.
-	Fix bug with change AVLList::root_ in swapLinks / copyLinks
-	Fix bug with (not) updating the child parent in AVLList::fixParentAndChildren_
-	Show memory list type in help screeens
-	new command TEST
-	Release

### 1.3.5.2

-	Refactor DiskFileAllocatorPredicate to check and flush before insert.
	This fixing logical error with iterators,
	also make use of all available memory in the ArenaAllocator.
-	Prevent data loss in case there is not enought memory,
	by refactor List::insertF return type and DiskFileAllocatorPredicate.
-	Change list used in db_net's replayBinlogFile_() from UnsortedList to AVLList

### 1.3.5.3

-	new command LISTMAINTAINANCE
-	Make sure flushlist do not flush twice (should never happen).
	If this ever happens, data is lost as it was in previous version.
-	Introduce MMapAllocator with Linux HugeTLB support.
	Support is automatic as long as vm.nr_hugepages are allowed.

### 1.3.6

-	Release

### 1.3.6.1

-	Refactor arena allocator
-	Fix potencial bug with clients who try to set data over the value limit.
	In such cases the server will disconnect the client, since the client will not check the error message and will continue to push the data.
	HM4 currently 256 MB, Redis support 512 MB.
	This is the same way Redis handle the same problem.
-	Add reverse iterator to AVLList and VectorList

### 1.3.7

-	Release

### 1.3.7.1

-	new utility db_compact_tombstones
-	rename commands COUNT, SUM, MIN, MAX to XNCOUNT, XNSUM, XNMIN, XNMAX
-	add alias commands GETX -> XNGET
-	new range commands XRGET, XUGET
-	new range commands XRCOUNT, XRSUM, XRMIN, XRMAX
-	new range commands XRDEL, XRPERSIST, XREXPIRE
-	XN / XR commands no longer accept empty prefix / range_end
-	new range commands XRAVG, XRAVG
-	refactor XN / XR accumulators to accept only 2 arguments, but keep COUNT and SUM to accept traditional 3 arguments
-	new range commands XNFIRST, XRFIRST, XNLAST, XRLAST
-	clang / Apple silicon build

### 1.3.7.2

-	bugfix on redis parser when CRLF is not read in full.
-	Release

### 1.3.7.4
-	new range commands XUGETKEYS, XNGETKEYS, XRGETKEYS
-	Release

### 1.3.7.5
-	make GETX, XNGET, XRGET, XUGET, XUGETKEYS, XNGETKEYS, XRGETKEYS always return next key.
-	new command TIME
-	new command EXPIRETIME, EXPIREAT
-	new command XNEXPIREAT, XREXPIREAT, HEXPIREATALL

### 1.3.7.6
-	new module mutable_get
-	new command GETEX, GETEXAT, GETPERSIST
-	make DiskList not open empty files
-	add List::empty() for future development
-	differentiate hm4::insert from hm4::insertTS and hm4::erase
-	new command DBSIZEMUTABLE
-	add new erase method in DualList DualListEraseType::SMART_TOMBSTONE
-	compile time setting for using DualListEraseType::TOMBSTONE or DualListEraseType::SMART_TOMBSTONE
-	introduce FlushList::mutable_version() in order to detect flushes more correct and easily
-	fix bug with PERSIST when TTL is under 1 sec
-	add RedisProtocol INT responce.
-	Fix incompatibility with PHPRedis when using functions returning INT such TTL, INCR, DECR, STRLEN etc.
-	add RedisProtocol simple string responce.
-	fix incompatibility with PHPRedis when using functions returning TYPE
-	fix bug in AVLList with AVLList::erase_, affecting only the development version.
-	fix bug in AVLList with AVLList::rebalanceAfterErase_, affecting only the development version.
-	update software prefetch
-	fix bug with misscalculation of bytes in INFO command
-	fix bug with BFMEXISTS, CMSMCOUNT when accessing empty key.
-	fix feature with XNGET / XRGET where result have no "tail"
-	increase Redis param buffer to 256
-	add Redis user readable error messages
-	fix error with GEORADIUS cells enumeration
-	Release

### 1.3.7.7
-	pull zset-like functions from geo module.
-	new module morton curve: MC2GET, MC2MGET, MC2EXISTS, MC2SCORE,
	MC2ADD, MC2REM, MC2POINT, MC2RANGENAIVE, MC2RANGE, MC2ENCODE, MC2DECODE
-	make zset-like functions to use ScoreController,
	in order geo functions to avoid additional key retreaval.
-	new module linear curve: MC1GET, MC1MGET, MC1EXISTS, MC1SCORE,
	MC1ADD, MC1REM, MC1POINT, MC1RANGE
-	fix bug with checking value size in APPEND
-	refactor flushing, so references to old data stay stable until data is inserted
-	refactor INCR / DECR in order to use hints if possible
-	new commands INCRTO / DECRTO
-	fix BFRESERVE, CMSRESERVE, HLLRESERVE behavour
-	fix bug with use after remove in BF, CMS, HLL
-	new module heavy hitter: HHINCR, HHDECR, HHRESERVE, HHGET
-	make PFADD result redis compatible
-	new command PFADDCOUNT
-	Release

### 1.3.7.8
-	fix return type of HHINCR, HHDECR.
-	refactor HHINCR, HHDECR, PFADD, PFADDCOUNT
-	new module Misra-Gries: MRADD, MRADDGET, MRRESERVE, MRGET
-	increase limits on HH
-	new command CMSADDCOUNT
-	bugfix on mutable_x when table is flushed during commands like DELX.
-	new module counting bloom filter: CBFADD, CBFADDCOUNT, CBFRESERVE, CBFCOUNT, CBFMCOUNT
-	refactor count min sketch
-	new commands CMSREM, CMSREMCOUNT
-	new commands CBFREM, CBFREMCOUNT
-	INCR / DECR does not overflow
-	new commands INCRLIMIT / DECRLIMIT
-	disable negative increace_val/decrease_val in INCRLIMIT / DECRLIMIT
-	bugfix with removing key in GEOREM
-	return ~256 MB memory (1 pair) to OS after doing flush
-	new commands XNNEXT, XRNEXT, XUNEXT
-	new module reservoir sampling: RSADD, RSRESERVE, RSGET, RSGETCOUNT
-	bugfix on searching disktable keys with size 8 characters

### 1.3.8
-	refactor geo, linear curve, morton curve modules. This change is NOT breaking with previous versions.
-	new module INDEX: IX*GET, IX*MGET, IX*GETINDEXES, IX*ADD, IX*REM, IX*RANGE
-	new module TDigest: TDRESERVE, TDADD, TDADDWEIGHT, TDMERGE, TDMERGECAPACI, TDPERCENTILE, TDMPERCENTILE, TDMEDIAN, TDTRIMMEDMEAN, TDMEAN, TDMIN, TDMAX, TDSIZE, TDINFO
-	skip using OutputBlob for small vectors that can stay on the stack
-	Release

### 1.3.9
-	Introduce FileWriter instead of std::ofstream
-	Redis protocol output very large numbers as strings to avoid errors with redis-cli
-	Refactor buffers and memory resources
-	Add hash indexes
-	Free 8 bit memory per AVLList node by moving balance into the up pointer
-	Add SBO (small buffer optimization) for disktables smaller than 2KB to avoid unnecessary VM preasure
-	Release

### 1.3.10
-	Fix bug in AVLList deallocation (part of tests, not affecting  db_net)
-	Introduce List::getPair(), which eliminate using of iterators and memory allocations in several places
-	Merge / Replace StaticVector with new version
-	Introduce SmallVector in order to eliminate memory allocation in CollectionIterator
-	Introduce buffered writting when creating disktables for speedup of 5%

### 1.3.10.1
-	Changing HeapSparePool MinMaxHeap to less sophisticated standard MaxHeap.
-	Find bug with "bad_alloc" in network protocol when too many clients come at once.

### 1.3.10.2
-	Fix non-working tcp_reuseport config option
-	Change std::vector in AsyncLoop to SmallVector to avoid allocation.
-	new module morton curve 3D: MC3GET, MC3MGET, MC3EXISTS, MC3SCORE,
	MC3ADD, MC3REM, MC3POINT, MC3RANGENAIVE, MC3RANGE, MC3ENCODE, MC3DECODE
-	Optimized MC2POINT and MC3POINT
-	New module morton curve 4D: MC4GET, MC4MGET, MC4EXISTS, MC4SCORE,
	MC4ADD, MC4REM, MC4POINT, MC4RANGENAIVE, MC4RANGE, MC4ENCODE, MC4DECODE
-	Add aliases MC2RANGEFLAT, MC3RANGEFLAT, MC4RANGEFLAT
-	Refactor morton curve 2D, 3D, 4D modules.

### 1.3.11
-	Release

### 1.3.12
-	Change version int scheme (this feature was never used)
