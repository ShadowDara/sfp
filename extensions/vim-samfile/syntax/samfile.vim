" Samfile Syntax Vim

if exists("b:current_syntax")
  finish
endif

syntax case match

" ==========================
" Comments
" ==========================

syntax match samfileComment "//.*$"
syntax match samfileComment "--.*$"
syntax match samfileComment "^\s*#\(define\)\@!.*$"

syntax region samfileBlockComment start="/\*" end="\*/"


" ==========================
" Macros
" ==========================

syntax match samfileDefine "^\s*#define\s\+[A-Za-z_][A-Za-z0-9_]*.*$"

syntax match samfileMacroName "^\s*#define\s\+\zs[A-Za-z_][A-Za-z0-9_]*"


" ==========================
" Targets
" ==========================

syntax match samfileTarget "^[A-Za-z0-9_-]\+\ze:"


" ==========================
" Commands
" ==========================

syntax keyword samfileCommand contained
      \ RUNWIN
      \ TASK
      \ TOUCHMAC
      \ ERRORWIN
      \ MVMAC
      \ RUNLIN
      \ TOUCH
      \ RMLIN
      \ TOUCHWIN
      \ CPMAC
      \ SHELLLIN
      \ ERROR
      \ ECHOWIN
      \ ECHOLIN
      \ WRITEMAC
      \ MV
      \ WARNMAC
      \ ENVMAC
      \ WRITEWIN
      \ SHELLMAC
      \ CP
      \ MKDIRLIN
      \ TASKLIN
      \ WRITELIN
      \ PROMPTLIN
      \ APPENDMAC
      \ APPENDWIN
      \ SHELLWIN
      \ UNSETENV
      \ WARN
      \ SLEEPWIN
      \ ECHO
      \ MVLIN
      \ ERRORLIN
      \ ERRORMAC
      \ RUNMAC
      \ CDLIN
      \ RM
      \ ENVLIN
      \ CDMAC
      \ CPLIN
      \ UNSETENVMAC
      \ WARNLIN
      \ APPEND
      \ ENVWIN
      \ UNSETENVLIN
      \ CPWIN
      \ MVWIN
      \ CDWIN
      \ RUN
      \ SLEEPMAC
      \ CD
      \ PROMPT
      \ UNSETENVWIN
      \ WARNWIN
      \ APPENDLIN
      \ PROMPTWIN
      \ PROMPTMAC
      \ ENV
      \ MKDIRWIN
      \ MKDIR
      \ RMMAC
      \ ECHOMAC
      \ SLEEP
      \ RMWIN
      \ SLEEPLIN
      \ TOUCHLIN
      \ WRITE
      \ TASKWIN
      \ TASKMAC
      \ SHELL
      \ MKDIRMAC


syntax match samfileCommand "\<\%(RUNWIN\|TASK\|TOUCHMAC\|ERRORWIN\|MVMAC\|RUNLIN\|TOUCH\|RMLIN\|TOUCHWIN\|CPMAC\|SHELLLIN\|ERROR\|ECHOWIN\|ECHOLIN\|WRITEMAC\|MV\|WARNMAC\|ENVMAC\|WRITEWIN\|SHELLMAC\|CP\|MKDIRLIN\|TASKLIN\|WRITELIN\|PROMPTLIN\|APPENDMAC\|APPENDWIN\|SHELLWIN\|UNSETENV\|WARN\|SLEEPWIN\|ECHO\|MVLIN\|ERRORLIN\|ERRORMAC\|RUNMAC\|CDLIN\|RM\|ENVLIN\|CDMAC\|CPLIN\|UNSETENVMAC\|WARNLIN\|APPEND\|ENVWIN\|UNSETENVLIN\|CPWIN\|MVWIN\|CDWIN\|RUN\|SLEEPMAC\|CD\|PROMPT\|UNSETENVWIN\|WARNWIN\|APPENDLIN\|PROMPTWIN\|PROMPTMAC\|ENV\|MKDIRWIN\|MKDIR\|RMMAC\|ECHOMAC\|SLEEP\|RMWIN\|SLEEPLIN\|TOUCHLIN\|WRITE\|TASKWIN\|TASKMAC\|SHELL\|MKDIRMAC\)\>"


" ==========================
" Strings
" ==========================

syntax region samfileStringDouble start='"' end='"'

syntax region samfileStringSingle start="'" end="'"

syntax match samfileFilename "\<[\w./-]\+\.\(html\|cpp\|c\|hpp\|h\|js\|ts\|json\|wasm\|data\|png\|jpg\|txt\|bat\|exe\)\>"


" ==========================
" URLs
" ==========================

syntax match samfileURL "https\?://[^\s]\+"


" ==========================
" Highlight mapping
" ==========================

highlight default link samfileComment Comment
highlight default link samfileBlockComment Comment

highlight default link samfileCommand Keyword

highlight default link samfileTarget Function

highlight default link samfileStringDouble String
highlight default link samfileStringSingle String

highlight default link samfileFilename String

highlight default link samfileURL Underlined

highlight default link samfileDefine PreProc
highlight default link samfileMacroName Constant


let b:current_syntax = "samfile"
