local leda=require "leda"

local io=nil

s0=leda.stage{
   handler=function(file)
      local io=require'io'
      print("Opening file",file)
      local f = assert(io.open(file,"r"))
      leda.send("file",f)
   end,
}

s1=leda.stage{
   handler=function(file)
      print("Reading file",file)
      local buffer,err = assert(file:aread(4096))
      local bytes=0
      while buffer do
	      bytes=bytes+#buffer
   	   leda.send("line",buffer)
	      buffer,err = file:aread(4096)
      end
		leda.send("EOF",bytes)
   end,
--	init=function() async=true end
}

s2=leda.stage{
	handler=function (str) print(str) end,
}

sQuit=leda.stage{
	handler=function (...) leda.quit(...) end,
}

--three ways to connect two stages:
g=leda.graph{
	--leda.connect function 
	leda.connect(s0,'file',s1),
	--stage object metamethod
	s1:connect('line',s2),
	--stage concat operator
	s1'EOF'..sQuit
}

s0:send(assert(arg[1],"Type a filename"))

print(g:run())
