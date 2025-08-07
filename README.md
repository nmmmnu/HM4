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
-   Bloom Filters
-   Counting Bloom Filters
-   Count Min Sketch
-   Heavy Hitters
-   Misra Gries Heavy Hitters
-   Vectors


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

Memtable is stored in memory in SkipList or AVLList.



---
### Memtable with SkipList

SkipList is very fast O(Log N) structure, with performance very similar to binary tree.

It is much faster than a vector O(Amortized Log N), but slower than hashtable O(Amortized 1).



---
### Memtable with AVLList

AVLList is based on non-recursive AVL Tree with storing just the balance and parent pointer.
It is extremely fast O(Log N) structure, because it is perfectly balanced.

AVL Tree can be slow when data is deleted, but in our case we have kind of free lunch,
because instead of deleting the data, we insert tombstones.

Minor problem of AVLList is it requires about 5% memory compared to SkipList.
However our implementation put the key/value pair directly into AVLTree node and
memory difference was changed down to about 2%.

AVLList is much faster than SkipList. Performance tests using db_builder_concurrent
show AVLList can be up to 40% faster than SkipList.



---
### Memtable with VectorList

VectorList performance was very good, but worse than SkipList.

However if you just want to load data, is much faster to use SkipList and sort just before store it on the disk.

VectorList also have much low memory consumption, so you can fit 30-40% more data in same memory.



---
### HugeTLB support

If you are running Linux, you can automatically use huge memory pages for memtable.

All you need to do is to enable it.

Suppose you need 4 GB for the memtable. This means system needs to allocate 2 x 4 GB = 8 GB.

Since standard Linux huge page is 2 MB, you will need 8192 MB / 2 MB = 4096 pages.

All you need to do is following:

	sudo sysctl vm.nr_hugepages=4096

Also you will need to make sure this settings is kept after reboot.

Following command will show HugeTLB information:

	grep -i huge /proc/meminfo

If HugeTLB support is turned off or there is no enough pages, system will use conventional memory.



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

LinkedList performance was poor. There are no benefits of using it, except testing SkipList algorithm strategies.

UnrolledLinkList was made for completeness. It performance is better than LinkedList, but worse than VectorList.
There are no benefits of using it, except testing UnrolledSkipList algorithm strategies.

UnrolledSkipList have lower memory consumption than SkipList.
Theoretically it have less cache misses than SkipList, but it seems to be slower than SkipList.

Because of C++ classes all VectorList, LinkedList, SkipList, UnrolledLinkedList, UnrolledSkipList are available.



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
- db_logger	- create bin-log from tab delimited file - similar to **db_builder**, but not very usable - **REMOVED IN 1.3.4.6**
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

HyperLogLog (HLL) is a probabilistic algorithm for counting unique elements, in a constant space.

HLL store just elements "fingerprints", so is GDPR friendly :)

[HyperLogLog] at Wikipedia.

#### Implementation

The implementation uses 12bits. This means each key is 4096 bytes and the error rate is 1.62%.

There are no fancy encoding, these 4096 bytes are uint8_t counters from the standard HLL.

#### How it works:

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



---
### Bloom filters

Bloom filter (BF) is a probabilistic algorithm for checking if unique elements are members of a set, in a constant space.

- If the element is not member of the set, BF will return "definitely not a member".
- If the element is a member of the set, BF will return "maybe a member". This is so called false positive.

BF store just elements "fingerprints", so is GDPR friendly :)

[Bloom_filter] at Wikipedia.

#### Configuration of a BF

- **number of bits**
- **number of the hash functions**

These are derived constants from the configuration options:

- **elements until BF is saturated** - derived from **number of bits** and **number of the hash functions** - If a BF is saturated, it will almost always return "maybe a member".
- **error rate** - derived only from **count of the hash functions**

These can be calculated using of of many "Bloom Filter Calculators".

Some nice "round" values can be seen in [bloom_filter_calc].

