#include "connection.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

int GetProcessId()
{
    return ::getpid();
}

struct ConnectionUnix : public BaseConnection
{
    int sock{-1};

    bool Open() override;
    bool Close() override;

    bool Write(const void* data, size_t length) override;
    bool Read(void* data, size_t length) override;

    ~ConnectionUnix() {}
};

static ConnectionUnix Connection;
static sockaddr_un PipeAddr{};
#ifdef MSG_NOSIGNAL
static int MsgFlags = MSG_NOSIGNAL;
#else
static int MsgFlags = 0;
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

BaseConnection* BaseConnection::Create()
{
    PipeAddr.sun_family = AF_UNIX;
    return &Connection;
}

void BaseConnection::Destroy(BaseConnection*& c)
{
    c->Close();
    c = nullptr;
}

bool ConnectionUnix::Open()
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

    for (int pipeNum = 0; pipeNum < 10; ++pipeNum)
    {
        snprintf(PipeAddr.sun_path, sizeof(PipeAddr.sun_path), "%s/discord-ipc-%d", tempPath, pipeNum);
        int err = connect(sock, (const sockaddr*)&PipeAddr, sizeof(PipeAddr));
        if (err == 0)
        {
            isOpen = true;
            return true;
        }
    }
    Close();
    return false;
}

bool ConnectionUnix::Close()
{
    if (sock == -1)
        return false;

    close(sock);
    sock = -1;
    isOpen = false;
    return true;
}

bool ConnectionUnix::Write(const void* data, size_t length)
{
    if (sock == -1)
        return false;

    ssize_t sentBytes = send(sock, data, length, MsgFlags);
    if (sentBytes < 0)
        Close();

    return sentBytes == (ssize_t)length;
}

bool ConnectionUnix::Read(void* data, size_t length)
{
    if (sock == -1)
        return false;

    int res = (int)recv(sock, data, length, MsgFlags);
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
