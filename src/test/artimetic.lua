require "leda"

local graph,connector,stage=leda.graph,leda.connector,leda.stage

function event_copy(n,...)
   for i=1,n do
      leda.get_output(1):send(...)
   end
end

function produzir(seed,periodo)
   print(string.format("Produzir dado: seed=%d periodo=%f",seed,periodo))
   math.randomseed( seed )
   local i=0
   while true do
       local data=math.random(1,1024)
       i=i+1
--       print("Produziu",data,i)
 --      local sock=wrap(socket.tcp(9999))
       local out=leda.get_output(1):send(data,i)
--       print(sock)
--       print("Produziu (cont)",data,i)
       local t=os.time()

       sleep(periodo)
   end
end

function p_init() 
   require "socket" 
   sleep=function(p) 
--      while os.time()-t<periodo do end
--      socket.select(nil,nil,p) 
   end 
end

function quadratico(dado,i)
--   print("Elevando ao quadrado",dado,i)
      local out=leda.get_output(1):send(dado*dado,i)
--   print("Elevando ao quadrado (cont)",dado,i)
end

function consumir(dado,i)
--   print(string.format("Consumindo dado: %d  i='%d'",dado,i))   
   for waste=1,dado do
   end
   print(string.format("Dado consumido: %d  i='%d'",dado,i))
end

function insert(s1,s2)
   s1.input.name=tostring(s1).."_input"
   s1.input=s2.input
   s2.input=connector{}
   s1:add_output(s2.input)
end

local g=graph{"Prodcons",
      prod=stage{name="Produtor",handler=produzir,init=p_init},
      cons=stage{name="Consumidor",handler=consumir},
      quad=stage{name="Quadratificador",handler=quadratico},
      copy=stage{name="Copiador",handler=event_copy},
}

--g.copy=g.copy+(g.prod+(g.quad+g.cons))

insert(g.prod,g.cons)

insert(g.quad,g.cons)
insert(g.copy,g.prod)

g.copy.input.name="Copy_input"
g.prod.input.name="Prod_input"
g.quad.input.name="Quad_input"
g.cons.input.name="Cons_input"


--g.copy:set_method(leda.t)
g.prod:set_method(leda.e)
g.quad:set_method(leda.e)
g.cons:set_method(leda.e)

g.copy:send(10,os.time(),2)

--g:verify()
--g:dump()
g:run(leda.controller.fixed_thread_pool.get(10))
