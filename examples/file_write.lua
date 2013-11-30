local leda=require "leda"

s0=leda.stage{
   handler=function(file)
      local io=require'io'
      print("Opening file",file)
      local f = io.open(file,"a")
      leda.send("file",f)
   end,
	init=function() require "io" end
}

s1=leda.stage{
   handler=function(file)
      print("Writing file",file)
      local size,err = file:awrite("Hello world\n")
  		assert(size,err)
      for i=1,10 do
      	print("Writed",size)
     		assert(size,err)
         size,err = file:awrite("Hello world\n")
      end
  		assert(size,err)
      leda.quit()
   end,
	init=function() require "leda.utils.io" end
}

s2=require'leda.stage.util.print'

g=leda.graph{leda.connect(s1,'line',s2),leda.connect(s0,'file',s1)}

s0:send("/tmp/test")

g:run()
