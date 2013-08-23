local _=require "leda"
local stage = _.stage{
	handler = function(str)
		local f=string.gmatch(str,self.pattern)
		local matches = {f()}
		if #matches > 0 then
			assert(leda.push(unpack(matches)))
   		matches = {f()}
		end
	end,
	init = function()
		require "string"
	end,
	bind = function(self,output)
		assert(self.pattern,"Pattern field must be defined")
		assert(output[1],"Default (1) port must be connected")
	end,
	name='match'
}
return stage
