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

// Derived from raspbootcom (https://github.com/mrvn/raspbootin)

#define _BSD_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>

#include "crc32.h"
#include "raspbootin.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/socket.h>
#include <sys/un.h>
#include <termios.h>
#ifndef UNIX_SOCKETS
#define UNIX_SOCKETS
#endif
#endif // _WIN32

#define READ_BUF_LEN                0x1000

#define SERVER_CAPABILITIES         0x1f

#ifdef _WIN32
typedef HANDLE port_addr;
#else
typedef int port_addr;
#define INVALID_HANDLE_VALUE        -1
#endif

static uint32_t magic = MAGIC;
static uint32_t server_caps = SERVER_CAPABILITIES;

static int do_cmd(int cmd_id, port_addr fd);
static int send_error_msg(port_addr fd, int error_code);

#ifdef UNIX_SOCKETS
static struct sockaddr *sock_addr;
static socklen_t addrlen;
#endif

static inline ssize_t serial_write(port_addr fd, const void *buf, size_t count)
{
#ifdef _WIN32
    ssize_t ret = 0;
    int wf_ret = WriteFile(fd, buf, (DWORD)count, (LPDWORD)&ret, NULL);
    if(wf_ret == 0)
    {
        fprintf(stderr, "WriteFile() failed: %08x\n", GetLastError());
        return 0;
    }
    return ret;
#else
    return write(fd, buf, count);
#endif
}

static inline ssize_t serial_read(port_addr fd, void *buf, size_t count)
{
#ifdef _WIN32
    ssize_t ret = 0;
    int rf_ret = ReadFile(fd, buf, (DWORD)count, (LPDWORD)&ret, NULL);
    if(rf_ret == 0)
    {
        fprintf(stderr, "ReadFile() failed: %08x\n", GetLastError());
        return 0;
    }
    return ret;
#else
    return read(fd, buf, count);
#endif
}

#ifdef UNIX_SOCKETS
static port_addr wait_connection(port_addr fd)
{
    // Check fd type
    struct stat buf;
    if(fstat(fd, &buf) < 0)
    {
        fprintf(stderr, "Error calling fstat(), errno = %i\n", errno);
        return -1;
    }

    if(S_ISSOCK(buf.st_mode))
    {
        // Put in listening mode
        int ret = listen(fd, 1);
        if(ret == -1)
        {
            fprintf(stderr, "Error calling listen(), errno = %i\n", errno);
            return -1;
        }

        // Accept the next connection
        ret = accept(fd, sock_addr, &addrlen);
        if(ret == -1)
        {
            fprintf(stderr, "Error calling accept(), errno = %i\n", errno);
            return -1;
        }

        return fd;
    }
    else
        return fd;
}
#else
#define wait_connection(a) (a)
#endif

static port_addr open_serial(const char *dev)
{
    // Decide on the type of the file
#ifdef UNIX_SOCKETS
    struct stat stat_buf;
    int stat_ret = stat(dev, &stat_buf);

    int is_socket = 0;

    if((stat_ret == -1) && (errno == ENOENT))
        is_socket = 1;
    else if((stat_ret == 0) && S_ISSOCK(stat_buf.st_mode))
        is_socket = 1;

    if(is_socket)
    {
        // Open a unix domain socket
        unlink(dev);
        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if(fd == -1)
        {
            fprintf(stderr, "Error creating unix domain socket, errno = %i\n",
                    errno);
            return -1;
        }

        // Prepare the address structure
        struct sockaddr_un *address;
        address = (struct sockaddr_un *)malloc(sizeof(struct sockaddr_un));
        sock_addr = (struct sockaddr *)address;

        memset(address, 0, sizeof(struct sockaddr_un));
        address->sun_family = AF_UNIX;
        strncpy(address->sun_path, dev, sizeof(address->sun_path));

        addrlen = sizeof(struct sockaddr_un);

         // Bind to the socket
        int bind_ret = bind(fd, sock_addr, sizeof(struct sockaddr_un));
        if(bind_ret == -1)
        {
            fprintf(stderr, "Error binding to unix domain socket, errno = %i\n",
                    errno);
            return -1;
        }

        // Wait for a new connection
        return wait_connection(fd);
    }
    else
#endif
    {
#ifdef _WIN32
        port_addr fd = CreateFile(dev, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_FLAG_NO_BUFFERING, NULL);
        if(fd == INVALID_HANDLE_VALUE)
        {
            fprintf(stderr, "Error opening %s, error: %08x\n", dev, GetLastError());
            return fd;
        }

        DCB config;
        if(GetCommState(fd, &config) == 0)
        {
            fprintf(stderr, "Error calling GetCommState, error %08x\n", GetLastError());
            return INVALID_HANDLE_VALUE;
        }

        // Set the port settings
        config.BaudRate = 115200;
        config.ByteSize = 8;
        config.Parity = NOPARITY;
        config.StopBits = ONESTOPBIT;

        if(SetCommState(fd, &config) == 0)
        {
            fprintf(stderr, "Error calling SetCommState, error %08x\n", GetLastError());
            return INVALID_HANDLE_VALUE;
        }

        // Set the timeouts
        COMMTIMEOUTS timeouts;
        timeouts.ReadIntervalTimeout = 1;
        timeouts.ReadTotalTimeoutMultiplier = 1;
        timeouts.ReadTotalTimeoutConstant = 1;
        timeouts.WriteTotalTimeoutMultiplier = 1;
        timeouts.WriteTotalTimeoutConstant = 1;
        if(SetCommTimeouts(fd, &timeouts) == 0)
        {
            fprintf(stderr, "Error calling SetCommTimeouts, error %08x\n",
                    GetLastError());
            return INVALID_HANDLE_VALUE;
        }

        return fd;
#else
        // The termios structure, to be configured for serial interface
        struct termios termios;

        // Open the device, read only
        int fd = open(dev, O_RDWR | O_NOCTTY);
        if(fd == -1)
        {
            // failed to open
            return -1;
        }

        // Must be a tty to continue set-up, otherwise assume pipe
        if(!isatty(fd))
            return fd;

        // Get the attributes
        if(tcgetattr(fd, &termios) == -1)
        {
            fprintf(stderr, "Failed to get attributes of device %s\n", dev);
            return -1;
        }

        // Poll only
        termios.c_cc[VTIME] = 0;
        termios.c_cc[VMIN] = 0;

        // 8N1 mode
        termios.c_iflag = 0;
        termios.c_oflag = 0;
        termios.c_cflag = CS8 | CREAD | CLOCAL;
        termios.c_lflag = 0;

        // Set 115200 baud
        if((cfsetispeed(&termios, B115200) < 0) ||
        (cfsetospeed(&termios, B115200) < 0))
        {
            fprintf(stderr, "Failed to set baud rate\n");
            return -1;
        }

        // Write the attributes
        if(tcsetattr(fd, TCSAFLUSH, &termios) == -1)
        {
            fprintf(stderr, "Failed to write attributes\n");
            return -1;
        }

        return fd;
#endif
    }
}

