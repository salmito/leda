local leda=require "leda"
local args={...}
local n=args[1] or 10
local type=arg[2] or "decoupled"
local nice=arg[3] or false
local threads=arg[4] or leda.kernel.cpu()
local maxpar=arg[5] or 100

local function gen()
	local init=leda.gettime()
	local i=tonumber(n)
	while i>0 do
		leda.send('output1',leda.gettime(),i)
		if nice=="true" then leda.nice() end
		i=i-1
	end
end

local f=function(...)
     local s,err=leda.send('output1',...) 
     if not s then
        print("ERROR",leda.stage.name,err)
     end
   local s,err=leda.send('output2',...)
   if not s then
      print("ERROR",leda.stage.name,err)
   end
end

local f2=function(...) 
  local s,err=leda.send('output1',...) 
   if not s then
      print("ERROR",leda.stage.name,err)
   end
end

local f3=function(event)
end

local t=leda.gettime()

local function pend(time,i)
	local latency=leda.gettime()-time
	if i%10000==0 then
		print("raw-transaction",type,n,latency,threads,maxpar,i,nice,'end')
	end
	if i==1 then
		print("raw-transaction-troughput",type,n,n/(leda.gettime()-t),threads,maxpar,n,nice,'end')
		leda.quit()
	end
end

stage1=leda.stage{name="stage1",handler=gen}
stage2=leda.stage{name="stage2",handler=f2,stateful=true}
stage3=leda.stage{name="stage3",handler=f3}
stage4=leda.stage{name="stage4",handler=f}
stage5=leda.stage{name="stage5",handler=pend}
stage6=leda.stage{name="stage6",handler=f3}



local grafo=leda.graph{"Grafo",
	start=stage1, --opcional
	stage1:connect('output1',stage2,type),
	stage1:connect('output2',stage3,type),
	stage2:connect('output1',stage4,type),
	stage4:connect('output1',stage5,type),
	stage4:connect('output2',stage6,type),
	stage5:connect('output1',stage1)
}
grafo:plot()
local a1=leda.cluster(stage1,stage2)
local a2=leda.cluster(stage3)
local a3=leda.cluster(stage4)
local a4=leda.cluster(stage5,stage6)

--grafo:part{a1,a2,a3,a4}

--for i=1,n do
	stage1:send("event")
--end

g=grafo

--g:part(stage1,g:all()-stage1):map('localhost:9999','localhost:8888')

grafo:run{maxpar=maxpar,controller=leda.controller.interactive}

