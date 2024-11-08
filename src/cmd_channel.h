#pragma once
#include <string_view>
#include "msg_queue.h"
#include "presence.h"

class RpcConnection;
struct CDiscordRichPresence;

class CmdChannel
{
	RpcConnection& connection;

	PresenceEvent presenceUpdate;
	MsgQueue<Buffer, 8> sendQueue;

	Buffer presenceBuff;
	int nonce{1};
	int pid;

public:
	CmdChannel(RpcConnection& connection);

	void Reset();
	void SendData();

	bool SubscribeEvent(const char* evtName);
	bool UnsubscribeEvent(const char* evtName);
	bool ReplyJoinRequest(const std::string_view& userId, int reply);
	void UpdatePresence(const CDiscordRichPresence* presence);
};
