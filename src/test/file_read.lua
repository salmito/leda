require "leda"
require "leda.utils"

s0=leda.stage{
   handler=function(file)
      print("Opening file",file)
      local f = io.open(file,"r")
      leda.send("file",f)
   end,
	init=function() require "leda.utils.io" end
}

s1=leda.stage{
   handler=function(file)
      print("Reading file",file)
      local line = file:read()
      while line do
         leda.send("line",line)
         line = file:read()
      end
   end,
	init=function() require "leda.utils.io" end
}

s2=leda.stage(leda.utils.print)

g=leda.graph{leda.connect(s1,'line',s2),leda.connect(s0,'file',s1)}

s0:send("Makefile")

g:run()
