project('openMenu', 'c',
  version : '0.1',
  default_options : ['c_std=gnu11','b_staticpic=false'])

# Compiler arguments
arg_base = ['-w']
arg_code = []
arg_linker = []

# Dependencies
cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)
#libintrafont_dep = dependency('libintrafont', fallback : ['libintrafont', 'libintrafont_dep'])

# Options
#opt_renderer = get_option('renderer')
#opt_platform = get_option('platform')
opt_platform = 'dc'

inc_base = []
#inc_base += include_directories('clay')
#inc_base += include_directories('external')

src_main = [ 'main.c' ]
src_external = [ 'external/easing.c' ]
src_texture = ['texture/block_pool.c', 'texture/lru.c', 'texture/txr_manager.c',
  'texture/dat_reader.c', 'texture/serial_sanitize.c', 'texture/simple_texture_allocator.c' ]
src_backend = ['external/ini.c', 'backend/gd_list.c', 'backend/gdemu_sdk.c',
  'backend/gdemu_control.c', 'backend/db_list.c']
src_ui_menus = [ 'ui/ui_bios.c', 'ui/ui_gdmenu.c', 'ui/ui_line_large.c', 'ui/ui_line_desc.c',
  'ui/ui_grid.c', 'ui/global_settings.c', 'ui/ui_menu_credits.c', 'ui/theme_manager.c' ]
src_ui = [ 'ui/dc/font_bios.c', 'ui/dc/font_bmf.c', 'ui/dc/font_bitmap.c', 'ui/dc/pvr_texture.c',
  'ui/dc/input.c', 'ui/draw_kos.c', 'ui/animation.c' ]

src_files = [ src_main, src_external, src_texture, src_backend, src_ui_menus, src_ui]

# Determine Platform
platform_dep = []

platform_inc = []
#platform_inc += include_directories('platforms/shared')

#platform_src = ['clay_impl.c']
platform_src = []
#platform_src += ['platforms/shared/input.c']

renderer_inc = []

if opt_platform == 'dc'
  arg_code += ['-DPLATFORM_DC', '-DNDEBUG']
  arg_code += ['-DPATH_ASSETS="/cd/"', '-DPATH_USERDATA="/pc/"']
  arg_base += ['-mpretend-cmove','-fsingle-precision-constant', '-ffast-math', '-O2', '-g0',
                 '-fomit-frame-pointer', '-finline-functions',  '-mfsrra', '-mfsca',
                 '-ffunction-sections', '-fdata-sections' ]

  dc_debug_dep = declare_dependency(compile_args: [], link_args :  ['-g3', '-Wl,-Map=output.map'])
  platform_dep += [dc_debug_dep]
  #platform_inc += include_directories('platforms/dreamcast')

  libcrayon_dep = declare_dependency(
    dependencies : cc.find_library('crayon_vmu', dirs : [meson.current_source_dir() + '/lib']),
    include_directories : include_directories('external/libcrayonvmu'))
  platform_dep += [ libcrayon_dep ]

else
  error('No platform chosen!')
endif

inc = [ inc_base, platform_inc, renderer_inc ]
src = [ src_files ]
dep = [ m_dep, platform_dep ]

arg_c = [ arg_base, arg_code ]

exe_themeMenu = executable('themeMenu',
           c_args: arg_c,
           link_args: arg_linker,
           sources: src,
           include_directories : inc,
           dependencies: dep,
           install : true)

# Post processing
# Dreamcast
if opt_platform == 'dc'
  dc_binary = '1ST_READ.BIN'
  dc_cdi = 'disc.cdi'

  target_to_pkg = exe_themeMenu

  bin_script_path = join_paths(meson.current_source_dir(), 'pkg_dreamcast.sh')
  custom_target(dc_binary,
    depends: target_to_pkg,
    input : target_to_pkg,
    output : dc_binary,
    command : [bin_script_path, '@INPUT@', '@OUTPUT@'],
    build_by_default : true)

  cdi_script_path = join_paths(meson.current_source_dir(), 'pkg_dreamcast_cdi.sh')
  custom_target(dc_cdi,
    depends: target_to_pkg,
    input : target_to_pkg,
    output : dc_cdi,
    command : [cdi_script_path, '@INPUT@', '@OUTPUT@'],
    build_by_default : true)
endif
