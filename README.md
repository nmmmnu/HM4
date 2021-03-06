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
- db_builder	- create DiskTable(s) from tab delimited file
- db_logger	- create bin-log from tab delimited file - similar to **db_builder**, but not very usable
- db_replay	- create DiskTable(s) from binlog
- db_mkbtree	- create btree to existing DiskTable for much faster access
- db_file	- reads DiskTable or LSM
- db_merge	- merge DiskTable(s)
- db_preload	- preload DiskTable into system cache, by reading 1024 samples from the DiskTable




[Log structured merge tree]: http://en.wikipedia.org/wiki/Log-structured_merge-tree
[Differential files: their application to the maintenance of large databases - University of Minnesota, Minneapolis]: http://www-users.cs.umn.edu/~he/diff/p256-severance.pdf
[commands]: https://htmlpreview.github.io/?https://raw.githubusercontent.com/nmmmnu/HM4/master/commands.html
