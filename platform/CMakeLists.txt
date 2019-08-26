add_sources(SOURCE_GROUP "ChocolateDescent\\\\platform"
    "${METADATA_BASE_PATH}platform/disk.h"
    "${METADATA_BASE_PATH}platform/findfile.h"
    "${METADATA_BASE_PATH}platform/i_sound.h"
    "${METADATA_BASE_PATH}platform/joy.cpp"
    "${METADATA_BASE_PATH}platform/joy.h"
    "${METADATA_BASE_PATH}platform/key.cpp"
    "${METADATA_BASE_PATH}platform/key.h"
    "${METADATA_BASE_PATH}platform/mono.cpp"
    "${METADATA_BASE_PATH}platform/mono.h"
    "${METADATA_BASE_PATH}platform/mouse.cpp"
    "${METADATA_BASE_PATH}platform/mouse.h"
    "${METADATA_BASE_PATH}platform/s_midi.cpp"
    "${METADATA_BASE_PATH}platform/s_midi.h"
    "${METADATA_BASE_PATH}platform/timer.cpp"
    "${METADATA_BASE_PATH}platform/timer.h"
)

if (SDL2_FOUND)
    add_sources(SOURCE_GROUP "ChocolateDescent\\\\platform\\\\sdl"
        "${METADATA_BASE_PATH}platform/sdl/gr_sdl.cpp"
        "${METADATA_BASE_PATH}platform/sdl/key_sdl.cpp"
        "${METADATA_BASE_PATH}platform/sdl/mouse_sdl.cpp"
    )
endif()

if (WIN32) #[ISB] The portions that would normally be replaced with SDL2 or OpenAL will automatically ifdef itself out if SDL or OpenAL are specified.
    add_sources(SOURCE_GROUP "ChocolateDescent\\\\platform\\\\win32"
        "${METADATA_BASE_PATH}platform/win32/disk.cpp"
        "${METADATA_BASE_PATH}platform/win32/findfile.cpp"
        "${METADATA_BASE_PATH}platform/win32/hid.cpp"
        "${METADATA_BASE_PATH}platform/win32/hid.h"
        "${METADATA_BASE_PATH}platform/win32/win32audio.cpp"
        "${METADATA_BASE_PATH}platform/win32/win32joystick.cpp"
        "${METADATA_BASE_PATH}platform/win32/win32joystick.h"
        "${METADATA_BASE_PATH}platform/win32/win32window.cpp"
    )
endif()