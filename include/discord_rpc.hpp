#pragma once
#include "discord_rpc_shared.h"

class DiscordRpc
{
public:
	virtual ~DiscordRpc() = 0;

	virtual void Initialize(const char* applicationId, const DiscordEventHandlers* handlers) = 0;
	virtual void Shutdown() = 0;

	virtual void RunCallbacks() = 0;
	virtual void UpdateHandlers(const DiscordEventHandlers* handlers) = 0;

	virtual void UpdatePresence(const DiscordRichPresence* presence) = 0;
	virtual void ClearPresence() = 0;
	virtual void Respond(const char* userId, DiscordReply reply) = 0;

#ifdef DISCORD_DISABLE_IO_THREAD
	virtual void UpdateConnection() = 0;
#endif
};

extern "C" DISCORD_EXPORT DiscordRpc* CreateDiscordRpc();