#### Implementation:

The implementation is not Redis compatible.

There are no fancy encoding, the implementation uses standard bitfield, same as in BITSET / BITGET.

#### How it works:

- Value can be added using **BFADD**. If the key does not exists or contains different information, it is overwritten.

- Membershib can be check using **BFEXISTS**.

      redis> bfadd a 2097152 7 john
      OK
      redis> bfadd a 2097152 7 steven
      OK
      redis> bfexists a 2097152 7 john
      (integer) 1
      redis> bfexists a 2097152 7 steven
      (integer) 1
      redis> bfexists a 2097152 7 peter
      (integer) 0
      redis> bfmexists a 2097152 7 john steven peter
      1) "1"
      2) "1"
      3) "0"

The numbers 2097152 and 7 are the BF configuration options:

- 2097152 is count of bits.
- 7 is count of the hash functions.

You need to provide same config values on each BF command.



---
### Counting Bloom filters

Counting Bloom filter (CBF) is a probabilistic algorithm for checking if unique elements are members of a set, in a constant space.
It also supports removing elements.

- If the element is not member of the set, CBF will return "definitely not a member" and count of zero.
- If the element is a member of the set, CBF will return "maybe a member" and count of non zero. This is so called false positive.

CBF store just elements "fingerprints", so is GDPR friendly :)

[Counting_Bloom_filter] at Wikipedia.

#### Configuration of a CBF

- **number of counters**
- **number of the hash functions**
- **counter type**

#### Counter types

| Bits          | C/C++ name | Memory per counter | Max value                  |
|          ---: | :---       |               ---: |                       ---: |
|             4 | n/a        |           1/2 byte |                         16 |
|             8 | uint8_t    |             1 byte |                        255 |
|            16 | uint16_t   |            2 bytes |                     65,535 |
|            32 | uint32_t   |            4 bytes |              4,294,967,295 |
|            64 | uint64_t   |            8 bytes | 18,446,744,073,709,551,615 |

When counter increases to the max value, it does not reset to zero, instead it stay at max values.

4 bit counters are under development.

#### Implementation:

There are no fancy encoding, the implementation uses array of integer counters.

#### How it works:

- Value can be added using **CBFADD**. If the key does not exists or contains different information, it is overwritten.
- Value can be removed using **CBFREM**.
- Membershib can be check using **CBFCOUNT**.

      redis> cbfadd a 2097152 7 8 john 1
      OK
      redis> cbfadd a 2097152 7 8 steven 1
      OK
      redis> cbfadd a 2097152 7 8 bob 1
      OK
      redis> cbfmcount a 2097152 7 8 john steven bob peter
      1) "1"
      2) "1"
      3) "1"
      4) "0"
      redis> cbfrem a 2097152 7 8 bob 1
      OK
      redis> cbfmcount a 2097152 7 8 john steven bob peter
      1) "1"
      2) "1"
      3) "0"
      4) "0"
      redis> cbfaddcount a 2097152 7 8 john 1
      (integer) 2

The numbers 2097152 and 7 and 8 are the CBF configuration options:

- 2097152 is number of counters.
- 7 is number of the hash functions.
- 8 is bits per counter.

You need to provide same config values on each CBF command.



---
### Count Min Sketch

Count min sketch (CMS) is a probabilistic algorithm for counting several unique elements that are members of a set, in a constant space.

- If the element is not member of the set, CMS will return 0.
- If the element is a member of the set, CMS will return a number. The number can be overestimated.

CMS store just elements "fingerprints", so is GDPR friendly :)

[Count–min sketch] at Wikipedia.

#### Configuration of a CMS

- **width**
- **height** - e.g.count of the hash functions
- **counter type**

These are derived constants from the configuration options:

- **elements until CMS is saturated** - derived from **width** and **height** - If a CMS is saturated, it will almost always return some number, even for elements with zero count.
- **error rate** - derived only from **height**.

