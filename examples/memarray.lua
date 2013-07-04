local exec=[[lua -l leda -e "leda.start{port=8888,controller=require 'leda.controller.thread_pool'}"&]]
os.execute(exec)
require 'leda'
leda.kernel.sleep(1)

local n=n or 100
local it=it or 100000

local s1=leda.stage{name="S1",
	handler=function()
   	memarray=require 'leda.utils.memarray'
      local c=memarray('uint',500)
      for i=0,499 do
         c[i]=4e6
   	end
   	print("s1",c[499],c)
   	print(c)
   	leda.send(1,c)
   	print(c)
   	leda.send(2,c)
--   	print(c) --raise error
   end,
   init=function () 
   end
}

local s3=leda.stage{name="S3",
	handler=function(array)
		print("s3",array[499],array)
		leda.quit()
	end
}


local s2=leda.stage{name="S2",
	handler=function(array)
		print("s2",array[499],array)
		leda.yield(array)
	end
}

s1:send(1)

local g=leda.graph{s1:connect(1,s2),
s1:connect(2,s3,"local")}

g:part(s2,s1+s3):map('localhost','localhost:8888')



print("result",g:run{maxpar=4})
