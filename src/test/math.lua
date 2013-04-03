require "leda"

local arg={...}

local n = n or tonumber(arg[2]) or 10000

local graph,connector,stage=leda.graph,leda.connector,leda.stage

function produce(seed,interval)
   math.randomseed( seed )
   for i=1,n do
      local data=math.random(1,1024)
      assert(leda.send(1,data,i))
--    print("Produced",data)
      if interval>0 then
         leda.sleep(interval)
      end
   end
end

function square(data,...)
   leda.send(1,data*data,...)
end

function consume(data,i)
--   local acc=acc
   --print(string.format("Data consumed: %d  i='%d' n='%d' %s",data,i,n,tostring(i==n)))
   if i==n then
      print('quit')
      leda.quit()
   end
end

local prod=stage{name="Producer",handler=produce,init="require 'math'"}
local cons=stage{name="Consumer",handler=consume,init="require 'string'"}
local sqrt=stage{name="Sqrt",handler=square}


local g=graph{"Producer-consumer",
   leda.connect(prod,sqrt),
   leda.connect(sqrt,cons)
}


prod:send(os.time(),tonumber(arg[1]) or 0.1)

local th = th or tonumber(arg[3]) or leda.kernel.cpu()

g:run{maxpar=16,controller=leda.controller.fixed_thread_pool.get(th)}
