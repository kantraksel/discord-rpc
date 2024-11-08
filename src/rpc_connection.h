#pragma once
#include <cstdint>
#include <functional>
#include "connection.h"
#include "fixed_string.h"

// libuv's buffer size for named pipes; discord will never use this
constexpr size_t MaxRpcFrameSize = 64 * 1024;
class JsonDocument;

class RpcConnection
{
public:
	typedef std::function<void(JsonDocument& message)> OnConnect;
	typedef std::function<void(int errorCode, const std::string_view& message)> OnDisconnect;

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
		Connecting,
		Connected,
	};

	BaseConnection connection;
	State state{State::Disconnected};
	FixedString<64> appId;
	int lastErrorCode{(int)ErrorCode::Success};
	FixedString<256> lastErrorMessage;
	MessageFrame frame;

	OnConnect onConnect{ nullptr };
	OnDisconnect onDisconnect{ nullptr };

public:
	void SetEvents(OnConnect onConnect, OnDisconnect onDisconnect);
	void SetApplicationId(const std::string_view& id);

	inline bool IsOpen() const { return state == State::Connected; }

	void Open();
	void Close();
	bool Write(const void* data, size_t length);
	bool Read(JsonDocument& message);
};
