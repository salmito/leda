package = 'leda-stage'
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
  'leda-window',
  'leda-compress',
  'leda-util',
  'leda-net',
}
build = {
  type    = 'builtin',
  modules = {
    ['stage.empty'] = 'empty.lua',
  },
}
