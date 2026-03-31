project(
    'omnishell',
    'cpp',
    version: '1.0.2',
    default_options: ['warning_level=0', 'cpp_std=c++17'],
)

add_languages('c', native: false)

# Generate a compile-time config header with the project version.
conf = configuration_data()
conf.set('PROJECT_VERSION', meson.project_version())
conf.set('OMNISHELL_VERSION', meson.project_version())
configure_file(
    input: 'config.h.in',
    output: 'config.h',
    configuration: conf,
)

pkgdatadir = get_option('datadir') / meson.project_name()

fs = import('fs')
cpp = meson.get_compiler('cpp')
c = meson.get_compiler('c')

# libbas
bas_c_dep = dependency('bas-c', required: true)
bas_cpp_dep = dependency('bas-cpp', required: true)
bas_ui_dep = dependency('bas-ui', required: true)

boost_sub = subproject('boost')
boost_json_dep = boost_sub.get_variable('boost_json_dep')

bison = find_program('bison', required: true)
curl_dep = dependency('libcurl', required: true)
date_dep = subproject('date_tz').get_variable('date_dep')
flex = find_program('flex', required: true)
openssl_dep = dependency('openssl', required: true)

wx_sub = subproject('wx_logic')
wx_dep = wx_sub.get_variable('wx_dep')
wx_ver = wx_sub.get_variable('wx_ver')

zash_bison = custom_target(
    'zash_bison',
    input: 'src/cli/zash/zash.y',
    output: ['zash.tab.c', 'zash.tab.h'],
    command: [bison, '-d', '-o', '@OUTPUT0@', '@INPUT@'],
)

zash_flex = custom_target(
    'zash_flex',
    input: 'src/cli/zash/zash.lex',
    output: 'lex.yy.c',
    command: [flex, '-o', '@OUTPUT0@', '@INPUT@'],
    depends: zash_bison,
)

zash_sources = files(
    'src/cli/zash/builtins/builtins.cpp',
    'src/cli/zash/parser/zash_alloc.c',
    'src/cli/zash/zash_driver.c',
    'src/cli/zash/zash_interpreter.cpp',
)

zash_inc = [
    include_directories('src/cli/zash'),
    include_directories('src/cli/zash/parser'),
]

# Disable all warnings (wx 3.0 and deps are noisy)
cpp_nowarn = cpp.get_supported_arguments(['-w', '/w'])
cpp_nowarn = cpp_nowarn

# Boost dependency (needs to be declared after add_project_arguments()).

# Precompiled header for wxWidgets
wx_pch = 'src/wx_pch.h'

