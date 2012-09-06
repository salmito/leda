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
       local t=os.time()

       sleep(interval)
   end
end

function p_init() 
   require "socket" 
   require "os" 
   sleep=function(p) 
--      while os.time()-t<p do end
      socket.select(nil,nil,p) 
   end 
end

function square(data,i)
      local out=leda.get_output(1):send(math.sqrt(data),i)
end

function consume(data,i)
   print(string.format("Data consumed: %d  i='%d'",data,i))
end

local g=graph{"Producer-consumer",
      stage{name="Producer",handler=produce,init=p_init}:connect
      cons=stage{name="Consumer",handler=consume},
      sqrt=stage{name="Sqrt",handler=square},
}


leda.connect(g.prod,g.cons)
leda.connect(g.sqrt,g.cons)

g.prod.input.name="Prod_input"
g.sqrt.input.name="Sqrt_input"
g.cons.input.name="Cons_input"


g.prod.input:method(leda.t)
g.sqrt.input:method(leda.t)
g.cons.input:method(leda.t)

g.prod:send(os.time(),0)

g:run(leda.controller.fixed_thread_pool.get(2))
