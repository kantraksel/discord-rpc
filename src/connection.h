#pragma once
#include <cstdlib>

// not really connectiony, but need per-platform
int GetProcessId();

struct BaseConnection
{
	union
	{
		void* pipe; //Win
		int sock; //Unix
	};

	BaseConnection();
	~BaseConnection();

	bool isOpen{false};
	bool Open();
	void Close();

	bool Write(const void* data, size_t length);
	bool Read(void* data, size_t length);
};
