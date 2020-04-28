cmake_minimum_required(VERSION 3.11)
project(ChocolateDescent)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(BUILD_EDITOR OFF CACHE BOOL "Build the game editor. Descent 1 only. Won't link outside of Windows ATM. ")
#These features are intended for testing.
set(DESCENT1_TEXMAP_IN_DESCENT2 OFF CACHE BOOL "Swaps Descent 1's texture mapper into Descent 2.")
set(DESCENT2_TEXMAP_IN_DESCENT1 OFF CACHE BOOL "Swaps Descent 2's texture mapper into Descent 1. ")

set(SHARED_SOURCES
	2d/bitblt.cpp
	2d/bitmap.cpp
	2d/box.cpp
	2d/canvas.cpp
	2d/circle.cpp
	2d/clip.h
	2d/disc.cpp
	2d/effect2d.cpp
	2d/effect2d.h
	2d/gpixel.cpp
	2d/gr.cpp
	2d/gr.h
	2d/grdef.h
	2d/i_gr.h
	2d/ibitblt.cpp
	2d/ibitblt.h
	2d/lbitblt.cpp
	2d/line.cpp
	2d/palette.cpp
	2d/palette.h
	2d/pcx.cpp
	2d/pcx.h
	2d/pixel.cpp
	2d/poly.cpp
	2d/rect.cpp
	2d/rle.cpp
	2d/rle.h
	2d/roller.cpp
	2d/scanline2d.cpp
	2d/sdl_conf.cpp
	3d/3d.h
	3d/clipper.cpp
	3d/clipper.h
	3d/draw.cpp
	3d/globvars.cpp
	3d/globvars.h
	3d/horizon.cpp
	3d/instance.cpp
	3d/interp.cpp
	3d/matrix.cpp
	3d/points.cpp
	3d/rod.cpp
	3d/setup.cpp
	cfile/cfile.cpp
	cfile/cfile.h
	fix/fix.cpp
	fix/fix.h
	fix/tables.cpp
	iff/iff.cpp
	iff/iff.h
	mem/mem.cpp
	mem/mem.h
	misc/byteswap.cpp
	misc/byteswap.h
	misc/error.cpp
	misc/error.h
	misc/parsarg.cpp
	misc/parsarg.h
    misc/rand.h
    misc/rand.cpp
	misc/stb_vorbis.h
	misc/types.h
	platform/disk.h
	platform/findfile.h
	platform/i_sound.h
	platform/i_midi.h
	platform/joy.cpp
	platform/joy.h
	platform/key.cpp
	platform/key.h
	platform/mono.cpp
	platform/mono.h
	platform/mouse.cpp
	platform/mouse.h
	platform/posixstub.h
	platform/s_midi.cpp
	platform/s_midi.h
	platform/s_sequencer.cpp
	platform/s_sequencer.h
	platform/timer.cpp
	platform/timer.h
	vecmat/vecmat.cpp
	vecmat/vecmat.h
)

