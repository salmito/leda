local s=require'leda.stage'
local c=require'leda.core'
local stage=s.new(c.encode(function() print'aew' end),10)
local ss=s.new(c.encode(function() 
   print('inside',stage:push())
   print('size',stage:queue_size())
   print('inside',stage:push('event',false,42,math.pi))
   print('size',stage:queue_size())
   stage:set_capacity(2)
   print('inside',stage:push('event',false,42,math.pi))
   print('size',stage:capacity(),stage:queue_size())
   
end))

s.add(stage)

print(stage)

local stage2,err=stage:__wrap()()

print(stage2,stage2:env(),err)

print(stage:capacity(),stage2:max_instances())

stage2:set_capacity(100)
stage:set_max_instances(1024)

--stage:push("event")


print(c.decode(ss:env())())

print(stage:capacity(),stage2:max_instances(),stage:id(),stage2:id())
assert(s.is_stage(stage2))
s.is_stage("not a stage")