int main(int argc, char *argv[])
{
    port_addr fd = INVALID_HANDLE_VALUE;

    // Open the serial device
    while(fd == INVALID_HANDLE_VALUE)
    {
        fd = open_serial(argv[1]);

#ifndef _WIN32
        if((fd == INVALID_HANDLE_VALUE) && ((errno == ENOENT) || (errno == ENODEV)))
        {
            fprintf(stderr, "Waiting for %s\n", argv[1]);
            sleep(1);
            continue;
        }
        else
#endif
        if(fd == INVALID_HANDLE_VALUE)
        {
            fprintf(stderr, "Error opening %s\n", argv[1]);
            return -1;
        }
    }

    uint8_t buf;
    int breaks_read = 0;

    for(;;)
    {
        int bytes_read = serial_read(fd, &buf, 1);

        if(bytes_read > 0)
        {
            if(breaks_read == 2)
            {
                int cmd = (int)buf;
                do_cmd(cmd, fd);
                breaks_read = 0;

                // Pause to let the client read the message
                usleep(10000);
            }
            else if(buf == '\003')
                breaks_read++;
            else
            {
                breaks_read = 0;
                putchar(buf);
            }
        }
    }

    return 0;
}

static int do_cmd(int cmd, port_addr fd)
{
    switch(cmd)
    {
        case 0:
        {
            // Identify command

            // Read the command crc
            uint32_t ccrc;
            serial_read(fd, &ccrc, 4);
            uint32_t eccrc = crc32((void *)0, 0);
            if(ccrc != eccrc)
                return send_error_msg(fd, CRC_ERROR);

            // Set the response length and error code
            uint32_t resp_length = 12;
            uint32_t error_code = SUCCESS;

            // Build the response crc
            uint32_t crc = crc32_start();
            crc = crc32_append(crc, &magic, 4);
            crc = crc32_append(crc, &resp_length, 4);
            crc = crc32_append(crc, &error_code, 4);
            crc = crc32_append(crc, &server_caps, 4);
            crc = crc32_finish(crc);

            // Send the response
            serial_write(fd, &magic, 4);
            serial_write(fd, &resp_length, 4);
            serial_write(fd, &error_code, 4);
            serial_write(fd, &server_caps, 4);
            serial_write(fd, &crc, 4);

            fprintf(stderr, "Sent CMD0 response\n");

            return 0;
        }
    }
    (void)fd;
    return 0;
}

static int send_error_msg(port_addr fd, int error_code)
{
    // Send a fail response - <magic><resp_length = 8><error_code><crc>

    uint32_t resp_length = 8;
    uint32_t crc = crc32_start();
    crc = crc32_append(crc, &magic, 4);
    crc = crc32_append(crc, &resp_length, 4);
    crc = crc32_append(crc, &error_code, 4);
    crc = crc32_finish(crc);

    serial_write(fd, &magic, 4);
    serial_write(fd, &resp_length, 4);
    serial_write(fd, &error_code, 4);
    serial_write(fd, &crc, 4);

    fprintf(stderr, "Sent ERROR %i response\n", error_code);

    return 0;
}
