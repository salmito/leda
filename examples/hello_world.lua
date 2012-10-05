require "leda"

local hello=leda.stage{
   handler=function(port)
      print "Hello"
      leda.send(1,"World")
   end
}

local world=leda.stage{
   handler=function(str)
      print(str)
      leda.quit()
   end
}

local g=leda.graph{hello:connect(world)}
hello:send()
g:part(hello,world):map('localhost','r1i0n0')
g:run()
