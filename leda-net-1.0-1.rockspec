package = 'leda-net'
version = '1.0-1'
source  = {
    url = 'git://github.com/Salmito/leda-stage.git'
}
description = {
  summary  = [[Collection of useful stages.]],
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
    ['stage.net.url'] = 'net/url.lua',
  },
}
