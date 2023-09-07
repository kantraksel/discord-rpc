#pragma once

#include <algorithm>
#include <random>
#include <stdint.h>
#include <chrono>

struct Backoff
{
    int64_t minAmount;
    int64_t maxAmount;
    int64_t current;
    int fails;
	std::chrono::system_clock::time_point nextAttempt;

    std::mt19937_64 randGenerator;
    std::uniform_real_distribution<> randDistribution;
    inline double rand01() { return randDistribution(randGenerator); }

    Backoff(int64_t min, int64_t max)
      : minAmount(min)
      , maxAmount(max)
      , current(min)
      , fails(0)
      , randGenerator(std::random_device()())
    {
    }

    void reset()
    {
        fails = 0;
        current = minAmount;
    }

    int64_t nextDelay()
    {
        ++fails;
        int64_t delay = (int64_t)((double)current * 2.0 * rand01());
        current = std::min(current + delay, maxAmount);
        return current;
    }

	bool tryConsume()
	{
		auto now = std::chrono::system_clock::now();
		if (now >= nextAttempt)
		{
			nextAttempt = now + std::chrono::duration<int64_t, std::milli>{ nextDelay() };
			return true;
		}

		return false;
	}

	void setNewDelay()
	{
		nextAttempt = std::chrono::system_clock::now() + std::chrono::duration<int64_t, std::milli>{ nextDelay() };
	}
};