#### Counter types

| Bits          | C/C++ name | Memory per counter | Max value                  |
|          ---: | :---       |               ---: |                       ---: |
|             4 | n/a        |           1/2 byte |                         16 |
|             8 | uint8_t    |             1 byte |                        255 |
|            16 | uint16_t   |            2 bytes |                     65,535 |
|            32 | uint32_t   |            4 bytes |              4,294,967,295 |
|            64 | uint64_t   |            8 bytes | 18,446,744,073,709,551,615 |

When counter increases to the max value, it does not reset to zero, instead it stay at max values.

4 bit counters are under development.

#### Calculation of CMS configuration options

Calculation of the **width** is unclear. The formula we found useful is as following:

```
// Math notation:
width = |number_of_elements * (e / 10)|

// C / Java notation:
unsigned width = (unsigned) (2.71828 * 0.1 * (double) number_of_elements)
```

Calculation of the **height**:

Number 5 is always a good value here. Seriously :)

#### Implementation:

The implementation is not Redis compatible.

There are no fancy encoding, the implementation uses array of integer counters.

#### How it works:

- Value can be added using **CMSADD**. If the key does not exists or contains different information, it is overwritten.

- Membershib can be check using **CMSCOUNT**.

      redis> cmsadd a 2048 7 8 john 1
      OK
      redis> cmsadd a 2048 7 8 steven 2
      OK
      redis> cmsadd a 2048 7 8 john 1 steven 2
      OK
      redis> cmscount a 2048 7 8 john
      "2"
      redis> cmscount a 2048 7 8 steven
      "4"
      redis> cmscount a 2048 7 8 peter
      "0"
      redis> cmsmcount a 2048 7 8 john steven peter
      1) "2"
      2) "4"
      3) "0"

Notice how "steven" was increased first with 2, then with 2 more.

The numbers 2048, 7 and 8 are the CMS configuration options:

- 2048 is width.
- 7 is height.
- 8 is counter type (1 byte, 0 - 255)

You need to provide same config values on each CMS command.

#### Counters does not overflow

      redis> cmsadd a 2097152 7 8 max 100
      OK
      redis> cmscount a 2097152 7 8 max
      "100"
      redis> cmsadd a 2097152 7 8 max 100
      OK
      redis> cmscount a 2097152 7 8 max
      "200"
      redis> cmsadd a 2097152 7 8 max 100
      OK
      redis> cmscount a 2097152 7 8 max
      "255"

Notice how "max" was increased three times, with 100 each.

However, because **8 bit** counter can not hold value more than 255, final result instead of 300, is 255.



---
### Heavy Hitters (HH)

Heavy Hitter can be used to find most frequent elements in a stream. For example what are top 50 IP addresses that visited a website.

Instead of using a heap, it is implemented as flat array. This was made, because if you adding new value to specific item, you need to scan the whole heap anyway.

#### Configuration of a HH

- **hh_count** - count of the heavy hitters items. For the example with the IP addresses, if we want to know the top 25 addresses, it has to be set to 25.
- **hh_value_size** - size of fixed lenght string. See table. For the example with the IP addresses, 40 is best, which will give you size of 39 characters. IP6 is exactly 39 characters.

#### HH Value Size

| Config | Max size | Comment       |
|   ---: |     ---: | :---          |
|     16 |       15 |               |
|     32 |       31 |               |
|     40 |       39 | IP6           |
|     64 |       63 |               |
|    128 |      127 |               |
|    256 |      255 | Pascal string |

