#include "discord_rpc_impl.h"

extern "C" DISCORD_EXPORT DiscordRpc* CreateDiscordRpc()
{
	return new DiscordRpcImpl();
}

DiscordRpc::~DiscordRpc()
{
}

DiscordRpcImpl::DiscordRpcImpl() : sendChannel(connection), receiveChannel(connection, sendChannel), backoff(500, 60 * 1000)
{
	using namespace std::placeholders;
	connection.SetEvents(std::bind(&DiscordRpcImpl::OnConnect, this, _1), std::bind(&DiscordRpcImpl::OnDisconnect, this, _1, _2));
	isInitialized = false;
}

DiscordRpcImpl::~DiscordRpcImpl()
{
}

void DiscordRpcImpl::Initialize(const char* applicationId, const DiscordEventHandlers* handlers)
{
	if (isInitialized || !applicationId)
		return;
	isInitialized = true;

	receiveChannel.SetHandlers(handlers);
	connection.SetApplicationId(applicationId);
	thread.Start(std::bind(&DiscordRpcImpl::UpdateConnection, this));
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
	receiveChannel.UpdateHandlers(handlers);
	if (isInitialized)
		thread.Notify();
}

void DiscordRpcImpl::UpdatePresence(const DiscordRichPresence* presence)
{
	sendChannel.UpdatePresence(presence);
	if (isInitialized)
		thread.Notify();
}

void DiscordRpcImpl::ClearPresence()
{
	UpdatePresence(nullptr);
}

void DiscordRpcImpl::Respond(const char* userId, DiscordReply reply)
{
	if (!connection.IsOpen() || !userId)
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

void DiscordRpcImpl::OnDisconnect(int err, const std::string_view& message)
{
	receiveChannel.OnDisconnect(err, message);
	backoff.setNewDelay();
}
