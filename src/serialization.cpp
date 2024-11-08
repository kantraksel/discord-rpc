#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "serialization.h"
#include "discord_rpc_shared.h"

class DirectStringBuffer
{
public:
	using Ch = char;
	char* buffer;
	char* end;
	char* current;

	DirectStringBuffer(char* buffer, size_t maxLen)
		: buffer(buffer)
		, end(buffer + maxLen)
		, current(buffer)
	{
	}

	void Put(char c)
	{
		if (current < end)
			*current++ = c;
	}

	void Flush() {}
	size_t GetSize() const { return (size_t)(current - buffer); }
};

// writer appears to need about 16 bytes per nested object level (with 64bit size_t)
constexpr size_t WriterNestingLevels = 2048 / (2 * sizeof(size_t));

using JsonWriterBase = rapidjson::Writer<DirectStringBuffer, UTF8, UTF8, StackAllocator>;
class JsonWriter : public JsonWriterBase
{
public:
	DirectStringBuffer stringBuffer;
	StackAllocator stackAlloc;

	JsonWriter(char* dest, size_t maxLen)
		: JsonWriterBase(stringBuffer, &stackAlloc, WriterNestingLevels)
		, stringBuffer(dest, maxLen)
		, stackAlloc()
	{
	}

	size_t Size() const { return stringBuffer.GetSize(); }
};

template <typename T>
void NumberToString(char* dest, T number)
{
	if (!number)
	{
		*dest++ = '0';
		*dest++ = 0;
		return;
	}
	if (number < 0)
	{
		*dest++ = '-';
		number = -number;
	}
	char temp[32];
	int place = 0;
	while (number)
	{
		auto digit = number % 10;
		number = number / 10;
		temp[place++] = '0' + (char)digit;
	}
	for (--place; place >= 0; --place)
	{
		*dest++ = temp[place];
	}
	*dest = 0;
}

// it's ever so slightly faster to not have to strlen the key
template <typename T>
void WriteKey(JsonWriter& w, T& k)
{
	w.Key(k, sizeof(T) - 1);
}

struct WriteObject
{
	JsonWriter& writer;
	WriteObject(JsonWriter& w)
	  : writer(w)
	{
		writer.StartObject();
	}
	template <typename T>
	WriteObject(JsonWriter& w, T& name)
	  : writer(w)
	{
		WriteKey(writer, name);
		writer.StartObject();
	}
	~WriteObject() { writer.EndObject(); }
};

struct WriteArray
{
	JsonWriter& writer;
	template <typename T>
	WriteArray(JsonWriter& w, T& name)
	  : writer(w)
	{
		WriteKey(writer, name);
		writer.StartArray();
	}
	~WriteArray() { writer.EndArray(); }
};

template <typename T>
void WriteOptionalString(JsonWriter& w, T& k, const char* value)
{
	if (value && value[0]) {
		w.Key(k, sizeof(T) - 1);
		w.String(value);
	}
}

static void JsonWriteNonce(JsonWriter& writer, int nonce)
{
	WriteKey(writer, "nonce");
	char nonceBuffer[32];
	NumberToString(nonceBuffer, nonce);
	writer.String(nonceBuffer);
}

