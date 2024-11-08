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

static CDiscordEventHandlers CreateHandlers(const DiscordEventHandlers& handlers)
{
	CDiscordEventHandlers wrapper;
	wrapper.ready = [handlers](const CDiscordUser& user)
		{
			DiscordUser u{
				user.userId.data(),
				user.username.data(),
				user.discriminator.data(),
				user.avatar.data(),
			};
			handlers.ready(&u);
		};
	wrapper.disconnected = [handlers](int errorCode, const std::string_view& message)
		{
			handlers.disconnected(errorCode, message.data());
		};
	wrapper.errored = [handlers](int errorCode, const std::string_view& message)
		{
			handlers.errored(errorCode, message.data());
		};
	wrapper.joinGame = [handlers](const std::string_view& secret)
		{
			handlers.joinGame(secret.data());
		};
	wrapper.spectateGame = [handlers](const std::string_view& secret)
		{
			handlers.spectateGame(secret.data());
		};
	wrapper.joinRequest = [handlers](const CDiscordUser& user)
		{
			DiscordUser u{
				user.userId.data(),
				user.username.data(),
				user.discriminator.data(),
				user.avatar.data(),
			};
			handlers.joinRequest(&u);
		};
	return wrapper;
}

static CDiscordEventHandlers WrapHandlers(const DiscordEventHandlers* handlers)
{
	if (!handlers)
		return {};
	return CreateHandlers(*handlers);
}

extern "C" DISCORD_EXPORT void Discord_Initialize(const char* applicationId, const DiscordEventHandlers* handlers)
{
	if (!applicationId)
		return;
	cinstance.Initialize(applicationId, WrapHandlers(handlers));
}

extern "C" DISCORD_EXPORT void Discord_Shutdown(void)
{
	cinstance.Shutdown();
}

extern "C" DISCORD_EXPORT void Discord_UpdatePresence(const DiscordRichPresence* presence)
{
	if (presence)
		cinstance.UpdatePresence(*presence);
	else
		cinstance.ClearPresence();
}

extern "C" DISCORD_EXPORT void Discord_ClearPresence(void)
{
	cinstance.ClearPresence();
}

extern "C" DISCORD_EXPORT void Discord_Respond(const char* userId, enum DiscordReply reply)
{
	if (!userId)
		return;
	cinstance.Respond(userId, reply);
}

extern "C" DISCORD_EXPORT void Discord_RunCallbacks(void)
{
	cinstance.RunCallbacks();
}

extern "C" DISCORD_EXPORT void Discord_UpdateHandlers(const DiscordEventHandlers* handlers)
{
	cinstance.UpdateHandlers(WrapHandlers(handlers));
}
