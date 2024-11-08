#pragma once
#include "discord_rpc.hpp"
#include "rpc_connection.h"
#include "cmd_channel.h"
#include "event_channel.h"
#include "io_thread.h"
#include "backoff.h"

class DiscordRpcImpl : public DiscordRpc
{
	RpcConnection connection;
	CmdChannel sendChannel;
	EventChannel receiveChannel;
	IoThread thread;
	Backoff backoff;
	bool isInitialized;

	void OnConnect(JsonDocument& readyMessage);
	void OnDisconnect(int err, const std::string_view& message);

public:
	DiscordRpcImpl();
	~DiscordRpcImpl() override;

	void Initialize(const std::string_view& applicationId, const CDiscordEventHandlers& handlers) override;
	void Shutdown() override;

	void RunCallbacks() override;
	void UpdateHandlers(const CDiscordEventHandlers& handlers) override;

	void UpdatePresence(const CDiscordRichPresence& presence) override;
	void ClearPresence() override;
	void Respond(const std::string_view& userId, DiscordReply reply) override;

	void UpdateConnection();
};