set(DESCENT1_SOURCES
	main_d1/ai.cpp
	main_d1/ai.h
	main_d1/aipath.cpp
	main_d1/aistruct.cpp
	main_d1/aistruct.h
	main_d1/arcade.h
    main_d1/args.cpp
	main_d1/args.h
	main_d1/automap.cpp
	main_d1/automap.h
	main_d1/bm.cpp
	main_d1/bm.h
	main_d1/bmread.cpp
	main_d1/cntrlcen.cpp
	main_d1/cntrlcen.h
	main_d1/collide.cpp
	main_d1/collide.h
	main_d1/compbit.h
	main_d1/config.cpp
	main_d1/config.h
	main_d1/controls.cpp
	main_d1/controls.h
	main_d1/credits.cpp
	main_d1/credits.h
	main_d1/desc_id.h
	main_d1/digi.cpp
	main_d1/digi.h
	main_d1/dumpmine.cpp
	main_d1/effects.cpp
	main_d1/endlevel.cpp
	main_d1/endlevel.h
	main_d1/fireball.cpp
	main_d1/fireball.h
	main_d1/fuelcen.cpp
	main_d1/fuelcen.h
	main_d1/fvi.cpp
	main_d1/fvi.h
	main_d1/game.cpp
	main_d1/game.h
	main_d1/gamefont.cpp
	main_d1/gamefont.h
	main_d1/gamemine.cpp
	main_d1/gamemine.h
	main_d1/gamesave.cpp
	main_d1/gamesave.h
	main_d1/gameseg.cpp
	main_d1/gameseg.h
	main_d1/gameseq.cpp
	main_d1/gameseq.h
	main_d1/gauges.cpp
	main_d1/gauges.h
	main_d1/hash.cpp
	main_d1/hash.h
	main_d1/hostage.cpp
	main_d1/hostage.h
	main_d1/hud.cpp
	main_d1/i_main.cpp
	main_d1/inferno.cpp
	main_d1/inferno.h
	main_d1/joydefs.cpp
	main_d1/joydefs.h
	main_d1/kconfig.cpp
	main_d1/kconfig.h
	main_d1/kmatrix.cpp
	main_d1/kmatrix.h
	main_d1/laser.cpp
	main_d1/laser.h
	main_d1/lighting.cpp
	main_d1/lighting.h
	main_d1/menu.cpp
	main_d1/menu.h
	main_d1/mglobal.cpp
	main_d1/mission.cpp
	main_d1/mission.h
	main_d1/modem.cpp
	main_d1/modem.h
	main_d1/morph.cpp
	main_d1/morph.h
	main_d1/multi.cpp
	main_d1/multi.h
	main_d1/multibot.cpp
	main_d1/multibot.h
	main_d1/netmisc.cpp
	main_d1/netmisc.h
	main_d1/network.cpp
	main_d1/network.h
	main_d1/newdemo.cpp
	main_d1/newdemo.h
	main_d1/newmenu.cpp
	main_d1/newmenu.h
	main_d1/nocomlib.cpp
	main_d1/nocomlib.h
	main_d1/object.cpp
	main_d1/object.h
	main_d1/paging.cpp
	main_d1/paging.h
	main_d1/physics.cpp
	main_d1/physics.h
	main_d1/piggy.cpp
	main_d1/piggy.h
	main_d1/player.cpp
	main_d1/player.h
	main_d1/playsave.cpp
	main_d1/playsave.h
	main_d1/polyobj.cpp
	main_d1/polyobj.h
	main_d1/powerup.cpp
	main_d1/powerup.h
	main_d1/render.cpp
	main_d1/render.h
	main_d1/robot.cpp
	main_d1/robot.h
	main_d1/scores.cpp
	main_d1/scores.h
	main_d1/screens.h
	main_d1/segment.h
	main_d1/segpoint.h
	main_d1/slew.cpp
	main_d1/slew.h
	main_d1/songs.cpp
	main_d1/songs.h
	main_d1/sounds.h
	main_d1/state.cpp
	main_d1/state.h
	main_d1/switch.cpp
	main_d1/switch.h
	main_d1/terrain.cpp
	main_d1/terrain.h
	main_d1/texmerge.cpp
	main_d1/texmerge.h
	main_d1/text.cpp
	main_d1/text.h
	main_d1/textures.h
	main_d1/titles.cpp
	main_d1/titles.h
	main_d1/vclip.cpp
	main_d1/vclip.h
	main_d1/vers_id.h
	main_d1/vfx.cpp
	main_d1/vfx.h
	main_d1/wall.cpp
	main_d1/wall.h
	main_d1/weapon.cpp
	main_d1/weapon.h
	2d/font.cpp
	2d/scale.cpp
)

set(DESCENT1_TEXMAP_SOURCES
	texmap/ntmap.cpp
    texmap/scanline.cpp
    texmap/scanline.h
    texmap/texmap.h
    texmap/texmapl.h
    texmap/tmapflat.cpp
)

