local lstage=require'lstage'

local th=lstage.scheduler.new_thread()

local a=0

local stage=lstage.stage(function()
	dummy=dummy or lstage.stage(function()
		print('dummy',a)
		lstage.scheduler.kill_thread()
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
local all=require'lstage.stage'.all()
print('defined stages',#all)
for i,s in pairs(all) do
	print("defined stage",i,s)
	print("",s:instances(),s:parent())
end
th:rawkill()

