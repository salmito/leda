require "leda"

local hello=leda.stage"Self"{
   handler=function()
      print(self.str)
      leda.yield()
   end,
   str="Hello world from self",
   autostart=true
}:run{controller=require 'leda.controller.singlethread'}
