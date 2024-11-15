#pragma once
#include <stdint.h>

#if defined(DISCORD_DYNAMIC_LIB)
#  if defined(_WIN32)
#    if defined(DISCORD_BUILDING_SDK)
#      define DISCORD_EXPORT __declspec(dllexport)
#    else
#      define DISCORD_EXPORT __declspec(dllimport)
#    endif
#  else
#    define DISCORD_EXPORT __attribute__((visibility("default")))
#  endif
#else
#  define DISCORD_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct DiscordRichPresence
	{
		const char* state;   /* max 128 bytes */
		const char* details; /* max 128 bytes */
		int64_t startTimestamp;
		int64_t endTimestamp;
		const char* largeImageKey;  /* max 32 bytes */
		const char* largeImageText; /* max 128 bytes */
		const char* smallImageKey;  /* max 32 bytes */
		const char* smallImageText; /* max 128 bytes */
		const char* partyId;        /* max 128 bytes */
		int partySize;
		int partyMax;
		int partyPrivacy;
		const char* matchSecret;    /* max 128 bytes */
		const char* joinSecret;     /* max 128 bytes */
		const char* spectateSecret; /* max 128 bytes */
		int8_t instance;
	} DiscordRichPresence;

	typedef struct DiscordUser
	{
		const char* userId;
		const char* username;
		const char* discriminator;
		const char* avatar;
	} DiscordUser;

	typedef struct DiscordEventHandlers
	{
		void (*ready)(const DiscordUser* request);
		void (*disconnected)(int errorCode, const char* message);
		void (*errored)(int errorCode, const char* message);
		void (*joinGame)(const char* joinSecret);
		void (*spectateGame)(const char* spectateSecret);
		void (*joinRequest)(const DiscordUser* request);
	} DiscordEventHandlers;

	enum DiscordReply
	{
		DISCORD_REPLY_NO = 0,
		DISCORD_REPLY_YES = 1,
		DISCORD_REPLY_IGNORE = 2,
	};

	enum DiscordPartyPrivacy
	{
		DISCORD_PARTY_PRIVATE = 0,
		DISCORD_PARTY_PUBLIC = 1,
	};

#ifdef __cplusplus
} /* extern "C" */
#endif
