local s=require'leda.stage'
local sched=require'leda.scheduler'
local c=require'leda.core'

local stage=s.new(c.encode(function() print'aew' end))
stage:push()

local t=sched:new_thread()
print(t)
t:join(1)



