local lstage=require'lstage'
local time=lstage.now()

local th=lstage.scheduler.new_thread()

lstage.stage(function() end,50000)

lstage.scheduler.kill_thread()
th:join()
print("ended",lstage.now()-time)
