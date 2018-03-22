/*
 * Syncex - For cross platform exchanger syncronization your data on multi-platform PC
 *
 * Support WinXP/Vista/7, Mac OS X Leopard/Snow Leopard, Ubunto
 * Author : Evan Chang
 *
 * Reserved Copyrightc Evan Chang
 */

#pragma once
#ifndef SYNCACTION_H
#define SYNCACTION_H

#define DEFAULT_SERVER_PORT 8402
#define DEFAULT_SCAN_PERIOD 60000
#define DEFAULT_SCAN_IN_MIN 30

/*
Maintain file system list:
Name - A file name list relative to specify syncronization root
       , not a file system root.
Md5 - A hash code of each file.
last ModifyTime - The last modify time of each file.
Status - The status of each file, which includes
         add,
         remove,
         move,
         copy
first 8 bytes data - The first 8 bytes data of each file
                     , see also "last 8 bytes data"
last 8 bytes data - The last 8 Bytes data of each file, these two lists
                    are not included in sending sockets, it works only
                    for local maintenance.


Syncex socket syntax:

SyncSocket()
{
    data type           name                    size(Bytes)

    unsigned int32      ui32StartCode           4
    unsigned int64      ui32AtomSocketSize      8
    unsigned int8       ui8MainCommand          1
    unsigned int8       ui8SubCommand           1
                        SyncSocketData()
    unsigned int32      ui32EndCode             4
}

ui32StartCode           Start code with 0x00000100.

ui32AtomSocketSize      This represents to atomically socket size which is include startcode and endcode.

ui8MainCommand          GET         1   Get from remote.
                        POST        2   Post to remote.
                        REPLY       3   Reply remote request.
                        OPERATION   4   Remote operations.

ui8SubCommand           MD5_LIST                1    A list for each block data format is File##MD5##LastModifyDate.
                        SUB_FILE                2    A file include path.
                        SYNC_SUB_FILE_ADD       3
                        SYNC_SUB_FILE_REMOVE    4
                        SYNC_SUB_FILE_RENAME    5
                        SYNC_SUB_FILE_MODIFY    6

ui32EndCode             End code with 0x00000001.



SyncSocketData()
{
    data type           name                    size(Bytes)

    unsigned int32      ui32DataSize              4
    data_field
}

*/

namespace SYNCACTION
{
    enum SyncSyntax_ // 4 BYTES
    {
        SYNC_STARTCODE  = 0x00000100L,
        SYNC_ENDCODE    = 0x00000001L
    };

    enum SyncMainAction_ // 1 BYTE
    {
        SYNC_GET        = 1,
        SYNC_POST,
        SYNC_REPLY,
        SYNC_OPERATION,
    };

    enum SyncSubAction_ // 1 BYTE
    {
        // GET, POST or REPLY
        SYNC_SUB_MD5_LIST    = 1,
        SYNC_SUB_FILE,
        // OPERATION
        SYNC_SUB_FILE_ADD,
        SYNC_SUB_FILE_REMOVE,
        SYNC_SUB_FILE_RENAME,
        SYNC_SUB_FILE_MODIFY
    };
};

#endif // SYNCACTION_H