set(EDITOR_SOURCES
	ui/barbox.cpp
	ui/button.cpp
	ui/checkbox.cpp
	ui/file.cpp
	ui/func.cpp
	ui/func.h
	ui/gadget.cpp
	ui/icon.cpp
	ui/inputbox.cpp
	ui/keypad.cpp
	ui/keypress.cpp
	ui/keytrap.cpp
	ui/listbox.cpp
	ui/menubar.cpp
	ui/message.cpp
	ui/popup.cpp
	ui/radio.cpp
	ui/scroll.cpp
	ui/ui.cpp
	ui/ui.h
	ui/uidraw.cpp
	ui/uimenu.cpp
	ui/uimouse.cpp
	ui/userbox.cpp
	ui/window.cpp
	main_d1/editor/med.cpp
	main_d1/editor/einit.cpp
	main_d1/editor/eglobal.cpp
	main_d1/editor/mine.cpp
	main_d1/editor/curves.cpp
	main_d1/editor/kcurve.cpp
	main_d1/editor/khelp.cpp
	main_d1/editor/ksegsel.cpp
	main_d1/editor/kbuild.cpp
	main_d1/editor/kview.cpp
	main_d1/editor/kmine.cpp
	main_d1/editor/seguvs.cpp
	main_d1/editor/ksegsize.cpp
	main_d1/editor/ksegmove.cpp
	main_d1/editor/ktmap.cpp
	main_d1/editor/kfuncs.cpp
	main_d1/editor/autosave.cpp
	main_d1/editor/group.cpp
	main_d1/editor/texture.cpp
	main_d1/editor/fixseg.cpp
	main_d1/editor/eobject.cpp
	main_d1/editor/elight.cpp
	main_d1/editor/texpage.cpp
	main_d1/editor/objpage.cpp
	main_d1/editor/medmisc.cpp
	main_d1/editor/medsel.cpp
	main_d1/editor/kgame.cpp
	main_d1/editor/info.cpp
	main_d1/editor/medwall.cpp
	main_d1/editor/eswitch.cpp
	main_d1/editor/medrobot.cpp
	main_d1/editor/meddraw.cpp
	main_d1/editor/segment.cpp
	main_d1/editor/centers.cpp
	main_d1/editor/ehostage.cpp
	main_d1/editor/centers.h
	main_d1/editor/editor.h
	main_d1/editor/ehostage.h
	main_d1/editor/eobject.h
	main_d1/editor/eswitch.h
	main_d1/editor/info.h
	main_d1/editor/kdefs.h
	main_d1/editor/meddraw.h
	main_d1/editor/medmisc.h
	main_d1/editor/medrobot.h
	main_d1/editor/medsel.h
	main_d1/editor/medwall.h
	main_d1/editor/objpage.h
	main_d1/editor/seguvs.h
	main_d1/editor/texpage.h
	main_d1/editor/func.h
	main_d1/editor/nocfile.h
)

