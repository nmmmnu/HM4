HM4 - key/value NoSQL database that utilize LSM trees
=====================================================

---
### Why another key/value?

We worked a lot with Apache Cassandra, but we never needed its distributed functionality.
We also worked with Redis, but problem there is data must fit in memory.

Goals of the project are.

-   Key/Value store
-   Redis protocol
-   Async network I/O
-   Data not need to fit in memory
-   Speed
-   Consistent
-   High quality code
-   Supported [commands]
-   Complexity is a lie :)
-   Atomic queues
-   HyperLogLog

---
### Architecture

Architecture is derived from Apache Cassandra.

HM4 works with **sorted** list of key-value pairs.

There are memtable and several disktables (files on the disk).

Writes are sent to the memtable.

Reading are sent to Memtable and ALL of the Disktables. Then system finds most recent pair.

*This operation is not as slow as it seems :)*

This is called "LSM tree" or much simpler - "Differential files".

Writes should be always fast. Reads should be fast if there are not too many disktables.

As of 1.2.3 additionally there is optional binlog.
In case of power loss or system crash, memtable can re recovered from binlog

---
### Memtable

Memtable is stored in memory in SkipList. SkipList is very fast O(Log N) structure, very similar to binary tree.

It is much faster than a vector O(Amortized Log N), but slower than hashtable O(Amortized 1).

---
### Memtable with VectorList

VectorList performance was very good, but worse than SkipList.

However if you just want to load data, is much faster to use SkipList and sort just before store it on the disk.

VectorList also have much low memory consumption, so you can fit 30-40% more data in same memory.

---
### Testing memtable with other structures

Prior SkipList we tested with hashtables, VectorList (dynamic array with shifting) and LinkedList.

General speed of hashtables was much faster than skiplists, but there were some problems, such:

-   Hashtable buckets must be fixed, because data must later be stored on the disk with LSM tree.
-   Because of hashtable buckets, disktables have more complicated structure and are initially larger - all bucket must be stored.
-   Slow creation of disktables.
    Because buckets are fixed, just as it is in Tokyo Cabinet, one may choose high number of buckets, say 1M.
    This means even "empty" memtable is translated to huge disktable with at least 1M "records" on the disk.
-   Merging of disktable with "buckets" is more difficult, but also waste more unused space in the file.
    This is not discussed yet, but merging two tables with sizes N1 and N2 elements, will need allocation of N1 + N2 space on the "output" disktable.

VectorList performance was very good, but worse than SkipList.

LinkedList performance was poor.

Because of C++ classes all VectorList, LinkedList, SkipList are available.

---
### DiskTables

Periodicaly, the memtable is flushed on the disk in DiskTable.
DiskTable file have similar order - vector/array like structure with sorted elements.

DiskTable(s) are immutable. This means very good OS cache and easy backup.
Downside is that deletes must be implemented by markers called tombstones (same key, empty value).

DiskTable's header and all control data are 64bit integers (uint64_t) stored as BigEndian.

HM4 is reading DiskTable using MMAP().

Effors were made HM4 to work on 32bit OS, but soon or later these runs out of address space, so we abandoned the idea.

HM4 "officially" needs 64bit OS.

---
### LSM tree notes.

Here is what Wikipedia say about:

[Log structured merge tree]

However there is 1976's research by Dennis G. Severance and Guy M. Lohman on the similar topic here:

[Differential files: their application to the maintenance of large databases - University of Minnesota, Minneapolis]

---
### LSM compaction.

When database works, many disktables are created and this will slow down the reads very much.

By this reason disktables must be merged.

Because disktables are read only, merge can be implemented as separate user space process.

Unlike Cassandra, it is your responcibility to run the process yourself.

Unlike Apache Cassandra, there are a safe way to compact several tables into single file.

---
### List of software.

- db_net	- server
- db_compact	- automatic merge DiskTables
- db_builder	- create DiskTable(s) from tab delimited file
- db_merge	- merge DiskTable(s)
- db_logger	- create bin-log from tab delimited file - similar to **db_builder**, but not very usable
- db_replay	- create DiskTable(s) from binlog
- db_mkbtree	- create btree to existing DiskTable for much faster access
- db_file	- query DiskTable or LSM
- db_preload	- preload DiskTable into system cache, by reading 1024 samples from the DiskTable

