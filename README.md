HM3 - key/value NoSQL database that utilize LSM trees
=====================================================

---
### Why another key/value?

We worked a lot with Apache Cassandra, but we never needed its distributed functionality.
We also worked with Redis, but problem there is data must fit in memory.

Goals of the project are.

-   Key/Value store
-   Redis / Memcached protocol
-   Async network I/O
-   Data not need to fit in memory
-   Speed
-   Consistent
-   High quality code

---
### Architecture

Architecture is derived from Apache Cassandra. There is memtable and several disk files.

Writes will be done in Memtable.
Reading will be done first in memtable and if data will not be found there, system will try read each disktable until find the key.
This is called "LSM tree" or much simpler - "Differential files".

Writes should be always fast. Reads should be fast if there are not too many disktables.

---
### Memtable

Memtable is stored in memory in skiplist. Skiplist is very fast O(Log N) structure, very similar to binary tree.

It is much faster than a vector O(Amortized Log N), but slower than hashtable O(Amortized 1).

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

Hashtable is not avaliable because tests were made long ago in C version.

---
### DiskTables

Periodicaly, the memtable is flushed on the disk in disktable.
Disktable file have similar order - vector/array like structure with sorted elements.

Disktables are immutable. This means very good OS cache and easy backup.
Downside is that deletes must be implemented by markers called tombstones (same key, NULL value).

Disktables header and control data are 64bit integers (uint64_t) stored as BigEndian.
Simple 1 byte XOR check checksum is included for each pair.

Disktables ment to be load using MMAP().
Effors are made things to works on 32bit OS-es, but soon or later these will run out of address space.
This means that for production you will definitely need 64bit OS.

---
### LSM tree notes.

Here is what Wikipedia say about:

[Log structured merge tree][]



However there is 1976's research by Dennis G. Severance and Guy M. Lohman on the similar topic here:

[Differential files: their application to the maintenance of large databases - University of Minnesota, Minneapolis][]

When database works, many disktables will be created.
This will slow down the reads very much.
By this reason disktables must be merged.

Because disktables are read only, merge can be implemented as separate user space process.
For read only databases you may choose not to start it at all.

Unlike Apache Cassandra, there are a way to compact several tables into single file.
However this will complicate the things and probably only basic two table merge will be implemented.



[Log structured merge tree]: http://en.wikipedia.org/wiki/Log-structured_merge-tree
[Differential files: their application to the maintenance of large databases - University of Minnesota, Minneapolis]: http://www-users.cs.umn.edu/~he/diff/p256-severance.pdf
