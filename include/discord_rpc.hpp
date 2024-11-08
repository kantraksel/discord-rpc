#pragma once
#include <string_view>
#include <functional>
#include "discord_rpc_shared.h"

struct CDiscordRichPresence
{
	std::string_view state;   /* max 128 bytes */
	std::string_view details; /* max 128 bytes */
	int64_t startTimestamp;
	int64_t endTimestamp;
	std::string_view largeImageKey;  /* max 32 bytes */
	std::string_view largeImageText; /* max 128 bytes */
	std::string_view smallImageKey;  /* max 32 bytes */
	std::string_view smallImageText; /* max 128 bytes */
	std::string_view partyId;        /* max 128 bytes */
	int partySize;
	int partyMax;
	DiscordPartyPrivacy partyPrivacy;
	std::string_view matchSecret;    /* max 128 bytes */
	std::string_view joinSecret;     /* max 128 bytes */
	std::string_view spectateSecret; /* max 128 bytes */
	int8_t instance;

	CDiscordRichPresence()
	{
		startTimestamp = 0;
		endTimestamp = 0;
		partySize = 0;
		partyMax = 0;
		partyPrivacy = DISCORD_PARTY_PRIVATE;
		instance = 0;
	}

	CDiscordRichPresence(const DiscordRichPresence& presence)
	{
		if (presence.state)
			state = presence.state;
		if (presence.details)
			details = presence.details;
		startTimestamp = presence.startTimestamp;
		endTimestamp = presence.endTimestamp;
		if (presence.largeImageKey)
			largeImageKey = presence.largeImageKey;
		if (presence.largeImageText)
			largeImageText = presence.largeImageText;
		if (presence.smallImageKey)
			smallImageKey = presence.smallImageKey;
		if (presence.smallImageText)
			smallImageText = presence.smallImageText;
		if (presence.partyId)
			partyId = presence.partyId;
		partySize = presence.partySize;
		partyMax = presence.partyMax;
		partyPrivacy = (DiscordPartyPrivacy)presence.partyPrivacy;
		if (presence.matchSecret)
			matchSecret = presence.matchSecret;
		if (presence.joinSecret)
			joinSecret = presence.joinSecret;
		if (presence.spectateSecret)
			spectateSecret = presence.spectateSecret;
		instance = presence.instance;
	}
};

struct CDiscordUser
{
	std::string_view userId;
	std::string_view username;
	std::string_view discriminator;
	std::string_view avatar;
};

struct CDiscordEventHandlers
{
	std::function<void(const CDiscordUser& user)> ready;
	std::function<void(int errorCode, const std::string_view& message)> disconnected;
	std::function<void(int errorCode, const std::string_view& message)> errored;
	std::function<void(const std::string_view& secret)> joinGame;
	std::function<void(const std::string_view& secret)> spectateGame;
	std::function<void(const CDiscordUser& user)> joinRequest;
};

class DiscordRpc
{
public:
	virtual ~DiscordRpc() = 0;

	virtual void Initialize(const std::string_view& applicationId, const CDiscordEventHandlers& handlers) = 0;
	virtual void Shutdown() = 0;

	virtual void RunCallbacks() = 0;
	virtual void UpdateHandlers(const CDiscordEventHandlers& handlers) = 0;

	virtual void UpdatePresence(const CDiscordRichPresence& presence) = 0;
	virtual void ClearPresence() = 0;
	virtual void Respond(const std::string_view& userId, DiscordReply reply) = 0;

#ifdef DISCORD_DISABLE_IO_THREAD
	virtual void UpdateConnection() = 0;
#endif
};

extern "C" DISCORD_EXPORT DiscordRpc* CreateDiscordRpc();
