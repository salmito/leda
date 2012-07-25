require "leda"

local graph,connector,stage=leda.graph,leda.connector,leda.stage

local s1=stage{name="s1",
   handler=
   function () 
      test(100,10)
      test(1000,10)
      test(10000,10)
      test(100000,10)
      test(1000000,10)
      test(10000000,10)         

      test(100,100)
      test(1000,100)
      test(10000,100)
      test(100000,100)
      test(1000000,100)
      test(10000000,100)         

      test(100,1000)
      test(1000,1000)
      test(10000,1000)
      test(100000,1000)
      test(1000000,1000)
    --  test(10000000,1000)         

   end
   ,
   init=
      function () 
         require "socket" 
         function test(N,M)
            local init=socket.gettime()
            par={}
            for j=0,M do par[j]=1 end
            for i=1,N do
               leda.get_output(1):send(i,init,N,M,unpack(par))
            end
         end
      end 
   
}

local s2=stage{
   name="s2",
   handler=
      function(i,init,N,M)
         if i==N then
            print(3,"Thread_based",N,M,socket.gettime()-init)
         end
      end,
   init=function () require "socket" end 
}

s1.output={s2.input}

g=graph{s1,s2}
s2:set_method(leda.t)
s1:send()
g:run(leda.controller.fixed_thread_pool.get(2))
