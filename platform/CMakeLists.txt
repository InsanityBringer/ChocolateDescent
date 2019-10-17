target_sources(ChocolateDescent PRIVATE
    "${METADATA_BASE_PATH}/platform/disk.h"
    "${METADATA_BASE_PATH}/platform/findfile.h"
    "${METADATA_BASE_PATH}/platform/i_sound.h"
    "${METADATA_BASE_PATH}/platform/i_midi.h"
    "${METADATA_BASE_PATH}/platform/joy.cpp"
    "${METADATA_BASE_PATH}/platform/joy.h"
    "${METADATA_BASE_PATH}/platform/key.cpp"
    "${METADATA_BASE_PATH}/platform/key.h"
    "${METADATA_BASE_PATH}/platform/mono.cpp"
    "${METADATA_BASE_PATH}/platform/mono.h"
    "${METADATA_BASE_PATH}/platform/mouse.cpp"
    "${METADATA_BASE_PATH}/platform/mouse.h"
    "${METADATA_BASE_PATH}/platform/s_midi.cpp"
    "${METADATA_BASE_PATH}/platform/s_midi.h"
    "${METADATA_BASE_PATH}/platform/s_sequencer.cpp"
    "${METADATA_BASE_PATH}/platform/s_sequencer.h"
    "${METADATA_BASE_PATH}/platform/timer.cpp"
    "${METADATA_BASE_PATH}/platform/timer.h"
)

target_sources(ChocolateDescent2 PRIVATE
    "${METADATA_BASE_PATH}/platform/disk.h"
    "${METADATA_BASE_PATH}/platform/findfile.h"
    "${METADATA_BASE_PATH}/platform/i_sound.h"
    "${METADATA_BASE_PATH}/platform/i_midi.h"
    "${METADATA_BASE_PATH}/platform/joy.cpp"
    "${METADATA_BASE_PATH}/platform/joy.h"
    "${METADATA_BASE_PATH}/platform/key.cpp"
    "${METADATA_BASE_PATH}/platform/key.h"
    "${METADATA_BASE_PATH}/platform/mono.cpp"
    "${METADATA_BASE_PATH}/platform/mono.h"
    "${METADATA_BASE_PATH}/platform/mouse.cpp"
    "${METADATA_BASE_PATH}/platform/mouse.h"
    "${METADATA_BASE_PATH}/platform/s_midi.cpp"
    "${METADATA_BASE_PATH}/platform/s_midi.h"
    "${METADATA_BASE_PATH}/platform/s_sequencer.cpp"
    "${METADATA_BASE_PATH}/platform/s_sequencer.h"
    "${METADATA_BASE_PATH}/platform/timer.cpp"
    "${METADATA_BASE_PATH}/platform/timer.h"
)

if (SDL2_FOUND)
    target_sources(ChocolateDescent PRIVATE
        "${METADATA_BASE_PATH}/platform/sdl/gr_sdl.cpp"
        "${METADATA_BASE_PATH}/platform/sdl/key_sdl.cpp"
        "${METADATA_BASE_PATH}/platform/sdl/joy_sdl.cpp"
        "${METADATA_BASE_PATH}/platform/sdl/mouse_sdl.cpp"
    )
    target_sources(ChocolateDescent2 PRIVATE
        "${METADATA_BASE_PATH}/platform/sdl/gr_sdl.cpp"
        "${METADATA_BASE_PATH}/platform/sdl/key_sdl.cpp"
        "${METADATA_BASE_PATH}/platform/sdl/joy_sdl.cpp"
        "${METADATA_BASE_PATH}/platform/sdl/mouse_sdl.cpp"
    )
endif()

if (OPENAL_FOUND)
    target_sources(ChocolateDescent PRIVATE
        "${METADATA_BASE_PATH}/platform/openal/al_sound.cpp"
    )
    target_sources(ChocolateDescent2 PRIVATE
        "${METADATA_BASE_PATH}/platform/openal/al_sound.cpp"
    )
endif()

if (FLUIDSYNTH_FOUND)
    target_sources(ChocolateDescent PRIVATE
        "${METADATA_BASE_PATH}/platform/fluidsynth/fluid_midi.cpp"
    )
    target_sources(ChocolateDescent2 PRIVATE
        "${METADATA_BASE_PATH}/platform/fluidsynth/fluid_midi.cpp"
    )
endif()

if (WIN32) #[ISB] The portions that would normally be replaced with SDL2 or OpenAL will automatically ifdef itself out if SDL or OpenAL are specified.
    target_sources(ChocolateDescent PRIVATE
        "${METADATA_BASE_PATH}/platform/win32/disk.cpp"
        "${METADATA_BASE_PATH}/platform/win32/findfile.cpp"
        "${METADATA_BASE_PATH}/platform/win32/hid.cpp"
        "${METADATA_BASE_PATH}/platform/win32/hid.h"
        "${METADATA_BASE_PATH}/platform/win32/win32audio.cpp"
        "${METADATA_BASE_PATH}/platform/win32/win32joystick.cpp"
        "${METADATA_BASE_PATH}/platform/win32/win32joystick.h"
        "${METADATA_BASE_PATH}/platform/win32/win32window.cpp"
    )
    target_sources(ChocolateDescent2 PRIVATE
        "${METADATA_BASE_PATH}/platform/win32/disk.cpp"
        "${METADATA_BASE_PATH}/platform/win32/findfile.cpp"
        "${METADATA_BASE_PATH}/platform/win32/hid.cpp"
        "${METADATA_BASE_PATH}/platform/win32/hid.h"
        "${METADATA_BASE_PATH}/platform/win32/win32audio.cpp"
        "${METADATA_BASE_PATH}/platform/win32/win32joystick.cpp"
        "${METADATA_BASE_PATH}/platform/win32/win32joystick.h"
        "${METADATA_BASE_PATH}/platform/win32/win32window.cpp"
    )
endif()