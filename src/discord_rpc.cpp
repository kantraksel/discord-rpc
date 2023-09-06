#include "discord_rpc.h"

#include "backoff.h"
#include "rpc_connection.h"
#include "serialization.h"
#include "io_thread.h"
#include "data_channel.h"
#include "event_channel.h"

#include <atomic>
#include <chrono>
#include <mutex>

static RpcConnection Connection;
static DataChannel SendChannel(Connection);
static EventChannel ReceiveChannel(Connection, SendChannel);

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
		ReceiveChannel.ReceiveData();
		SendChannel.SendData();
    }
}

static void onConnect(JsonDocument& readyMessage)
{
    ReceiveChannel.InitHandlers();
	ReceiveChannel.OnConnect(readyMessage);
    
    ReconnectTimeMs.reset();
}

static void onDisconnect(int err, const char* message)
{
    ReceiveChannel.OnDisconnect(err, message);
    UpdateReconnectTime();
}

extern "C" DISCORD_EXPORT void Discord_Initialize(const char* applicationId,
                                                  DiscordEventHandlers* handlers)
{
    ReceiveChannel.SetHandlers(handlers);
    Connection.Initialize(applicationId, onConnect, onDisconnect);
    Thread.Start();
}

extern "C" DISCORD_EXPORT void Discord_Shutdown(void)
{
    Thread.Stop();
    Connection.Close();

    ReceiveChannel.SetHandlers(nullptr);
	SendChannel.Reset();
}

extern "C" DISCORD_EXPORT void Discord_UpdatePresence(const DiscordRichPresence* presence)
{
	SendChannel.UpdatePresence(presence);

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

    if (SendChannel.ReplyJoinRequest(userId, reply))
        Thread.Notify();
}

extern "C" DISCORD_EXPORT void Discord_RunCallbacks(void)
{
	ReceiveChannel.RunCallbacks();
}

extern "C" DISCORD_EXPORT void Discord_UpdateHandlers(DiscordEventHandlers* newHandlers)
{
    ReceiveChannel.UpdateHandlers(newHandlers);
	Thread.Notify();
}
