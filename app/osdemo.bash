_osdemo()
{
    local cur prev opts longopts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    opts="-D -l -o -h -V"
    longopts="--dump --local --open --help --version"

    case "${prev}" in
        -l|--local|-o|--open)
            # Path completion for volume roots and open targets
            _filedir
            return 0
            ;;
    esac

    if [[ "${cur}" == -* ]]; then
        COMPREPLY=( $(compgen -W "${opts} ${longopts}" -- "${cur}") )
        return 0
    fi

    # Fallback to file completion for bare arguments
    _filedir
}

complete -F _osdemo osdemo

