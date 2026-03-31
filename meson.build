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

subproject('bas-c')
subproject('bas-cpp')
subproject('bas-ui')
subproject('zash')

bas_c_dep = dependency('bas-c', required: true)
bas_cpp_dep = dependency('bas-cpp', required: true)
bas_ui_dep = dependency('bas-ui', required: true)

bas_c_static_dep = dependency('bas-c-static', required: true)
bas_cpp_static_dep = dependency('bas-cpp-static', required: true)
bas_ui_static_dep = dependency('bas-ui-static', required: true)

boost_sub = subproject('boost')
boost_json_dep = boost_sub.get_variable('boost_json_dep')
curl_dep = dependency('libcurl', required: true)
date_dep = subproject('date_tz').get_variable('date_dep')
openssl_dep = dependency('openssl', required: true)
wx_sub = subproject('wx_logic')
wx_dep = wx_sub.get_variable('wx_dep')
wx_ver = wx_sub.get_variable('wx_ver')
zash_dep = dependency('zash', required: true)
zash_static_dep = dependency('zash-static', required: true)

# Disable all warnings (wx 3.0 and deps are noisy)
cpp_nowarn = cpp.get_supported_arguments(['-w', '/w'])
cpp_args = cpp_nowarn + ['-DLOGGER_NAME=omni', '-DASSETS_NAME=omni']
cpp_pch = 'src/wx_pch.h'
cpp_sources = files(
    'src/omnishell/core/App.cpp',
    'src/omnishell/core/Category.cpp',
    'src/omnishell/core/Module.cpp',
    'src/omnishell/core/ModuleRegistry.cpp',
    'src/omnishell/core/Process.cpp',
    'src/omnishell/core/ServiceManager.cpp',
    'src/omnishell/core/TrayIcon.cpp',
    'src/omnishell/core/VolUrl.cpp',
    'src/omnishell/core/registry/FileAssociations.cpp',
    'src/omnishell/core/registry/IRegistry.cpp',
    'src/omnishell/core/registry/JsonRegistry.cpp',
    'src/omnishell/core/registry/LocalRegistry.cpp',
    'src/omnishell/core/registry/RegistryKeyUtil.cpp',
    'src/omnishell/core/registry/RegistryService.cpp',
    'src/omnishell/core/registry/RegistryValueIo.cpp',
    'src/omnishell/core/registry/VolumeRegistry.cpp',
    'src/omnishell/daemon/ApiDirIndex.cpp',
    'src/omnishell/daemon/ApiFileController.cpp',
    'src/omnishell/daemon/ApiVolumeIndex.cpp',
    'src/omnishell/daemon/AssetController.cpp',
    'src/omnishell/daemon/DirIndex.cpp',
    'src/omnishell/daemon/FileController.cpp',
    'src/omnishell/daemon/FileIndex.cpp',
    'src/omnishell/daemon/HttpDaemon.cpp',
    'src/omnishell/daemon/VolumeIndex.cpp',
    'src/omnishell/mod/background/BackgroundSettingsApp.cpp',
    'src/omnishell/mod/background/BackgroundSettingsBody.cpp',
    'src/omnishell/mod/background/BackgroundSettingsFrame.cpp',
    'src/omnishell/mod/browser/BrowserApp.cpp',
    'src/omnishell/mod/browser/BrowserBody.cpp',
    'src/omnishell/mod/browser/BrowserFrame.cpp',
    'src/omnishell/mod/calculator/CalculatorApp.cpp',
    'src/omnishell/mod/calculator/CalculatorBody.cpp',
    'src/omnishell/mod/calculator/CalculatorFrame.cpp',
    'src/omnishell/mod/calendar/CalendarApp.cpp',
    'src/omnishell/mod/calendar/CalendarBody.cpp',
    'src/omnishell/mod/calendar/CalendarFrame.cpp',
    'src/omnishell/mod/camera/CameraApp.cpp',
    'src/omnishell/mod/camera/CameraBody.cpp',
    'src/omnishell/mod/camera/CameraFrame.cpp',
    'src/omnishell/mod/camera/CameraGstPreview.cpp',
    'src/omnishell/mod/console/ConsoleApp.cpp',
    'src/omnishell/mod/console/ConsoleBody.cpp',
    'src/omnishell/mod/console/ConsoleFrame.cpp',
    'src/omnishell/mod/controlpanel/ControlPanelApp.cpp',
    'src/omnishell/mod/controlpanel/ControlPanelBody.cpp',
    'src/omnishell/mod/controlpanel/ControlPanelFrame.cpp',
    'src/omnishell/mod/explorer/ExplorerApp.cpp',
    'src/omnishell/mod/explorer/ExplorerBody.cpp',
    'src/omnishell/mod/explorer/ExplorerFrame.cpp',
    # 'src/omnishell/mod/fiveormore/FiveOrMoreApp.cpp',
    # 'src/omnishell/mod/fiveormore/FiveOrMoreBody.cpp',
    # 'src/omnishell/mod/fiveormore/FiveOrMoreFrame.cpp',
    'src/omnishell/mod/mediaplayer/MediaPlayerApp.cpp',
    'src/omnishell/mod/mediaplayer/MediaPlayerBody.cpp',
    'src/omnishell/mod/mediaplayer/MediaPlayerFrame.cpp',
    # 'src/omnishell/mod/minesweeper/MinesweeperApp.cpp',
    # 'src/omnishell/mod/minesweeper/MinesweeperBody.cpp',
    # 'src/omnishell/mod/minesweeper/MinesweeperFrame.cpp',
    'src/omnishell/mod/musicbox/MusicBoxApp.cpp',
    'src/omnishell/mod/musicbox/MusicBoxBody.cpp',
    'src/omnishell/mod/musicbox/MusicBoxFrame.cpp',
    'src/omnishell/mod/notepad/NotepadApp.cpp',
    'src/omnishell/mod/notepad/NotepadBody.cpp',
    'src/omnishell/mod/notepad/NotepadFrame.cpp',
    'src/omnishell/mod/paint/PaintApp.cpp',
    'src/omnishell/mod/paint/PaintBody.cpp',
    'src/omnishell/mod/paint/PaintFrame.cpp',
    'src/omnishell/mod/passwordmanager/LoginDialog.cpp',
    'src/omnishell/mod/passwordmanager/PasswordManager.cpp',
    'src/omnishell/mod/photoviewer/PhotoViewerApp.cpp',
    'src/omnishell/mod/photoviewer/PhotoViewerBody.cpp',
    'src/omnishell/mod/photoviewer/PhotoViewerFrame.cpp',
    'src/omnishell/mod/registry/RegistryApp.cpp',
    'src/omnishell/mod/registry/RegistryBody.cpp',
    'src/omnishell/mod/registry/RegistryFrame.cpp',
    'src/omnishell/mod/snippingtool/SnippingToolApp.cpp',
    'src/omnishell/mod/snippingtool/SnippingToolBody.cpp',
    'src/omnishell/mod/snippingtool/SnippingToolFrame.cpp',
    # 'src/omnishell/mod/solitaire/SolitaireApp.cpp',
    # 'src/omnishell/mod/solitaire/SolitaireBody.cpp',
    # 'src/omnishell/mod/solitaire/SolitaireFrame.cpp',
    'src/omnishell/mod/soundrecorder/SoundRecorderApp.cpp',
    'src/omnishell/mod/soundrecorder/SoundRecorderBody.cpp',
    'src/omnishell/mod/soundrecorder/SoundRecorderFrame.cpp',
    # 'src/omnishell/mod/stopwatch/StopWatchApp.cpp',
    # 'src/omnishell/mod/stopwatch/StopWatchBody.cpp',
    # 'src/omnishell/mod/stopwatch/StopWatchFrame.cpp',
    # 'src/omnishell/mod/sudoku/SudokuApp.cpp',
    # 'src/omnishell/mod/sudoku/SudokuBody.cpp',
    # 'src/omnishell/mod/sudoku/SudokuFrame.cpp',
    'src/omnishell/mod/taskmanager/TaskManagerApp.cpp',
    'src/omnishell/mod/taskmanager/TaskManagerBody.cpp',
    'src/omnishell/mod/taskmanager/TaskManagerFrame.cpp',
    'src/omnishell/shell/DesktopWindow.cpp',
    'src/omnishell/shell/Shell.cpp',
    'src/omnishell/shell/StartMenu.cpp',
    'src/omnishell/shell/Taskbar.cpp',
    'src/omnishell/shell/VfsDaemonHost.cpp',
    'src/omnishell/shell/VfsWebViewHandlers.cpp',
    'src/omnishell/ui/IconTheme.cpp',
    'src/omnishell/ui/Location.cpp',
    'src/omnishell/ui/LocationHistory.cpp',
    'src/omnishell/ui/dialogs/ChooseFileDialog.cpp',
    'src/omnishell/ui/dialogs/ChooseFolderDialog.cpp',
    'src/omnishell/ui/dialogs/ChoosePrinterDialog.cpp',
    'src/omnishell/ui/widget/BreadcrumbNav.cpp',
    'src/omnishell/ui/widget/DirTreeView.cpp',
    'src/omnishell/ui/widget/FileListView.cpp',
    'src/omnishell/wx/LabeledIcon.cpp',
    'src/omnishell/wx/WxChecked.cpp',
    'src/omnishell/wx/term/xterm.cpp',
    'src/omnishell/wx/wxConsole.cpp',
    'src/omnishell/wx/wxTerminal.cpp',
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
    zash_dep,
]

