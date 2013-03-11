-- vim: set ts=3 et:

print('_VERSION = ' .. _VERSION)

require 'luagl'
require 'luaglut'
require 'memarray'

print('luagl.VERSION = '   .. luagl.VERSION)
print('luaglut.VERSION = ' .. luaglut.VERSION)

local quit = false
local fps = 15
local fps_str = ''
local msec = 1000 / fps
local frames = 0
local start_time
local elapsed_time
local ppm, width, height

local draw_mode = 'solid'
local fps_mode  = 'fixed'

local function glutBitmapString(font, str)
   for i = 1, string.len(str) do
      glutBitmapCharacter(font, string.byte(str, i))
   end
end

local function set_texture(IMAGE,TYPE,width,height)
   glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, TYPE, GL_UNSIGNED_BYTE, IMAGE)
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP)
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP)
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_DECAL)
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL)
   glEnable(GL_TEXTURE_2D)
end

function load_texture(fname)
     local img,err=imlib2.image.load('lena.jpg')
     local IMAGE=img:get_data_r()
     local TYPE=GL_BGRA_EXT
     set_texture(IMAGE,TYPE,512,512);
     
end

local function set_material_clay()
   glMaterialfv(GL_FRONT, GL_AMBIENT,  {0.2125, 0.1275, 0.054, 1.0})
   glMaterialfv(GL_FRONT, GL_DIFFUSE,  {0.514, 0.4284, 0.18144, 1.0})
   glMaterialfv(GL_FRONT, GL_SPECULAR, {0.393548, 0.271906, 0.166721, 1.0})
   glMaterialf(GL_FRONT, GL_SHININESS, 0.2 * 128.0)

   glMaterialfv(GL_BACK, GL_AMBIENT,  {0.1, 0.18725, 0.1745, 1.0})
   glMaterialfv(GL_BACK, GL_DIFFUSE,  {0.396, 0.74151, 0.69102, 1.0})
   glMaterialfv(GL_BACK, GL_SPECULAR, {0.297254, 0.30829, 0.306678, 1.0})
   glMaterialf(GL_BACK, GL_SHININESS, 0.1 * 128.0)

   glEnable(GL_LIGHT0)
   glLightfv(GL_LIGHT0, GL_AMBIENT, {0.2, 0.2, 0.2, 1})
   glLightfv(GL_LIGHT0, GL_DIFFUSE, {1, 1, 1, 1})
   glLightfv(GL_LIGHT0, GL_POSITION, {0.0, 1.0, 0.0, 0.0})

   glEnable(GL_LIGHT1)
   glLightfv(GL_LIGHT1, GL_AMBIENT, {0.2, 0.2, 0.2, 1})
   glLightfv(GL_LIGHT1, GL_DIFFUSE, {1, 1, 1, 1})
   glLightfv(GL_LIGHT1, GL_POSITION, {1.0, 0.0, 1.0, 0.0})

   glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE)
   glFrontFace(GL_CW)
end

function resize_func(w, h)
   local ratio = w / h
   glMatrixMode(GL_PROJECTION)
   glLoadIdentity()
   glViewport(0,0,w,h)
   gluPerspective(45,ratio,1,1000)
   glMatrixMode(GL_MODELVIEW)
   glLoadIdentity()
   set_material_clay()
   glEnable(GL_DEPTH_TEST)
   glEnable(GL_NORMALIZE)
end

function timer_func()
   if quit then return end
   if fps_mode == 'fixed' then
      glutSetWindow(window)
      glutTimerFunc(msec, timer_func, 0)
      glutPostRedisplay()
   end
end

local angle = 0
local angle2 = 0



local object="cube"

