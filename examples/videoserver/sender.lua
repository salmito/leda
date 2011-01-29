sender={name="Sender",videodir="/home/tiago/videos"}

function sender.run(who)
	--use this to access the instance local functions
	if this.validate(who.file) then
		local path=this.videodir..'/'..who.file
		print("Opening ",path)
		local file=io.open(path,"r")
		if file==nil then
			print("404-Not found")
			leda.done()
		end
		print(string.format("Connecting to tcp://%s:%d",who.host,who.port))
		local cli,err=socket.connect(who.host,who.port)
		if cli==nil then
			print("Error: "..err)
			leda.done()
		end
		print("Sending video")
		while true do
			local block=file:read(2^13) --buffer 8k
			if not block then break end
			local s,err=cli:send(block)
			if s==nil then
				print("Error: "..err)
				break
			end
		end
		print("Closing connection")
		cli:close()
		leda.done()
	end
end

function sender.validate(filename)
	if filename=="" then
		return false
	end
	if string.sub(filename,1,1)=="." then
		return false
	end
	if string.sub(filename,1,1)=="/" then
		return false
	end
	return true
end

return sender
