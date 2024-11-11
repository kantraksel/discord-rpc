# Discord RPC (Updated)

This is a fork of officially discontinued Discord Rich Presence library (https://github.com/discord/discord-rpc)

Because Discord Game SDK has been deprecated (in it's major functionality), this library stands as a long-term solution in embedding your game into Discord!

## Build & run

### With CMake

Requirements:
- CMake 3.15.0 or later
- C++ compiler with C++20 support
- RapidJSON (prefer latest commits)

```sh
    cd <path to discord-rpc>
    mkdir build
    cd build
    cmake .. -DCMAKE_INSTALL_PREFIX=<path to install discord-rpc to>
    cmake --build . --config Release --target install
```
Available CMake switches:

| flag                                                                                     | default | does                                                                                                                                                  |
| ---------------------------------------------------------------------------------------- | ------- | ----------------------------------------------------------------------------------------------------------------------------------------------------- |
| `ENABLE_IO_THREAD`                                                                       | `ON`    | Starts up a thread to do io processing, if disabled - define `DISCORD_DISABLE_IO_THREAD` before including header and call `Discord_UpdateConnection` yourself. |
| `USE_STATIC_CRT`                                                                         | `OFF`   | (Windows) Enable to link runtime library statically, removing dependency on redistributable package.                                                  |
| `BUILD_SHARED_LIBS`                                                                      | `OFF`   | Build as shared library.                                                                                                                              |
| `ENABLE_C_API`                                                                           | `ON`    | Add legacy C api to generated project.                                                                                                                |

### Without CMake

Requirements:
- C++ compiler with C++20 support
- RapidJSON (prefer latest commits)

Create new project, copy `src` and `include` directories, add files inside them to the projects. Remember to set C++ to C++20 and link the project to your app project.

You need to either download RapidJSON via your favourite package manager or import it from [the repo](https://github.com/Tencent/rapidjson) in similar way you imported Discord RPC.

## Basic Usage

First, head on over to the [Discord Developers Portal](https://discord.com/developers/applications) and make yourself an app. Keep track of `Application ID` -- you'll need it here to pass to the init function.

Then include `discord_rpc.hpp` (C++ API) or `discord_rpc.h` (C API) and start developing your integration. When using C++ API, use DiscordRpc class (create object using `CreateDiscordRpc()`), in C API use functions prefixed with `Discord_`.

Also there's one trick. Presence and handler functions do NOT require the library to be initialized - any presence calls are cached until you initialize the library and handlers are always updated.

## Unimplemented feature

It's possible to create Rich Presence with 2 link buttons, but there are no plans in adding it to the library. Maybe it will be added... soon :)
