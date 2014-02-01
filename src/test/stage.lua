require'leda.event'
require'leda.scheduler'
require'leda.stage'
local leda=require'leda.new'

local th=leda.scheduler.new_thread()

local a=0

local dummy=leda.stage(function()
	print('dummy',a)
	leda.scheduler.kill_thread()
end)

local stage=leda.stage(function()
	a=a+1
	if a==10 then
		print('pushing',dummy)
		dummy:push()
	end
	print('a=',a)
end,1)
print("calling",stage,dummy)
for i=1,10 do stage:push() end
th:join(0)
th:rawkill()
