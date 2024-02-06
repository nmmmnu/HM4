Changelog
=========

---

### 1.2.15
-	Release

### 1.2.16

-	bug fix expiration in GETSET
-	bug fix tombstone in DEL

### 1.2.17

-	Release

### 1.2.18

-	bug fix, prevent crash if db file exists, but other files does not.

### 1.2.19

-	fix potential bug in INCR / INCRBY / DECR / DECRBY

### 1.3.1

-	fix bug with system commands EXIT, SHUTDOWN
-	fix bug with command INFO - shows pair limits correctly

### 1.3.2

-	fix potential buffer overflow with inserting hints in BITSET

### 1.3.4.1

-	fix bug with comparing 8 bytes strings

### 1.3.4.2

-	fix bug with calculating List::bytes() in List::mutable_notify()

### 1.3.4.6

-	Fix bug with binlog reload
-	Fix bug with AVL root update

### 1.3.5.1

-	Fix bug with change AVLList::root_ in swapLinks / copyLinks
-	Fix bug with (not) updating the child parent in AVLList::fixParentAndChildren_

### 1.3.6.1

-	Fix potencial bug with clients who try to set data over the value limit.
	In such cases the server will disconnect the client, since the client will not check the error message and will continue to push the data.
	HM4 currently 256 MB, Redis support 512 MB.

### 1.3.7.2

-	bugfix on redis parser when CRLF is not read in full.

### 1.3.7.6
-	fix bug with PERSIST when TTL is under 1 sec
-	fix incompatibility with PHPRedis when using functions returning INT such TTL, INCR, DECR, STRLEN etc.
-	fix bug with checking value size in APPEND
