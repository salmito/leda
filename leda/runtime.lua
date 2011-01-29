module("leda.runtime",package.seeall)

require("socket")
require("leda.debug")

--function new(mainid,nextid,code) return {mainid=mainid,nextd=nextid,this=code,dump=require(datadumper).DataDumper}

--function enqueue(data) 
--	datacode=dump(data,"data")
--	alua.send(nextid,"leda.scheduler.enqueue_event("..datacode..")")
--end

rt={
	processes={},
	remotedaemon="",
	stage={},
	scheduler="",
	corun=""
}

--exporta as funcoes visiveis nos estagios (enqueue, etc)
function	rt.init()
	--print("Runtime: Created for worker '"..alua.id.."'")
	leda=leda or {}
	leda.enqueue=rt.enqueue
	this=rt.stage.code
	leda.done=rt.done
	run=this.run
	leda.send=rt.send
	leda.receive=rt.receive
	leda.close=rt.close
	leda.shared=rt.shared

	if this.init==nil then
		rt.finish()
	else
		rt.corun=coroutine.create(this.init)
		rt.resume()
	end
	--rt.stage.code.init()
end

function rt.sendcb(reply)
	if reply.status ~= alua.ALUA_STATUS_OK then
		dbg("Runtime: Error sending to '"..reply.src.."': "..reply.error)
	end
end

function	rt.update_processes(p)
	processes=p
end

function rt.send(sock,str)
	alua.rawsend.setfd("client",sock)
	alua.rawsend.send("client",str)
end

function rt.receive(sock,str)
	local cli=socket.tcp()
	cli:setfd(sock)
	return cli:receiveraw()
end

function rt.close(sock)
	local cli=socket.tcp()
	cli:setfd(sock)
	return cli:close()
end

function rt.enqueue(stage,data)
	if data == nil then
		data=stage
		stage=""
	end
	return coroutine.yield("enqueue",stage,data)
end

function rt.shared(func)
	return coroutine.yield("shared","",func)
end

function rt.enqueue_ack()
	--print("Runtime: Received enqueue ack")
	--ativando corotina
	rt.resume(true)
end

function rt.shared_ack(ret)
	--print("Runtime: Received shared ack")
	--ativando corotina
	rt.resume(ret)
end

function rt.resume(data)
	local ok,yield,stage,ret=coroutine.resume(rt.corun,data)
	--print("YIELDOU",ok,coroutine.status(rt.corun),yield,ret)
	if coroutine.status(rt.corun)=="dead" or ok==false then
		rt.finish()
		return
	end
	if yield=="enqueue" then
		local receiver=rt.stage.nextid
		if stage~="" then
			receiver=""
			for _,v in pairs(rt.stages) do
				if v.stage==stage then
					receiver=v.id
				end
			end
		end
		if receiver=="" or receiver==nil then
			if stage=="" then stage=rt.stage.nextmod or "" end
			dbg("Runtime: Error: Stage '"..stage.."' not running")
			alua.send(alua.id,"rt.resume(false)")
			return
		else
			dbg("Runtime: Receiver is "..receiver)
			alua.send(rt.scheduler,"alua.send('"..receiver.."','leda.scheduler.enqueue(\\'"..rt.dumper(ret).."\\')',leda.scheduler.sendcb)",rt.sendcb)
			alua.send(alua.id,"rt.enqueue_ack()",rt.sendcb)
		end

	elseif yield=="shared" then
		alua.send(rt.scheduler,"local func="..rt.dumper(ret).." local ret=leda.scheduler.dumper(func()) alua.send('"..alua.id.."','rt.shared_ack('..ret..')')",rt.sendcb)

	elseif
		yield=="done" then
		alua.send(rt.scheduler,"leda.scheduler.bye('"..alua.id.."')",rt.sendcb)
		alua.quit()
	end

end

function rt.finish()
	rt.corun=nil
	alua.send(rt.scheduler,"leda.scheduler.finished('"..alua.id.."')")
end

function rt.consume(data)
	rt.corun=coroutine.create(this.run)
	rt.resume(data)
end

function rt.done()
	coroutine.yield("done")
end
