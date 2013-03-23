require 'leda'
require 'leda.controller.fixed_thread_pool'

capture=leda.stage{
   name='Capture',
   handler=function(camera)
      require "v4l"

      dev = v4l.open(camera)
      while true do
         local a = v4l.getframeraw()
         leda.send('frame',a,v4l.width(),v4l.height())
      end
   end,
}

copy=leda.stage{
name="Frame Copy",
handler=function(frame,width,height) 
   local copy=filter.copy(frame,width,height)
   if copy then
      leda.send(1,frame,width,height,1)
      leda.send(2,copy,width,height,2)
   end
end,
init="require 'filter'"
}


golden=leda.stage{
handler=function(frame,width,height,...)
   filter.gold(frame,width,height)
   leda.send(1,frame,width,height,...)
end,
init="require 'filter'",
name="Golden filter"
}

display=leda.stage{
   name="Display",
   handler=function()
      require 'imlib2_leda'
      require 'table'
      require 'os'
      require 'io'
      require 'filter'      
      require 'string'
      require 'gl'
      glutMainLoop()
   end,
   serial=true
}

display:send()
capture:send("/dev/video0")

g=leda.graph{
   leda.connect(capture,'frame',copy),
   leda.connect(copy,1,display),
   leda.connect(copy,2,golden),
   leda.connect(golden,1,display)
}

local c=require 'leda.controller.fixed_thread_pool'

g:plot()
g:run{controller=c.get(4)}

