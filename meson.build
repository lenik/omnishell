project('omnishell', 'cpp',
  version : '1.0.2',
  default_options : ['warning_level=0', 'cpp_std=c++17'])

add_languages('c', native : false)

pkgdatadir = get_option('datadir') / meson.project_name()

fs = import('fs')
cpp = meson.get_compiler('cpp')
c = meson.get_compiler('c')

# libbas
    bas_c_dep = dependency('bas-c', required : true)
    bas_cpp_dep = dependency('bas-cpp', required : true)
    bas_ui_dep = dependency('bas-ui', required : true)
    date_tz_lib = cpp.find_library('date-tz', required : true)
    threads_dep = dependency('threads')
    date_dep = declare_dependency(dependencies: [threads_dep, date_tz_lib])
    openssl_dep = dependency('openssl', required : true)
    curl_dep = dependency('libcurl', required : true)

# boost libraries
    boost_system_dep = dependency('boost', modules: ['system'], required: true)

    # wxWidgets dependencies (must use a single GTK toolkit to avoid "GTK+ 2.x and GTK+ 3 in the same process" error)
    wx_dep = dependency('wxwidgets', method: 'config-tool', required: true)

    # Decide whether we need vendored Boost.JSON; actual boost_dep is defined later
    # (Meson requires add_project_arguments() to happen before any build targets).
    use_vendored_boost_json = (not boost_system_dep.found() or (boost_system_dep.version() < '1.68' or wx_dep.version() < '3.1'))
    if use_vendored_boost_json
        boost_inc = [
            include_directories('third_party/boost_json/include'),
            include_directories('third_party/boost_core/include'),
            include_directories('third_party/boost_assert/include'),
        ]
    else
        boost_inc = []
    endif

    # Detect toolkit from default wx-config so we link only gtk2 or only gtk3
    wx_config = find_program('wx-config', required: true)
    wx_libs_out = run_command(wx_config, '--libs', check: true, capture: true).stdout()
    wx_use_gtk3 = wx_libs_out.contains('gtk3')
    if wx_use_gtk3
        wx_gtk_prefix = 'wx_gtk3u'
        wx_ver = '3.0'
    else
        wx_gtk_prefix = 'wx_gtk2u'
        wx_ver = '3.1'
    endif
    # wxWidgets media (wxMediaCtrl) and extras - use same toolkit as core
    # Media/webview optional on GTK2 (often not built for gtk2)
    wx_media_lib = cpp.find_library(wx_gtk_prefix + '_media-' + wx_ver, required: false)
    wx_richtext_lib = cpp.find_library(wx_gtk_prefix + '_richtext-' + wx_ver, required: true)
    wx_stc_lib = cpp.find_library(wx_gtk_prefix + '_stc-' + wx_ver, required: true)
    wx_webview_lib = cpp.find_library(wx_gtk_prefix + '_webview-' + wx_ver, required: true)

bison = find_program('bison', required : true)
flex = find_program('flex', required : true)

zash_bison = custom_target(
    'zash_bison',
    input : 'src/cli/zash/zash.y',
    output : ['zash.tab.c', 'zash.tab.h'],
    command : [bison, '-d', '-o', '@OUTPUT0@', '@INPUT@'],
)

zash_flex = custom_target(
    'zash_flex',
    input : 'src/cli/zash/zash.lex',
    output : 'lex.yy.c',
    command : [flex, '-o', '@OUTPUT0@', '@INPUT@'],
    depends : zash_bison,
)

zash_sources = files(
    'src/cli/zash/parser/zash_alloc.c',
    'src/cli/zash/zash_driver.c',
    'src/cli/zash/zash_interpreter.cpp',
    'src/cli/zash/builtins/builtins.cpp',
)

zash_inc = [
    include_directories('src/cli/zash'),
    include_directories('src/cli/zash/parser'),
]

# wx_dep = dependency('wxwidgets', modules : [
#     'core',
#     'media',
#     'richtext',
#     'stc',
#     'webview',
#     ], required : true)

    if wx_dep.version() < '3.1'
        # wx 3.0: media and webview are in separate libs; core setup has wxUSE_*=0, so enable for our build
        wx_defines = ['-DwxUSE_MEDIACTRL=1', '-DwxUSE_WEBVIEW=1']
        add_project_arguments(wx_defines, language: 'cpp')
    endif

