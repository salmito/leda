local _=require'leda'

local events={}

return _.stage{
	handler=function(...)
		local ev={...}
		table.insert(events,ev)
		if #events==self.size then
			leda.push(events)
			events={}
		end
	end,
	serial=true,
	init=function()
		require 'table'
	end,
	name="Spatial window",
	bind=function(self,out)
		assert(out[1],"Output 1 of stage '"..tostring(self).."' must be connected")
		assert(self.size,"'size' must be set for stage "..tostring(self))
		self.name=self.name.."(size="..self.size..")"
	end
}
