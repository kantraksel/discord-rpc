#pragma once
#include "discord_rpc_shared.h"

#ifdef __cplusplus
extern "C" {
#endif

DISCORD_EXPORT void Discord_Initialize(const char* applicationId, const DiscordEventHandlers* handlers);
DISCORD_EXPORT void Discord_Shutdown(void);

DISCORD_EXPORT void Discord_RunCallbacks(void);
DISCORD_EXPORT void Discord_UpdateHandlers(const DiscordEventHandlers* handlers);

DISCORD_EXPORT void Discord_UpdatePresence(const DiscordRichPresence* presence);
DISCORD_EXPORT void Discord_ClearPresence(void);
DISCORD_EXPORT void Discord_Respond(const char* userId, enum DiscordReply reply);

#ifdef DISCORD_DISABLE_IO_THREAD
DISCORD_EXPORT void Discord_UpdateConnection(void);
#endif

#ifdef __cplusplus
} /* extern "C" */
#endif
