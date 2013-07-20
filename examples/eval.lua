local utils=require "leda.utils"

s1=leda.stage{ name="s1",
   handler=function (i)
      i=0
      while i<10 do
         leda.send(1,string.format("print('event %d') leda.send(1,'string')",i))
         leda.sleep(0)
         i=i+1
      end
      leda.send(1,"leda.quit()")
   end,
   init=function() require "string" end,
	bind=function(self,output)
		assert(output[1],"default output must be connected")
	end
}

eval=leda.stage(function (str,...) loadstring(str)(...) end)
printer=leda.stage(function (...) print(...) end)

s1:send(1)

leda.graph{s1:connect(eval),eval:connect(printer)}:run()
