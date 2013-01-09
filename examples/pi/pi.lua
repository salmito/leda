require "leda"
local fixed=require "leda.controller.fixed_thread_pool"

local fast_rand=require "fast_rand"

local dispatcher=leda.stage{
   handler=function (n,iterations)
      local ite=math.floor(iterations/n)
      for i=1,n do
         leda.send(1,n,ite)
      end
   end,
	init=function() require "math" end,
   name="dispatcher"
}

local dart=leda.stage{
   handler=function (n,iterations)
      local count=0
      for i=1,iterations do
        local x,y=fast_rand2.rand(),fast_rand2.rand()
         local z=x*x+y*y
         if z<=1 then count=count+1 end
      end
      leda.send(1,n,iterations,count)
   end,
   init= function ()
      fast_rand2=require "fast_rand"
   end,
   name="dart"
}

local pi_reducer=leda.stage{
   handler=function (n,iterations,hit_count)
      counter=counter+1
      hit=hit+hit_count
      if counter == n then
         local pi=4*hit/(n*iterations)
         hit=0
         counter=0
         leda.send(1,n,iterations,pi)
     end
      
   end,
   serial=true,
   init=function ()
      hit=0
      counter=0
   end,
   name="pi_reducer"
}

local init_time=leda.gettime()

local result=leda.stage{
   handler=function(n,iterations,pi)
     io.stderr:write(string.format("pi_parallel\t%f\t%.12f\t%d\t%d\t%f\n",pi,math.abs(math.pi-pi),iterations*n,n,leda.gettime()-init_time))
		leda.quit()
   end,
   init=function ()
      require "math"
      require "io"
      require "string"
   end,
   name="result"
}

local pi=leda.graph{
   dispatcher:connect(dart),
   dart:connect(pi_reducer),
   pi_reducer:connect(result)
}


local n,it =tonumber(arg[1]),tonumber(arg[2])
dispatcher:send(n,it)
pi:run{controller=fixed.get(4)}
