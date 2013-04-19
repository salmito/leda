package = 'leda-lines'
version = '1.0-1'
source  = {
    url = 'https://github.com/Salmito/leda/raw/unstable/stages/lines.lua'
}
description = {
  summary  = [[Receives a filename as input and outputs 
each line of file to the 'line' port along with a line counter and the filename.
EOF event is emmited at the end of file.]],
  homepage = 'http://leda.co',
  license  = 'MIT',
}
dependencies = {
  'lua >= 5.1'
}
build = {
  type    = 'builtin',
  modules = {
    ['leda.stage.lines'] = 'lines.lua',
  },
}
