#include <atomic>
#include "rpc_connection.h"
#include "serialization.h"

constexpr int RpcVersion = 1;

void RpcConnection::SetEvents(OnConnect onConnect, OnDisconnect onDisconnect)
{
	this->onConnect = onConnect;
	this->onDisconnect = onDisconnect;
}

void RpcConnection::SetApplicationId(const std::string_view& id)
{
	appId = id;
}

void RpcConnection::Open()
{
	if (state == State::Disconnected)
	{
		if (!connection.Open())
			return;

		frame.opcode = Opcode::Handshake;
		frame.length = (uint32_t)JsonWriteHandshakeObj(frame.message, sizeof(frame.message), RpcVersion, &appId);

		if (connection.Write(&frame, sizeof(MessageFrameHeader) + frame.length))
			state = State::Connecting;
		else
			Close();
	}
	else if (state == State::Connecting)
	{
		JsonDocument message;
		if (Read(message))
		{
			auto cmd = GetStrMember(&message, "cmd");
			auto evt = GetStrMember(&message, "evt");
			if (cmd && evt && strcmp(cmd, "DISPATCH") == 0 && strcmp(evt, "READY") == 0)
			{
				state = State::Connected;
				if (onConnect)
					onConnect(message);
			}
		}
	}
}

void RpcConnection::Close()
{
	if (state != State::Disconnected && onDisconnect)
		onDisconnect(lastErrorCode, lastErrorMessage);

	connection.Close();
	state = State::Disconnected;
	lastErrorCode = (int)ErrorCode::Success;
	lastErrorMessage.clear();
}

bool RpcConnection::Write(const void* data, size_t length)
{
	if (length > sizeof(frame.message))
		return false;

	frame.opcode = Opcode::Frame;
	frame.length = (uint32_t)length;
	memcpy(frame.message, data, length);
	
	if (!connection.Write(&frame, sizeof(MessageFrameHeader) + length))
	{
		Close();
		return false;
	}
	return true;
}

bool RpcConnection::Read(JsonDocument& message)
{
	if (state == State::Disconnected)
		return false;

	for (;;)
	{
		if (!connection.Read(&frame, sizeof(MessageFrameHeader)))
		{
			if (!connection.isOpen)
			{
				lastErrorCode = (int)ErrorCode::PipeClosed;
				lastErrorMessage = "Pipe closed";
				Close();
			}
			return false;
		}

		if (frame.length > 0)
		{
			if (!connection.Read(frame.message, frame.length))
			{
				lastErrorCode = (int)ErrorCode::ReadCorrupt;
				lastErrorMessage = "Partial data in frame";
				Close();
				return false;
			}
			frame.message[frame.length] = 0;
		}

		switch (frame.opcode)
		{
			case Opcode::Close:
			{
				message.ParseInsitu(frame.message);
				lastErrorCode = GetIntMember(&message, "code");
				lastErrorMessage = GetStrMember(&message, "message", "");
				Close();
				return false;
			}

			case Opcode::Frame:
				message.ParseInsitu(frame.message);
				return true;

			case Opcode::Ping:
				frame.opcode = Opcode::Pong;
				if (!connection.Write(&frame, sizeof(MessageFrameHeader) + frame.length))
					Close();
				break;

			case Opcode::Pong:
				break;

			case Opcode::Handshake:
			default:
				// something bad happened
				lastErrorCode = (int)ErrorCode::ReadCorrupt;
				lastErrorMessage = "Bad ipc frame";
				Close();
				return false;
		}
	}
}
