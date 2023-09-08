#include "discord_rpc.h"
#include "discord_rpc_impl.h"

static DiscordRpcImpl cinstance;

#ifdef DISCORD_DISABLE_IO_THREAD
extern "C" DISCORD_EXPORT void Discord_UpdateConnection(void)
#else
void Discord_UpdateConnection(void)
#endif
{
	cinstance.UpdateConnection();
}

extern "C" DISCORD_EXPORT void Discord_Initialize(const char* applicationId, const DiscordEventHandlers* handlers)
{
	cinstance.Initialize(applicationId, handlers);
}

extern "C" DISCORD_EXPORT void Discord_Shutdown(void)
{
	cinstance.Shutdown();
}

extern "C" DISCORD_EXPORT void Discord_UpdatePresence(const DiscordRichPresence* presence)
{
	cinstance.UpdatePresence(presence);
}

extern "C" DISCORD_EXPORT void Discord_ClearPresence(void)
{
	cinstance.ClearPresence();
}

extern "C" DISCORD_EXPORT void Discord_Respond(const char* userId, enum DiscordReply reply)
{
	cinstance.Respond(userId, reply);
}

extern "C" DISCORD_EXPORT void Discord_RunCallbacks(void)
{
	cinstance.RunCallbacks();
}

extern "C" DISCORD_EXPORT void Discord_UpdateHandlers(const DiscordEventHandlers* handlers)
{
	cinstance.UpdateHandlers(handlers);
}
