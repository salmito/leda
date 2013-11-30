local leda=require "leda"

local test=leda.stage"Self"{
   handler=function()
      print(self.str)
      leda.quit()
   end,
   str="Hello world from self",
   autostart=true
}:run{controller=require 'leda.controller.single_thread'}