# Source files - reorganized structure
cpp_sources = files(
    'src/core/App.cpp',
    'src/core/Category.cpp',
    'src/core/Module.cpp',
    'src/core/ModuleRegistry.cpp',
    'src/core/Process.cpp',
    'src/core/ServiceManager.cpp',
    'src/core/TrayIcon.cpp',
    'src/core/VolUrl.cpp',
    'src/core/registry/FileAssociations.cpp',
    'src/core/registry/IRegistry.cpp',
    'src/core/registry/JsonRegistry.cpp',
    'src/core/registry/LocalRegistry.cpp',
    'src/core/registry/RegistryKeyUtil.cpp',
    'src/core/registry/RegistryService.cpp',
    'src/core/registry/RegistryValueIo.cpp',
    'src/core/registry/VolumeRegistry.cpp',
    'src/daemon/ApiDirIndex.cpp',
    'src/daemon/ApiFileController.cpp',
    'src/daemon/ApiVolumeIndex.cpp',
    'src/daemon/AssetController.cpp',
    'src/daemon/DirIndex.cpp',
    'src/daemon/FileController.cpp',
    'src/daemon/FileIndex.cpp',
    'src/daemon/HttpDaemon.cpp',
    'src/daemon/VolumeIndex.cpp',
    'src/mod/background/BackgroundSettingsApp.cpp',
    'src/mod/background/BackgroundSettingsBody.cpp',
    'src/mod/background/BackgroundSettingsFrame.cpp',
    'src/mod/browser/BrowserApp.cpp',
    'src/mod/browser/BrowserBody.cpp',
    'src/mod/browser/BrowserFrame.cpp',
    'src/mod/calculator/CalculatorApp.cpp',
    'src/mod/calculator/CalculatorBody.cpp',
    'src/mod/calculator/CalculatorFrame.cpp',
    'src/mod/calendar/CalendarApp.cpp',
    'src/mod/calendar/CalendarBody.cpp',
    'src/mod/calendar/CalendarFrame.cpp',
    'src/mod/camera/CameraApp.cpp',
    'src/mod/camera/CameraBody.cpp',
    'src/mod/camera/CameraFrame.cpp',
    'src/mod/camera/CameraGstPreview.cpp',
    'src/mod/console/ConsoleApp.cpp',
    'src/mod/console/ConsoleBody.cpp',
    'src/mod/console/ConsoleFrame.cpp',
    'src/mod/controlpanel/ControlPanelApp.cpp',
    'src/mod/controlpanel/ControlPanelBody.cpp',
    'src/mod/controlpanel/ControlPanelFrame.cpp',
    'src/mod/explorer/ExplorerApp.cpp',
    'src/mod/explorer/ExplorerBody.cpp',
    'src/mod/explorer/ExplorerFrame.cpp',
    # 'src/mod/fiveormore/FiveOrMoreApp.cpp',
    # 'src/mod/fiveormore/FiveOrMoreBody.cpp',
    # 'src/mod/fiveormore/FiveOrMoreFrame.cpp',
    'src/mod/mediaplayer/MediaPlayerApp.cpp',
    'src/mod/mediaplayer/MediaPlayerBody.cpp',
    'src/mod/mediaplayer/MediaPlayerFrame.cpp',
    # 'src/mod/minesweeper/MinesweeperApp.cpp',
    # 'src/mod/minesweeper/MinesweeperBody.cpp',
    # 'src/mod/minesweeper/MinesweeperFrame.cpp',
    'src/mod/musicbox/MusicBoxApp.cpp',
    'src/mod/musicbox/MusicBoxBody.cpp',
    'src/mod/musicbox/MusicBoxFrame.cpp',
    'src/mod/notepad/NotepadApp.cpp',
    'src/mod/notepad/NotepadBody.cpp',
    'src/mod/notepad/NotepadFrame.cpp',
    'src/mod/paint/PaintApp.cpp',
    'src/mod/paint/PaintBody.cpp',
    'src/mod/paint/PaintFrame.cpp',
    'src/mod/passwordmanager/LoginDialog.cpp',
    'src/mod/passwordmanager/PasswordManager.cpp',
    'src/mod/photoviewer/PhotoViewerApp.cpp',
    'src/mod/photoviewer/PhotoViewerBody.cpp',
    'src/mod/photoviewer/PhotoViewerFrame.cpp',
    'src/mod/registry/RegistryApp.cpp',
    'src/mod/registry/RegistryBody.cpp',
    'src/mod/registry/RegistryFrame.cpp',
    'src/mod/snippingtool/SnippingToolApp.cpp',
    'src/mod/snippingtool/SnippingToolBody.cpp',
    'src/mod/snippingtool/SnippingToolFrame.cpp',
    # 'src/mod/solitaire/SolitaireApp.cpp',
    # 'src/mod/solitaire/SolitaireBody.cpp',
    # 'src/mod/solitaire/SolitaireFrame.cpp',
    'src/mod/soundrecorder/SoundRecorderApp.cpp',
    'src/mod/soundrecorder/SoundRecorderBody.cpp',
    'src/mod/soundrecorder/SoundRecorderFrame.cpp',
    # 'src/mod/stopwatch/StopWatchApp.cpp',
    # 'src/mod/stopwatch/StopWatchBody.cpp',
    # 'src/mod/stopwatch/StopWatchFrame.cpp',
    # 'src/mod/sudoku/SudokuApp.cpp',
    # 'src/mod/sudoku/SudokuBody.cpp',
    # 'src/mod/sudoku/SudokuFrame.cpp',
    'src/mod/taskmanager/TaskManagerApp.cpp',
    'src/mod/taskmanager/TaskManagerBody.cpp',
    'src/mod/taskmanager/TaskManagerFrame.cpp',
    'src/shell/DesktopWindow.cpp',
    'src/shell/Shell.cpp',
    'src/shell/StartMenu.cpp',
    'src/shell/Taskbar.cpp',
    'src/shell/VfsDaemonHost.cpp',
    'src/shell/VfsWebViewHandlers.cpp',
    'src/ui/IconTheme.cpp',
    'src/ui/Location.cpp',
    'src/ui/LocationHistory.cpp',
    'src/ui/dialogs/ChooseFileDialog.cpp',
    'src/ui/dialogs/ChooseFolderDialog.cpp',
    'src/ui/dialogs/ChoosePrinterDialog.cpp',
    'src/ui/widget/BreadcrumbNav.cpp',
    'src/ui/widget/DirTreeView.cpp',
    'src/ui/widget/FileListView.cpp',
    'src/wx/LabeledIcon.cpp',
    'src/wx/WxChecked.cpp',
    'src/wx/term/xterm.cpp',
    'src/wx/wxConsole.cpp',
    'src/wx/wxTerminal.cpp',
)

