local utils=require "leda.utils"
require "leda.utils.plot"

local a=stage(utils.print)

local g=utils.linear_pipeline(a,a,a,a,a,a,a,a,a,a)
a:send()
g:plot()

