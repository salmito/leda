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
local fps_str2 = ''
local msec = 1000 / fps
local frames = 0
local frames2 = 0
local start_time
local start_time2
local elapsed_time
local ppm, width, height

local draw_mode = 'solid'
local fps_mode  = 'fastest'

local function glutBitmapString(font, str)
   for i = 1, string.len(str) do
      glutBitmapCharacter(font, string.byte(str, i))
   end
end


local function set_texture()
   glTexImage2D(GL_TEXTURE_2D, 0, 3, width, height, 0, TYPE, GL_UNSIGNED_BYTE, IMAGE)
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP)
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP)
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
   glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_ENV_MODE, GL_DECAL)
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL)
   glEnable(GL_TEXTURE_2D)
end

local function set_texture2()
   glTexImage2D(GL_TEXTURE_2D, 0, 3, width2, height2, 0, TYPE2, GL_UNSIGNED_BYTE, IMAGE2)
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
   IMAGE=img:get_data_r()
   TYPE=GL_BGRA_EXT
   width=img:get_width()
   height=img:get_height()
   set_texture()
   free=nil
   return img
end

function load_texture2(fname)
   local img,err=imlib2.image.load('lena.jpg')
   IMAGE2=img:get_data_r()
   TYPE2=GL_BGRA_EXT
   width2=img:get_width()
   height2=img:get_height()
   set_texture2()
   free2=nil
   return img
end



local function update_textures()
   local frame,wi,he,w=leda.debug.peek_event()
   if frame then
      if w==1 then
         local old=IMAGE
         IMAGE=frame
         width, height = wi,he
         TYPE=GL_RGB
         glutSetWindow(window)
         set_texture()
    	if free then
         filter.free(old)
        end
        free=true
      else 
         local old=IMAGE2
         IMAGE2=frame
         width2, height2 = wi,he
         TYPE2=GL_RGB
         glutSetWindow(window2)
         set_texture2()
	if free2 then
         filter.free(old)
        end
         free2=true
      end      
   end
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
   glutSetWindow(window)
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

function resize_func2(w, h)
   glutSetWindow(window2)
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

function timer_func2()
   if quit then return end
   if fps_mode == 'fixed' then
      glutSetWindow(window2)
      glutTimerFunc(msec, timer_func2, 0)
      glutPostRedisplay()
   end
end


local angle = 0
local angle2 = 0

local angl = 0
local angl2 = 0
local rotate=true

local object="cube"

function display_func()
   if quit then return end
   update_textures()
   glutSetWindow(window)
   local elapsed_time = glutGet(GLUT_ELAPSED_TIME) - start_time
   if rotate then
   angle = angle + 100 / (msec)
   angle2 = angle2 + 85 / (msec)
   end
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
end

function display_func2()
   if quit then return end
   update_textures()
   glutSetWindow(window2)
   local elapsed_time = glutGet(GLUT_ELAPSED_TIME) - start_time2
   if rotate then
   angl = angl + 100 / (msec)
   angl2 = angl2 + 85 / (msec)
   end
   glClear(GL_COLOR_BUFFER_BIT + GL_DEPTH_BUFFER_BIT)


   glMatrixMode(GL_MODELVIEW)
   glPushMatrix()
   glTranslated(0,0,-5)
   glRotated(angl, 0, 1, 0)
   glRotated(angl2, 0, 0, 1)
   glColor3d(1,1,1)
   if draw_mode == 'wireframe' then
      glutWireTeapot(0.75)
   else
      glEnable(GL_LIGHTING)
      

      
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
   glutBitmapString(GLUT_BITMAP_HELVETICA_12,  fps_str2)
   glPopMatrix()
   glEnable(GL_TEXTURE_2D)

   glutSwapBuffers()

   if fps_mode == 'fastest' then glutPostRedisplay() end

   frames2 = frames2 + 1

   if elapsed_time > 1000 then
      fps_str2 = 'measured fps: ' .. frames2 * 1000 / elapsed_time
      --print(fps_str2)
      start_time2 = start_time2 + elapsed_time
      frames2 = 0
   end
end


function keyboard_func(key,x,y)
   --print(key,x,y)
   if key == 27 then
      quit = true
      glutDestroyWindow(window)
      os.exit(0)
   elseif key==114 then
      rotate=not rotate
   end
end


glutInit(arg)
glutInitDisplayMode(GLUT_RGB + GLUT_DOUBLE + GLUT_DEPTH)

glutInitWindowPosition(0, 0);
window = glutCreateWindow('Window 1')

glutDisplayFunc(display_func)
glutKeyboardFunc(keyboard_func)
glutReshapeFunc(resize_func)
glutTimerFunc(msec, timer_func, 0)
load_texture()

glutInitWindowPosition(600, 0);
window2 = glutCreateWindow('Window 2')
glutDisplayFunc(display_func2)
glutKeyboardFunc(keyboard_func)
glutReshapeFunc(resize_func2)
load_texture2()
glutTimerFunc(msec, timer_func2, 0)

local menu1 = glutCreateMenu(function(value) if value==1 then object='cube' else object='teapot' end end)
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
      glutSetWindow(window2)
      glutPostRedisplay()
   end)

glutAddMenuEntry('fixed FPS(' .. fps .. ')',
   function()
      fps_mode = 'fixed'
      glutTimerFunc(msec, timer_func, 0)
      glutTimerFunc(msec, timer_func2, 0)
   end)

glutAddMenuEntry('toggle rotation',
   function()
      rotate = not rotate
   end)

glutAddSubMenu('object', menu1)

glutAddMenuEntry('quit',
   function()
      quit = true
      glutDestroyWindow(window)
      glutDestroyWindow(window2)
      os.exit(0)
    end)
glutSetWindow(window2)
glutAttachMenu(GLUT_RIGHT_BUTTON)
glutSetWindow(window)
glutAttachMenu(GLUT_RIGHT_BUTTON)
start_time = glutGet(GLUT_ELAPSED_TIME)
start_time2 = glutGet(GLUT_ELAPSED_TIME)
