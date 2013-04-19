require 'leda'

local stage={}

function stage.handler(period,n,...)
	local i=tonumber(n)
	local loop=i<1
	repeat
		if period <= 0 then
			leda.nice()
		else
			leda.sleep(period)
		end
		leda.send(1,...)
		if not loop then
			i=i-1
		end
	until not (loop or i>0)
	leda.send('end',period,n)
end

stage.name="Timer"

stage._DESCRIPTION=[[
This stage implements a generic timer.
It is activated by sending an event containing
a period and the number of events that occur
when the specified timer interval has elapsed.

Notes:
If period <= 0 then it will not sleep between iterations
If n < 1 then it will loop forever
]]

stage._VERSION='0.1'

stage.test=function(p,n)
   local utils=require 'leda.utils'
   local s=leda.stage(stage)
   s:send(p or 1,n or 1,"this is an event with PI",math.pi, function (pi) print('Callback! '..pi) end)
   local g=leda.graph{
      s:connect(leda.stage'printer'{function (str,pi,callback) callback(pi) print(str,pi) end}),
      s:connect('end',leda.stage'end'("print('END') leda.quit()"))
   }
   --g:plot()
   g:run()
end

return leda.stage(stage)
