#include "discord_rpc.h"

#include "backoff.h"
#include "msg_queue.h"
#include "rpc_connection.h"
#include "serialization.h"
#include "io_thread.h"
#include "events.h"
#include "data_channel.h"

#include <atomic>
#include <chrono>
#include <mutex>

constexpr size_t JoinQueueSize{8};

static RpcConnection Connection;
static DiscordEventHandlers QueuedHandlers{};
static DiscordEventHandlers Handlers{};
static ConnectEvent OnConnect;
static DisconnectEvent OnDisconnect;
static ErrorEvent OnError;
static JoinGameEvent OnJoinGame;
static SpectateGameEvent OnSpectateGame;
static std::mutex HandlerMutex;
static DataChannel Channel(Connection);
static MsgQueue<User, JoinQueueSize> JoinAskQueue;

// We want to auto connect, and retry on failure, but not as fast as possible. This does expoential
// backoff from 0.5 seconds to 1 minute
static Backoff ReconnectTimeMs(500, 60 * 1000);
static auto NextConnect = std::chrono::system_clock::now();

static IoThread Thread;

static void UpdateReconnectTime()
{
    NextConnect = std::chrono::system_clock::now() +
      std::chrono::duration<int64_t, std::milli>{ReconnectTimeMs.nextDelay()};
}

