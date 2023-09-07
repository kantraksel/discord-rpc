#include <cstdio>
#include <cstring>

#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "connection.h"

int GetProcessId()
{
    return ::getpid();
}

// original code leftover
#ifndef MSG_NOSIGNAL
#   define MSG_NOSIGNAL 0
#endif

static const char* GetTempPath()
{
    const char* temp = getenv("XDG_RUNTIME_DIR");
    temp = temp ? temp : getenv("TMPDIR");
    temp = temp ? temp : getenv("TMP");
    temp = temp ? temp : getenv("TEMP");
    temp = temp ? temp : "/tmp";
    return temp;
}

BaseConnection::BaseConnection()
{
    sock = -1;
}

BaseConnection::~BaseConnection()
{
    Close();
}

bool BaseConnection::Open()
{
    const char* tempPath = GetTempPath();
    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1)
        return false;

    fcntl(sock, F_SETFL, O_NONBLOCK);
#ifdef SO_NOSIGPIPE
    int optval = 1;
    setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &optval, sizeof(optval));
#endif

    sockaddr_un addr{AF_UNIX, ""};
    for (int pipeNum = 0; pipeNum < 10; ++pipeNum)
    {
        snprintf(addr.sun_path, sizeof(addr.sun_path), "%s/discord-ipc-%d", tempPath, pipeNum);
        int err = connect(sock, (const sockaddr*)&addr, sizeof(addr));
        if (err == 0)
        {
            isOpen = true;
            return true;
        }
    }
    Close();
    return false;
}

void BaseConnection::Close()
{
    if (sock != -1)
    {
        close(sock);
        sock = -1;
    }

    isOpen = false;
}

bool BaseConnection::Write(const void* data, size_t length)
{
    if (sock == -1)
        return false;

    ssize_t sentBytes = send(sock, data, length, MSG_NOSIGNAL);
    if (sentBytes < 0)
        Close();

    return sentBytes == (ssize_t)length;
}

bool BaseConnection::Read(void* data, size_t length)
{
    if (sock == -1)
        return false;

    int res = (int)recv(sock, data, length, MSG_NOSIGNAL);
    if (res < 0)
    {
        if (errno == EAGAIN)
            return false;

        Close();
    }
    else if (res == 0)
        Close();

    return res == (int)length;
}
