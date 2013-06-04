require "leda"
require "leda.utils.plot"

s1=leda.stage{name="s1",handler=function() end}
s2=leda.stage{name="s2",handler=function() end}
s3=leda.stage{name="s3",handler=function() end}
s4=leda.stage{name="s4",handler=function() end}
s5=leda.stage{name="s5",handler=function() end}
s6=leda.stage{name="s6",handler=function() end}

local graph=leda.graph{"Graph",
	start=s1,
	s1:connect('output1',s2),
	s1:connect('output2',s3),
	s2:connect('output1',s4),
	s4:connect('output1',s5),
	s4:connect('output2',s6,leda.couple),
	s5:connect('output1',s1)
}
graph:plot()

