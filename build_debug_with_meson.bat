@REM first argument (%1) = game name
meson setup meson_%1_debug -Dbuildtype=debug -Dvcpkg_triplet_path="./vcpkg_installed/x64-windows/x64-windows/debug/" -Dvcpkg_include_path="./vcpkg_installed/x64-windows/x64-windows/include/" -Dgame_name=%1 -Dgame_source_path=%1
meson compile -C meson_%1_debug