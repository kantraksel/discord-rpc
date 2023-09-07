#include <atomic>
#include "rpc_connection.h"
#include "serialization.h"

constexpr int RpcVersion = 1;

void RpcConnection::Initialize(const char* applicationId, OnConnect onConnect, OnDisconnect onDisconnect)
{
	StringCopy(appId, applicationId);
	this->onConnect = onConnect;
	this->onDisconnect = onDisconnect;
}

void RpcConnection::Open()
{
	if (state == State::Connected)
		return;

	if (state == State::Disconnected && !connection.Open())
		return;

	if (state == State::SentHandshake)
	{
		JsonDocument message;
		if (Read(message))
		{
			auto cmd = GetStrMember(&message, "cmd");
			auto evt = GetStrMember(&message, "evt");
			if (cmd && evt && !strcmp(cmd, "DISPATCH") && !strcmp(evt, "READY"))
			{
				state = State::Connected;
				if (onConnect)
					onConnect(message);
			}
		}
	}
	else
	{
		frame.opcode = Opcode::Handshake;
		frame.length = (uint32_t)JsonWriteHandshakeObj(frame.message, sizeof(frame.message), RpcVersion, appId);

		if (connection.Write(&frame, sizeof(MessageFrameHeader) + frame.length))
			state = State::SentHandshake;
		else Close();
	}
}

void RpcConnection::Close()
{
	if (onDisconnect && state != State::Disconnected)
		onDisconnect(lastErrorCode, lastErrorMessage);

	connection.Close();
	state = State::Disconnected;
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
				StringCopy(lastErrorMessage, "Pipe closed");
				Close();
			}
			return false;
		}

		if (frame.length > 0)
		{
			if (!connection.Read(frame.message, frame.length))
			{
				lastErrorCode = (int)ErrorCode::ReadCorrupt;
				StringCopy(lastErrorMessage, "Partial data in frame");
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
			StringCopy(lastErrorMessage, GetStrMember(&message, "message", ""));
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
			StringCopy(lastErrorMessage, "Bad ipc frame");
			Close();
			return false;
		}
	}
}
