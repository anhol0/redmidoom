cmake_minimum_required(VERSION 3.20)

project(redmidoom VERSION 0.1 LANGUAGES C)

set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR mipsel)
set(CMAKE_C_COMPILER mipsel-openwrt-linux-musl-gcc)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(DOOMGENERIC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doomgeneric/doomgeneric")

set(DOOMGENERIC_SOURCES
    ${DOOMGENERIC_DIR}/dummy.c
    ${DOOMGENERIC_DIR}/am_map.c
    ${DOOMGENERIC_DIR}/doomdef.c
    ${DOOMGENERIC_DIR}/doomstat.c
    ${DOOMGENERIC_DIR}/dstrings.c
    ${DOOMGENERIC_DIR}/d_event.c
    ${DOOMGENERIC_DIR}/d_items.c
    ${DOOMGENERIC_DIR}/d_iwad.c
    ${DOOMGENERIC_DIR}/d_loop.c
    ${DOOMGENERIC_DIR}/d_main.c
    ${DOOMGENERIC_DIR}/d_mode.c
    ${DOOMGENERIC_DIR}/d_net.c
    ${DOOMGENERIC_DIR}/f_finale.c
    ${DOOMGENERIC_DIR}/f_wipe.c
    ${DOOMGENERIC_DIR}/g_game.c
    ${DOOMGENERIC_DIR}/hu_lib.c
    ${DOOMGENERIC_DIR}/hu_stuff.c
    ${DOOMGENERIC_DIR}/info.c
    ${DOOMGENERIC_DIR}/i_cdmus.c
    ${DOOMGENERIC_DIR}/i_endoom.c
    ${DOOMGENERIC_DIR}/i_joystick.c
    ${DOOMGENERIC_DIR}/i_scale.c
    ${DOOMGENERIC_DIR}/i_sound.c
    ${DOOMGENERIC_DIR}/i_system.c
    ${DOOMGENERIC_DIR}/i_timer.c
    ${DOOMGENERIC_DIR}/memio.c
    ${DOOMGENERIC_DIR}/m_argv.c
    ${DOOMGENERIC_DIR}/m_bbox.c
    ${DOOMGENERIC_DIR}/m_cheat.c
    ${DOOMGENERIC_DIR}/m_config.c
    ${DOOMGENERIC_DIR}/m_controls.c
    ${DOOMGENERIC_DIR}/m_fixed.c
    ${DOOMGENERIC_DIR}/m_menu.c
    ${DOOMGENERIC_DIR}/m_misc.c
    ${DOOMGENERIC_DIR}/m_random.c
    ${DOOMGENERIC_DIR}/p_ceilng.c
    ${DOOMGENERIC_DIR}/p_doors.c
    ${DOOMGENERIC_DIR}/p_enemy.c
    ${DOOMGENERIC_DIR}/p_floor.c
    ${DOOMGENERIC_DIR}/p_inter.c
    ${DOOMGENERIC_DIR}/p_lights.c
    ${DOOMGENERIC_DIR}/p_map.c
    ${DOOMGENERIC_DIR}/p_maputl.c
    ${DOOMGENERIC_DIR}/p_mobj.c
    ${DOOMGENERIC_DIR}/p_plats.c
    ${DOOMGENERIC_DIR}/p_pspr.c
    ${DOOMGENERIC_DIR}/p_saveg.c
    ${DOOMGENERIC_DIR}/p_setup.c
    ${DOOMGENERIC_DIR}/p_sight.c
    ${DOOMGENERIC_DIR}/p_spec.c
    ${DOOMGENERIC_DIR}/p_switch.c
    ${DOOMGENERIC_DIR}/p_telept.c
    ${DOOMGENERIC_DIR}/p_tick.c
    ${DOOMGENERIC_DIR}/p_user.c
    ${DOOMGENERIC_DIR}/r_bsp.c
    ${DOOMGENERIC_DIR}/r_data.c
    ${DOOMGENERIC_DIR}/r_draw.c
    ${DOOMGENERIC_DIR}/r_main.c
    ${DOOMGENERIC_DIR}/r_plane.c
    ${DOOMGENERIC_DIR}/r_segs.c
    ${DOOMGENERIC_DIR}/r_sky.c
    ${DOOMGENERIC_DIR}/r_things.c
    ${DOOMGENERIC_DIR}/sha1.c
    ${DOOMGENERIC_DIR}/sounds.c
    ${DOOMGENERIC_DIR}/statdump.c
    ${DOOMGENERIC_DIR}/st_lib.c
    ${DOOMGENERIC_DIR}/st_stuff.c
    ${DOOMGENERIC_DIR}/s_sound.c
    ${DOOMGENERIC_DIR}/tables.c
    ${DOOMGENERIC_DIR}/v_video.c
    ${DOOMGENERIC_DIR}/wi_stuff.c
    ${DOOMGENERIC_DIR}/w_checksum.c
    ${DOOMGENERIC_DIR}/w_file.c
    ${DOOMGENERIC_DIR}/w_main.c
    ${DOOMGENERIC_DIR}/w_wad.c
    ${DOOMGENERIC_DIR}/z_zone.c
    ${DOOMGENERIC_DIR}/w_file_stdc.c
    ${DOOMGENERIC_DIR}/i_input.c
    ${DOOMGENERIC_DIR}/i_video.c
    ${DOOMGENERIC_DIR}/doomgeneric.c
)

add_executable(redmidoom
    main.c
    doomgeneric_posix.c
    ${DOOMGENERIC_SOURCES}
)

target_include_directories(redmidoom PRIVATE ${DOOMGENERIC_DIR})

target_compile_features(redmidoom PRIVATE c_std_11)
target_compile_definitions(redmidoom PRIVATE
    CMAP256
    DOOMGENERIC_RESX=320
    DOOMGENERIC_RESY=200
    NORMALUNIX
    LINUX
    SNDSERV
    _DEFAULT_SOURCE
)

target_compile_options(redmidoom PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-Wall;-Wextra;-Wpedantic>
)

target_link_libraries(redmidoom PRIVATE m)
