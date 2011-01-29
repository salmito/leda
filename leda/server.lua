module("leda.server",package.seeall)

require("leda.dumper")
local cmdline=require("leda.cmdline")
require("alua")
require("leda.debug")
local remotedaemon=""
local run=false
local running=false
local procs=1
local stages={}

local port=9999

local function assertgood(...) return assert(cmdline.getparam(...)) end

local options={"link","port","connect","ip","procs","run"}
local localip="127.0.0.1"

local function conncb(reply)
	if reply.status == alua.ALUA_STATUS_OK then
		dbg("Server: Connected to daemon '"..reply.daemonid.."'")
		rt.remotedaemon=reply.daemonid
		dbg("Server: Sending hello")
		local param="false"
		--Sending run flag to daemon
		if run==true then
			param="true"
		end
		alua.send(alua.daemonid,"leda.server.hello({'"..alua.id.."'},"..param..")",sendcb)
	else
		dbg("Server: Error connecting: "..reply.error)
		--assert(false,"Error")
	end
end
function linkcb(reply)
	if reply.status == alua.ALUA_STATUS_OK then
		dbg("Server: Successfully linked with daemon")
		for i,v in ipairs(reply.daemons) do
			if v~=alua.id then
				--mandar hello
				local param="false"
				if run==true then
					param="true"
				end
				local n=procs-1
				local ids="{'"..alua.id.."'"
				for i=1,n do
					ids=ids..",'"..string.sub(alua.id,1,-2)..i.."'"
				end
				ids=ids.."}"

				alua.send(v,"leda.server.hello("..ids..","..param..")",sendcb)
			end
		end
	elseif reply.status == alua.ALUA_STATUS_ERROR then
		dbg("Server: Error: "..reply.error)
		alua.quit()
	end
end

