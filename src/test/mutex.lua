local utils=require "leda.utils"

s1=stage{ name="s1",
   handler=function (i)
      print("S1",i)
      while true do
         print("S1 sending",i)
         leda.get_output():send(mutex,2)
         --wait for s2 to complete
         leda.mutex.lock(mutex)
         leda.mutex.unlock(mutex)
         coroutine.yield()
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
      print("S2 lock")
      leda.mutex.lock(mutex)
      local i=os.time()
      while os.time()-i<time do end
      leda.mutex.unlock(mutex)
      print("S2 unlock")
   end,
   input=s1.output[1]
}
s2:set_method(leda.e)

s1.input:send(1)

graph{s1,s2}:run(leda.controller.fixed_thread_pool.get(1))
