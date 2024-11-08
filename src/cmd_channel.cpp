#include "cmd_channel.h"
#include "rpc_connection.h"
#include "serialization.h"

CmdChannel::CmdChannel(RpcConnection& connection) : connection(connection)
{
	pid = GetProcessId();
}

void CmdChannel::Reset()
{
	presenceUpdate.Reset();
	presenceBuff.length = 0;

	while (sendQueue.HavePendingSends())
		sendQueue.CommitSend();
}

void CmdChannel::SendData()
{
	if (presenceUpdate.Consume())
	{
		auto local = presenceUpdate.GetBuffer();
		if (!connection.Write(local.buffer, local.length))
			presenceUpdate.Set(local);
	}

	while (sendQueue.HavePendingSends())
	{
		auto* qmessage = sendQueue.GetNextSendMessage();
		connection.Write(qmessage->buffer, qmessage->length);
		sendQueue.CommitSend();
	}
}

bool CmdChannel::SubscribeEvent(const char* evtName)
{
	auto* qmessage = sendQueue.GetNextAddMessage();
	if (qmessage)
	{
		qmessage->length = JsonWriteSubscribeCommand(qmessage->buffer, sizeof(qmessage->buffer), nonce++, evtName);
		sendQueue.CommitAdd();
		return true;
	}
	return false;
}

bool CmdChannel::UnsubscribeEvent(const char* evtName)
{
	auto* qmessage = sendQueue.GetNextAddMessage();
	if (qmessage)
	{
		qmessage->length = JsonWriteUnsubscribeCommand(qmessage->buffer, sizeof(qmessage->buffer), nonce++, evtName);
		sendQueue.CommitAdd();
		return true;
	}
	return false;
}

bool CmdChannel::ReplyJoinRequest(const std::string_view& userId, int reply)
{
	auto* qmessage = sendQueue.GetNextAddMessage();
	if (qmessage)
	{
		qmessage->length = JsonWriteJoinReply(qmessage->buffer, sizeof(qmessage->buffer), userId, reply, nonce++);
		sendQueue.CommitAdd();
		return true;
	}
	return false;
}

void CmdChannel::UpdatePresence(const CDiscordRichPresence* presence)
{
	presenceBuff.length = JsonWriteRichPresenceObj(presenceBuff.buffer, sizeof(presenceBuff.buffer), nonce++, pid, presence);
	presenceUpdate.Set(presenceBuff);
}
