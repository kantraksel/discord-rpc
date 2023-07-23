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

BaseConnection::BaseConnection()
{
	pipe = INVALID_HANDLE_VALUE;
}

BaseConnection::~BaseConnection()
{
	Close();
}

bool BaseConnection::Open()
{
	wchar_t pipeName[]{L"\\\\?\\pipe\\discord-ipc-0"};
	wchar_t* pipeDigit = pipeName + (sizeof(pipeName) / sizeof(wchar_t)) - 2;

	for (;;)
	{
		pipe = CreateFileW(pipeName, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		if (pipe != INVALID_HANDLE_VALUE)
		{
			isOpen = true;
			return true;
		}

		auto lastError = GetLastError();
		if (lastError == ERROR_FILE_NOT_FOUND)
		{
			if (*pipeDigit < '9')
			{
				(*pipeDigit)++;
				continue;
			}
		}
		else if (lastError == ERROR_PIPE_BUSY)
			if (WaitNamedPipeW(pipeName, 10000))
				continue;

		return false;
	}
}

void BaseConnection::Close()
{
	if (pipe != INVALID_HANDLE_VALUE)
	{
		CloseHandle(pipe);
		pipe = INVALID_HANDLE_VALUE;
	}

	isOpen = false;
}

bool BaseConnection::Write(const void* data, size_t length)
{
	if (length == 0)
		return true;

	if (pipe == INVALID_HANDLE_VALUE || !data)
		return false;

	DWORD bytesLength = (DWORD)length;
	DWORD bytesWritten = 0;
	if (WriteFile(pipe, data, bytesLength, &bytesWritten, nullptr))
		return bytesWritten == bytesLength;

	return false;
}

bool BaseConnection::Read(void* data, size_t length)
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
