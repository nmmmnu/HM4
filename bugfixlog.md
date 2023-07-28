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

