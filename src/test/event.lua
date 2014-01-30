local event=require'leda.event'

--marshal test
local a=event.encode("Test")

assert(event.decode(a)=="Test")

local leda=require'leda.new'
local thread=leda.scheduler.new_thread()

local function handler(str,thread)
	print(str,thread)
end

local stage=leda.stage(handler,1,1)
stage:push('test',thread)
print('sent',thread)
