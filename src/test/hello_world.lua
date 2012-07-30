require "leda.utils"

hello=stage{
   handler=function()
      print "Hello"
      leda.get_output():send()
   end
}

world=stage{
   handler=function()
      print "World"
   end
}

leda.insert_before(hello,world)

g=graph{hello,world}

hello:send()

g:run()
