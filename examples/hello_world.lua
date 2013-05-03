require "leda"

local hello=leda.stage"Hello"{
   handler=function(port)
      print "Hello world"
      leda.send('quit')
   end,
   autostart=true
}

local quit=leda.stage{
   handler=function(...)
      print('indeed')
      leda.quit()
   end
}

leda.graph{hello:connect('quit',quit)}
      :run{controller=leda.controller.singlethread}
