local utils=require "leda.utils"

local args={...}
local threads=args[1] or 1

local loop=stage{utils.loop,output={connector()},init="print('iniciou')"}
local serial=stage{serial=true,handler=function() a=a or 0 a=a+1 print("Serial  ",a) end}
local parallel=stage{handler=function() a=a or 0 a=a+1 print("Parallel",a) end}

serial.input=loop.output[1]
parallel.input=loop.output[1]

serial:set_method(leda.e)
parallel:set_method(leda.e)

print("Threads: ",threads)

for i=1,threads do
   loop:send(1)
end

local g=graph{copy,loop,serial,parallel}

g:run(leda.controller.fixed_thread_pool.get(threads))
