local s=require'leda.stage'
local stage=s.new("Aew")
s.add(stage)

print(stage)

local stage2,err=s.get(stage:id())
print(stage2,err)

print(stage:capacity(),stage2:max_instances())

stage2:set_capacity(100)
stage:set_max_instances(1024)

print(stage:capacity(),stage2:max_instances(),stage:id(),stage2:id())
