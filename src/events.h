#pragma once
#include <atomic>
#include <mutex>
#include "fixed_string.h"

struct User
{
	// snowflake (64bit int), ascii decimal string (max 20 bytes)
	FixedString<21> userId;
	// name size => 32 unicode glyphs => 4 bytes per glyph in the worst case
	FixedString<32 * 4 + 1> username;
	// 4 decimal digits + null terminator
	FixedString<5> discriminator;
	// optional 'a_' + md5 hex digest (32 bytes) + null terminator
	FixedString<35> avatar;
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
	FixedString<256> message;

public:
	inline void Set(int code, const std::string_view& message)
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

	inline std::pair<int, FixedString<256>> GetArgs()
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
	FixedString<256> secret;

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

	inline FixedString<256> GetSecret()
	{
		std::lock_guard lock(mutex);
		return secret;
	}
};

typedef JoinGameEvent SpectateGameEvent;
