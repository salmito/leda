require "leda"
local utils=require "leda.utils"

local arg={...}


local to10=leda.stage "From 1 to 10" {
   handler=function(n)
      for i=1,n do leda.send(1,i) end
   end,
   autostart=tonumber(arg[2]) or 10,
   init=--function () 
   "print('FUCK')"-- end
}

local power2=leda.stage "Power of two" {
   handler=function (data,...)
      leda.send(1,data*data,...)
   end
}

local sqrt=leda.stage "sqrt" {
   handler=function(data,...)
       local math=require('math')
      leda.send(1,math.sqrt(data))
   end
}

local printer=leda.stage(utils.print)
printer.serial=true

local g=leda.graph{"Producer-consumer",
--   to10:compose(1,power2):compose(1,cons)
   to10:compose(power2):connect(printer),
--   power2:connect(sqrt), sqrt:connect(printer)
}

local th = th or tonumber(arg[3]) or leda.kernel.cpu()

g:run()--{maxpar=16,controller=leda.controller.fixed_thread_pool.get(th)}
