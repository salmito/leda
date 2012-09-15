require "leda"

local dir=dir or './dir'
local outdir=outdir or './outdir'

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
         print("Processing images: ",totalimages)
      elseif ended then
         n=n-1
      end
      if n==0 then
         print("Result: images processed:",totalimages,"time:",leda.gettime()-init_time)
         global_counter=0
         n=nil
         leda.quit()
      end
   end, 
   serial=true
}

local image_open=leda.stage{
   name="Open image",  
   handler=function (file)
      local img,err=imlib2.image.load(dir..'/'..file)
      assert(img,err)
      assert(leda.send("image",{image=img,outfile=file.."_o.jpg"}))
   end,
   bind=function(output)
      assert(output.image,"'image' port must be connected")
      output.image.type=leda.couple
      assert(output.image.type==leda.couple,"'image' port must be connected to a coupled connector")
   end,
   init=function ()
      require "imlib2_leda"
   end,
   serial=true
}

local radius=radius or 0

local image_blur=leda.stage{
   name="Blur image",  
   handler=function (img)
      if radius > 0 then
         assert(img.image:blur(radius))
      end
      assert(leda.send("image",img))
   end,
   bind=function(output)
      assert(output.image,"'image' port must be connected")
      output.image.type=leda.couple
      assert(output.image.type==leda.couple,"'image' port must be connected to a coupled connector")
   end,
   init=function ()
      require "imlib2_leda"
   end
}

local angle=angle or 0

local image_rotate=leda.stage{
   name="Rotate image",  
   handler=function (img)
      if angle > 0 then
         img.image:rotate(angle)
      end
      assert(leda.send("image",img))
   end,
   bind=function(output)
      assert(output.image,"'image' port must be connected")
      output.image.type=leda.couple
      assert(output.image.type==leda.couple,"'image' port must be connected to a coupled connector")
   end,
   init=function ()
      require "imlib2_leda"
   end
}

local image_save=leda.stage{
   name="Save image",  
   handler=function (img)
      img.image:save(outdir..'/'..img.outfile)
      leda.send("finished",true)
      img=nil
      collectgarbage()
   end,
   init=function ()
      require "imlib2_leda"
   end
}

local g=leda.graph{
   start=dir_scanner,
   dir_scanner:connect('file',image_open),
   image_open:connect('image',image_blur),
   image_blur:connect('image',image_rotate),
   image_rotate:connect('image',image_save),
   image_save:connect('finished',dir_scanner)
}

g:plot()
g:send()
g:run{controller=leda.controller.interactive.get(5),maxpar=5}
