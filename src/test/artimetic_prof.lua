profile_output='/home/tiago/trace.csv'
profiler_resolution=0.1

require "leda"

local graph,connector,stage=leda.graph,leda.connector,leda.stage

local n=3000000

function produce(seed,interval)
--   print(string.format("Producing random data: seed=%d interval=%f",seed,interval))
   math.randomseed( seed )
   local i=0
   while i<=n do
       local data=math.random(1,1024)
       i=i+1
       leda.send(1,data,i)
   end
end

function p_init() 
   require "math"
   require "string"
end

function square(data,i)
   leda.send(1,data*data,i)
end

function consume(data,i)
   --print(string.format("Data consumed: %d  i='%d'",data,i))
   if i == n then leda.quit() end
end

local prod=stage{name="Producer",handler=produce,init=p_init}
local cons=stage{name="Consumer",handler=consume,init=p_init}
local sqrt=stage{name="Sqrt",handler=square,init=p_init}


local g=graph{"Producer-consumer",
   leda.connect(prod,sqrt),
   leda.connect(sqrt,cons)
}


prod:send(os.time(),0.1)

--g:run{maxpar=10}
g:run{controller=leda.controller.profiler,maxpar=10}
