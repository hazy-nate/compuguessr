
"****h* config/.vimrc
" NAME
"   .vimrc
"******

set colorcolumn=80

function! CustomFold(lnum)
    let line = getline(a:lnum)
    if line =~# '^\s*[;#/%]\+\*\*\*\*'
        return '>2'
    endif
    if line =~# '^\s*[;#/%]\+\s*SOURCE\>'
        return '1'
    endif

    let prev_line = getline(a:lnum - 1)
    if prev_line =~ '^;==='
        return '1'
    endif
    if line =~ '^;==='
        return '0'
    endif

    return '='
endfunction

augroup AssemblyFolding
    autocmd!
    autocmd FileType asm setlocal foldexpr=CustomFold(v:lnum) foldmethod=expr
augroup END

augroup CStyle
    autocmd!
    autocmd FileType c setlocal noexpandtab shiftwidth=4 softtabstop=4 tabstop=8
augroup END

augroup FileTemplates
    autocmd!
    autocmd BufNewFile *.c 0read templates/source.c
    autocmd BufNewFile *.h 0read templates/header_guarded.h
augroup END

set foldlevelstart=99

