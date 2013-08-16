package = 'leda-util'
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
    ['stage.lines'] = 'util/lines.lua',
    ['stage.timer'] = 'util/timer.lua',
    ['stage.broadcast'] = 'util/broadcast.lua', 
    ['stage.eval'] = 'util/eval.lua',
    ['stage.roundrobin'] = 'util/roundrobin.lua',
    ['stage.switch'] = 'util/switch.lua',
  },
}