/* Copyright (C) 2013 by John Cronin <jncronin@tysos.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

 /* This is a file system based upon the raspbootin bootloader by mrvn:
            https://github.com/mrvn/raspbootin

    It is a protocol which supports transfer of a kernel over a serial link.

    It supports two versions of the protocol, the first defined as per the
    above link and a version two which is defined here.

    All commands are initiated by the client, and responses are returned
    by the server.

    Definitions:

    <byte>          A single byte
    <lsb16>         A 16-bit number transmitted in LSB format
    <lsb32>         A 32-bit number transmitted in LSB format
    <msb32>         A 32-bit number transmitted in MSB format
    <string>        A string transmitted in the format:
                        <lsb16 length><byte 0><byte 1>...<byte n>
                    Note not null-terminated and string is in UTF-8 format
    <magic>         The four bytes <byte 0x31><byte 0x41><byte 0x59><byte 0x27>
    <crc>           A CRC-32 checksum of the command or response transmitted as
                    a <msb32>.  If this fails the command should be re-sent.
                        The particular CRC polynomial is 0x04C11DB7 as per
                        IEEE 802.3
    <dir_entry>     A directory entry in the form:
                        <lsb32 byte_size><lsb32 user_id><lsb32 group_id>
                        <lsb32 properties><string name>
                        properties is a bit-mask of file/directory properties
                        as defined later.

    Commands:

    All commands take the form: <byte \003><byte \003><byte cmd_id><options>

    Compatibility:
        RBTIN_V1            Required for raspbootin v1
        RBTIN_V2_REQ        Required for raspbootin v2
        RBTIN_V2_SPEC       Optional for raspbootin v2 (support is defined
                                in the response to cmd_id 0)

    Error codes:
        0                   Success (returned in error response to cmd_id 1 if
                            there are no entries within the directory)
        1                   Path not found
        2                   EOF
        3                   CRC error in request

    File/directory properties:

    These are defined as per the st_mode field in the man page to lstat(2).
    In particular, S_IFDIR = 0x40000 is set if the entry is a directory.

    Command list:

    cmd_id  Compatibility   Function

    0       RBTIN_V2_REQ    ID and query functionality
                            <options> is <crc>
                            Returns <magic><lsb32 error_code><lsb32 functions>
                            <crc> if the server is a valid v2 server.
                            Functions is a bit-mask specifying support for each
                            function number (1 to support).  For example, if a
                            server supports functions 0-4 inclusive, this will
                            be 11111b = 0x1f.

    1       RBTIN_V2_SPEC   Read directory entries
                            <options> is <string dir_name><crc>
                            Returns the entries within the specified directory
                            Returns:
                                <magic><lsb32 error_code><lsb32 0><crc>
                                    - if no entries or error
                                <magic><lsb32 0><lsb32 entry_count><byte 0>
                                    <dir_entry 0><dir_entry 1>...<dir_entry n>
                                    <crc>
                                    - if success return a list of the entries
                                    - the <byte 0> defines the dir_entry
                                    version and can be changed in future
                                    versions.

    2       RBTIN_V2_SPEC   Read part of a file
                            <options> is <string file_name><lsb32 start>
                            <lsb32 length><crc>
                            Reads the part of a file starting at address start
                            and of length 'length'
                            Returns:
                                <magic><lsb32 error_code><lsb32 0><crc>
                                    - if error
                                <magic><lsb32 0><lsb32 bytes_read><byte 0>
                                <byte 1>...<byte n><crc>
                                    - if success

    3       RBTIN_V1        Send the default kernel (all of it)
                            <options> is empty
                            Returns:
                                <lsb32 length><byte 0><byte 1>...<byte n>

    4       RBTIN_V2_SPEC   Display a string on the server console
                            <options> is <string message><crc>
                            Returns:
                                <magic><lsb32 error_code><crc>
*/

/* Specifics of the rpi_boot implementation:

    During init, we try and communicate with the server by sending cmd_id 0.
    If a valid response is returned we cache the result and continue.
    If not, we still assume a server is present but that it is v1 (and
    cache supported functionality as 0x8 - only cmd_id 3 supported).

    During directory enumeration/file loading, if a v2 response is cached
    we use that to try and load the file via the v2 functions.

    If v1, and the root directory is enumerated, we return a single member:
        /raspbootin of type file and unknown length
    If v1, and a directory other than the root directory is enumerated, we
        return error code ENOENT
    If v1, and the file /raspbootin is requested, we send cmd_id 3 and save
        the appropriate section to the buffer (and discard the rest)
    If v1, and any other file is requested, we return error code ENOENT
*/

#include "fs.h"

int raspbootin_init(struct fs **fs)
{
    (void)fs;
    return 0;
}
