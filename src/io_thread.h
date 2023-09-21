#pragma once

#ifndef DISCORD_DISABLE_IO_THREAD
	#include <condition_variable>
	#include <mutex>
	#include <thread>
#endif

#include <functional>

class IoThread
{
private:
	using UpdateFunc = std::function<void()>;

#ifndef DISCORD_DISABLE_IO_THREAD
	std::mutex waitForIOMutex;
	std::condition_variable waitForIOActivity;
	std::jthread thread;
	UpdateFunc callback;
#endif

public:
	~IoThread();

	void Start(UpdateFunc update);
	void Notify();
	void Stop();
};
