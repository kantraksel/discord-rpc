#include "rpc_connection.h"
#include "cmd_channel.h"
#include "event_channel.h"
#include "serialization.h"

EventChannel::EventChannel(RpcConnection& connection, CmdChannel& sendChannel) : connection(connection), sendChannel(sendChannel)
{
}

static bool DeserializeUser(JsonValue* data, User& connectedUser)
{
	auto* user = GetObjMember(data, "user");
	if (!user)
		return false;

	auto* userId = GetStrMember(user, "id");
	auto* username = GetStrMember(user, "username");
	if (userId && username)
	{
		connectedUser.userId = userId;
		connectedUser.username = username;
		auto* discriminator = GetStrMember(user, "discriminator");
		if (discriminator)
			connectedUser.discriminator = discriminator;

		auto* avatar = GetStrMember(user, "avatar");
		if (avatar)
			connectedUser.avatar = avatar;
		else
			connectedUser.avatar.clear();

		return true;
	}

	return false;
}

static void DeserializeUser(JsonDocument& message, User& connectedUser)
{
	auto data = GetObjMember(&message, "data");
	if (!data)
		return;

	DeserializeUser(data, connectedUser);
}

void EventChannel::OnConnect(JsonDocument& readyMessage)
{
	User connectedUser{};
	DeserializeUser(readyMessage, connectedUser);

	onConnect.Set(connectedUser);
}

void EventChannel::OnDisconnect(int err, const std::string_view& message)
{
	onDisconnect.Set(err, message);
}

void EventChannel::ReceiveData()
{
	for (;;)
	{
		JsonDocument message;
		if (!connection.Read(message))
			break;

		auto* evtName = GetStrMember(&message, "evt");
		auto* data = GetObjMember(&message, "data");

		if (!evtName || !data)
			continue;
		std::string_view eventName = evtName;

		if (eventName == "ERROR")
			onError.Set(GetIntMember(data, "code"), GetStrMember(data, "message", ""));
		else if (eventName == "ACTIVITY_JOIN")
		{
			auto* secret = GetStrMember(data, "secret");
			if (secret)
				onJoinGame.Set(secret);
		}
		else if (eventName == "ACTIVITY_SPECTATE")
		{
			auto* secret = GetStrMember(data, "secret");
			if (secret)
				onSpectateGame.Set(secret);
		}
		else if (eventName == "ACTIVITY_JOIN_REQUEST")
		{
			auto* joinReq = joinAskQueue.GetNextAddMessage();
			if (joinReq && DeserializeUser(data, *joinReq))
				joinAskQueue.CommitAdd();
		}
	}
}

void EventChannel::SetHandlers(const CDiscordEventHandlers& newHandlers)
{
	// used in Discord_Initialize
	std::lock_guard<std::mutex> guard(mutex);
	handlers = newHandlers;
}

void EventChannel::InitHandlers()
{
	// used in onConnect event
	std::lock_guard<std::mutex> guard(mutex);
	if (handlers.joinGame)
		sendChannel.SubscribeEvent("ACTIVITY_JOIN");
	if (handlers.spectateGame)
		sendChannel.SubscribeEvent("ACTIVITY_SPECTATE");
	if (handlers.joinRequest)
		sendChannel.SubscribeEvent("ACTIVITY_JOIN_REQUEST");
}

void EventChannel::UpdateHandlers(const CDiscordEventHandlers& newHandlers)
{
	if (!connection.IsOpen())
	{
		SetHandlers(newHandlers);
		return;
	}

	// mutex prevents bugs related to un/subscribed events
	std::lock_guard<std::mutex> guard(mutex);

	// register for events if not registered and handler was added
		// deregister for events if registered and handler was removed
	if (!handlers.joinGame && newHandlers.joinGame)
		sendChannel.SubscribeEvent("ACTIVITY_JOIN");
	else if (handlers.joinGame && !newHandlers.joinGame)
		sendChannel.UnsubscribeEvent("ACTIVITY_JOIN");

	if (!handlers.spectateGame && newHandlers.spectateGame)
		sendChannel.SubscribeEvent("ACTIVITY_SPECTATE");
	else if (handlers.spectateGame && !newHandlers.spectateGame)
		sendChannel.UnsubscribeEvent("ACTIVITY_SPECTATE");

	if (!handlers.joinRequest && newHandlers.joinRequest)
		sendChannel.SubscribeEvent("ACTIVITY_JOIN_REQUEST");
	else if (handlers.joinRequest && !newHandlers.joinRequest)
		sendChannel.UnsubscribeEvent("ACTIVITY_JOIN_REQUEST");

	handlers = newHandlers;
}

void EventChannel::RunCallbacks()
{
	std::lock_guard<std::mutex> guard(mutex);

	// we want the sequence to seem sane, so any other signals
	// are book-ended by calls to ready and disconnect.

	bool wasDisconnected = onDisconnect.Consume();
	bool isConnected = connection.IsOpen();

	if (isConnected && wasDisconnected)
	{
		// if we are connected, disconnect cb first
		if (handlers.disconnected)
		{
			auto args = onDisconnect.GetArgs();
			handlers.disconnected(args.first, args.second);
		}
	}

	if (onConnect.Consume() && handlers.ready)
	{
		auto connectedUser = onConnect.GetUser();
		CDiscordUser du{
			connectedUser.userId,
			connectedUser.username,
			connectedUser.discriminator,
			connectedUser.avatar };
		handlers.ready(du);
	}

	if (onError.Consume() && handlers.errored)
	{
		auto args = onError.GetArgs();
		handlers.errored(args.first, args.second);
	}

	if (onJoinGame.Consume() && handlers.joinGame)
	{
		handlers.joinGame(onJoinGame.GetSecret());
	}

	if (onSpectateGame.Consume() && handlers.spectateGame)
	{
		handlers.spectateGame(onSpectateGame.GetSecret());
	}

	while (joinAskQueue.HavePendingSends())
	{
		auto req = joinAskQueue.GetNextSendMessage();
		if (handlers.joinRequest)
		{
			CDiscordUser du{
				req->userId,
				req->username,
				req->discriminator,
				req->avatar };
			handlers.joinRequest(du);
		}
		joinAskQueue.CommitSend();
	}

	if (!isConnected && wasDisconnected)
	{
		// if we are not connected, disconnect message last
		if (handlers.disconnected)
		{
			auto args = onDisconnect.GetArgs();
			handlers.disconnected(args.first, args.second);
		}
	}
}
