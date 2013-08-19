local _=require 'leda'

local stage={}

function stage.handler(period,n,...)
	local i=tonumber(n)
	local it,start=1,leda.gettime()
	local loop=i<1
	repeat
		if period <= 0 then
			leda.nice()
		else
			local now=(start+(it*period)) - leda.gettime()
			if now>0 then
				leda.sleep(now)
			end
			it=it+1
		end
		leda.send(1,...)
		if not loop then
			i=i-1
		end
	until not (loop or i>0)
	leda.send('end',period,n)
end

function stage:bind(out)
	assert(out[1],"Output port '1' must be connected for stage '"..tostring(self).."'")
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

return _.stage(stage)