exe_static_deps = [
    bas_c_static_dep,
    bas_cpp_static_dep,
    bas_ui_static_dep,
    boost_json_dep,
    date_dep,
    openssl_dep,
    curl_dep,
    wx_dep,
    zash_static_dep,
]

if host_machine.system() != 'windows'
    util_lib = cpp.find_library('util', required: true)
    exe_deps += util_lib
    exe_static_deps += util_lib
endif

alsa_dep = cpp.find_library('asound', required: false)
if host_machine.system() == 'linux' and alsa_dep.found()
    cpp_args += ['-DOMNISHELL_HAVE_ALSA=1']
    exe_deps += alsa_dep
    exe_static_deps += alsa_dep
endif

gst_dep = dependency('gstreamer-1.0', required: false)
gst_app_dep = dependency('gstreamer-app-1.0', required: false)
if gst_dep.found() and gst_app_dep.found()
    cpp_args += ['-DOMNISHELL_HAVE_GSTREAMER=1']
    exe_deps += [gst_dep, gst_app_dep]
    exe_static_deps += [gst_dep, gst_app_dep]
endif

override_options = ['b_lundef=false']  # Allows undefined symbols at link time

lib_omnishell = library(
    'omnishell',
    cpp_sources,
    'src/module.cpp',
    assets_o,
    dependencies: exe_deps,
    include_directories: exe_inc,
    c_args: c.get_supported_arguments(['-Wno-unused-parameter']),
    cpp_args: cpp_args,
    cpp_pch: cpp_pch,
    link_args: ['-rdynamic'],
    override_options: override_options, # Allows undefined symbols at link time
    version: meson.project_version(),
    soversion: meson.project_version().split('.')[0],
    install: true,
)

lib_omnishell_static = static_library(
    'omnishell',
    cpp_sources,
    'src/module.cpp',
    assets_o,
    dependencies: exe_static_deps,
    include_directories: exe_inc,
    cpp_args: cpp_args,
    cpp_pch: cpp_pch,
    override_options: override_options,
    install: true,
)

omnishell_dep = declare_dependency(
    link_with: lib_omnishell,
    include_directories: exe_inc,
    dependencies: exe_deps,
)

omnishell_static_dep = declare_dependency(
    link_with: lib_omnishell_static,
    include_directories: exe_inc,
    dependencies: exe_static_deps,
)

meson.override_dependency('omnishell', omnishell_dep)
meson.override_dependency('omnishell-static', omnishell_static_dep)

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

pkgconfig.generate(
    lib_omnishell_static,
    name: 'omnishell-static',
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
    install_dir: get_option('includedir'),
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
        cpp_pch: cpp_pch,
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
        cpp_pch: cpp_pch,
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
