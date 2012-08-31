require "leda"

local f=function(event) print(leda.stage.name,event) local r,err=leda.send('output1',event) end --print(leda.stage.name,a) end

stage1=leda.stage{name="stage1",handler=f,bind=function (o) assert(o.output1) o.output1.type=leda.cohort end}
stage2=leda.stage{name="stage2",handler=f}
stage3=leda.stage{name="stage3",handler=f}
stage4=leda.stage{name="stage4",handler=f}
stage5=leda.stage{name="stage5",handler=f}
stage6=leda.stage{name="stage6",handler=f}

local grafo=leda.graph{"Grafo",
	start=stage1, --opcional
	stage1:connect('output1',stage2,leda.cohort),
	stage1:connect('output2',stage3),
	stage2:connect('output1',stage4),
	stage4:connect('output1',stage5),
	stage4:connect('output2',stage6,leda.couple),
--	stage6:connect('output1',stage1)
}
--grafo:plot()
local a1=leda.cluster(stage1,stage2)
local a2=leda.cluster(stage3)
local a3=leda.cluster(stage4)
local a4=leda.cluster(stage5,stage6)

--grafo:part{a1,a2,a3,a4}

stage1:send("e1")

--grafo:run()

return grafo