local p_spawned=1
local function spawncb(reply)
	 if reply.status == alua.ALUA_STATUS_OK then
		rt.processes[#rt.processes+1]=reply.id
		dbg("Server: process "..reply.id.." spawned")
		 p_spawned=p_spawned+1
		 if p_spawned==procs and run then --terminou
			 execute()
		 end
	 else
		dbg("Server: Error spawning process: "..reply.error)
		alua.quit()
	 end
end

local function sendcb(reply)
	 if reply.status == alua.ALUA_STATUS_OK then
		-- print("Message sent to "..reply.src)
	 else
		dbg("Server: Error sending to "..reply.src..": "..reply.error)
	 end
end

local daemons={}
--TODO ver problema dos daemons encadeados
local function linkspawncb(reply)
	 if reply.status == alua.ALUA_STATUS_OK then
		 --alua.send(rt.remotedaemon,"leda.server.hello('"..reply.id.."')",sendcb)
		 dbg("Server: process '"..reply.id.."' spawned")
		 p_spawned=p_spawned+1
		 if p_spawned==procs then --terminou todos os processos helpers
			 alua.link(daemons,linkcb)
		 end
	 else
		 dbg("Server: Error spawning process: "..reply.error)
		 alua.quit()
	 end
end

local function pingcb(reply)
	if reply.status == alua.ALUA_STATUS_OK then
		dbg("Server: Ping sent to "..reply.src)
		local proc=rt.processes
		proc[#proc+1]=reply.src

		local procstr=leda.dumper(proc)
		--for _,p in pairs(proc) do
		--	alua.send(p,"rt.update_processes("..procstr..")",sendcb)
		--end
	else
		dbg("Server: Error pinging: "..reply.error)
	end
end

--TODO timer para ficar chamando ping
function ping()
	--print("Ping2")
	local proc=rt.processes
	rt.processes={}
	for _,p in pairs(proc) do
		if p==alua.id then
			rt.processes[#rt.processes+1]=alua.id
		else
			alua.send(p,"dbg('Server: Received ping')",pingcb)
		end
	end
end


function execute() 
	if running==true then
		dbg("Server: Error: Already running code")
		return
	end
	running=true
	local num_stages = 0;
	for k,v in pairs(stages) do
		num_stages=num_stages+1
	end

	--TODO por enquanto cada modulo tem um processo pra ele
	if num_stages > #rt.processes then
		dbg(string.format("Server: Error: Insuficient processes (%d) for %d stages, spwan %d more process(es)",#rt.processes,num_stages, num_stages-#rt.processes))
		running=false
		return
	end

	--Distribuindo com preferencia a daemons
	local sp=rt.processes
	table.sort(sp,function(a,b) return string.match(a,".*/(%d)")<string.match(b,".*/(%d)")end)

	local nextproc=1
	rt.stages={}

	for k,v in pairs(stages) do
		v.aluaid=sp[nextproc]
		nextproc=nextproc+1
		rt.stages[#rt.stages+1]={id=v.aluaid,stage=k}
		dbg(string.format("Server: Stage '%s' is on process '%s'",v.name,v.aluaid))
	end
	--Todos os modulos associados
	--Associando as filas dos modulos
	for k,v in pairs(stages) do
		if v.nextmod~=nil then
			v.nextid=stages[v.nextmod].aluaid
			
			dbg(string.format("Server: Stage '%s' connected to stage '%s'",v.aluaid,v.nextid))
		end
	end
	--inicializa o runtime
	--print("CODE",leda.dumper(rt))

	--PAREI AQUI: Ja sei onde vai cada modulo e quais ids tao conectados

	--Distribuindo runtimes
	
	local sched=require("leda.scheduler")
	sched.compile()
	local schserial=leda.dumper(sched)
	local dumperserial=leda.dumper(leda.dumper)
	local debugserial=leda.dumper(dbg)
	for n,s in pairs(stages) do
		dbg("Server: Deployng Stage '"..n.."' scheduler in '"..s.aluaid.."'")
		rt.stage=s
	
		local rtcode=leda.dumper(rt)
--		print("KD?",s.aluaid,rtcode)

		local code=[[_G.leda=_G.leda or {}
		]]..schserial..[[=t
		leda.scheduler=t
		]]..dumperserial..[[=t
		local ldumper=t
		dbg=]]..debugserial..[[
		leda.scheduler.debugstr=ldumper(dbg)
		leda.scheduler.dumperstr=ldumper(ldumper)
		leda.scheduler.stagename=']]..n..[['
		local lrt=]]..rtcode..[[
		leda.scheduler.runtimestr=ldumper(lrt)
		leda.scheduler.start()
		]]
		--print("MERDA")
		--leda.scheduler.dumperstr=]].."[["..dumperserial.."]]"..[[
		--leda.scheduler.runtimestr=]].."[["..rtcode.."]]"..[[
		--print("ENV ",code)
		alua.send(s.aluaid,code,sendcb)
	end
end

--TODO quando receber e ja estiver executando,instanciar modulo que pode ser usado varias vezes
function hello(new_processes,run_now)
	for k,p in pairs(new_processes) do
		dbg("Hello from "..p)
		rt.processes[#rt.processes+1]=p
	end
	if run_now==true then
		execute()
	end
	alua.send(alua.id,"leda.server.ping()",sendcb)
end

local function handlecmd(t) 
	local host,port="",0
	local processport=t.port or 9999
	procs=t.procs or 1
	if procs<1 then procs=1 end

	localip=t.ip or "127.0.0.1"

	for i=1,#arg do
		if arg[i]=="run" then
			run=true
		end
	end

	if t.connect~=nil then
		host, port = string.match(t.connect, "(.+):(%d+)")
		assert(host,"Leda: Sintax error: argument syntax is 'addr:port'")
		assert(port,"Leda: Sintax error: argument syntax is 'addr:port'")
		
		dbg(string.format("Server: Connecting to daemon '%s:%d'",host,port))
		local reply,error=alua.connect(host,port,conncb)
		if not reply then
			dbg("Server: Error connecting: "..error)
			alua.quit()
		end

		alua.loop()
		return
	end

	if t.link~=nil then
		 host, port = string.match(t.link, "(.+):(%d+)")
		
		assert(host,"Leda: Sintax error: argument syntax is 'addr:port'")
		assert(port,"Leda: Sintax error: argument syntax is 'addr:port'")
		
		dbg(string.format("Server: Creating daemon on port '%d' and linking with '%s:%d'",processport,host,port))
	
		alua.create(localip,processport)
		rt.remotedaemon=string.format("%s:%d/0",host,port)
		
		daemons={alua.id,rt.remotedaemon}

		if procs==1 then
			alua.link(daemons,linkcb)
		else
			for i=2,procs do
				alua.spawn("",false,linkspawncb)
			end
		end

		alua.loop()
		return
	end
	dbg(string.format("Server: Creating daemon '%s:%d'",localip,processport))
	alua.create(localip,processport)
	rt.processes[#rt.processes+1]=alua.id
	rt.remotedaemon=alua.id

	for i=2,procs do
--	rt.processes[#rt.processes+1]=alua.id
		alua.spawn("",false,spawncb)
	end
	if procs==1 and run then
		execute()
	end
	alua.loop()
end

function start(stg)
		stages=stg
		local t_out=assertgood(arg,options)
		handlecmd(t_out)
		--alua.create("127.0.0.1",port)
		--processes[#processes+1]=alua.id
		--alua.loop()
end
