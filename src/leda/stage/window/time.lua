local _=require'leda'

return _.stage{
	handler=function()
	  	while true do
			local buf={}
			leda.sleep(self.period)
			local event=leda.debug.get_event()
			while event do
				table.insert(buf,event)
				event=leda.debug.get_event()
			end
   	   leda.push(buf)
		end
	end,
	serial=true,
	autostart=true,
	init=function()
		require 'table'
	end,
	name="Time window",
	bind=function(self,out)
		assert(out[1],"Output 1 of stage '"..tostring(self).."' must be connected")
		assert(self.period,"'period' must be set for stage "..tostring(self))
		self.period=assert(tonumber(self.period),"'period' field must be a number")
		self.name=self.name.."(period="..self.period.."s)"
	end
}
