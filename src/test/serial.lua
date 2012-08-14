local utils=require "leda.utils"

local args={...}
local threads=args[1] or 10

local loop=
stage{
handler=function(th)
   print('iniciou thread',th)
   local i=0
   while true do
      i=i+1
      leda.send(1,i)
   end
end,
output={connector{sendf=leda.te}}}

local serial=stage{serial=true,
handler=
   function(i) 
      a=a or 0 
      a=a+1 
      print("Serial  ",a,i)
   end
}

local parallel=stage{
handler=
   function(i) 
      a=a or 0 
      a=a+1 
      print("Parallel ",a,i) 
      end}

serial.input=loop.output[1]
parallel.input=loop.output[1]

serial:set_method(leda.te)
parallel:set_method(leda.te)

print("Threads: ",threads)

for i=1,threads do
   loop:send(i)
end

local g=graph{copy,loop,serial,parallel}

g:run(leda.controller.fixed_thread_pool.get(threads))
