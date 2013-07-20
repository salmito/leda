local exec=[[lua -l leda -e "leda.start{port=8888,controller=require'leda.controller.thread_pool'.get(4)}"&]]
os.execute(exec)
require 'leda'
leda.kernel.sleep(1)

local n=n  or 100
local it=it or 100000

local s1=leda.stage{name="S1",
	handler=function()
		local t={}
		for i=1,n do
			t[#t+1]=i
		end
		for i=1,it do
   		assert(leda.send(1,t,i))
       end
      print("Process 1 end")
       leda.quit()
	end
}

local s2=leda.stage{name="S2",
	handler=function(t,i)
		print(t,i)
		if i==it then print("Process 2 end") leda.quit() end
	end
}

s1:send(1)

local g=leda.graph{s1:connect(1,s2)}

g:part(s2,s1):map('localhost','localhost:8888')

g:run{maxpar=leda.kernel.cpu(),controller=require'leda.controller.thread_pool'}
