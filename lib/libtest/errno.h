//
//  errno.h
//  libtest
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef _ERRNO_H_
#define _ERRNO_H_

#include "syscall.h"
#include "tls.h"

#define errno *(__tls_errno())

#define E2BIG             1 // Argument list too long.
#define EACCES            2 // Permission denied.
#define EADDRINUSE        3 // Address in use.
#define EADDRNOTAVAIL     4 // Address not available.
#define EAFNOSUPPORT      5 // Address family not supported.
#define EAGAIN            6 // Resource unavailable, try again (may be the same value as [EWOULDBLOCK]).
#define EALREADY          7 // Connection already in progress.
#define EBADF             8 // Bad file descriptor.
#define EBADMSG           9 // Bad message.
#define EBUSY            10 // Device or resource busy.
#define ECANCELED        11 // Operation canceled.
#define ECHILD           12 // No child processes.
#define ECONNABORTED     13 // Connection aborted.
#define ECONNREFUSED     14 // Connection refused.
#define ECONNRESET       15 // Connection reset.
#define EDEADLK          16 // Resource deadlock would occur.
#define EDESTADDRREQ     17 // Destination address required.
#define EDOM             18 // Mathematics argument out of domain of function.
#define EDQUOT           19 // Reserved.
#define EEXIST           20 // File exists.
#define EFAULT           21 // Bad address.
#define EFBIG            22 // File too large.
#define EHOSTUNREACH     23 // Host is unreachable.
#define EIDRM            24 // Identifier removed.
#define EILSEQ           25 // Illegal byte sequence.
#define EINPROGRESS      26 // Operation in progress.
#define EINTR            27 // Interrupted function.
#define EINVAL           28 // Invalid argument.
#define EIO              29 // I/O error.
#define EISCONN          30 // Socket is connected.
#define EISDIR           31 // Is a directory.
#define ELOOP            32 // Too many levels of symbolic links.
#define EMFILE           33 // File descriptor value too large.
#define EMLINK           34 // Too many links.
#define EMSGSIZE         35 // Message too large.
#define EMULTIHOP        36 // Reserved.
#define ENAMETOOLONG     37 // Filename too long.
#define ENETDOWN         38 // Network is down.
#define ENETRESET        39 // Connection aborted by network.
#define ENETUNREACH      40 // Network unreachable.
#define ENFILE           41 // Too many files open in system.
#define ENOBUFS          42 // No buffer space available.
#define ENODEV           43 // No such device.
#define ENOENT           44 // No such file or directory.
#define ENOEXEC          45 // Executable file format error.
#define ENOLCK           46 // No locks available.
#define ENOLINK          47 // Reserved.
#define ENOMEM           48 // Not enough space.
#define ENOMSG           49 // No message of the desired type.
#define ENOPROTOOPT      50 // Protocol not available.
#define ENOSPC           51 // No space left on device.
#define ENOSYS           52 // Function not supported.
#define ENOTCONN         53 // The socket is not connected.
#define ENOTDIR          54 // Not a directory.
#define ENOTEMPTY        55 // Directory not empty.
#define ENOTRECOVERABLE  56 // State not recoverable.
#define ENOTSOCK         57 // Not a socket.
#define ENOTSUP          58 // Not supported (may be the same value as [EOPNOTSUPP]).
#define ENOTTY           59 // Inappropriate I/O control operation.
#define ENXIO            60 // No such device or address.
#define EOPNOTSUPP  ENOTSUP // Operation not supported on socket (may be the same value as [ENOTSUP]).
#define EOVERFLOW        61 // Value too large to be stored in data type.
#define EOWNERDEAD       62 // Previous owner died.
#define EPERM            63 // Operation not permitted.
#define EPIPE            64 // Broken pipe.
#define EPROTO           65 // Protocol error.
#define EPROTONOSUPPORT  66 // Protocol not supported.
#define EPROTOTYPE       67 // Protocol wrong type for socket.
#define ERANGE           68 // Result too large.
#define EROFS            69 // Read-only file system.
#define ESPIPE           70 // Invalid seek.
#define ESRCH            71 // No such process.
#define ESTALE           72 // Reserved.
#define ETIMEDOUT        73 // Connection timed out.
#define ETXTBSY          74 // Text file busy.
#define EWOULDBLOCK  EAGAIN // Operation would block (may be the same value as [EAGAIN]).
#define EXDEV            75 // Cross-device link.

#endif
