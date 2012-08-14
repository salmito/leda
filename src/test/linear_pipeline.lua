require "leda.utils.plot"

local args={...}
local stages=args[1] or 1
local it=args[2] or 1
local thr=args[3] or 1

local g=leda.graph{}

local first_stage;

local loop_hand=function(it) for i=1,it do leda.output[1]:send() end end

print(loop_hand)

local loop=stage{handler=loop_hand,name="Looper"}

g:add_stage(loop)
local last_output=connector{connector{sendf=leda.e}};
loop.output[1]=last_output
local last_stage=loop

for i=1,stages do
   local h=function (i) print("executou",i) leda.output[1]:send(i) end
   local ss=stage{handler=h,init=init,output={connector{sendf=leda.e}},name="s"..i}
   
   leda.connect(last_stage,ss)   
   g:add_stage(ss)
   
   last_stage=ss
end


loop:send(it)
g:verify()
g:dump()
leda.utils.plot.plot_graph(g)

g:run(leda.controller.fixed_thread_pool.get(thr))
