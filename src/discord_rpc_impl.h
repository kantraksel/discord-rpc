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

	void Initialize(const char* applicationId, const DiscordEventHandlers* handlers) override;
	void Shutdown() override;

	void RunCallbacks() override;
	void UpdateHandlers(const DiscordEventHandlers* handlers) override;

	void UpdatePresence(const DiscordRichPresence* presence) override;
	void ClearPresence() override;
	void Respond(const char* userId, DiscordReply reply) override;

	void UpdateConnection();
};
