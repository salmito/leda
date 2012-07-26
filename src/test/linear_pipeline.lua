local utils=require "leda.utils"

local args={...}
local stages=args[1] or 1
local it=args[2] or 1
local thr=args[3] or 1

local g=graph{}

local first_stage;

local loop_hand="for i=1,"..it.." do leda.output[1]:send() end"

local loop=stage{handler=loop_hand,name="Looper"}

g:add_stage(loop)
local last_output=connector{};
loop.output[1]=last_output
local last_stage=loop

for i=1,stages do
   local init="print('iniciou "..i.."')"
   local h="print('executou "..i.."') leda.output[1]:send()"
   local ss=stage{handler=h,init=init,output={connector{sendf=leda.e}},name="s"..i}
   
   utils.insert_after(last_stage,ss)   
   g:add_stage(ss)
   
   last_stage=ss
end


loop:send(0)
g:dump()

g:run(leda.controller.fixed_thread_pool.get(thr))
