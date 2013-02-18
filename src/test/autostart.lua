local utils=require "leda.utils"

autostart_stage=leda.stage{
	handler=function(...) 
		print(...)
		leda.quit()
	end,
	autostart={'Autostarted','param',10,math.pi}
}

local g=leda.graph{start=autostart_stage}

g:run()
