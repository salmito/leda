require "leda"

local f=function(event)
   leda.sleep(1)
   local s,err=leda.send('output1',event) 
   if not s then
      print("ERROR",leda.stage.name,err)
   end
   local s,err=leda.send('output2',event)
   if not s then
      print("ERROR",leda.stage.name,err)
   end
end

local f2=function(event) 
  local s,err=leda.send('output1',event) 
   if not s then
      print("ERROR",leda.stage.name,err)
   end
end

local f3=function(event)
end

stage1=leda.stage{name="stage1",handler=f}
stage2=leda.stage{name="stage2",handler=f2}
stage3=leda.stage{name="stage3",handler=f3}
stage4=leda.stage{name="stage4",handler=f}
stage5=leda.stage{name="stage5",handler=f2}
stage6=leda.stage{name="stage6",handler=f3}

local grafo=leda.graph{"Grafo",
	start=stage1, --opcional
	stage1:connect('output1',stage2),
	stage1:connect('output2',stage3),
	stage2:connect('output1',stage4),
	stage4:connect('output1',stage5),
	stage4:connect('output2',stage6,leda.couple),
	stage5:connect('output1',stage1)
}
--grafo:plot()
local a1=leda.cluster(stage1,stage2)
local a2=leda.cluster(stage3)
local a3=leda.cluster(stage4)
local a4=leda.cluster(stage5,stage6)

--grafo:part{a1,a2,a3,a4}

stage1:send("e1")
stage1:send("e2")
stage1:send("e3")
stage1:send("e4")
stage1:send("e5")
stage1:send("e6")

g=grafo

grafo:run()

