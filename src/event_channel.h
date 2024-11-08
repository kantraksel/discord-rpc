#pragma once
#include "discord_rpc.hpp"
#include "msg_queue.h"
#include "events.h"

class RpcConnection;
class CmdChannel;
class JsonDocument;

class EventChannel
{
	RpcConnection& connection;
	CmdChannel& sendChannel;
	std::mutex mutex;
	CDiscordEventHandlers handlers;

	ConnectEvent onConnect;
	DisconnectEvent onDisconnect;
	ErrorEvent onError;
	JoinGameEvent onJoinGame;
	SpectateGameEvent onSpectateGame;
	MsgQueue<User, 8> joinAskQueue;

public:
	EventChannel(RpcConnection& connection, CmdChannel& sendChannel);

	void OnConnect(JsonDocument& readyMessage);
	void OnDisconnect(int err, const std::string_view& message);
	void ReceiveData();

	void SetHandlers(const CDiscordEventHandlers& newHandlers);
	void InitHandlers();
	void UpdateHandlers(const CDiscordEventHandlers& newHandlers);

	void RunCallbacks();
};
