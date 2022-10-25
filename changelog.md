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



