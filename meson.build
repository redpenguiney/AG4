project(
    'AG4_meson', 
    'cpp', 
    default_options : [
        'warning_level=0', 
        'cpp_std=c++20', 
        'c_std=17',
    ],
)

cppcompiler = meson.get_compiler('cpp')

add_global_arguments('-DSOL_ALL_SAFETIES_ON', language : 'cpp')
add_global_arguments('-DGLM_FORCE_XYZW_ONLY', language : 'cpp')
add_global_arguments('-DGLM_ENABLE_EXPERIMENTAL', language : 'cpp')

engine_source_files_grabbed = run_command('./meson_file_grabber.bat', './Engine', '*.cpp', '', check: true, env: environment())
engine_source_files = engine_source_files_grabbed.stdout().strip().split('\n')

#message('Source files',engine_source_files)

vcpkg_include_location = get_option('vcpkg_include_path')
if vcpkg_include_location == '' 
    error('please provide vcpkg include path')
endif

engine_dependency_include_dirs = [
    include_directories('.', 'Engine', vcpkg_include_location)
]

vcpkg_install_location = get_option('vcpkg_triplet_path')
if vcpkg_install_location == '' 
    error('please provide vcpkg triplet path')
endif

vcpkg_libs_grabbed = run_command('./meson_file_grabber_absolute.bat', vcpkg_install_location / 'lib', '*.lib', check: true, env: environment())
vcpkg_libs = vcpkg_libs_grabbed.stdout().strip().split('\n')

#message('Lib files', vcpkg_libs)

vcpkg_dlls_grabbed = run_command('./meson_file_grabber_absolute.bat', vcpkg_install_location / 'bin', '*.dll', check: true, env: environment())
vcpkg_dlls = vcpkg_dlls_grabbed.stdout().strip().split('\n')

#message('Dll files', vcpkg_dlls)

engine_vcpkg_lib_dependency = declare_dependency(
    link_args: vcpkg_libs, 
)

fs = import('fs')
foreach dllpath : vcpkg_dlls
    dllname = fs.name(dllpath)
    configure_file(
        input: dllpath,
        output: dllname,
        copy: true
    )
endforeach

engine_dependencies = [
    engine_vcpkg_lib_dependency
]

AG4_Engine_lib = static_library(
    'Engine_meson', 
    sources: engine_source_files, 
    dependencies: engine_dependencies,
    include_directories : engine_dependency_include_dirs
)

AG4_Engine_dep = declare_dependency(
    #link_args: vcpkg_libs,
    link_with: [AG4_Engine_lib]
)

game_src_dir = get_option('game_source_path')
if game_src_dir == '' 
    error('please provide game source path')
endif

game_source_files_grabbed = run_command('./meson_file_grabber.bat', game_src_dir, '*.cpp', '', check: true, env: environment())
game_source_files = game_source_files_grabbed.stdout().strip().split('\n')

if get_option('game_name') == '' 
    error('please provide game name')
endif

game = executable(
    get_option('game_name'),
    include_directories : engine_dependency_include_dirs,
    dependencies: [AG4_Engine_dep, engine_vcpkg_lib_dependency],
    sources: game_source_files
)