#### Example usage

      127.0.0.1:6379> incr ip:192.168.0.1
      (integer) 1
      127.0.0.1:6379> hhincr visits 25 40 192.168.0.1 1
      (integer) 1
      127.0.0.1:6379> incr ip:192.168.0.1
      (integer) 2
      127.0.0.1:6379> hhincr visits 25 40 192.168.0.1 2
      (integer) 1
      127.0.0.1:6379> incr ip:192.168.0.5
      (integer) 1
      127.0.0.1:6379> hhincr visits 25 40 192.168.0.15 1
      (integer) 1
      127.0.0.1:6379> incr ip:192.168.0.5
      (integer) 2
      127.0.0.1:6379> hhincr visits 25 40 192.168.0.15 2
      (integer) 1
      127.0.0.1:6379> incr ip:192.168.0.5
      (integer) 3
      127.0.0.1:6379> hhincr visits 25 40 192.168.0.15 3
      (integer) 1
      127.0.0.1:6379> hhget visits 25 40
      1) "192.168.0.15"
      2) "3"
      3) "192.168.0.1"
      4) "2"

- Instead of <i>INCR</i> you can use count min sketch or some sensor reading.
- Note how the result is **not** sorted.
- If you need to only top heavy hitter without value, you can probably use <i>INCRTO</i> command.



---
### Misra Gries Heavy Hitters (MG)

Misra Gries Heavy Hitters or Misra Gries Summary can be used to find most frequent elements in a stream. For example what are top 50 IP addresses that visited a website.

Unlike normal Heavy Hitters (HH), they does not requre the count of the counted elements, however they does not give you correct count, because the count decay over time.
However they can find abnormalities in the stream.

They are implemented as flat array in a way similar to Heavy Hitters (HH).

#### Configuration of a MG

- **mg_count** - count of the Misra Gries heavy hitters items. For the example with the IP addresses, if we want to know the top 25 addresses, it has to be set to 25.
- **mg_value_size** - size of fixed lenght string. See table. For the example with the IP addresses, 40 is best, which will give you size of 39 characters. IP6 is exactly 39 characters.

#### HH Value Size

| Config | Max size | Comment       |
|   ---: |     ---: | :---          |
|     16 |       15 |               |
|     32 |       31 |               |
|     40 |       39 | IP6           |
|     64 |       63 |               |
|    128 |      127 |               |
|    256 |      255 | Pascal string |

#### Example usage

      127.0.0.1:6379> mgadd visits 25 40 192.168.0.1
      (integer) 1
      127.0.0.1:6379> mgadd visits 25 40 192.168.0.1
      (integer) 1
      127.0.0.1:6379> mgadd visits 25 40 192.168.0.15
      (integer) 1
      127.0.0.1:6379> mgadd visits 25 40 192.168.0.15
      (integer) 1
      127.0.0.1:6379> mgadd visits 25 40 192.168.0.15
      (integer) 1
      127.0.0.1:6379> mgget visits 25 40
      1) "192.168.0.15"
      2) "3"
      3) "192.168.0.1"
      4) "2"

- Note because we add just 2 elements out of 25, the counts never decayed and counters are still correct.
- Note how the result is **not** sorted.



---
### Vectors

HM4 can also be used as a **vector database**.

Unlike most vector DB implementations, HM4 stores data **on disk**. This makes the commonly used **HNSW algorithm** impractical, as it requires fast random memory access. Instead, HM4 uses two alternative algorithms:

- **Flat Search**
- **Locality-Sensitive Hashing (LSH)**

#### Modes of Operation

HM4 supports two main modes for vector similarity search:

- **With LSH indexing**:
  Use `VADD`, `VSIMFLAT`, and `VSIMLSH`

- **Without LSH (key-only storage)**:
  Use `VKADD` and `VKSIMFLAT`. In this mode, LSH indexing is **not available**

> Some vector features are currently in **beta**.



#### Building a Vector Index

To create a vector index, use the `VADD` command.

##### Example 1 – With Random Projection (Dimensionality Reduction)

`VADD words 300 150 F b BLOB0 frog BLOB1 cat`

This command adds the keys `"frog"` and `"cat"` into a vector index named `"words"`.

-   **300** – original vector dimensionality

