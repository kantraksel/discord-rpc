#pragma once

#ifndef DISCORD_DISABLE_IO_THREAD
	#include <atomic>
	#include <condition_variable>
	#include <mutex>
	#include <thread>
#endif

class IoThread
{
private:
#ifndef DISCORD_DISABLE_IO_THREAD
	std::mutex waitForIOMutex;
	std::condition_variable waitForIOActivity;
	std::jthread thread;
#endif

public:
	~IoThread();

	void Start();
	void Notify();
	void Stop();
};
