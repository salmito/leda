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
      local buffer,err = file:aread(100)
      while buffer do
         leda.send("line",buffer)
         buffer,err = file:aread(100)
      end
      assert(err=="EOF",err)
      leda.quit()
   end,
	init=function() require "io" end
}

s2=leda.stage(leda.utils.print)

g=leda.graph{leda.connect(s1,'line',s2),leda.connect(s0,'file',s1)}

s0:send("/etc/passwd")

g:run()