#pragma once
#include <cstdint>
#include "rapidjson/document.h"

template <size_t Len>
inline size_t StringCopy(char (&dest)[Len], const char* src)
{
	if (!src || !Len)
		return 0;

	size_t copied;
	char* out = dest;
	for (copied = 1; *src && copied < Len; ++copied)
	{
		*out++ = *src++;
	}
	*out = 0;
	return copied - 1;
}

// StackAllocator used to reduce heap allocations
template <size_t Size>
class FixedLinearAllocator
{
public:
	static const bool kNeedFree = false;
	char buffer[Size]{};
	char* pointer;
	char* end;

	FixedLinearAllocator()
	  : pointer(buffer)
	  , end(buffer + Size)
	{
	}

	void* Malloc(size_t size)
	{
		char* res = pointer;
		pointer += size;
		if (pointer > end)
		{
			pointer = res;
			return nullptr;
		}
		return res;
	}

	void* Realloc(void* originalPtr, size_t originalSize, size_t newSize)
	{
		if (newSize == 0)
			return nullptr;

		assert(!originalPtr && !originalSize);
		(void)originalPtr; (void)originalSize;

		return Malloc(newSize);
	}

	static void Free(void* ptr)
	{
		(void)ptr;
	}
};

using MallocAllocator = rapidjson::CrtAllocator;
using PoolAllocator = rapidjson::MemoryPoolAllocator<>;
using UTF8 = rapidjson::UTF8<>;
using StackAllocator = FixedLinearAllocator<2048>;

using JsonDocumentBase = rapidjson::GenericDocument<UTF8, PoolAllocator, StackAllocator>;
class JsonDocument : public JsonDocumentBase
{
public:
	static const int kDefaultChunkCapacity = 32 * 1024;

	MallocAllocator mallocAllocator;
	PoolAllocator poolAllocator;
	StackAllocator stackAllocator;

	// use the buffer first, then allocate more; the size seems to be too big
	char parseBuffer[32 * 1024];
	
	JsonDocument()
	  : JsonDocumentBase(rapidjson::kObjectType,
						 &poolAllocator,
						 sizeof(stackAllocator.buffer),
						 &stackAllocator)
	  , poolAllocator(parseBuffer, sizeof(parseBuffer), kDefaultChunkCapacity, &mallocAllocator)
	  , stackAllocator()
	{
	}
};

// object writers
struct DiscordRichPresence;
size_t JsonWriteHandshakeObj(char* dest, size_t maxLen, int version, const char* applicationId);
size_t JsonWriteRichPresenceObj(char* dest, size_t maxLen, int nonce, int pid, const DiscordRichPresence* presence);
size_t JsonWriteSubscribeCommand(char* dest, size_t maxLen, int nonce, const char* evtName);
size_t JsonWriteUnsubscribeCommand(char* dest, size_t maxLen, int nonce, const char* evtName);
size_t JsonWriteJoinReply(char* dest, size_t maxLen, const char* userId, int reply, int nonce);

// object property getters
using JsonValue = rapidjson::GenericValue<UTF8>;

JsonValue* GetObjMember(JsonValue* obj, const char* name);
int GetIntMember(JsonValue* obj, const char* name, int notFoundDefault = 0);
const char* GetStrMember(JsonValue* obj, const char* name, const char* notFoundDefault = nullptr);
