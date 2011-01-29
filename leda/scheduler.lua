require("leda.debug")
local sched={
	MAX_WORKERS=30,
	threads=alua.inc_threads(0) or 0,
	runtimestr="",
	stagename="",
	dumper=leda.dumper,
	nworkers=0,
	workers={},
	idle={},
	queue={
		first=0,last=-1,
		size=function(self)
			return self.last+1-self.first
		end,
		push=function(self,data)
			local last=self.last+1
			self.last=last
			self[last]=data
			--print(data)
		end,
		pop=function(self)
			local first=self.first
			if first > self.last then dbg("Scheduler: Queue is empty") return nil end
			local data=self[first]
			self[first]=nil
			self.first=first+1
			return data
		end
	}
}

function sched.sendcb(reply)
	if reply.status ~= alua.ALUA_STATUS_OK then
		dbg("Scheduler: Error sending to '"..reply.src.."': "..reply.error)
	end
end


function sched.compile()
end

function sched.start()
	dbg("Scheduler: Started on '"..alua.id.."' with stage '"..sched.stagename.."'")
	sched.createWorker(false)	
end

function sched.spawncb(reply)
  	if reply.status == alua.ALUA_STATUS_OK then
		dbg("Scheduler: Worker '"..sched.stagename.."' spawned: '"..reply.id.."'")
		sched.workers[#sched.workers+1]=reply.id
		--sched.idle[#sched.idle+1]=reply.id
		--alua.send(alua.id,"leda.scheduler.dispatch()",sched.sendcb)
  	else 
		dbg("Scheduler: Error spawning worker: "..reply.error)
	end
end

function sched.createWorker(create_thread)
	--Inicia runtime num estado alua separado
	local code=[[
		rt=]]..sched.runtimestr..[[
		]]..sched.dumperstr..[[=t
		rt.dumper=t
		dbg=]]..sched.debugstr..[[
		rt.scheduler=']]..alua.id..[['
		rt.init()
	]]
	if create_thread ~= false then
		sched.threads=alua.inc_threads(1)
	end
	sched.nworkers=sched.nworkers+1
	alua.spawn(code,true,sched.spawncb)
end

function sched.enqueue(datastr)
	sched.queue:push(datastr)

	if sched.queue:size() == 1 then
		--Fila estava vazia, chama o scheduler para agendar
		--print("Primeiro elemento",#sched.workers,sched.stagename)
		alua.send(alua.id,"leda.scheduler.dispatch()")
		--sched.dispatch()
	end
end

function sched.dispatch() 
	--Nao tem nada para dispachar
	--print("DESPACHANDO ",sched.nworkers,sched.MAX_WORKERS,sched.queue:size(),#sched.idle)
	if sched.queue:size()==0 then return end

	while #sched.idle>=1 do
		dbg(string.format("Scheduler: Dispatching '%s' queue_size=%d idle_workers=%d workers=%d threads=%d",sched.stagename,sched.queue:size(),#sched.idle,sched.nworkers,sched.threads))
		--tenho runtimes idle, mandar para o primeiro que achar
		local poped=sched.queue:pop()
		if poped == nil then break end
		local data="rt.consume("..poped..")"
		local worker=sched.idle[#sched.idle]
		sched.idle[#sched.idle]=nil

		alua.send(worker,data,sched.sendcb)
	end
	if sched.queue:size() > 0 and sched.nworkers < sched.MAX_WORKERS then
		--ainda tem fila e pode criar workers
		sched.createWorker()
		--print("CRIANDO ",sched.nworkers,sched.MAX_WORKERS,sched.queue:size(),#sched.idle)
	end
end

--Worker terminou, colocando ele no idle
function sched.finished(who)
	sched.idle[#sched.idle+1]=who
	
	alua.send(alua.id,"leda.scheduler.dispatch()")
end

function sched.bye(who)
	--tirando da lista de worker
	dbg("Scheduler: Removing worker '"..who.."'")
	sched.threads=alua.dec_threads(1)
	sched.nworkers=sched.nworkers-1
	for i=1,#sched.workers do
		if sched.workers[i]==who then
			sched.workers[i]=sched.workers[#sched.workers]
			break
		end
	end
	sched.workers[#sched.workers]=nil
	--tirando da lista de idle
	for i=1,#sched.idle do
		if sched.idle[i]==who then
			sched.idle[i]=sched.idle[#sched.idle]
			break
		end
	end
	sched.idle[#sched.idle]=nil
	if sched.nworkers<sched.MAX_WORKERS and sched.queue:size()>0 then
		alua.send(alua.id,"leda.scheduler.dispatch()")
	end
end

return sched
