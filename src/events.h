#include <atomic>
#include <mutex>
#include <string>

struct User
{
	// snowflake (64bit int), ascii decimal string (max 20 bytes)
	char userId[21];
	// 32 unicode glyphs is max name size => 4 bytes per glyph in the worst case
	char username[129];
	// 4 decimal digits + 1 null terminator
	char discriminator[5];
	// optional 'a_' + md5 hex digest (32 bytes) + null terminator
	char avatar[35];
};

class ConnectEvent
{
	std::atomic_bool awaiting{false};
	std::mutex mutex;
	User user{};

public:
	inline void Set(const User& user)
	{
		std::lock_guard lock(mutex);
		this->user = user;
		awaiting = true;
	}

	inline bool Consume()
	{
		return awaiting.exchange(false);
	}

	inline User GetUser()
	{
		std::lock_guard lock(mutex);
		return user;
	}
};

class DisconnectEvent
{
	std::atomic_bool awaiting{false};
	std::mutex mutex;
	int code{0};
	std::string message;

public:
	inline void Set(int code, const char* message)
	{
		std::lock_guard lock(mutex);
		this->code = code;
		this->message = message;

		awaiting = true;
	}

	inline bool Consume()
	{
		return awaiting.exchange(false);
	}

	inline std::pair<int, std::string> GetArgs()
	{
		std::lock_guard lock(mutex);
		return std::make_pair(code, message);
	}
};

typedef DisconnectEvent ErrorEvent;

class JoinGameEvent
{
	std::atomic_bool awaiting{false};
	std::mutex mutex;
	std::string secret;

public:
	inline void Set(const char* secret)
	{
		std::lock_guard lock(mutex);
		this->secret = secret;
		awaiting = true;
	}

	inline bool Consume()
	{
		return awaiting.exchange(false);
	}

	inline std::string GetSecret()
	{
		std::lock_guard lock(mutex);
		return secret;
	}
};

typedef JoinGameEvent SpectateGameEvent;
