local leda=require'leda.new'
local time=leda.now()

local th=leda.scheduler.new_thread()

for i=1,100000 do
	leda.stage(function() end)
end

leda.scheduler.kill_thread()
th:join()
print("ended",leda.now()-time)
