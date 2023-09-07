#include "msg_queue.h"
#include "presence.h"

class RpcConnection;
struct DiscordRichPresence;

class DataChannel
{
	RpcConnection& connection;

	PresenceEvent presenceUpdate;
	MsgQueue<Buffer, 8> sendQueue;

	Buffer presenceBuff;
	int nonce{1};
	int pid;

public:
	DataChannel(RpcConnection& connection);

	void Reset();
	void SendData();

	bool SubscribeEvent(const char* evtName);
	bool UnsubscribeEvent(const char* evtName);
	bool ReplyJoinRequest(const char* userId, int reply);
	void UpdatePresence(const DiscordRichPresence* presence);
};
