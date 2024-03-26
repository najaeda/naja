Objects identification
----------------------

Each Design DB object has a unique identifier:
`SNLID <https://github.com/xtofalex/naja/blob/main/src/snl/snl/kernel/SNLID.h>`_.

+-------------+-----------+--------------+-------------------+
| Field       | Type      | Size (bytes) | Value range       |
+=============+===========+==============+===================+
| Object type | uint8_t   | 1            | 0-255             |
+-------------+-----------+--------------+-------------------+
| DB          | uint8_t   | 1            | 0-255             |
+-------------+-----------+--------------+-------------------+
| Library     | uint16_t  | 2            | 0 - 65535         |
+-------------+-----------+--------------+-------------------+
| Design      | uint32_t  | 4            | 0 - 4,294,967,295 |
+-------------+-----------+--------------+-------------------+
| Instance    | uint32_t  | 4            | 0 - 4,294,967,295 |
+-------------+-----------+--------------+-------------------+
| Net object  | uint32_t  | 4            | 0 - 4,294,967,295 |
+-------------+-----------+--------------+-------------------+
| Bit         | int32_t   | 4            | 0 - 4,294,967,295 |
+-------------+-----------+--------------+-------------------+

Each object SNLID can be accessed with :func:`getSNLID` method.

**SNLIDs** allow to:

- compare and sort objects.
- reference uniquely objects.
- access objects from SNLUniverse.