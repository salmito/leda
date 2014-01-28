local s=require'leda.scheduler'
local stage=s.stage_new("Aew")
s.stage_add(10,stage)

print(stage)

local stage2,err=s.stage_get(10)
print(stage2,err)

print(stage:capacity(),stage2:max_instances())

stage2:set_capacity(100)
stage:set_max_instances(1024)

print(stage:capacity(),stage2:max_instances())
