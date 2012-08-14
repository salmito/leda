require "leda.utils.plot"

s1=leda.stage{"s1",handler=function() end}
s2=leda.stage{"s2",handler=function() end}
s3=leda.stage{"s3",handler=function() end}
s4=leda.stage{"s4",handler=function() end}
s5=leda.stage{"s5",handler=function() end}
s6=leda.stage{"s6",handler=function() end}

s1.output[1]=s2.input
s1.output[2]=s3.input
s2.output[1]=s4.input
s4.output[1]=s5.input
s4.output[2]=s6.input
s6.output[1]=s1.input

s1:input_method(leda.e)
s2:input_method(leda.t)
s3:input_method(leda.t)
s4:input_method(leda.e)
s5:input_method(leda.e)
s6:input_method(leda.e)


g=leda.graph{s1,s2,s3,s4,s5,s6}

leda.utils.plot.plot_graph(g)