static bool DeserializeUser(JsonValue* data, User& connectedUser)
{
    auto user = GetObjMember(data, "user");
    if (!user)
        return false;

    auto userId = GetStrMember(user, "id");
    auto username = GetStrMember(user, "username");
    if (userId && username)
    {
        StringCopy(connectedUser.userId, userId);
        StringCopy(connectedUser.username, username);
        auto discriminator = GetStrMember(user, "discriminator");
        if (discriminator)
            StringCopy(connectedUser.discriminator, discriminator);

        auto avatar = GetStrMember(user, "avatar");
        if (avatar)
            StringCopy(connectedUser.avatar, avatar);
        else
            connectedUser.avatar[0] = 0;

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

static void ReceiveData()
{
    for (;;)
    {
        JsonDocument message;
        if (!Connection.Read(message))
            break;

        const char* evtName = GetStrMember(&message, "evt");
        auto* data = GetObjMember(&message, "data");

        if (!evtName || !data)
            continue;

        std::string_view eventName = evtName;

        if (eventName == "ERROR")
            OnError.Set(GetIntMember(data, "code"), GetStrMember(data, "message", ""));
        else if (eventName == "ACTIVITY_JOIN")
        {
            auto secret = GetStrMember(data, "secret");
            if (secret)
                OnJoinGame.Set(secret);
        }
        else if (eventName == "ACTIVITY_SPECTATE")
        {
            auto secret = GetStrMember(data, "secret");
            if (secret)
                OnSpectateGame.Set(secret);
        }
        else if (eventName == "ACTIVITY_JOIN_REQUEST")
        {
            auto* joinReq = JoinAskQueue.GetNextAddMessage();
            if (joinReq && DeserializeUser(data, *joinReq))
                JoinAskQueue.CommitAdd();
        }
    }
}

//TODO: syntax
#ifdef DISCORD_DISABLE_IO_THREAD
extern "C" DISCORD_EXPORT void Discord_UpdateConnection(void)
#else
void Discord_UpdateConnection(void)
#endif
{
    if (!Connection.IsOpen()) {
        if (std::chrono::system_clock::now() >= NextConnect) {
            UpdateReconnectTime();
            Connection.Open();
        }
    }
    else
    {
        ReceiveData();
        Channel.SendData();
    }
}

static void onConnect(JsonDocument& readyMessage)
{
    Discord_UpdateHandlers(&QueuedHandlers);

    User connectedUser{};
    DeserializeUser(readyMessage, connectedUser);

    OnConnect.Set(connectedUser);
    ReconnectTimeMs.reset();
}

static void onDisconnect(int err, const char* message)
{
    OnDisconnect.Set(err, message);
    UpdateReconnectTime();
}

extern "C" DISCORD_EXPORT void Discord_Initialize(const char* applicationId,
                                                  DiscordEventHandlers* handlers)
{
    {
        std::lock_guard<std::mutex> guard(HandlerMutex);

        if (handlers)
            QueuedHandlers = *handlers;
        else
            QueuedHandlers = {};

        Handlers = {};
    }

    Connection.Initialize(applicationId, onConnect, onDisconnect);
    Thread.Start();
}

extern "C" DISCORD_EXPORT void Discord_Shutdown(void)
{
    Thread.Stop();
    Connection.Close();

    Handlers = {};
    Channel.Reset();
}

extern "C" DISCORD_EXPORT void Discord_UpdatePresence(const DiscordRichPresence* presence)
{
    Channel.UpdatePresence(presence);

    Thread.Notify();
}

extern "C" DISCORD_EXPORT void Discord_ClearPresence(void)
{
    Discord_UpdatePresence(nullptr);
}

extern "C" DISCORD_EXPORT void Discord_Respond(const char* userId, /* DISCORD_REPLY_ */ int reply)
{
    // if we are not connected, let's not batch up stale messages for later
    if (!Connection.IsOpen())
        return;

    if (Channel.ReplyJoinRequest(userId, reply))
        Thread.Notify();
}

extern "C" DISCORD_EXPORT void Discord_RunCallbacks(void)
{
    // Note on some weirdness: internally we might connect, get other signals, disconnect any number
    // of times inbetween calls here. Externally, we want the sequence to seem sane, so any other
    // signals are book-ended by calls to ready and disconnect.

    bool wasDisconnected = OnDisconnect.Consume();
    bool isConnected = Connection.IsOpen();

    if (isConnected)
    {
        // if we are connected, disconnect cb first
        std::lock_guard<std::mutex> guard(HandlerMutex);
        if (wasDisconnected && Handlers.disconnected)
        {
            auto args = OnDisconnect.GetArgs();
            Handlers.disconnected(args.first, args.second.c_str());
        }
    }

    if (OnConnect.Consume())
    {
        std::lock_guard<std::mutex> guard(HandlerMutex);
        if (Handlers.ready)
        {
            auto connectedUser = OnConnect.GetUser();
            DiscordUser du{connectedUser.userId,
                           connectedUser.username,
                           connectedUser.discriminator,
                           connectedUser.avatar};
            Handlers.ready(&du);
        }
    }

    if (OnError.Consume())
    {
        std::lock_guard<std::mutex> guard(HandlerMutex);
        if (Handlers.errored)
        {
            auto args = OnError.GetArgs();
            Handlers.errored(args.first, args.second.c_str());
        }
    }

    if (OnJoinGame.Consume())
    {
        std::lock_guard<std::mutex> guard(HandlerMutex);
        if (Handlers.joinGame)
        {
            auto secret = OnJoinGame.GetSecret();
            Handlers.joinGame(secret.c_str());
        }
    }

    if (OnSpectateGame.Consume())
    {
        std::lock_guard<std::mutex> guard(HandlerMutex);
        if (Handlers.spectateGame)
        {
            auto secret = OnJoinGame.GetSecret();
            Handlers.spectateGame(secret.c_str());
        }
    }

    // Right now this batches up any requests and sends them all in a burst; I could imagine a world
    // where the implementer would rather sequentially accept/reject each one before the next invite
    // is sent. I left it this way because I could also imagine wanting to process these all and
    // maybe show them in one common dialog and/or start fetching the avatars in parallel, and if
    // not it should be trivial for the implementer to make a queue themselves.
    while (JoinAskQueue.HavePendingSends())
    {
        auto req = JoinAskQueue.GetNextSendMessage();
        {
            std::lock_guard<std::mutex> guard(HandlerMutex);
            if (Handlers.joinRequest)
            {
                DiscordUser du{req->userId, req->username, req->discriminator, req->avatar};
                Handlers.joinRequest(&du);
            }
        }
        JoinAskQueue.CommitSend();
    }

    if (!isConnected)
    {
        // if we are not connected, disconnect message last
        std::lock_guard<std::mutex> guard(HandlerMutex);
        if (wasDisconnected && Handlers.disconnected)
        {
            auto args = OnDisconnect.GetArgs();
            Handlers.disconnected(args.first, args.second.c_str());
        }
    }
}

extern "C" DISCORD_EXPORT void Discord_UpdateHandlers(DiscordEventHandlers* newHandlers)
{
    if (newHandlers)
    {
#define HANDLE_EVENT_REGISTRATION(handler_name, event)                  \
        if (!Handlers.handler_name && newHandlers->handler_name)        \
            Channel.SubscribeEvent(event);                              \
        else if (Handlers.handler_name && !newHandlers->handler_name)   \
            Channel.UnsubscribeEvent(event);
// end HANDLE_EVENT_REGISTRATION

        std::lock_guard<std::mutex> guard(HandlerMutex);
        HANDLE_EVENT_REGISTRATION(joinGame, "ACTIVITY_JOIN")
        HANDLE_EVENT_REGISTRATION(spectateGame, "ACTIVITY_SPECTATE")
        HANDLE_EVENT_REGISTRATION(joinRequest, "ACTIVITY_JOIN_REQUEST")

#undef HANDLE_EVENT_REGISTRATION

        Handlers = *newHandlers;
        Thread.Notify();
    }
    else
    {
        std::lock_guard<std::mutex> guard(HandlerMutex);
        Handlers = {};
    }
    return;
}
