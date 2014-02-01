local leda=require'leda.new'

local th=leda.scheduler.new_thread()

local a=0

local stage=leda.stage(function()
	dummy=dummy or leda.stage(function()
		print('dummy',a)
		leda.scheduler.kill_thread()
	end)
	a=a+1
	if a==10 then
		print('pushing',dummy)
		dummy:push()
	end
	print('a=',a)
end)
print("calling",stage,dummy)
for i=1,10 do stage:push() end
th:join()
local all=require'leda.stage'.all()
print('defined stages',#all)
for i,s in pairs(all) do
	print("defined stage",i,s)
	print("",s:instances(),s:parent())
end
th:rawkill()

