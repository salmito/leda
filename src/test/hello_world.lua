require "leda.utils"

local hello=stage{
   handler=function()
      print "Hello"
      leda.get_output():send()
   end
}

local world=stage{
   handler=function()
      print "World"
   end
}

leda.connect(hello,world)

local g=graph{hello,world}

hello:send()

g:run()
