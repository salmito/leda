require "leda"

local utils=require "leda.utils"
require "leda.utils.plot"

local it=require('leda.controller.interactive')

local graph,connector,stage=leda.graph,leda.connector,leda.stage

s1=stage{
   handler=function (i)
--      print("S1",i)
      while true do
         leda.sleep(0.1)
         local t='upvalue'
         a={test="testing"}
         a.a=a
         print("sending")
         local ret,err=leda.get_output('out'):send(a,function () return t end)
         print("SENT",ret,err)
         print("S1",i,"(cont 1)")
         leda.get_output('out1'):send("S3")
         print("S1",i,"(cont 2)")
      end
      end
   ,
	bind=function(self)
		assert(self.output.out,"out must be connected")
		assert(self.output.out1,"out1 must be connected")
	end,
	name="S1",
	backpressure=true
}

s2=stage{
   handler=function (a,f)
--      while 1 do
      print("RECEIVED")
      print("S2",a.a.a.a.a.a.a.test,f())
      print("===== sleeping zzzz =====")
      leda.sleep(0.2)
      local str=leda.get_output(1):send("S4")
      print("S2\t\t(cont)")
--      str=leda.debug.wait_event()
      print("VIXE",str)
--      end
   end,
   name="S2",
   serial=true,
}

s3=stage{function () end,name="S3"}
s4=stage{utils.print,name="S4"}

c=connector{sendf=leda.pass_thread}

s1.output={out=c,out1=s3.input}
s2.input=c
s2.output={s4.input}
local g=graph{s1,s2,s3,s4}

s1:input_method(leda.e)
s2:input_method(leda.e)
s3:input_method(leda.e)
s4:input_method(leda.e)

s1.input:send(1)

--leda.plot_graph(g)

g:run(it.get(3))

