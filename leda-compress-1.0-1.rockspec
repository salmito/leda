package = 'leda-compress'
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
  'lzlib',
}
build = {
  type    = 'builtin',
  modules = {
    ['stage.compress'] = 'compress/compress.lua',
    ['stage.decompress'] = 'compress/decompress.lua',
  },
}