set(DESCENT2_SOURCES
	main_d2/ai.cpp
	main_d2/ai.h
	main_d2/ai2.cpp
	main_d2/aipath.cpp
	main_d2/aistruct.cpp
	main_d2/aistruct.h
	main_d2/arcade.h
	main_d2/automap.cpp
	main_d2/automap.h
	main_d2/bm.cpp
	main_d2/bm.h
	main_d2/bmread.cpp
	main_d2/bmread.h
	main_d2/cntrlcen.cpp
	main_d2/cntrlcen.h
	main_d2/collide.cpp
	main_d2/collide.h
	main_d2/compbit.h
	main_d2/config.cpp
	main_d2/config.h
	main_d2/controls.cpp
	main_d2/controls.h
	main_d2/credits.cpp
	main_d2/credits.h
	main_d2/crypt.cpp
	main_d2/desc_id.h
	main_d2/digi.cpp
	main_d2/digi.h
	main_d2/digiobj.cpp
	main_d2/dumpmine.cpp
	main_d2/effects.cpp
	main_d2/effects.h
	main_d2/endlevel.cpp
	main_d2/endlevel.h
	main_d2/escort.cpp
	main_d2/fireball.cpp
	main_d2/fireball.h
	main_d2/fuelcen.cpp
	main_d2/fuelcen.h
	main_d2/fvi.cpp
	main_d2/fvi.h
	main_d2/game.cpp
	main_d2/game.h
	main_d2/gamecntl.cpp
	main_d2/gamefont.cpp
	main_d2/gamefont.h
	main_d2/gamemine.cpp
	main_d2/gamemine.h
	main_d2/gamepal.cpp
	main_d2/gamepal.h
	main_d2/gamerend.cpp
	main_d2/gamesave.cpp
	main_d2/gamesave.h
	main_d2/gameseg.cpp
	main_d2/gameseg.h
	main_d2/gameseq.cpp
	main_d2/gameseq.h
	main_d2/gamestat.h
	main_d2/gauges.cpp
	main_d2/gauges.h
	main_d2/hostage.cpp
	main_d2/hostage.h
	main_d2/hud.cpp
	main_d2/i_main.cpp
	main_d2/inferno.cpp
	main_d2/inferno.h
	main_d2/joydefs.cpp
	main_d2/joydefs.h
	main_d2/kconfig.cpp
	main_d2/kconfig.h
	main_d2/kmatrix.cpp
	main_d2/kmatrix.h
	main_d2/laser.cpp
	main_d2/laser.h
	main_d2/lighting.cpp
	main_d2/lighting.h
	main_d2/menu.cpp
	main_d2/menu.h
	main_d2/mglobal.cpp
	main_d2/mission.cpp
	main_d2/mission.h
	main_d2/modem.cpp
	main_d2/modem.h
	main_d2/morph.cpp
	main_d2/morph.h
	main_d2/movie.cpp
	main_d2/movie.h
	main_d2/multi.cpp
	main_d2/multi.h
	main_d2/multibot.cpp
	main_d2/multibot.h
	main_d2/netmisc.cpp
	main_d2/netmisc.h
	main_d2/network.cpp
	main_d2/network.h
	main_d2/newdemo.cpp
	main_d2/newdemo.h
	main_d2/newmenu.cpp
	main_d2/newmenu.h
	main_d2/object.cpp
	main_d2/object.h
	main_d2/paging.cpp
	main_d2/paging.h
	main_d2/physics.cpp
	main_d2/physics.h
	main_d2/piggy.cpp
	main_d2/piggy.h
	main_d2/player.cpp
	main_d2/player.h
	main_d2/playsave.cpp
	main_d2/playsave.h
	main_d2/polyobj.cpp
	main_d2/polyobj.h
	main_d2/powerup.cpp
	main_d2/powerup.h
	main_d2/render.cpp
	main_d2/render.h
	main_d2/robot.cpp
	main_d2/robot.h
	main_d2/scores.cpp
	main_d2/scores.h
	main_d2/screens.h
	main_d2/settings.h
	main_d2/segment.h
	main_d2/segpoint.h
	main_d2/slew.cpp
	main_d2/slew.h
	main_d2/songs.cpp
	main_d2/songs.h
	main_d2/sounds.h
	main_d2/state.cpp
	main_d2/state.h
	main_d2/switch.cpp
	main_d2/switch.h
	main_d2/terrain.cpp
	main_d2/terrain.h
	main_d2/texmerge.cpp
	main_d2/texmerge.h
	main_d2/text.cpp
	main_d2/text.h
	main_d2/textures.h
	main_d2/titles.cpp
	main_d2/titles.h
	main_d2/vclip.cpp
	main_d2/vclip.h
	main_d2/vers_id.h
	main_d2/wall.cpp
	main_d2/wall.h
	main_d2/weapon.cpp
	main_d2/weapon.h
	misc/hash.cpp
	misc/hash.h
    misc/args.cpp
	misc/args.h
	mve/decoder8.cpp
	mve/decoder16.cpp
	mve/decoders.h
	mve/libmve.h
	mve/mve_audio.cpp
	mve/mve_audio.h
	mve/mvelib.cpp
	mve/mvelib.h
	mve/mveplay.cpp
	2d/font_d2.cpp
	2d/scale_d2.cpp
)

set(DESCENT2_TEXMAP_SOURCES
    texmap/ntmap_d2.cpp
    texmap/scanline_d2.cpp
    texmap/scanline.h
    texmap/texmap.h
    texmap/texmapl.h
    texmap/tmapflat_d2.cpp
)

set(SDL_SOURCES
	platform/sdl/gr_sdl.cpp
	platform/sdl/key_sdl.cpp
	platform/sdl/mouse_sdl.cpp
    platform/sdl/joy_sdl.cpp
)

set(OPENAL_SOURCES
	platform/openal/al_sound.cpp
	platform/openal/al_player.cpp
)

set(FLUIDSYNTH_SOURCES
	platform/fluidsynth/fluid_midi.cpp
)

set(WIN32_SOURCES
	platform/win32/disk.cpp
	platform/win32/findfile.cpp
	platform/win32/hid.cpp
	platform/win32/hid.h
	platform/win32/win32audio.cpp
	platform/win32/win32joystick.cpp
	platform/win32/win32joystick.h
	platform/win32/win32window.cpp
	ChocolateDescent.manifest
)