---
### Complexity is a lie :)

Time complexity of most of the operations is O(Log N). However it heavily depends of the operation.

First example is **GET** command.

- First it need to do search in memlist O(Log N).
- Then it need to do search in disklist. If we have M files on the disk, thats another O(M Log N). However, compared to memlist, this is much slower operation.
- Total complexity is "Mem + M * Disk"

Second example is **DEL** command.

- Only thing it need to do is to insert tombstone in memlist O(Log N).
- Total complexity is "Mem"

Next example is **GETX**

- First it need to do search in memlist O(Log N).
- Then it need to do search in disklist. If we have M files on the disk, thats another O(M Log N). However, compared to memlist, this is much slower operation.
- To find subsequent keys, no time is wasted.
- Total complexity is "Mem + M * Disk"

Last example is **INCR** command.

- First it need to get the current value of the key. This is same as **GET**
- Second it need to store new value - this is same as **SET** or **DEL**
- Total complexity is "2 * Mem + M * Disk"

---
### Atomic queues

HM4 supports **atomic queues**. Supported commands are **SADD** and **SPOP**.

Each queue is stored as several keys stored continuous.

- Control key - same name as queue name.
If queue name is "q", then the control key is also "q". This key may or may not exists.
- Data keys - each key name is same as queue name + current time with microseconds.
If queue name is "q", one of the data keys could be "q62fabbc0.000490be".
"62fabbc0" was current time at the moment when key was created, "000490be" were current microseconds.

How it works:

- When a value is pushed in the queue, e.g. **SADD**, the system just set new key, for example "q\~62fabbc0.000490be".
  Since current time with microseconds is as good as UUID, no collision can happen.

- When a value is removed from the "head" of the queue, e.g. **SPOP**, the system search for the control key, for exaple "q".
  - If control key is present, it is read. It contains the last removed key from the queue.
  Then new search is made to retrieve the "head" of the queue.
  - If control key is not present, this means that the search is already positioned to the "head" of the queue.
- Finally the system deletes the data key and eventually updates the control key.

---
### HyperLogLog

HyperLogLog (HLL) is an algorithm for counting unique elements.

[HyperLogLog]

Implementation uses 12bits. This means each key is 4096 bytes and the error rate is 1.62%.

There are no fancy encodings, these 4096 bytes are uint8_t counters from the standard HLL.

How it works:

- Value can be added using **PFADD**. If the key does not exists or contains different information, it is overwritten.

- Cardinality can be estimated using **PFCOUNT**. It can be called with zero, one or up to 5 keys / HLL sets.
  If more than one key is supplied, a HLL union is performed and result is returned.

      redis> pfadd a john
      "1"
      redis> pfadd a steven
      "1"
      redis> pfadd b bob
      "1"
      redis> pfadd b steven
      "1"
      redis> pfcount a
      "2"
      redis> pfcount b
      "2"
      redis> pfcount a b
      "3"

  Note how "pfcount a b" returns just 3 elements because "steven" was added in both a and b.

- Intersection of the cardinality can be estimated using **PFINTERSECT**.  It can be called with zero, one or up to 5 keys / HLL sets.

      redis> pfintersect a b
      "1"

  This command is not redis compatible and generally is slow, if it is performed with 5 keys / HLL sets.
  Also note if you intersect small HLL set with large HLL set, the error of the large HLL set, might be bigger than the cardinality of the small HLL set.
  This means the result error rate, might be much great from the standard error rate.

- HLL union can be stored using **PFMERGE**. It can be called with zero, one or up to 5 keys / HLL sets.
  Using **PFMERGE**, union can be calculated from more than 5 keys / HLL sets.

      redis> pfmerge dest a b
      OK
      redis> pfcount dest
      "3"

[Log structured merge tree]: http://en.wikipedia.org/wiki/Log-structured_merge-tree
[Differential files: their application to the maintenance of large databases - University of Minnesota, Minneapolis]: http://www-users.cs.umn.edu/~he/diff/p256-severance.pdf
[commands]: https://htmlpreview.github.io/?https://raw.githubusercontent.com/nmmmnu/HM4/master/commands.html
[HyperLogLog]: https://en.wikipedia.org/wiki/HyperLogLog