# Disable all warnings (wx 3.0 and deps are noisy)
cpp_nowarn = cpp.get_supported_arguments(['-w', '/w'])
cpp_nowarn = cpp_nowarn + wx_defines

# Boost dependency (needs to be declared after add_project_arguments()).
if use_vendored_boost_json
    boost_json_lib = static_library(
        'boost_json',
        'third_party/boost_json/src/src.cpp',
        include_directories: boost_inc,
    )
    boost_dep = declare_dependency(
        include_directories: boost_inc,
        link_with: boost_json_lib,
        dependencies: [
            dependency('boost', modules : [
                'locale',
                'system',
            ], required : true),
        ],
    )
else
    boost_dep = dependency('boost', modules : [
        'assert',
        'core',
        'json',
        'locale',
        'system',
    ], required : true)
endif

# Precompiled header for wxWidgets
wx_pch = 'src/wx_pch.h'

# Source files - reorganized structure
cpp_sources = files(
  'src/core/Module.cpp',
  'src/core/ModuleRegistry.cpp',
  'src/core/Category.cpp',
  'src/core/Process.cpp',
  'src/core/registry/IRegistry.cpp',
  'src/core/registry/RegistryValueIo.cpp',
  'src/core/registry/RegistryKeyUtil.cpp',
  'src/core/registry/LocalRegistry.cpp',
  'src/core/registry/JsonRegistry.cpp',
  'src/core/registry/VolumeRegistry.cpp',
  'src/core/registry/FileAssociations.cpp',
  'src/core/registry/RegistryService.cpp',
  'src/core/App.cpp',
  'src/core/VolUrl.cpp',
  'src/core/TrayIcon.cpp',
  'src/core/ServiceManager.cpp',
  'src/daemon/ApiDirIndex.cpp',
  'src/daemon/ApiFileController.cpp',
  'src/daemon/ApiVolumeIndex.cpp',
  'src/daemon/AssetController.cpp',
  'src/daemon/DirIndex.cpp',
  'src/daemon/FileController.cpp',
  'src/daemon/FileIndex.cpp',
  'src/daemon/HttpDaemon.cpp',
  'src/daemon/VolumeIndex.cpp',
  'src/shell/DesktopWindow.cpp',
  'src/shell/Taskbar.cpp',
  'src/shell/StartMenu.cpp',
  'src/shell/Shell.cpp',
  'src/shell/VfsDaemonHost.cpp',
  'src/shell/VfsWebViewHandlers.cpp',
  'src/mod/notepad/NotepadApp.cpp',
  'src/mod/notepad/NotepadFrame.cpp',
  'src/mod/notepad/NotepadBody.cpp',
  'src/mod/controlpanel/ControlPanelApp.cpp',
  'src/mod/controlpanel/ControlPanelFrame.cpp',
  'src/mod/controlpanel/ControlPanelBody.cpp',
  'src/mod/explorer/ExplorerApp.cpp',
  'src/mod/explorer/ExplorerFrame.cpp',
  'src/mod/explorer/ExplorerBody.cpp',
  'src/mod/browser/BrowserApp.cpp',
  'src/mod/browser/BrowserFrame.cpp',
  'src/mod/browser/BrowserBody.cpp',
  'src/mod/calculator/CalculatorApp.cpp',
  'src/mod/calculator/CalculatorBody.cpp',
  'src/mod/calculator/CalculatorFrame.cpp',
  'src/mod/mediaplayer/MediaPlayerApp.cpp',
  'src/mod/mediaplayer/MediaPlayerFrame.cpp',
  'src/mod/mediaplayer/MediaPlayerBody.cpp',
  'src/mod/registry/RegistryBody.cpp',
  'src/mod/registry/RegistryFrame.cpp',
  'src/mod/registry/RegistryApp.cpp',
  'src/mod/calendar/CalendarBody.cpp',
  'src/mod/calendar/CalendarFrame.cpp',
  'src/mod/calendar/CalendarApp.cpp',
  'src/mod/paint/PaintBody.cpp',
  'src/mod/paint/PaintFrame.cpp',
  'src/mod/paint/PaintApp.cpp',
  'src/mod/sudoku/SudokuApp.cpp',
  'src/mod/sudoku/SudokuFrame.cpp',
  'src/mod/sudoku/SudokuBody.cpp',
  'src/mod/solitaire/SolitaireApp.cpp',
  'src/mod/solitaire/SolitaireFrame.cpp',
  'src/mod/solitaire/SolitaireBody.cpp',
  'src/mod/snippingtool/SnippingToolApp.cpp',
  'src/mod/snippingtool/SnippingToolBody.cpp',
  'src/mod/snippingtool/SnippingToolFrame.cpp',
  'src/mod/soundrecorder/SoundRecorderApp.cpp',
  'src/mod/soundrecorder/SoundRecorderBody.cpp',
  'src/mod/soundrecorder/SoundRecorderFrame.cpp',
  'src/mod/camera/CameraApp.cpp',
  'src/mod/camera/CameraFrame.cpp',
  'src/mod/camera/CameraBody.cpp',
  'src/mod/camera/CameraGstPreview.cpp',
  'src/mod/photoviewer/PhotoViewerApp.cpp',
  'src/mod/photoviewer/PhotoViewerFrame.cpp',
  'src/mod/photoviewer/PhotoViewerBody.cpp',
  'src/mod/musicbox/MusicBoxApp.cpp',
  'src/mod/musicbox/MusicBoxFrame.cpp',
  'src/mod/musicbox/MusicBoxBody.cpp',
  'src/mod/fiveormore/FiveOrMoreApp.cpp',
  'src/mod/fiveormore/FiveOrMoreFrame.cpp',
  'src/mod/fiveormore/FiveOrMoreBody.cpp',
  'src/mod/minesweeper/MinesweeperApp.cpp',
  'src/mod/minesweeper/MinesweeperFrame.cpp',
  'src/mod/minesweeper/MinesweeperBody.cpp',
  'src/mod/stopwatch/StopWatchApp.cpp',
  'src/mod/stopwatch/StopWatchFrame.cpp',
  'src/mod/stopwatch/StopWatchBody.cpp',
  'src/mod/taskmanager/TaskManagerApp.cpp',
  'src/mod/taskmanager/TaskManagerBody.cpp',
  'src/mod/taskmanager/TaskManagerFrame.cpp',
  'src/mod/passwordmanager/LoginDialog.cpp',
  'src/mod/passwordmanager/PasswordManager.cpp',
  'src/mod/background/BackgroundSettingsApp.cpp',
  'src/mod/background/BackgroundSettingsFrame.cpp',
  'src/mod/background/BackgroundSettingsBody.cpp',
  'src/ui/Location.cpp',
  'src/ui/LocationHistory.cpp',
  'src/ui/widget/BreadcrumbNav.cpp',
  'src/ui/widget/DirTreeView.cpp',
  'src/ui/widget/FileListView.cpp',
  'src/ui/IconTheme.cpp',
  'src/ui/dialogs/ChooseFileDialog.cpp',
  'src/ui/dialogs/ChooseFolderDialog.cpp',
  'src/ui/dialogs/ChoosePrinterDialog.cpp',
  'src/wx/LabeledIcon.cpp',
  'src/wx/WxChecked.cpp',
  'src/wx/wxTerminal.cpp',
  'src/wx/wxConsole.cpp',
  'src/wx/term/xterm.cpp',
  'src/mod/console/ConsoleApp.cpp',
  'src/mod/console/ConsoleFrame.cpp',
  'src/mod/console/ConsoleBody.cpp',
)