# Assets will be zipped and linked as a section in the executable
all_sources = cpp_sources

# Build assets.zip every time (no file dependency)
assets_dir = meson.current_source_dir() / 'assets'
build_dir = meson.current_build_dir()
assets_zip = custom_target(
    'assets_zip',
    output: 'assets.zip',
    input: [],
    command: [
        'sh',
        '-c', 'dst=$(readlink -f "@1@"); cd "@0@" && zip -r "$dst" .'.format(assets_dir, '@OUTPUT@'),
    ],
    #build_always : true,
    build_by_default: true,
    install: false,
)

# Compile assembly file that embeds assets.zip using .incbin
# Use configure_file to copy assets.s to build directory
assets_s = configure_file(
    input: 'src/assets.s',
    output: 'assets.s',
    copy: true,
)

# Compile the assembly file (assets.zip will be in the same directory)
as = find_program('as', required: true)
assets_o = custom_target(
    'assets_o',
    output: 'assets.o',
    input: [assets_s, assets_zip],
    command: [as, '-o', '@OUTPUT@', '@INPUT0@'],
    depends: assets_zip,
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
    boost_json_dep,
    date_dep,
    openssl_dep,
    curl_dep,
    wx_dep,
]

lib_omnishell_cpp_args = cpp_nowarn + [
    '-DLOGGER_NAME=omni',
    '-DASSETS_NAME=omni',
]

if host_machine.system() != 'windows'
    util_lib = cpp.find_library('util', required: true)
    exe_deps += util_lib
endif

alsa_dep = cpp.find_library('asound', required: false)
if host_machine.system() == 'linux' and alsa_dep.found()
    exe_deps += alsa_dep
    lib_omnishell_cpp_args += ['-DOMNISHELL_HAVE_ALSA=1']
endif

gst_dep = dependency('gstreamer-1.0', required: false)
gst_app_dep = dependency('gstreamer-app-1.0', required: false)
if gst_dep.found() and gst_app_dep.found()
    exe_deps += [gst_dep, gst_app_dep]
    lib_omnishell_cpp_args += ['-DOMNISHELL_HAVE_GSTREAMER=1']
endif

lib_omnishell = library(
    'omnishell',
    cpp_sources,
    zash_sources,
    zash_bison,
    zash_flex,
    'src/module.cpp',
    assets_o,
    dependencies: exe_deps,
    include_directories: exe_inc + zash_inc,
    c_args: c.get_supported_arguments(['-Wno-unused-parameter']),
    cpp_args: lib_omnishell_cpp_args,
    cpp_pch: wx_pch,
    link_args: ['-rdynamic'],
    override_options: ['b_lundef=false'], # Allows undefined symbols at link time
    version: meson.project_version(),
    soversion: meson.project_version().split('.')[0],
    install: true,
)

omnishell_dep = declare_dependency(
    link_with: lib_omnishell,
    include_directories: exe_inc,
    dependencies: [boost_json_dep, date_dep, openssl_dep, curl_dep, wx_dep],
)

