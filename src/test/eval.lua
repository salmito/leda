local utils=require "leda.utils"

s1=stage{ name="s1",
   handler=function (i)
      send(1,string.format("",i))
   end,
   serial=true,
   init=function () 
      mutex=leda.mutex.new()
   end,
   output={connector()},
	bind=function(self)
		assert(self.output[1],"default output must be connected")
	end
}

s1.input:send(1)

graph{s1,s2}:run(leda.controller.fixed_thread_pool.get(10))
