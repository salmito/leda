local utils=require "leda.utils"

local args={...}
local threads=args[1] or 10

assert(tonumber(threads)>1,"At least 2 threads are required for this test")

local loop=stage{utils.loop,output={connector()}}
local serial=stage{handler=function() a=a or 0 a=a+1 print("Serial  ",a) leda.sleep(0) end,serial=true}
local parallel=stage{handler=function() a=a or 0 a=a+1 print("Parallel",a) leda.sleep(0) end}

serial.input=loop.output[1]
parallel.input=loop.output[1]

serial:set_method(leda.te)

print("Threads: ",threads)

for i=1,threads do
   loop:send(1.0)
end

local g=graph{copy,loop,serial,parallel}

g:run(leda.controller.fixed_thread_pool.get(threads))
