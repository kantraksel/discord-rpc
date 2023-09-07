#include "discord_rpc_shared.h"
#include "msg_queue.h"
#include "events.h"

constexpr size_t JoinQueueSize{8};

class RpcConnection;
class DataChannel;
class JsonDocument;

class EventChannel
{
	RpcConnection& connection;
	DataChannel& sendChannel;
	std::mutex mutex;
	DiscordEventHandlers handlers{};

	ConnectEvent onConnect;
	DisconnectEvent onDisconnect;
	ErrorEvent onError;
	JoinGameEvent onJoinGame;
	SpectateGameEvent onSpectateGame;
	MsgQueue<User, JoinQueueSize> joinAskQueue;

public:
	EventChannel(RpcConnection& connection, DataChannel& sendChannel);

	void OnConnect(JsonDocument& readyMessage);
	void OnDisconnect(int err, const char* message);
	void ReceiveData();

	void SetHandlers(const DiscordEventHandlers* newHandlers);
	void InitHandlers();
	void UpdateHandlers(const DiscordEventHandlers* newHandlers);

	void RunCallbacks();
};
