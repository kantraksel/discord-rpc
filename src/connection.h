#pragma once

#include <cstdlib>

// not really connectiony, but need per-platform
int GetProcessId();

struct BaseConnection
{
	static BaseConnection* Create();
	static void Destroy(BaseConnection*&);

	bool isOpen{false};
	virtual bool Open() = 0;
	virtual bool Close() = 0;

	virtual bool Write(const void* data, size_t length) = 0;
	virtual bool Read(void* data, size_t length) = 0;

protected:
	virtual ~BaseConnection() {}
};
