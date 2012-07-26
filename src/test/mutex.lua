local utils=require "leda.utils"

s1=stage{ name="s1",
   handler=function (i)
      print("S1",i)
      while true do
         print("S1 sending",i)
         --wait for s2 to complete
         leda.mutex.lock(mutex)
         leda.mutex.unlock(mutex)
         leda.get_output():send(mutex,1)
--         coroutine.yield()
      end
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

s2=stage{name="s2",
   handler=function (mutex,time)
      leda.mutex.lock(mutex)
      print("S2 locked")
      local i=os.time()
      while os.time()-i<time do end
      print("S2 unlocking")
      leda.mutex.unlock(mutex)
   end,
   input=s1.output[1]
}
s2:set_method(leda.e)

s1.input:send(1)

graph{s1,s2}:run(leda.controller.fixed_thread_pool.get(10))
