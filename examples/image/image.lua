require "leda"

local dir=dir or './dir'
local outdir=outdir or './outdir'
local arg={...}
local it=arg[1] or 1
local type=arg[2] or "decoupled"
local threads=arg[3] or 4
local maxpar=arg[4] or 4

local dir_scanner = leda.stage{
   name="Directory scanner",
   handler=function(ended)
      if not n then
         local lfs=require 'lfs'
         n=0 --number of files
         global_counter=0
         init_time=leda.gettime()
         for file in lfs.dir(tostring(dir)) do
             if lfs.attributes(dir..'/'..file,"mode") == "file" then 
               leda.send('file',file)
               n=n+1
             end
         end
         totalimages=n
         n=it*n*3
--         print("Processing images: ",totalimages)
      elseif ended then
      	if n%10==0 then
      		local latency=leda.gettime()-ended.start
	      	print("images-latency",totalimages*it*3,n,latency,0,threads,maxpar,type)
      	end
         n=n-1
      end
      if n==0 then
      	local t=leda.gettime()-init_time
         print("images-pipelined",totalimages*it,0,t,totalimages*it/t,threads,maxpar,type)
         global_counter=0
         n=nil
         leda.quit()
      end
   end, 
   stateful=true
}

local image_open=leda.stage{
   name="Open image",  
   handler=function (file)
      local img,err=imlib2.image.load(dir..'/'..file)
		if not img then leda.quit() end
		for i=1,it-1 do 
			local img2=img:clone()
	      assert(leda.send("image",{image=img2,outfile=outdir..'/'..file..i..".jpg",start=leda.gettime()}))

	   end
  	   print("aqui"..outdir..'/'..file..it..".jpg")
      assert(leda.send("image",{image=img,outfile=outdir..'/'..file..it..".jpg",start=leda.gettime()}))

   end,
   bind=function(output)
      assert(output.image,"'image' port must be connected")
      output.image.type="local"
   end,
   init=function ()
      require "imlib2_leda"
   end, stateful=true
}

local image_cloner=leda.stage{
	"Cloner",
	handler=function(event)
		local img2=event.image:clone()
		assert(leda.send("high_quality",{image=img2,outfile=event.outfile.."_high.jpg",start=event.start},1080))

		local img2=event.image:clone()
		assert(leda.send("thumbnail",{image=img2,outfile=event.outfile.."_low.jpg",start=event.start},128))
				
		assert(leda.send("medium_quality",{image=event.image,outfile=event.outfile.."_medium.jpg",start=event.start},768))
	end,
	init=function ()
      require "imlib2_leda"
   end
}

local image_scale=leda.stage{
   name="Scale image",  
   handler=function (event,scale_height)
   	local factor=event.image:get_width()/event.image:get_height()
   	local width=scale_height*factor
   	local height=scale_height
      event.image:crop_and_scale(0,0,event.image:get_width(),event.image:get_height(),width,height)
      assert(leda.send("image",event))
   end,
   bind=function(output)
      assert(output.image,"'image' port must be connected")
      output.image.type="local"
   end,
   init=function ()
      require "imlib2_leda"
   end,
}

--[[local image_equalize=leda.stage{
	"Equalizer",
	handler=function(event)
		equalize(event.image)
		assert(leda.send("image",event))
	end,
   init=function ()
      require "histogram"
   end,
	
}]]--

local image_save=leda.stage{
   name="Save image",  
   handler=function (event)
      event.image:save(event.outfile)
      event.image=nil
      collectgarbage()
      leda.send("finished",event)
   end,
   init=function ()
      require "imlib2_leda"
   end,
   stateful=true
}

local g=leda.graph{
   start=dir_scanner,
   dir_scanner:connect('file',image_open,type),
   image_open:connect('image',image_cloner,type),
   image_cloner:connect('high_quality',image_scale,type),
   image_cloner:connect('thumbnail',image_scale,type),
   image_cloner:connect('medium_quality',image_scale,type),   
   image_scale:connect('image',image_save,type),
--   image_equalize:connect('image',image_save,type),
   image_save:connect('finished',dir_scanner,type)
}

--g:plot()
g:send()
g:run{controller=leda.controller.interactive.get(threads),maxpar=maxpar}
