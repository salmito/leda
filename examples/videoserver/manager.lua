local manager={name="Manager"}

function manager.init()
	run({type="listen"})
end

function manager.run(op)
	if op.type == "listen" then
		local server = assert(socket.bind("*", 8080))
		local ip, port = server:getsockname()
		print("Server started on port '"..port.."'")
		while true do
			client = server:accept()
			local fd=client:getfd()
			client:send("Ok ")
			leda.enqueue("Manager",{type="read",socket=client:getfd()})
		end
	elseif op.type=="read" then
		if op.socket~=nil then
			local line,err=leda.receive(op.socket)
			local host,port,file=string.match(line,"(.*):(%d*)/(.*)")
			if host==nil or port==nil or file==nil then
				leda.send(op.socket,"Invalid syntax")
				leda.close(op.socket)
				leda.done()
			end
			leda.send(op.socket,"Syntax ok")
			leda.close(op.socket)
			leda.enqueue("Sender",{host=host,port=port,file=file})
		end
		done()
		--local line, err = client:receive()
	end
	
end
return manager