set(LINUX_SOURCES
	platform/linux/disk.cpp
	platform/linux/fileutil.cpp
	platform/linux/findfile.cpp
	platform/linux/strutil.cpp
)

set(SHARED_LIBS
)

set(DESCENT1_LIBS
)

set(DESCENT2_LIBS
)

find_package(OPENAL)
find_package(FLUIDSYNTH)
find_package(SDL2)

include_directories(.)

if (SDL2_FOUND)
	add_definitions(-DUSE_SDL)
	include_directories(${SDL2_INCLUDE_DIRS})
	set(SHARED_SOURCES ${SHARED_SOURCES} ${SDL_SOURCES})
	set(SHARED_LIBS ${SHARED_LIBS} ${SDL2_LIBRARIES})
    set(SHARED_LIBS ${SHARED_LIBS} ${SDL2_MAIN_LIBRARIES})
else()
	set(NOT_COMPILED_SOURCES ${NOT_COMPILED_SOURCES} ${SDL_SOURCES})
endif()

if (OPENAL_FOUND)
	add_definitions(-DUSE_OPENAL)
	include_directories(${OPENAL_INCLUDE_DIRS})
	set(SHARED_SOURCES ${SHARED_SOURCES} ${OPENAL_SOURCES})
	set(SHARED_LIBS ${SHARED_LIBS} ${OPENAL_LIBRARIES})
else()
	set(NOT_COMPILED_SOURCES ${NOT_COMPILED_SOURCES} ${OPENAL_SOURCES})
endif()

if (FLUIDSYNTH_FOUND)
	add_definitions(-DUSE_FLUIDSYNTH)
	include_directories(${FLUIDSYNTH_INCLUDE_DIRS})
    set(SHARED_SOURCES ${SHARED_SOURCES} ${FLUIDSYNTH_SOURCES})
	set(SHARED_LIBS ${SHARED_LIBS} ${FLUIDSYNTH_LIBRARIES})
else()
	set(NOT_COMPILED_SOURCES ${NOT_COMPILED_SOURCES} ${FLUIDSYNTH_SOURCES})
endif()

if (BUILD_EDITOR)
	set(DESCENT1_SOURCES ${DESCENT1_SOURCES} ${EDITOR_SOURCES})
else()
	set(NOT_COMPILED_SOURCES ${NOT_COMPILED_SOURCES} ${EDITOR_SOURCES})
endif()

if (DESCENT2_TEXMAP_IN_DESCENT1)
	set(DESCENT1_SOURCES ${DESCENT1_SOURCES} ${DESCENT2_TEXMAP_SOURCES})
else()
	set(DESCENT1_SOURCES ${DESCENT1_SOURCES} ${DESCENT1_TEXMAP_SOURCES})
endif()

if (DESCENT1_TEXMAP_IN_DESCENT2)
	set(DESCENT2_SOURCES ${DESCENT2_SOURCES} ${DESCENT1_TEXMAP_SOURCES})
else()
	set(DESCENT2_SOURCES ${DESCENT2_SOURCES} ${DESCENT2_TEXMAP_SOURCES})
endif()


if (WIN32)
	add_definitions(-DWIN32 -D_WIN32)
    set(SHARED_SOURCES ${SHARED_SOURCES} ${WIN32_SOURCES})
	set(NOT_COMPILED_SOURCES ${NOT_COMPILED_SOURCES} ${LINUX_SOURCES})
elseif(UNIX)
	add_definitions(-DUNIX -D_UNIX)
	add_link_options(-pthread)
    set(SHARED_SOURCES ${SHARED_SOURCES} ${LINUX_SOURCES})
	set(NOT_COMPILED_SOURCES ${NOT_COMPILED_SOURCES} ${WIN32_SOURCES})
endif()

