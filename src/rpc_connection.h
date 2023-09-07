#pragma once
#include <cstdint>
#include <functional>
#include "connection.h"

// libuv's buffer size for named pipes; discord will never use this
constexpr size_t MaxRpcFrameSize = 64 * 1024;
class JsonDocument;

class RpcConnection
{
public:
	typedef std::function<void(JsonDocument& message)> OnConnect;
	typedef std::function<void(int errorCode, const char* message)> OnDisconnect;

private:
	enum class ErrorCode : int
	{
		Success = 0,
		PipeClosed = 1,
		ReadCorrupt = 2,
	};

	enum class Opcode : uint32_t
	{
		Handshake = 0,
		Frame = 1,
		Close = 2,
		Ping = 3,
		Pong = 4,
	};

	struct MessageFrameHeader
	{
		Opcode opcode;
		uint32_t length;
	};

	struct MessageFrame : public MessageFrameHeader
	{
		char message[MaxRpcFrameSize - sizeof(MessageFrameHeader)];
	};

	enum class State : uint32_t
	{
		Disconnected,
		SentHandshake,
		Connected,
	};

	BaseConnection connection;
	State state{State::Disconnected};
	char appId[64]{};
	int lastErrorCode{0};
	char lastErrorMessage[256]{};
	MessageFrame frame;

	OnConnect onConnect{ nullptr };
	OnDisconnect onDisconnect{ nullptr };

public:
	void Initialize(const char* applicationId, OnConnect onConnect, OnDisconnect onDisconnect);

	inline bool IsOpen() const { return state == State::Connected; }

	void Open();
	void Close();
	bool Write(const void* data, size_t length);
	bool Read(JsonDocument& message);
};
