require "leda"
require "leda.utils.plot"

local f=function(a) print(leda.stage.name,a,leda.send(1,a)) end

s1=leda.stage{name="stage1",handler=f,serial=true}
s2=leda.stage{name="stage2",handler=f}
s3=leda.stage{name="stage3",handler=f}
nul=leda.stage{name="nul",handler=f}
s4=leda.stage{name="stage4",handler=f}
s5=leda.stage{name="stage5",handler=f}
s6=leda.stage{name="stage6",handler=f}

local g2=leda.graph{"Teste",
   start=nul,
}

local stage_graph=leda.graph{"Stage graph",
   start=s1,
   s1:connect(s2,leda.call),
   s1:connect(2,s3,leda.fork),
   s4:connect(2,s5,leda.call),
   s4:connect(1,s6),
   s6:connect(s1) --cycle
}

stage_graph:plot()

s0=leda.stage{name="stage0",serial=true,handler=f}
stage_graph:add_connector(s6:connect(s0))
stage_graph:add_connector(s2:connect(s4))

stage_graph:plot()

local p1=stage_graph:create_cluster("Cluster 1",s2,s3,s1)
local p2=stage_graph:create_cluster("Cluster 2",s6)
local p3=stage_graph:create_cluster("Cluster 3",s0)

--p2:add_daemon("127.0.0.1",8888)
p3:add_daemon("127.0.0.1",7777)
--p3:add_daemon("127.0.0.1",7777)

stage_graph:plot()
stage_graph:send("a1")
stage_graph:send("a2")
stage_graph:send("a3")
stage_graph:send("a4")
--s0:send("remote")
stage_graph:run()

