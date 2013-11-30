local leda=require "leda"

local hello=leda.stage"Hello"{
   handler=function(port)
      print "Hello world"
      leda.send('quit')
   end,
   autostart=true
}

local quit=leda.stage{
   handler=function(...)
      leda.quit('indeed')
   end
}

print(leda.graph{hello'quit'..quit}:run())
