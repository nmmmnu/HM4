HM4 - command reference
=====================================================

---
### List of commands

-   GET
-   HGETALL
-   SET
-   SETEX
-   INCR / INCRBY
-   DEL
-   INFO
-   SAVE / BGSAVE
-   EXIT
-   SHUTDOWN

---
### GET

``get [key]``

Gets [key] from the server. Exact match.

> This command is 100% Redis compatible.
> Blobs as values are supported, if your client support them.

---
### HGETALL

``hgetall [key] [number = 10]``

Gets [number] keys after [key] from the server.

The [number] is clamp between 10 and 1000.

> This command is not very useful and exists for iterating all keys in the database.
> Blobs as values are supported, if your client support them.

``hgetall [key] [number] [prefix]``

Gets [number] keys after [key] from the server, but returns only keys that matching the prefix.

The [number] is clamp between 10 and 1000.

> This command is useful.
> Blobs as values are supported, if your client support them.

__Example__:

Suppose we have keys like this:

- u:123:email -> office@domain.com
- u:123:country -> US
- u:123:city -> Boston

You can get this user's information records with following command:

``hgetall u:123: 1000 u:123:``

---
### COUNT

``count [key] [number = 10]``

Counts [number] keys after [key] from the server.

The [number] is clamp between 10 and 1000.

> This command is not useful and exists for compatibility with HGETALL command.
> This command is available after 1.2.4


``count [key] [number] [prefix]``

Counts [number] keys after [key] from the server, but counts only keys that matching the prefix.

The [number] is clamp between 10 and 1000.

> This command is useful.
> This command is available after 1.2.4

__Example__:

Suppose we have keys like this:

- u:123:email -> office@domain.com
- u:123:country -> US
- u:123:city -> Boston

You can count this user's information records with following command:

``count u:123: 1000 u:123:``

---
### SET

``set [key] [value]``

Set [key] -> [value] on the server.

> This command is 100% Redis compatible.
> Blobs as values are supported, if your client support them.

---
### SETEX

``setex [key] [seconds] [value]``

Set [key] -> [value] with expiration of [seconds] seconds on the server.

> This command is 100% Redis compatible.
> Blobs as values are supported, if your client support them.
> This command helps HM4 to be used as session handler for PHP servers.

---
### INCR / INCRBY

``incr [key] [increase value = 1]``

``incrby [key] [increase value = 1]``

Increase numerical value of the [key] with  [increase value]. It uses __int64_t__ as a number type.

> This command is 100% Redis compatible, assuming you are using it in correct Redis way.

---
### DEL

``del [key]``

Removes [key] from the server.

> This command is 100% Redis compatible.

---
### INFO

``info``

Returns server information.

---
### SAVE / BGSAVE

``save [complete = 0]``

``bgsave [complete = 0]``

On __mutable servers__ - flushes memtable on the disk and reloads the disktables.

If [complete] is present, it __suppose__ to bypass memtable flush, but current implementation is not 100% clear and might change in near future.

Save is always foreground, e.g. you have to wait, but usually is very fast.

On __immutable servers__ - reloads the disktables.

[complete] options is ignored.


> This command is 100% Redis compatible, assuming you do not use [complete] option.

---
### EXIT

``exit``

Disconnect from the server.

> This command is 100% Redis compatible

---
### SHUTDOWN

``shutdown``

Shutdown the server.

> This command is 100% Redis compatible
