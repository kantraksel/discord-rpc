#include "discord_rpc_impl.h"

extern "C" DISCORD_EXPORT DiscordRpc* CreateDiscordRpc()
{
	return new DiscordRpcImpl();
}

DiscordRpcImpl::DiscordRpcImpl() : sendChannel(connection), receiveChannel(connection, sendChannel), backoff(500, 60 * 1000)
{

}

void DiscordRpcImpl::Initialize(const char* applicationId, const DiscordEventHandlers* handlers)
{
	using namespace std::placeholders;

	receiveChannel.SetHandlers(handlers);
	connection.Initialize(applicationId, std::bind(&DiscordRpcImpl::OnConnect, this, _1), std::bind(&DiscordRpcImpl::OnDisconnect, this, _1, _2));
	thread.Start();
}

void DiscordRpcImpl::Shutdown()
{
	thread.Stop();
	connection.Close();

	receiveChannel.SetHandlers(nullptr);
	sendChannel.Reset();
}

void DiscordRpcImpl::RunCallbacks()
{
	receiveChannel.RunCallbacks();
}

void DiscordRpcImpl::UpdateHandlers(const DiscordEventHandlers* handlers)
{
	receiveChannel.UpdateHandlers(handlers);
	thread.Notify();
}

void DiscordRpcImpl::UpdatePresence(const DiscordRichPresence* presence)
{
	sendChannel.UpdatePresence(presence);
	thread.Notify();
}

void DiscordRpcImpl::ClearPresence()
{
	UpdatePresence(nullptr);
}

void DiscordRpcImpl::Respond(const char* userId, DiscordReply reply)
{
	// if we are not connected, let's not batch up stale messages for later
	if (!connection.IsOpen())
		return;

	if (sendChannel.ReplyJoinRequest(userId, (int)reply))
		thread.Notify();
}

void DiscordRpcImpl::UpdateConnection()
{
	if (connection.IsOpen())
	{
		receiveChannel.ReceiveData();
		sendChannel.SendData();
	}
	else if (backoff.tryConsume())
		connection.Open();
}

void DiscordRpcImpl::OnConnect(JsonDocument& readyMessage)
{
	receiveChannel.InitHandlers();
	receiveChannel.OnConnect(readyMessage);

	backoff.reset();
}

void DiscordRpcImpl::OnDisconnect(int err, const char* message)
{
	receiveChannel.OnDisconnect(err, message);
	backoff.setNewDelay();
}
