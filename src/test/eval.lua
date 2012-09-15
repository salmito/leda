local utils=require "leda.utils"

s1=stage{ name="s1",
   handler=function (i)
      i=0
      while true do
         leda.send(1,string.format("print('event %d') leda.send(1,'string')",i))
         leda.sleep(0)
         i=i+1
      end
   end,
   init=function() require "string" end,
	bind=function(output)
		assert(output[1],"default output must be connected")
	end
}

eval=stage(utils.eval)
printer=stage(utils.print)

s1:send(1)

leda.graph{s1:connect(eval),eval:connect(printer)}:run()
