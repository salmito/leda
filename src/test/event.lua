local event=require'lstage.event'

--marshal test
local a=event.encode("Test")
assert(event.decode(a)=="Test")
local lstage=require'lstage'
local thread=lstage.scheduler.new_thread()

local sleep=lstage.stage(function(...)
	print("sleeping")
	for i=1,10 do
		event.sleep(1)
		print((10-i)..'s remaining')
	end
	print("done",...)
	lstage.scheduler.kill_thread()
end)
sleep:push("event1")


local function handler(str,thread)
	local io=require'io'
	while true do
		event.waitfd(1,0)
		local a=io.stdin:read('*l')
		print("Typed",a)
	end	
end

local stage=lstage.stage(handler,1,1)
stage:push('test',thread)
print('Type something in the next 10s')

thread:join(11)
