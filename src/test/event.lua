local event=require'leda.event'

--marshal test
local a=event.encode("Test")
assert(event.decode(a)=="Test")
local leda=require'leda.new'
local thread=leda.scheduler.new_thread()

local sleep=leda.stage(function(...)
	print("sleeping")
	for i=1,10 do
		event.sleep(1)
		print((10-i)..'s remaining')
	end
	print("done",...)
	leda.scheduler.kill_thread()
end,1,1)
sleep:push("event1")


local function handler(str,thread)
	while true do
		event.waitfd(1,0)
		local a=io.stdin:read('*l')
		print("Typed",a)
	end	
end

local stage=leda.stage(handler,1,1)
stage:push('test',thread)
print('Type something in the next 10s')

thread:join(11)
