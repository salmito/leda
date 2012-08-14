require "leda"
require "leda.utils.plot"
require "socket"
local fast_rand=require "fast_rand"

local dispatcher=leda.stage{
   handler=function (n,iterations,total,init_clock)
      local ite=math.floor(iterations/n)
      for i=1,n do
         leda.send(1,n,ite,init_clock,total)
      end
   end,
	init=function() require "math" end,
   name="dispatcher"
}

local dart=leda.stage{
   handler=function (n,iterations,init_clock,total)
--      print("init")
      local count=0
      for i=1,iterations do
        local x,y=fast_rand2.rand(),fast_rand2.rand()
         local z=x*x+y*y
         if z<=1 then count=count+1 end
      end
      leda.send(1,n,iterations,count,init_clock,total)
   end,
   init= function ()
      fast_rand2=require "fast_rand"
   end,
   name="dart"
}

local pi_reducer=leda.stage{
   handler=function (n,iterations,hit_count,init_clock,total)
      counter=counter+1
      hit=hit+hit_count
      total_it=total_it+iterations
      if counter == n then
         local pi=(hit*4)/total_it
         local cpu_time=socket.gettime()-init_clock
         io.stderr:write(string.format("pi_parallel\t%f\t%.12f\t%f\t%d\t%d\n",pi,math.abs(math.pi-pi),cpu_time,iterations*n,n))
         hit=0
         counter=0
         total_it=0
         local init=socket.gettime()
         if iterations<total then leda.send(1,n,iterations*n*2,total,init) end
     end
      
   end,
   serial=true,
   init=function ()
      hit=0
      counter=0
      total_it=0
      require 'math'
      require 'io'
      require 'socket'
   end,
   name="pi_reducer"
}
dispatcher:connect(dart)
dart:connect(pi_reducer)
pi_reducer:connect(dispatcher)


local pi=leda.graph{dispatcher,dart,pi_reducer}
local init=socket.gettime()
local n,it =tonumber(arg[1]),tonumber(arg[2])
fast_rand.seed(os.time(),os.time())
dispatcher:send(n,1024,it,init)

leda.plot_graph(pi,"graph.png")

pi:run(leda.controller.fixed_thread_pool.get(n))
