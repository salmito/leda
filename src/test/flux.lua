local utils=require "leda.utils"

s1=leda.stage(function()
	for i=1,10 do
		leda.newflux()
		leda.setflux('a',i)
		leda.send(1,i)
	end
end,function() require 'leda.utils.flux' end)

s2=leda.stage(function(i)
	for j=1,10 do
		leda.setflux('b',j)
		leda.send(1,i,j)
	end
end,function() require 'leda.utils.flux' end)


s3=leda.stage(function(i,j)
	print(leda.getflux('a'),leda.getflux('b'))
	if i==10 and j==10 then
		leda.quit()
	end
end,function() require 'leda.utils.flux' end)


local g=leda.graph{s1:connect(s2),s2:connect(s3)}

s1:send()

g:run()