# Assets will be zipped and linked as a section in the executable
all_sources = cpp_sources

# Build assets.zip every time (no file dependency)
assets_dir = meson.current_source_dir() / 'assets'
build_dir = meson.current_build_dir()
assets_zip = custom_target(
    'assets_zip',
    output : 'assets.zip',
    input : [],
    command : [
        'sh', '-c',
        'dst=$(readlink -f "@1@"); cd "@0@" && zip -r "$dst" .'.format(assets_dir, '@OUTPUT@')
    ],
    #build_always : true,
    build_by_default : true,
    install : false,
)

# Compile assembly file that embeds assets.zip using .incbin
# Use configure_file to copy assets.s to build directory
assets_s = configure_file(
    input : 'app/assets.s',
    output : 'assets.s',
    copy : true,
)

# Compile the assembly file (assets.zip will be in the same directory)
as = find_program('as', required : true)
assets_o = custom_target(
    'assets_o',
    output : 'assets.o',
    input : [assets_s, assets_zip],
    command : [
        as,
        '-o', '@OUTPUT@',
        '@INPUT0@'
    ],
    depends : assets_zip,
)

exe_inc = [
    include_directories('src'),
    include_directories('src/daemon'),
    include_directories('.'),
]

exe_deps = [
    bas_c_dep,
    bas_cpp_dep,
    bas_ui_dep,
    boost_dep,
    openssl_dep,
    curl_dep,
    wx_dep,
    wx_webview_lib,
]

