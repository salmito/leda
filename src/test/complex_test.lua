require "leda"

local utils=require "leda.utils"

local graph,connector,stage=leda.graph,leda.connector,leda.stage

s1=stage{
   handler=function (i)
--      print("S1",i)
      while true do
         leda.sleep(2)   
         local ret,err=leda.get_output('out'):send("S2")
         --if not ret then print("ERROR",ret,err) end
         --print("S1",i,"(cont 1)")
         --leda.get_output('out1'):send("S3")
         --print("S1",i,"(cont 2)")
      end
      end
   ,
	bind=function(self)
		assert(self.output.out,"out must be connected")
		assert(self.output.out1,"out1 must be connected")
	end,
	name="S1"
}

s2=stage{
   handler=function ()
      while 1 do
      print("S2",a)
      print("===== sleeping zzzz =====")
      leda.get_output(1):send("S4")
      print("S2\t\t(cont)")
      str=leda.wait_event()
      print("VIXE",str)
      end
   end,
   name="S2",
   serial=true,
}

s3=stage{utils.print,name="S3"}
s4=stage{utils.print,name="S4"}

c=connector{sendf=leda.throw_event}

s1.output={out=c,out1=s3.input}
s2.input=c
s2.output={s4.input}
local g=graph{s1,s2,s3,s4}

--s1:set_method(leda.throw_event)
s1.output.out.sendf=leda.e
s2:set_method(leda.e)
s3:set_method(leda.e)
s4:set_method(leda.e)

s1.input:send(1)

g:run(leda.controller.fixed_thread_pool.get(10))