-   **150** – reduced dimensionality using **Random Projection**

-   `F` – vector elements are stored as **floats**

-   `b` – vectors are passed as **binary blobs** (little-endian)

The resulting index stores **150D float vectors**.

##### Example 2 – Without Random Projection (Full Dimensionality) and quantization to int8

`VADD words 300 300 I h BLOB0 frog BLOB1 cat`

This version keeps the vectors at their full dimensionality:

-   **300** – both input and stored vector dimensionality

-   `I` – vector elements are **quantized to int8 (1 byte)**

-   `h` – vectors are passed as **hex blobs** (little-endian)

The resulting index stores **300D int8 quantized vectors**.

##### Example 3 – With Random Projection (Dimensionality Reduction) and quantization to int8

`VADD words 300 64 I h BLOB0 frog BLOB1 cat`

The resulting index stores **64D int8 quantized vectors**.



#### Searching a Vector Index

To search a vector index, use the `VSIMFLAT` command.

##### Example 1

`VSIMFLAT words 300 150 F C b BLOB 100`

This command performs a similarity search on the "words" index using flat method (e.g. brute force)

The result is 100% accurate, but might be slow.

- **300** – dimensionality of the input query vector
- **150** – dimensionality of the index (must match how the index was built)
- **F** – vector elements are floats (must match how the index was built)
- **C** – use Cosine similarity
- **b** – the query vector is passed as a binary blob (little-endian)
- **BLOB** – the binary data representing the query vector
**100** – return the 100 nearest results

The query vector is projected from 300D to 150D using the same random projection as during indexing, and similarity is computed using the Cosine distance.

##### Example 2

`VSIMLSH words 300 300 I E b BLOB 100`

This command performs a similarity search on the "words" index using LSH method.

The result is NOT 100% accurate, but is fast.

- **300** – dimensionality of the input query vector
- **300** – dimensionality of the index (must match how the index was built)
- **F** – vector elements are floats (must match how the index was built)
- **E** – use Euclidean (L2) distance
- **b** – the query vector is passed as a binary blob (little-endian)
- **BLOB** – the binary data representing the query vector
**100** – return the 100 nearest results

The query vector is not projected, and similarity is computed using the Euclidean (L2) distance.

#### Distance Metrics in Vector Search

When performing vector similarity search in HM4 using commands like `VSIMFLAT`, you can choose from several **distance (or similarity) metrics** by specifying a corresponding letter flag.

##### Supported Metrics

| Flag | Metric Name      | Description                                                                                   |
|------|------------------|-----------------------------------------------------------------------------------------------|
| `E`  | **Euclidean (L2)**  | Standard straight-line distance between two points in Euclidean space.                        |
| `M`  | **Manhattan (L1)**  | Sum of absolute differences across dimensions. Also known as "taxicab" or "city-block" distance. |
| `C`  | **Cosine**          | Measures the cosine of the angle between two vectors (orientation, not magnitude). The result is transformed to be 0..1 |
| `K`  | **Canberra**        | A weighted version of L1 where each component is normalized by its sum. Useful when components vary greatly in scale. |

##### When to Use Each Metric

- **Euclidean (L2)** `E`
  Best for dense, normalized data where overall magnitude matters. It’s the default in many ML applications.

- **Manhattan (L1)** `M`
  More robust to outliers; works well when vector components are sparse or vary in only a few dimensions.

- **Cosine Similarity** `C`
  Focuses on **direction**, not magnitude. Ideal for text embeddings or any case where you care about **angular similarity** rather than size.

- **Canberra** `K`
  Useful for sparse data or when small differences in low-value components are important. More sensitive than L1/L2 in those regions.



#### Additional Vector Commands

HM4 provides several commands for managing and inspecting vectors in an index.

##### Delete a Vector from Index

`VREM words cat`
Removes the vector associated with the key "cat" from the "words" index.

##### Get Vector in Human-Readable Format

