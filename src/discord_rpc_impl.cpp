#include "discord_rpc_impl.h"

extern "C" DISCORD_EXPORT DiscordRpc* CreateDiscordRpc()
{
	return new DiscordRpcImpl();
}

DiscordRpcImpl::DiscordRpcImpl() : sendChannel(connection), receiveChannel(connection, sendChannel), backoff(500, 60 * 1000)
{
	isInitialized = false;
}

void DiscordRpcImpl::Initialize(const char* applicationId, const DiscordEventHandlers* handlers)
{
	using namespace std::placeholders;

	if (isInitialized)
		return;
	isInitialized = true;

	receiveChannel.SetHandlers(handlers);
	connection.Initialize(applicationId, std::bind(&DiscordRpcImpl::OnConnect, this, _1), std::bind(&DiscordRpcImpl::OnDisconnect, this, _1, _2));
	thread.Start();
}

void DiscordRpcImpl::Shutdown()
{
	if (!isInitialized)
		return;
	isInitialized = false;

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
	if (!isInitialized)
		return;

	receiveChannel.UpdateHandlers(handlers);
	thread.Notify();
}

void DiscordRpcImpl::UpdatePresence(const DiscordRichPresence* presence)
{
	// no isInitialized check, old C API behaviour
	// it's not queue, so it is safe

	sendChannel.UpdatePresence(presence);
	thread.Notify();
}

void DiscordRpcImpl::ClearPresence()
{
	UpdatePresence(nullptr);
}

void DiscordRpcImpl::Respond(const char* userId, DiscordReply reply)
{
	if (!isInitialized || !connection.IsOpen())
		return;

	if (sendChannel.ReplyJoinRequest(userId, (int)reply))
		thread.Notify();
}

void DiscordRpcImpl::UpdateConnection()
{
	if (!isInitialized)
		return;

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
