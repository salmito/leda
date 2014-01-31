local event=require'leda.event'

--marshal test
local a=event.encode("Test")


assert(event.decode(a)=="Test")

local leda=require'leda.new'
local thread=leda.scheduler.new_thread()

local function handler(str,thread)
	while true do
		event.waitfd(1,0)
		local a=io.stdin:read('*l')
		print("Typed",a)
	end	
	print(str,thread)
end

local stage=leda.stage(handler,1,1)
stage:push('test',thread)
print('sent',thread)
thread:join(1)
