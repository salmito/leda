#!/usr/bin/lua
require("leda")

produtor={name="Produtor"}

function produtor.init()
	run()
end

function	produtor.run(data)
	print("Produtor RUN ")
	local a=100
	
	while a>0 do
		a=a-1
		local data=math.random(1,255)
		print(string.format("Produzi dado: %s",data))
		bool=leda.enqueue("Consumidor",{a=data})
		if not bool then
			break
		end
		--os.execute("sleep " .. 1)
	end
end

consumidor={
	a=0; --variavel local do runtime
	name="Consumidor",
	init=function()
		leda.shared(function() var=var or 0  end) --var eh variavel global do estagio
	end,
	run=function(data)
		this.a=this.a+1
		os.execute("sleep " .. 1)
		local z=leda.shared(function () var=var+1 return var end)
		print(string.format("Recebi dado: %s local var=%d global var=%d",data.a,this.a,z))
		--leda.done()
	end
}

--Liga a saida padrao do produtor com o consumidor
prod=leda.createStage("Produtor",produtor)
cons=leda.createStage("Consumidor",consumidor)

leda.start()
