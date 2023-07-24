#include "io_thread.h"

#ifndef DISCORD_DISABLE_IO_THREAD
void Discord_UpdateConnection(void);

IoThread::~IoThread()
{
	Stop();
}

void IoThread::Start()
{
	thread = std::jthread([&](std::stop_token token)
	{
		constexpr std::chrono::duration<int64_t, std::milli> maxWait{500LL};
		std::unique_lock<std::mutex> lock(waitForIOMutex);
		do
		{
			Discord_UpdateConnection();
			waitForIOActivity.wait_for(lock, maxWait);
		} while (!token.stop_requested());
	});
}

void IoThread::Notify()
{
	waitForIOActivity.notify_one();
}

void IoThread::Stop()
{
	if (!thread.joinable())
		return;

	thread.request_stop();
	Notify();

	// may thrown even if we check joinable()
	try
	{
		thread.join();
	}
	catch (...)
	{}
}
#else
IoThread::~IoThread()
{
}

void IoThread::Start()
{
}

void IoThread::Notify()
{
}

void IoThread::Stop()
{
}
#endif
