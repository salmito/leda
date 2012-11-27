require 'leda.utils'

local init=leda.gettime()

local function f(...)
	print(leda.stage.name,os.time(),leda.gettime()-init,...)
end

local s1=leda.stage{handler=f,init="require 'os'",name='s1'}
local s2=leda.stage(s1)
local s3=leda.stage(s2)

local i=1
local sinit=leda.utils.get_copy_stage(1000,function(n) i=i or n i=i*2 leda.sleep(0.2) return "Event",i end)

--sinit.serial=true

sinit:send(1)

local rr=leda.stage(leda.utils.roundrobbin)


--s2.name="s2"
--s3.name="s3"
sinit.name="copy"


rr.name="roundrobbin"

--local c1,c2=

local timed=arg[1]

g=leda.graph{
	leda.connect(rr,'s1',s1),
	leda.connect(rr,'s2',s2),
	leda.connect(rr,'s3',s3),
	--leda.connect(sinit,1,rr),
    --leda.utils.event_recorder("/tmp/sinit.1.out",sinit,1,rr),
	leda.utils.event_replayer("/tmp/sinit.1.out",timed,1,rr),
}
--g:plot()
g:run()