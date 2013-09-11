require "leda"
require "leda.utils"

local io=nil

s0=leda.stage{
   handler=function(file)
      async=true
      local io=require'io'
      print("Opening file",file)
      local f = io.open(file,"r")
      leda.send("file",f)
   end,
	init=function() async=true end
}

s1=leda.stage{
   handler=function(file)
      print("Reading file",file)
      local buffer,err = assert(file:aread(4096))
      while buffer do
   	   leda.send("line",buffer)
	      buffer,err = file:aread(4096)
      end
		leda.send("EOF")
   end,
	init=function() async=true require "io" end
}

s2=leda.stage{
	handler=function (str) print(str) end,
--	serial=true
}

g=leda.graph{leda.connect(s1,'line',s2),leda.connect(s0,'file',s1)}

s0:send(assert(arg[1]))

g:run()