# Set up flags for MSVC
if (MSVC)
	set(CMAKE_CXX_FLAGS "/MP ${CMAKE_CXX_FLAGS}") # Use all cores for compilation
	set(CMAKE_CXX_FLAGS "/wd4267 ${CMAKE_CXX_FLAGS}") #warning C4267 conversion from 'size_t' to 'int', possible loss of data
	set(CMAKE_CXX_FLAGS "/wd4244 ${CMAKE_CXX_FLAGS}") #warning C4244: 'return': conversion from '__int64' to 'int', possible loss of data
	set(CMAKE_CXX_FLAGS "/wd4390 ${CMAKE_CXX_FLAGS}") #warning C4390: ';': empty controlled statement found; is this the intent?
	set(CMAKE_CXX_FLAGS "/wd4101 ${CMAKE_CXX_FLAGS}") #warning C4101: 'org_gamma': unreferenced local variable
	set(CMAKE_CXX_FLAGS "/wd4102 ${CMAKE_CXX_FLAGS}") #warning C4102: 'RePaintNewmenu4': unreferenced label
	set(CMAKE_CXX_FLAGS "/wd4018 ${CMAKE_CXX_FLAGS}") #warning C4018: '>': signed/unsigned mismatch
	set(CMAKE_CXX_FLAGS "/wd4715 ${CMAKE_CXX_FLAGS}") #warning C4715: 'get_num_faces': not all control paths return a value
	set(CMAKE_CXX_FLAGS "/wd4838 ${CMAKE_CXX_FLAGS}") #warning C4838: conversion from 'unsigned int' to 'long' requires a narrowing conversion
	set(CMAKE_CXX_FLAGS "/wd4309 ${CMAKE_CXX_FLAGS}") #warning C4309: 'argument': truncation of constant value
	add_definitions(-D_CRT_SECURE_NO_WARNINGS=1) # Suppress warning about insecure strxxx functions
endif()

add_executable(ChocolateDescent WIN32 MACOSX_BUNDLE ${SHARED_SOURCES} ${NOT_COMPILED_SOURCES} ${DESCENT1_SOURCES})
add_executable(ChocolateDescent2 WIN32 MACOSX_BUNDLE ${SHARED_SOURCES} ${NOT_COMPILED_SOURCES} ${DESCENT2_SOURCES})

if (BUILD_EDITOR)
	target_compile_definitions(ChocolateDescent PUBLIC "$<$<CONFIG:DEBUG>:EDITOR>")
endif()

set_target_properties(ChocolateDescent ChocolateDescent2 PROPERTIES CXX_STANDARD 11)

target_link_libraries(ChocolateDescent ${SHARED_LIBS} ${DESCENT1_LIBS})
target_link_libraries(ChocolateDescent2 ${SHARED_LIBS} ${DESCENT2_LIBS})

target_compile_definitions(ChocolateDescent2 PUBLIC BUILD_DESCENT2)
target_compile_definitions(ChocolateDescent PUBLIC "$<$<CONFIG:RELEASE>:RELEASE>")
target_compile_definitions(ChocolateDescent2 PUBLIC "$<$<CONFIG:RELEASE>:RELEASE>")
target_compile_definitions(ChocolateDescent PUBLIC "$<$<CONFIG:MINSIZEREL>:RELEASE>")
target_compile_definitions(ChocolateDescent2 PUBLIC "$<$<CONFIG:MINSIZEREL>:RELEASE>")
target_compile_definitions(ChocolateDescent PUBLIC "$<$<CONFIG:RELWITHDEBINFO>:RELEASE>")
target_compile_definitions(ChocolateDescent2 PUBLIC "$<$<CONFIG:RELWITHDEBINFO>:RELEASE>")

set_source_files_properties(${NOT_COMPILED_SOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)

source_group("2d" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/2d/.+")
source_group("3d" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/3d/.+")
source_group("cfile" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/cfile/.+")
source_group("conf" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/conf/.+")
source_group("fix" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/fix/.+")
source_group("iff" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/iff/.+")
source_group("main_d1" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/main_d1/.+")
source_group("main_d1\\editor" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/main_d1/editor/.+")
source_group("main_d2" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/main_d2/.+")
source_group("mem" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/mem/.+")
source_group("misc" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/misc/.+")
source_group("mve" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/mve/.+")
source_group("platform" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/platform/.+")
source_group("platform\\fluidsynth" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/platform/fluidsynth/.+")
source_group("platform\\openal" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/platform/openal/.+")
source_group("platform\\sdl" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/platform/sdl/.+")
source_group("platform\\win32" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/platform/win32/.+")
source_group("ui" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/ui/.+")
source_group("texmap" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/texmap/.+")
source_group("vecmat" REGULAR_EXPRESSION "${CMAKE_CURRENT_SOURCE_DIR}/vecmat/.+")