size_t JsonWriteRichPresenceObj(char* dest, size_t maxLen, int nonce, int pid, const DiscordRichPresence* presence)
{
	JsonWriter writer(dest, maxLen);

	{
		WriteObject top(writer);

		JsonWriteNonce(writer, nonce);

		WriteKey(writer, "cmd");
		writer.String("SET_ACTIVITY");

		{
			WriteObject args(writer, "args");

			WriteKey(writer, "pid");
			writer.Int(pid);

			if (presence != nullptr)
			{
				WriteObject activity(writer, "activity");

				WriteOptionalString(writer, "state", presence->state);
				WriteOptionalString(writer, "details", presence->details);

				if (presence->startTimestamp || presence->endTimestamp)
				{
					WriteObject timestamps(writer, "timestamps");

					if (presence->startTimestamp)
					{
						WriteKey(writer, "start");
						writer.Int64(presence->startTimestamp);
					}

					if (presence->endTimestamp)
					{
						WriteKey(writer, "end");
						writer.Int64(presence->endTimestamp);
					}
				}

				if ((presence->largeImageKey && presence->largeImageKey[0]) ||
					(presence->largeImageText && presence->largeImageText[0]) ||
					(presence->smallImageKey && presence->smallImageKey[0]) ||
					(presence->smallImageText && presence->smallImageText[0]))
				{
					WriteObject assets(writer, "assets");
					WriteOptionalString(writer, "large_image", presence->largeImageKey);
					WriteOptionalString(writer, "large_text", presence->largeImageText);
					WriteOptionalString(writer, "small_image", presence->smallImageKey);
					WriteOptionalString(writer, "small_text", presence->smallImageText);
				}

				if ((presence->partyId && presence->partyId[0]) || presence->partySize ||
					presence->partyMax || presence->partyPrivacy)
				{
					WriteObject party(writer, "party");
					WriteOptionalString(writer, "id", presence->partyId);
					if (presence->partySize && presence->partyMax)
					{
						WriteArray size(writer, "size");
						writer.Int(presence->partySize);
						writer.Int(presence->partyMax);
					}

					if (presence->partyPrivacy)
					{
						WriteKey(writer, "privacy");
						writer.Int(presence->partyPrivacy);
					}
				}

				if ((presence->matchSecret && presence->matchSecret[0]) ||
					(presence->joinSecret && presence->joinSecret[0]) ||
					(presence->spectateSecret && presence->spectateSecret[0]))
				{
					WriteObject secrets(writer, "secrets");
					WriteOptionalString(writer, "match", presence->matchSecret);
					WriteOptionalString(writer, "join", presence->joinSecret);
					WriteOptionalString(writer, "spectate", presence->spectateSecret);
				}

				writer.Key("instance");
				writer.Bool(presence->instance != 0);
			}
		}
	}

	return writer.Size();
}

size_t JsonWriteHandshakeObj(char* dest, size_t maxLen, int version, const char* applicationId)
{
	JsonWriter writer(dest, maxLen);

	{
		WriteObject obj(writer);
		WriteKey(writer, "v");
		writer.Int(version);
		WriteKey(writer, "client_id");
		writer.String(applicationId);
	}

	return writer.Size();
}

size_t JsonWriteSubscribeCommand(char* dest, size_t maxLen, int nonce, const char* evtName)
{
	JsonWriter writer(dest, maxLen);

	{
		WriteObject obj(writer);

		JsonWriteNonce(writer, nonce);

		WriteKey(writer, "cmd");
		writer.String("SUBSCRIBE");

		WriteKey(writer, "evt");
		writer.String(evtName);
	}

	return writer.Size();
}

size_t JsonWriteUnsubscribeCommand(char* dest, size_t maxLen, int nonce, const char* evtName)
{
	JsonWriter writer(dest, maxLen);

	{
		WriteObject obj(writer);

		JsonWriteNonce(writer, nonce);

		WriteKey(writer, "cmd");
		writer.String("UNSUBSCRIBE");

		WriteKey(writer, "evt");
		writer.String(evtName);
	}

	return writer.Size();
}

size_t JsonWriteJoinReply(char* dest, size_t maxLen, const std::string_view& userId, int reply, int nonce)
{
	JsonWriter writer(dest, maxLen);

	{
		WriteObject obj(writer);

		WriteKey(writer, "cmd");
		if (reply == DISCORD_REPLY_YES)
			writer.String("SEND_ACTIVITY_JOIN_INVITE");
		else
			writer.String("CLOSE_ACTIVITY_JOIN_REQUEST");

		WriteKey(writer, "args");
		{
			WriteObject args(writer);

			WriteKey(writer, "user_id");
			writer.String(userId.data(), userId.size());
		}

		JsonWriteNonce(writer, nonce);
	}

	return writer.Size();
}

JsonValue* GetObjMember(JsonValue* obj, const char* name)
{
	if (obj)
	{
		auto member = obj->FindMember(name);
		if (member != obj->MemberEnd() && member->value.IsObject())
			return &member->value;
	}
	return nullptr;
}

int GetIntMember(JsonValue* obj, const char* name, int notFoundDefault)
{
	if (obj)
	{
		auto member = obj->FindMember(name);
		if (member != obj->MemberEnd() && member->value.IsInt())
			return member->value.GetInt();
	}
	return notFoundDefault;
}

const char* GetStrMember(JsonValue* obj, const char* name, const char* notFoundDefault)
{
	if (obj)
	{
		auto member = obj->FindMember(name);
		if (member != obj->MemberEnd() && member->value.IsString())
			return member->value.GetString();
	}
	return notFoundDefault;
}
