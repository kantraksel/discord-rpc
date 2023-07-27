include_directories(${PROJECT_SOURCE_DIR}/include)

option(ENABLE_IO_THREAD "Start up a separate I/O thread, otherwise I'd need to call an update function" ON)
option(USE_STATIC_CRT "Use statically-linked runtime library" OFF)

set(CMAKE_CXX_STANDARD 20)

set(BASE_RPC_SRC
    ${PROJECT_SOURCE_DIR}/include/discord_rpc.h
    discord_rpc.cpp
    rpc_connection.h
    rpc_connection.cpp
    serialization.h
    serialization.cpp
    connection.h
    backoff.h
    msg_queue.h
    io_thread.h
    io_thread.cpp
    events.h
    presence.h
)

if(WIN32)
	if (${BUILD_SHARED_LIBS})
		set(BASE_RPC_SRC ${BASE_RPC_SRC} dllmain.cpp)
	endif(${BUILD_SHARED_LIBS})
	
    set(BASE_RPC_SRC ${BASE_RPC_SRC} connection_win.cpp)
    add_library(discord-rpc ${BASE_RPC_SRC})
	if(USE_STATIC_CRT)
		set_property(TARGET discord-rpc PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
	endif(USE_STATIC_CRT)
    target_link_libraries(discord-rpc PRIVATE psapi advapi32)
endif(WIN32)

if(UNIX)
    set(BASE_RPC_SRC ${BASE_RPC_SRC} connection_unix.cpp)

    add_library(discord-rpc ${BASE_RPC_SRC})
    target_link_libraries(discord-rpc PUBLIC pthread)

    if (APPLE)
        target_link_libraries(discord-rpc PRIVATE "-framework AppKit, -mmacosx-version-min=10.10")
    endif (APPLE)

    target_compile_options(discord-rpc PRIVATE
        -g
        -Wall
        -Wextra
        -Wpedantic
    )

    target_compile_options(discord-rpc PRIVATE
        -Wno-unknown-pragmas # pragma push thing doesn't work on clang
        -Wno-old-style-cast # it's fine
        -Wno-c++98-compat # that was almost 2 decades ago
        -Wno-c++98-compat-pedantic
        -Wno-missing-noreturn
        -Wno-padded # structure padding
        -Wno-covered-switch-default
        -Wno-exit-time-destructors # not sure about these
        -Wno-global-constructors
    )

    if (${BUILD_SHARED_LIBS})
        target_compile_options(discord-rpc PRIVATE -fPIC)
    endif (${BUILD_SHARED_LIBS})

    if (APPLE)
        target_link_libraries(discord-rpc PRIVATE "-framework AppKit")
    endif (APPLE)
endif(UNIX)

target_include_directories(discord-rpc PRIVATE ${RapidJSON_INCLUDE_DIRS})

if (NOT ${ENABLE_IO_THREAD})
    target_compile_definitions(discord-rpc PUBLIC -DDISCORD_DISABLE_IO_THREAD)
endif (NOT ${ENABLE_IO_THREAD})

if (${BUILD_SHARED_LIBS})
    target_compile_definitions(discord-rpc PUBLIC -DDISCORD_DYNAMIC_LIB)
    target_compile_definitions(discord-rpc PRIVATE -DDISCORD_BUILDING_SDK)
endif(${BUILD_SHARED_LIBS})

install(
    TARGETS discord-rpc
    EXPORT "discord-rpc"
    RUNTIME
        DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE
        DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install(
    FILES
        "../include/discord_rpc.h"
    DESTINATION "include"
)
