require "leda"

local utils=require "leda.utils"

local graph,connector,stage=leda.graph,leda.connector,leda.stage

s1=stage{
   handler=[[--function (i)
      print("S1",i)
      while true do
         leda.get_output('out'):send("S2")
         print("S1",i,"(cont 1)")
         leda.get_output('out1'):send("S3")
         print("S1",i,"(cont 2)")
         print("===== sleeping zzzz =====")
         sleep(2)
      end
      --end]]
   ,
   init=[[--function () 
   require "socket" 
   sleep=function(p) 
--      while os.time()-t<periodo do end
      socket.select(nil,nil,p) 
   end
--end]],
	bind=function(self)
		assert(self.output.out,"Tem que ter out")
		assert(self.output.out1,"Tem que ter out2")
	end
}

s2=stage{
   handler=[[--function ()
      print("S2")
      leda.get_output(1):send("S4")
      print("S2\t\t(cont)")
   --end]]
}

s3=stage{utils.print}
s4=stage{utils.print}

c=connector{sendf=leda.throw_event}

s1.output={out=c,out1=s3.input}
s2.input=c
s2.output={s4.input}
local g=graph{s1,s2,s3,s4}

--s1:set_method(leda.throw_event)
s1.output.out.sendf=leda.throw_event
s2:set_method(leda.te)
s3:set_method(leda.te)
s4:set_method(leda.te)

s1.input:send(1)
s1.input:send(2)
s1.input:send(1)
s1.input:send(1)
s1.input:send(1)
s1.input:send(1)

g:run(leda.controller.fixed_thread_pool.get(50))
