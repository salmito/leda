require "leda"

local fib=leda.stage"Fibonacci"{
	handler=function (fib,n,val,oldval)
	   if not fib then error("Invalig argument for stage") end
	   
		val=val or 0
		oldval=oldval or 1
		n=n or 0
      if n < fib then
			leda.send("value",n,val)
			leda.send("loopback",fib,n+1,val+oldval,val)
      else
         leda.send('value',fib,val)
         leda.send('end')
		end
	end,
	bind=function(self,output,graph)
		assert(#output.value.consumer,"Value output must be connected to someone")
      --print("I'm connected to: ",output.value.consumer)
      graph:add(self:connect('loopback',self))
	end,
   autostart=1477
}

local printer=leda.stage{
   handler=function(...) 
      print(...)
   end,
   name="Accumulator",
   serial=true
}

local graph=leda.graph{
   fib:connect("value",leda.stage"Print"("print(...)")),
   fib:connect("end",leda.stage"Quit"("leda.quit()"))
}

graph:run{controller=leda.controller.thread_pool.get(1)}
