module("leda",package.seeall)

local stages={}

require("leda.dumper")
require("leda.server")
require("leda.runtime")

_G.rt=leda.runtime.rt

function createStage(name,code)
	assert(name,"Leda: Error: A stage must be provided")

	if code==nil then
		code=name
		name=code.name
	end

	assert(name,"Leda: Error: Stages must have a name")
	assert(code.run,"Leda: Error: Stages must implement the run function")
	for k,v in pairs(stages) do
		assert(k~=name,"Leda: Error: Stages must have a unique name")
	end
	
	stages[name]={name=name,code=code}

	return stages[name]
end

function connect(m1,m2) 
	assert(m1,"Leda: Error: Stages must have been instantiated fist")
	assert(m2,"Leda: Error: Stages must have been instantiated fist")
	assert(m1.name,"Leda: Error: Stages must have been instantiated fist")
	assert(m2.name,"Leda: Error: Stages must have been instantiated fist")
	assert(m1.code.run,"Leda: Error: Stages must implement run function")
	assert(m2.code.run,"Leda: Error: Stages must implement run function")

	m1.nextmod=m2.name

	print("Leda: Stage "..m1.name.." connected to "..m2.name)
end

function start()
	server.start(stages)
end