have_wx_media = wx_media_lib.found()
lib_omnishell_cpp_args = cpp_nowarn + ['-DLOGGER_NAME=omni']
lib_omnishell_deps = exe_deps + [threads_dep]
if host_machine.system() != 'windows'
    util_lib = cpp.find_library('util', required : true)
    lib_omnishell_deps += util_lib
endif
if have_wx_media
    lib_omnishell_deps += [wx_media_lib]
    lib_omnishell_cpp_args += ['-DHAVE_WX_MEDIA=1']
endif

alsa_dep = cpp.find_library('asound', required: false)
if host_machine.system() == 'linux' and alsa_dep.found()
    lib_omnishell_deps += alsa_dep
    lib_omnishell_cpp_args += ['-DOMNISHELL_HAVE_ALSA=1']
endif

gst_dep = dependency('gstreamer-1.0', required: false)
gst_app_dep = dependency('gstreamer-app-1.0', required: false)
if gst_dep.found() and gst_app_dep.found()
    lib_omnishell_deps += [gst_dep, gst_app_dep]
    lib_omnishell_cpp_args += ['-DOMNISHELL_HAVE_GSTREAMER=1']
endif

# if wx_dep.version() < '3.1'
#     exe_deps += [
#         wx_media_lib,
#         wx_richtext_lib,
#         wx_stc_lib,
#         wx_webview_lib,
#     ]
# endif

lib_omnishell = library('omnishell',
    cpp_sources,
    zash_sources,
    zash_bison,
    zash_flex,
    'src/module.cpp',
    dependencies : lib_omnishell_deps,
    include_directories : exe_inc + zash_inc,
    c_args : c.get_supported_arguments(['-Wno-unused-parameter']),
    cpp_args : lib_omnishell_cpp_args,
    cpp_pch: wx_pch,
    link_args : ['-rdynamic'],
    override_options: ['b_lundef=false'], # Allows undefined symbols at link time
    version : meson.project_version(),
    soversion : meson.project_version().split('.')[0],
    install : true,
)

omni_dep_list = [boost_dep, date_dep, openssl_dep, curl_dep, wx_webview_lib]
if have_wx_media
    omni_dep_list += [wx_media_lib]
endif

omnishell_dep = declare_dependency(
    link_with: lib_omnishell,
    include_directories: exe_inc,
    dependencies: omni_dep_list,
)

# ---- pkg-config ----
pkgconfig = import('pkgconfig')
pkgconfig.generate(
    lib_omnishell,
    name : 'omnishell',
    description : 'omnishell',
    version : meson.project_version(),
    subdirs : 'omnishell',
    requires : ['bas-c', 'bas-cpp', 'bas-ui', 'boost', 'openssl', 'libcurl'],
)

# ---- Headers for libbas-c package (includedir/bas-c) ----
find_other_cmd = run_command('find', 'src',
          '-name', '*.h',
    '-o', '-name', '*.hpp',
    '-o', '-type', 'd',
    '-o', '-print',
    check: true)
other_files = find_other_cmd.stdout().strip().split('\n')

other_list = []
foreach f : other_files
    other_list += f.substring(4)
endforeach

install_subdir('src',
    install_dir : get_option('includedir') / 'omnishell',
    exclude_files : other_list,
    strip_directory : true,
)

