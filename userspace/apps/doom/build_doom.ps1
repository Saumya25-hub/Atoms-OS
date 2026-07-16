$doom_src = @("am_map.c", "doomdef.c", "doomstat.c", "dstrings.c", "d_event.c", "d_items.c", "d_iwad.c", "d_loop.c", "d_main.c", "d_mode.c", "d_net.c", "f_finale.c", "f_wipe.c", "g_game.c", "hu_lib.c", "hu_stuff.c", "info.c", "i_cdmus.c", "i_endoom.c", "i_joystick.c", "i_scale.c", "i_sound.c", "i_system.c", "i_timer.c", "memio.c", "m_argv.c", "m_bbox.c", "m_cheat.c", "m_config.c", "m_controls.c", "m_fixed.c", "m_menu.c", "m_misc.c", "m_random.c", "p_ceilng.c", "p_doors.c", "p_enemy.c", "p_floor.c", "p_inter.c", "p_lights.c", "p_map.c", "p_maputl.c", "p_mobj.c", "p_plats.c", "p_pspr.c", "p_saveg.c", "p_setup.c", "p_sight.c", "p_spec.c", "p_switch.c", "p_telept.c", "p_tick.c", "p_user.c", "r_bsp.c", "r_data.c", "r_draw.c", "r_main.c", "r_plane.c", "r_segs.c", "r_sky.c", "r_things.c", "sha1.c", "sounds.c", "statdump.c", "st_lib.c", "st_stuff.c", "s_sound.c", "tables.c", "v_video.c", "wi_stuff.c", "w_checksum.c", "w_file.c", "w_main.c", "w_wad.c", "z_zone.c", "w_file_stdc.c", "i_input.c", "i_video.c", "doomgeneric.c", "dummy.c")

$objs = @()
$build_dir = "d:\Signatures_OS\build"

foreach ($file in $doom_src) {
    $obj = "$build_dir\$($file -replace '\.c$', '.o')"
    $objs += $obj
    Write-Host "Compiling $file..."
    clang -target x86_64-pc-none-elf -ffreestanding -nostdlib -O2 -I"d:\Signatures_OS\userspace\apps\doom\libc" -c "d:\Signatures_OS\userspace\apps\doom\src\doomgeneric\$file" -o $obj
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

Write-Host "Compiling libc_impl..."
clang -target x86_64-pc-none-elf -ffreestanding -nostdlib -O2 -I"d:\Signatures_OS\userspace\apps\doom\libc" -c "d:\Signatures_OS\userspace\apps\doom\libc_impl.c" -o "$build_dir\libc_impl.o"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$objs += "$build_dir\libc_impl.o"

Write-Host "Compiling wrappers..."
clang -target x86_64-pc-none-elf -ffreestanding -nostdlib -O2 -I"d:\Signatures_OS\userspace\apps\doom\libc" -c "d:\Signatures_OS\userspace\apps\doom\doomgeneric_signaturesos.c" -o "$build_dir\doomgeneric_signaturesos.o"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
$objs += "$build_dir\doomgeneric_signaturesos.o"

# Need libbos
$objs += "$build_dir\syscalls.o"
$objs += "$build_dir\syscalls_gui.o"
$objs += "$build_dir\widgets.o"
$objs += "$build_dir\bos_gui.o"
$objs += "$build_dir\bpde.o"

Write-Host "Linking DOOM..."
ld.lld -T d:\Signatures_OS\userspace\linker.ld --strip-all $objs -o $build_dir\doom.elf
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "DOOM built successfully!"
