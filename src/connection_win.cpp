#include "connection.h"

#include <string>
#define WIN32_LEAN_AND_MEAN
#define NOMCX
#define NOSERVICE
#define NOIME
#include <windows.h>

int GetProcessId()
{
	return (int)GetCurrentProcessId();
}

struct ConnectionWin : public BaseConnection
{
	HANDLE pipe{INVALID_HANDLE_VALUE};

	bool Open() override;
	bool Close() override;

	bool Write(const void* data, size_t length) override;
	bool Read(void* data, size_t length) override;

	~ConnectionWin() {}
};

static ConnectionWin Connection;

BaseConnection* BaseConnection::Create()
{
	return &Connection;
}

void BaseConnection::Destroy(BaseConnection*& c)
{
	c->Close();
	c = nullptr;
}

bool ConnectionWin::Open()
{
	wchar_t pipeName[]{L"\\\\?\\pipe\\discord-ipc-0"};
	wchar_t* pipeDigit = pipeName + (sizeof(pipeName) / sizeof(wchar_t)) - 2;

	for (char d = '0'; d <= '9';)
	{
		*pipeDigit = d;
		pipe = CreateFileW(pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (pipe != INVALID_HANDLE_VALUE)
		{
			isOpen = true;
			return true;
		}

		auto lastError = GetLastError();
		if (lastError == ERROR_FILE_NOT_FOUND)
		{
			d++;
			continue;
		}
		else if (lastError == ERROR_PIPE_BUSY)
			if (WaitNamedPipeW(pipeName, 10000))
				continue;

		return false;
	}

	return false;
}

bool ConnectionWin::Close()
{
	CloseHandle(pipe);
	pipe = INVALID_HANDLE_VALUE;
	isOpen = false;
	return true;
}

bool ConnectionWin::Write(const void* data, size_t length)
{
	if (length == 0)
		return true;

	if (pipe == INVALID_HANDLE_VALUE || !data)
		return false;

	DWORD bytesLength = (DWORD)length;
	DWORD bytesWritten = 0;
	return WriteFile(pipe, data, bytesLength, &bytesWritten, nullptr) &&
		bytesWritten == bytesLength;
}

bool ConnectionWin::Read(void* data, size_t length)
{
	if (pipe == INVALID_HANDLE_VALUE || !data)
		return false;

	DWORD bytesAvailable = 0;
	if (PeekNamedPipe(pipe, nullptr, 0, nullptr, &bytesAvailable, nullptr))
	{
		if (bytesAvailable >= length)
		{
			DWORD bytesToRead = (DWORD)length;
			DWORD bytesRead = 0;
			if (ReadFile(pipe, data, bytesToRead, &bytesRead, nullptr))
				return bytesToRead == bytesRead;
			else Close();
		}
	}
	else Close();
	return false;
}
