#include <atomic>
#include <mutex>

struct Buffer
{
	size_t length{};
	char buffer[16 * 1024]{};

	Buffer()
	{

	}

	Buffer(const Buffer& other)
	{
		length = other.length;
		memcpy(buffer, other.buffer, length);
	}

	Buffer& operator =(const Buffer& other)
	{
		length = other.length;
		memcpy(buffer, other.buffer, length);
		
		return *this;
	}
};

class PresenceEvent
{
	std::atomic_bool awaiting{false};
	std::mutex mutex;
	Buffer data;

public:
	inline void Set(const Buffer& data)
	{
		std::lock_guard lock(mutex);
		this->data = data;
		awaiting = true;
	}

	inline bool Consume()
	{
		return awaiting.exchange(false);
	}

	inline Buffer GetBuffer()
	{
		std::lock_guard lock(mutex);
		return data;
	}

	inline void Reset()
	{
		data.length = 0;
		Consume();
	}
};
