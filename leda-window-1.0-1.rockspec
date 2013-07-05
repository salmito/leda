package = 'leda-window'
version = '1.0-1'
source  = {
    url = 'git://github.com/Salmito/leda-stage.git'
}
description = {
  summary  = [[Collection of stages.]],
  homepage = 'http://leda.co',
  license  = 'MIT',
}
dependencies = {
  'lua >= 5.1',
  'leda',
}
build = {
  type    = 'builtin',
  modules = {
    ['stage.window.sized'] = 'window/sized_window.lua',
    ['stage.window.time'] = 'window/time_window.lua',
    ['stage.window.sliding'] = 'window/sliding_window.lua',
  },
}