function display_func()
   if quit then return end

   local elapsed_time = glutGet(GLUT_ELAPSED_TIME) - start_time

   angle = angle + 200 / (msec)
   angle2 = angle2 + 170 / (msec)

   glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT)


   glMatrixMode(GL_MODELVIEW)
   glPushMatrix()
   glTranslated(0,0,-5)
   glRotated(angle, 0, 1, 0)
   glRotated(angle2, 0, 0, 1)
   glColor3d(1,0,0)
   if draw_mode == 'wireframe' then
      glutWireTeapot(0.75)
   else
      glEnable(GL_LIGHTING)
      
      --
      
      if object=="cube" then
         --front face
         glBegin (GL_QUADS);
         glTexCoord2d(0.0, 0.0); glVertex3d(-1.0,-1.0,1.0);
         glTexCoord2d(1.0, 0.0); glVertex3d(1.0,-1.0,1.0);
         glTexCoord2d(1.0, 1.0); glVertex3d(1.0,1.0,1.0);
         glTexCoord2d(0.0, 1.0); glVertex3d(-1.0,1.0,1.0);
         glEnd();
         glBegin (GL_QUADS);
         --Back Face
         glBegin (GL_QUADS);
         glTexCoord2d(1.0, 0.0); glVertex3d(-1.0,-1.0,-1.0);
         glTexCoord2d(1.0, 1.0); glVertex3d(-1.0,1.0,-1.0);
         glTexCoord2d(0.0, 1.0); glVertex3d(1.0,1.0,-1.0);
         glTexCoord2d(0.0, 0.0); glVertex3d(1.0,-1.0,-1.0);
         glEnd();
         --Top Face
         glBegin (GL_QUADS);
         glTexCoord2d(0.0, 1.0); glVertex3d(-1.0, 1.0, -1.0);
         glTexCoord2d(0.0, 0.0); glVertex3d(-1.0,1.0,1.0);
         glTexCoord2d(1.0, 0.0); glVertex3d(1.0,1.0,1.0);
         glTexCoord2d(1.0, 1.0); glVertex3d(1.0,1.0,-1.0);
         glEnd();
         --Bottom Face
         glBegin (GL_QUADS);
         glTexCoord2d(1.0, 1.0); glVertex3d(-1.0,-1.0,-1.0);
         glTexCoord2d(0.0, 1.0); glVertex3d(1.0,-1.0,-1.0);
         glTexCoord2d(0.0, 0.0); glVertex3d(1.0,-1.0,1.0);
         glTexCoord2d(1.0, 0.0); glVertex3d(-1.0,-1.0,1.0);
         glEnd();
         --Right Face
         glBegin (GL_QUADS);
         glTexCoord2d(1.0, 0.0); glVertex3d(1.0,-1.0,-1.0);
         glTexCoord2d(1.0, 1.0); glVertex3d(1.0,1.0,-1.0);
         glTexCoord2d(0.0, 1.0); glVertex3d(1.0,1.0,1.0);
         glTexCoord2d(0.0, 0.0); glVertex3d(1.0,-1.0,1.0);
         glEnd();
         -- Left Face
         glBegin (GL_QUADS);
         glTexCoord2d(0.0, 0.0); glVertex3d(-1.0,-1.0,-1.0);
         glTexCoord2d(1.0, 0.0); glVertex3d(-1.0,-1.0,1.0);
         glTexCoord2d(1.0, 1.0); glVertex3d(-1.0,1.0,1.0);
         glTexCoord2d(0.0, 1.0); glVertex3d(-1.0,1.0,-1.0);
         glEnd();
      else
         glutSolidTeapot(1)
      end   
      glDisable(GL_LIGHTING)
   end
   glPopMatrix()

   glDisable(GL_TEXTURE_2D)
   glMatrixMode(GL_PROJECTION)
   glPushMatrix()
   glLoadIdentity()
   glOrtho(0, glutGet(GLUT_WINDOW_WIDTH), 0, glutGet(GLUT_WINDOW_HEIGHT), -1, 1)
   glColor3d(1,1,1)
   glRasterPos2d(10,10)
   glutBitmapString(GLUT_BITMAP_HELVETICA_12, fps_str)
   glPopMatrix()
   glEnable(GL_TEXTURE_2D)

   glutSwapBuffers()

   if fps_mode == 'fastest' then glutPostRedisplay() end

   frames = frames + 1

   if elapsed_time > 1000 then
      fps_str = 'measured fps: ' .. frames * 1000 / elapsed_time
      --print(fps_str)
      start_time = start_time + elapsed_time
      frames = 0
   end

   local a,wi,he=leda.debug.peek_event()
   if a then
      local b,err=1,nil
      while b do
       b,err=leda.debug.peek_event()
        if b then print("skipping frame",b,err) 
        else print("NO skip",err) end
     end
      print("Old image",IMAGE,"new image",a,wi..'x'..he)
      set_texture(a,GL_RGB,wi,he)
     -- filter.free(IMAGE)
      --IMAGE=a
--   --   width, height = wi,he
   --   TYPE=GL_RGB
   end

end

function keyboard_func(key,x,y)
   if key == 27 then
      quit = true
      glutDestroyWindow(window)
      os.exit(0)
   end
end

load_texture()

glutInit(arg)
glutInitDisplayMode(GLUT_RGB + GLUT_DOUBLE + GLUT_DEPTH)
if arg then title = arg[0] else title = "glut" end
window = glutCreateWindow(title)
glutDisplayFunc(display_func)
glutKeyboardFunc(keyboard_func)
glutReshapeFunc(resize_func)
glutTimerFunc(msec, timer_func, 0)

--[[
glutTimerFunc(2000,
   function(value)
      io.write('another timer called with value = ',value,'\n')
   end,
   2)
--]]

local menu1 = glutCreateMenu(function(value) print('submenu callback ' .. value) if value==1 then object='cube' else object='teapot' end end)
glutAddMenuEntry('cube', 1)
glutAddMenuEntry('teapot', 2)

local mainmenu = glutCreateMenu(function(value) print('mainmenu callback ' .. value) end)
glutAddMenuEntry('wireframe', function() draw_mode = 'wireframe' end)
glutAddMenuEntry('solid', function() draw_mode = 'solid' end)

glutAddMenuEntry('fastest FPS',
   function()
      fps_mode = 'fastest'
      glutSetWindow(window)
      glutPostRedisplay()
   end)

glutAddMenuEntry('fixed FPS(' .. fps .. ')',
   function()
      fps_mode = 'fixed'
      glutTimerFunc(msec, timer_func, 0)
   end)

glutAddSubMenu('object', menu1)

glutAddMenuEntry('quit',
   function()
      quit = true
      glutDestroyWindow(window)
      os.exit(0)
    end)

glutAttachMenu(GLUT_LEFT_BUTTON)
start_time = glutGet(GLUT_ELAPSED_TIME)
