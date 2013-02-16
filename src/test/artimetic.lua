require "leda"

local graph,connector,stage=leda.graph,leda.connector,leda.stage

function produce(seed,interval)
   print(string.format("Producing random data: seed=%d interval=%f",seed,interval))
   math.randomseed( seed )
   local i=0
   while true do
       local data=math.random(1,1024)
       i=i+1
       local out=leda.get_output(1):send(data,i)
--       print("Produced",data)
       leda.sleep(interval)
   end
end

function p_init() 
   require "math"
   require "string"
end

function square(data,i)
   leda.get_output(1):send(data*data,i)
end

function consume(data,i)
   print(string.format("Data consumed: %d  i='%d'",data,i))
end

local prod=stage{name="Producer",handler=produce,init=p_init}
local cons=stage{name="Consumer",handler=consume,init=p_init}
local sqrt=stage{name="Sqrt",handler=square,init=p_init}


local g=graph{"Producer-consumer",
   leda.connect(prod,sqrt),
   leda.connect(sqrt,cons)
}


prod:send(os.time(),0.1)

g:run()