# ---- pkg-config ----
pkgconfig = import('pkgconfig')
pkgconfig.generate(
    lib_omnishell,
    name: 'omnishell',
    description: 'omnishell',
    version: meson.project_version(),
    subdirs: 'omnishell',
    requires: ['bas-c', 'bas-cpp', 'bas-ui', 'boost', 'openssl', 'libcurl'],
)

# ---- Headers for libbas-c package (includedir/bas-c) ----
find_other_cmd = run_command(
    'find',
    'src',
    '-name', '*.h',
    '-o',
    '-name', '*.hpp',
    '-o',
    '-type', 'd',
    '-o',
    '-print',
    check: true,
)
other_files = find_other_cmd.stdout().strip().split('\n')

other_list = []
foreach f : other_files
    other_list += f.substring(4)
endforeach

install_subdir(
    'src',
    install_dir: get_option('includedir') / 'omnishell',
    exclude_files: other_list,
    strip_directory: true,
)

# Unit tests generator
# Find all *_test.cpp files and build them automatically using the 'includes' utility
# Similar to Makefile pattern rule: %_test: %_test.cpp
#   sources: %_test.cpp $(includes -e -c %_test.cpp)
#   cmdline: $(CXX) $(CXXFLAGS) -o $@ $*_test.cpp `includes -c $*_test.cpp` -licuuc -licui18n

# List of all test source files (manually enumerated since Meson lacks fs module)
# This list is automatically generated from: find src -name '*_test.cpp' | sort
all_test_files = []

# Build each test using the 'includes' utility to get source dependencies
foreach test_rel_path : all_test_files
    test_cpp = 'src/' + test_rel_path

    # Extract test name (e.g., ByteBuffer_test from src/io/ByteBuffer_test.cpp)
    path_parts = test_rel_path.split('/')
    test_file = path_parts[path_parts.length() - 1]
    test_name = test_file.replace('.cpp', '')

    # Use 'includes' utility to get implementation dependencies
    # 'includes -e -c test.cpp' returns source files, one per line (not -I include paths)
    includes_result = run_command('includes', '-e', '-c', test_cpp, check: false)

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
    test_exe = executable(
        test_name,
        all_sources,
        include_directories: exe_inc,
        dependencies: exe_deps + [omnishell_dep],
        cpp_args: cpp_nowarn + ['-DLOGGER_NAME=' + test_name, '-DASSETS_NAME=omni'],
        cpp_pch: wx_pch,
        install: false,
    )

    # Register the test with meson
    test(test_name, test_exe)
endforeach

# --- Applications in app/ (one .cpp = one program), install to bindir + manpages to mandir ---
find_app = run_command('find', 'app', '-maxdepth', '1', '-name', '*.cpp', check: false)
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
    executable(
        app_name,
        app_cpp,
        include_directories: exe_inc,
        dependencies: exe_deps + [omnishell_dep],
        cpp_args: cpp_nowarn + ['-DLOGGER_NAME=' + app_name, '-DASSETS_NAME=omni'],
        cpp_pch: wx_pch,
        override_options: ['b_asneeded=false'],
        install: true,
        install_dir: install_dir,
    )
endforeach

# Manpages: app/*.1 -> install to mandir
find_man = run_command('find', 'app', '-maxdepth', '1', '-name', '*.1', check: false)
if find_man.returncode() == 0
    foreach line : find_man.stdout().strip().split('\n')
        if line == ''
            continue
        endif
        install_man(line)
    endforeach
endif

# ---- Bash completion (install to datadir/bash-completion/completions) ----
find_bash = run_command('find', 'app', '-maxdepth', '1', '-name', '*.bash', check: false)
app_bash_list = []
if find_bash.returncode() == 0
    foreach line : find_bash.stdout().strip().split('\n')
        if line == ''
            continue
        endif
        completion_name = fs.replace_suffix(line.split('/')[-1], '')
        install_data(
            line,
            rename: completion_name,
            install_dir: get_option('datadir') / 'bash-completion' / 'completions',
            install_mode: 'rw-r--r--',
        )
    endforeach
endif
