i18n = import('i18n')

i18n.gettext(
    'omnishell',
    args: ['--from-code=UTF-8', '--language=C++'],
    install: true,
    install_dir: join_paths(get_option('datadir'), 'locale'),
    preset: 'glib',
)

make_prog = find_program('make', required: false)
if make_prog.found()
    run_target(
        'l10n',
        command: [make_prog, '-C', meson.project_source_root(), 'l10n'],
    )
endif