`VGET words 150 i cat`

Retrieves the vector for key "cat" from the index "words".

- 150 – dimensionality of the vector
- i – elements are stored as int8 (quantized)

Output: a list of numbers representing each element of the vector

##### Get Normalized Vector with Magnitude

`VGETNORMALIZED words 150 i cat`

Retrieves and normalizes the vector for key "cat" from the index "words".

Returns:

Output: magnitude and list of numbers representing each element of the vector

##### Get Vector in Binary or Hex Format

`VGETRAW words 150 i h cat`

`VGETRAW words 150 i b cat`

Retrieves the raw representation of the vector for "cat":

h – output is in hexadecimal format (little-endian)

b – output is in hexadecimal format (little-endian)



#### Key-Based Vectors (No LSH Index)

In HM4, you can store and retrieve individual vectors directly by key without building an LSH index.

This mode uses the `VK*` command family:

- `VKSET` – store a vector under a key
- `VKGET`, `VKGETNORMALIZED`, `VKGETRAW` – retrieve the vector
- `VKSIMFLAT` – similarity search over keys with a common prefix

#### Store Vectors in keys

`VKSET word:frog 300 150 F b BLOB0`

`VKSET word:cat  300 150 F b BLOB1`

This stores vectors under keys:

- "word:frog"
- "word:cat"

Parameters:

- 300 – original dimensionality of the input vector
- 150 – dimensionality after Random Projection
- F – vector elements are float32
- b – vector is passed as a binary blob (little-endian)
- BLOB0, BLOB1 – the actual vector data (300D binary input)

Vectors are transformed from 300D to 150D before storage.



#### Searching a Vectors stored as keys

`VKSIMFLAT word: 300 160 F C b BLOB 100`

This command performs a similarity search over all keys starting with the prefix "word:", using flat method (e.g. brute force)

The result is 100% accurate, but might be slow.

- **300** – dimensionality of the input query vector
- **150** – dimensionality of the index (must match how the index was built)
- **F** – vector elements are floats (must match how the index was built)
- **C** – use Cosine similarity
- **b** – the query vector is passed as a binary blob (little-endian)
- **BLOB** – the binary data representing the query vector
**100** – return the 100 nearest results



#### Additional Vector Key Commands

##### Get Vector from Key in Human-Readable Format

`VKGET word:cat 150 F`

Retrieves the vector from key for key "word:cat".

- 150 – dimensionality of the vector
- F – elements are stored as float

Output: a list of numbers representing each element of the vector

##### Get Normalized Key Vector with Magnitude

`VGETNORMALIZED words:cat 150 i cat`

Retrieves and normalizes the vector from key  "word:cat".

Returns:

Output: magnitude and list of numbers representing each element of the vector

##### Get Key Vector in Binary or Hex Format

`VGETRAW words 150 i h cat`

`VGETRAW words 150 i b cat`

Retrieves the raw representation of the vector from key  "word:cat".

h – output is in hexadecimal format (little-endian)

b – output is in hexadecimal format (little-endian)





[Log structured merge tree]: http://en.wikipedia.org/wiki/Log-structured_merge-tree
[Differential files: their application to the maintenance of large databases - University of Minnesota, Minneapolis]: http://www-users.cs.umn.edu/~he/diff/p256-severance.pdf
[commands]: https://htmlpreview.github.io/?https://raw.githubusercontent.com/nmmmnu/HM4/master/commands.html
[HyperLogLog]: https://en.wikipedia.org/wiki/HyperLogLog
[Bloom_filter]: https://en.wikipedia.org/wiki/Bloom_filter
[Counting_Bloom_filter]: https://en.wikipedia.org/wiki/Bloom_filter
[bloom_filter_calc]: https://github.com/nmmmnu/HM4/blob/master/bloom_filter.md
[Count–min sketch]: https://en.wikipedia.org/wiki/Count%E2%80%93min_sketch