# Unit tests generator
# Find all *_test.cpp files and build them automatically using the 'includes' utility
# Similar to Makefile pattern rule: %_test: %_test.cpp 
#   sources: %_test.cpp $(includes -e -c %_test.cpp)
#   cmdline: $(CXX) $(CXXFLAGS) -o $@ $*_test.cpp `includes -c $*_test.cpp` -licuuc -licui18n

# List of all test source files (manually enumerated since Meson lacks fs module)
# This list is automatically generated from: find src -name '*_test.cpp' | sort
all_test_files = [
]

# Build each test using the 'includes' utility to get source dependencies
foreach test_rel_path : all_test_files
    test_cpp = 'src/' + test_rel_path
    
    # Extract test name (e.g., ByteBuffer_test from src/io/ByteBuffer_test.cpp)
    path_parts = test_rel_path.split('/')
    test_file = path_parts[path_parts.length() - 1]
    test_name = test_file.replace('.cpp', '')
    
    # Use 'includes' utility to get implementation dependencies
    # 'includes -e -c test.cpp' returns source files, one per line (not -I include paths)
    includes_result = run_command('includes', '-e', '-c', test_cpp, check : false)
    
    impl_deps = []
    if includes_result.returncode() == 0
        # Parse the output: each line is a .c/.cpp file path
        output = includes_result.stdout()
        lines = output.split('\n')
        foreach line : lines
            line = line.strip()
            # Keep non-empty lines that are .c/.cpp files
            if line.endswith('.cpp')
                impl_deps += line
            endif
        endforeach
    endif
    
    # Combine test file with implementation dependencies from 'includes'
    all_sources = [test_cpp] + impl_deps
    
    # Create the executable (LOGGER_NAME so program.h and logger.c work in tests)
    test_exe = executable(test_name,
        all_sources,
        assets_o,
        'src/module.cpp',
        include_directories : exe_inc,
        dependencies : exe_deps + [omnishell_dep],
        cpp_args : cpp_nowarn + ['-DLOGGER_NAME=' + test_name],
        cpp_pch: wx_pch,
        install : false,
    )
    
    # Register the test with meson
    test(test_name, test_exe)
endforeach

# --- Applications in app/ (one .cpp = one program), install to bindir + manpages to mandir ---
find_app = run_command('find', 'app', '-maxdepth', '1', '-name', '*.cpp', check : false)
app_cpp_list = []
if find_app.returncode() == 0
    foreach line : find_app.stdout().strip().split('\n')
        if line != ''
            app_cpp_list += line
        endif
    endforeach
endif

foreach app_cpp : app_cpp_list
    app_name = app_cpp.split('/')[-1].replace('.cpp', '')
    if app_name.startswith('os')
        install_dir = get_option('bindir')
    else
        install_dir = pkgdatadir / 'app'
    endif
    executable(app_name,
        app_cpp,
        assets_o,
        'src/module.cpp',
        include_directories : exe_inc,
        dependencies : exe_deps + [omnishell_dep],
        cpp_args : cpp_nowarn + ['-DLOGGER_NAME=' + app_name],
        cpp_pch: wx_pch,
        install : true,
        install_dir : install_dir,
    )
endforeach

# Manpages: app/*.1 -> install to mandir
find_man = run_command('find', 'app', '-maxdepth', '1', '-name', '*.1', check : false)
if find_man.returncode() == 0
    foreach line : find_man.stdout().strip().split('\n')
        if line == ''
            continue
        endif
        install_man(line)
    endforeach
endif

# ---- Bash completion (install to datadir/bash-completion/completions) ----
find_bash = run_command('find', 'app', '-maxdepth', '1', '-name', '*.bash', check : false)
app_bash_list = []
if find_bash.returncode() == 0
    foreach line : find_bash.stdout().strip().split('\n')
        if line == ''
            continue
        endif
        completion_name = fs.replace_suffix(line.split('/')[-1], '')
        install_data(
            line,
            rename : completion_name,
            install_dir : get_option('datadir') / 'bash-completion' / 'completions',
            install_mode : 'rw-r--r--',
        )
    endforeach
endif

