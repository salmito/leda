local _=require'leda'

local stage={}

function stage.handler(v,...)
	for i=1,#v do
		v[i]=self.f(v[i])
	end
	leda.push(v,...)
end

function stage:bind(out)
	assert(out[1],"Default output must be connected")
	assert(type(self.f)=='function',"Map function (f field) must be a function")
end

stage.name="Map"

return _.stage(stage)
