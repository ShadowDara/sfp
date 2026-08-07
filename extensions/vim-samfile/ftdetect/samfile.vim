augroup filetypedetect
  au!

  au BufRead,BufNewFile *.samfile setfiletype samfile
  au BufRead,BufNewFile samfile setfiletype samfile

augroup END
