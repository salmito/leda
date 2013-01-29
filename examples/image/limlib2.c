/* Wraps the Imlib2 library */
#include <string.h>
#include "lua.h"
#include "lauxlib.h"

#define X_DISPLAY_MISSING
#include "Imlib2.h"

typedef Imlib_Image Image;
typedef Imlib_Color_Modifier ColorModifier;
typedef Imlib_Color_Range Gradient;
typedef ImlibPolygon Polygon;
typedef Imlib_Font Font;

/*?lua
lib_name = "lua-imlib2"
?*/

static struct { const char *fmtstr; Imlib_Load_Error errno; } err_strings[] = {
  {"file '%s' does not exist", IMLIB_LOAD_ERROR_FILE_DOES_NOT_EXIST},
  {"file '%s' is a directory", IMLIB_LOAD_ERROR_FILE_IS_DIRECTORY},
  {"permission denied to read file '%s'", IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_READ},
  {"no loader for the file format used in file '%s'", IMLIB_LOAD_ERROR_NO_LOADER_FOR_FILE_FORMAT},
  {"path for file '%s' is too long", IMLIB_LOAD_ERROR_PATH_TOO_LONG},
  {"a component of path '%s' does not exist", IMLIB_LOAD_ERROR_PATH_COMPONENT_NON_EXISTANT},
  {"a component of path '%s' is not a directory", IMLIB_LOAD_ERROR_PATH_COMPONENT_NOT_DIRECTORY},
  {"path '%s' has too many symbolic links", IMLIB_LOAD_ERROR_TOO_MANY_SYMBOLIC_LINKS},
  {"ran out of file descriptors trying to access file '%s'", IMLIB_LOAD_ERROR_OUT_OF_FILE_DESCRIPTORS},
  {"denied write permission for file '%s'", IMLIB_LOAD_ERROR_PERMISSION_DENIED_TO_WRITE},
  {"out of disk space writing to file '%s'", IMLIB_LOAD_ERROR_OUT_OF_DISK_SPACE},
  {NULL, 0}
};

static int push_load_error_str(lua_State *L, Imlib_Load_Error err, const char *filepath)
{
  int i;
  for (i = 0; err_strings[i].fmtstr != NULL; i++)
  {
    if (err == err_strings[i].errno)
    {
      lua_pushfstring(L, err_strings[i].fmtstr, filepath);
      return 1;
    }
  }
  /* Unrecognised error */
  lua_pushfstring(L, "encountered an error accessing file '%s'", filepath);
  return 1;
}

/*?lua
module "imlib2.color"
description [[
  Represent colors in rgba format. Each instance of imlib2.color has the
  modifiable fields red, green, blue and alpha. An error will be rainsed on an
  attempt to set them to a value outside of the range 0 &lt;= x &lt;= 255.  
  Non-integer
  values will be truncated.
  ]]
?*/

static Imlib_Color *push_Color(lua_State *L)
{
  Imlib_Color *c = (Imlib_Color*)lua_newuserdata(L, sizeof(Imlib_Color));
  luaL_getmetatable(L, "imlib2.color");
  lua_setmetatable(L, -2);
  return c;
}

#define check_color(L,n)\
  (Imlib_Color*)luaL_checkudata(L, n, "imlib2.color")

/*?lua
signature "color.new(red, green, blue, alpha)"
description [[
  Creates a new color. alpha is optional. An error will be raised unless red, 
  green, blue and alpha are in the range 0 &lt;= x &lt;= 255
  ]]
?*/
static int color_new(lua_State *L)
{
  Imlib_Color *c;
  int i;
  int args[4];
  args[0]=luaL_checkint(L, 1); /* red */
  args[1]=luaL_checkint(L, 2); /* green */
  args[2]=luaL_checkint(L, 3); /* blue */
  args[3]=luaL_optint(L, 4, 255); /* alpha */
  for (i=0; i<4; i++)
    luaL_argcheck(L, args[i] >= 0 && args[i] <= 255, i+1, "values must be >= 0 and <= 255");
  
  c = push_Color(L);
  c->red=args[0];
  c->green=args[1];
  c->blue=args[2];
  c->alpha=args[3];
  return 1;
}

#define set_color(c)\
  imlib_context_set_color(c->red, c->green, c->blue, c->alpha)

/*** imlib2.color metamethods ***/

/*?lua
signature "col:__tostring()"
description [[
  Return a string representation such as &lt;imlib2.color r=0 g=0 b=0 a=0&gt; 
  (0x%p) where %p is the memory location
  ]]
?*/
static int colorm_tostring(lua_State *L)
{
  Imlib_Color *c = check_color(L,1);
  lua_pushfstring(L, "<imlib2.color r=%d g=%d b=%d a=%d> (%p)",
      c->red, c->green, c->blue, c->alpha, c);
  return 1;
}

/*?lua
for _,v in ipairs{"red", "green", "blue", "alpha"} do
  signature("col."..v)
  description(([[Get/set the %s field. Raises an exception if you try to set 
  the value outside of the range 0 &lt;= value &lt;= 255]]):format(v))
end
?*/
static int colorm__index(lua_State *L)
{
  Imlib_Color *c = check_color(L, 1);
  const char *field = luaL_checkstring(L, 2);

  if (strcmp(field, "red") == 0)
    lua_pushinteger(L, c->red);
  else if (strcmp(field, "green") == 0)
    lua_pushinteger(L, c->green);
  else if (strcmp(field, "blue") == 0)
    lua_pushinteger(L, c->blue);
  else if (strcmp(field, "alpha") == 0)
    lua_pushinteger(L, c->alpha);
  else
  {
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
  }
  return 1;
}

static int colorm__newindex(lua_State *L)
{
  Imlib_Color *c = check_color(L, 1);
  const char *field = luaL_checkstring(L, 2);
  int val = luaL_checkint(L, 3);
  luaL_argcheck(L, val >= 0 && val <= 255, 3, "values must be >= 0 and <= 255");

  if (strcmp(field, "red") == 0)
    c->red = val;
  else if (strcmp(field, "green") == 0)
    c->green = val;
  else if (strcmp(field, "blue") == 0)
    c->blue = val;
  else if (strcmp(field, "alpha") == 0)
    c->alpha = val;

  return 0;
}

/*?lua
module "imlib2.border"
description [[
  Represents a border. Contains left, top, right, bottom values as writeable 
  fields. the area at the edge of the image that does not scale with the rest 
  of the image when resized - the borders remain constant in size.
  ]]
?*/
static Imlib_Border *push_Border(lua_State *L)
{
  Imlib_Border *b = (Imlib_Border*)lua_newuserdata(L, sizeof(Imlib_Border));
  luaL_getmetatable(L, "imlib2.border");
  lua_setmetatable(L, -2);
  return b;
}

/*?lua
signature "border.new(left, top, right, bottom)"
description "Creates a new border. All parameters are mandatory."
?*/
static int border_new(lua_State *L)
{
  Imlib_Border *b;
  int left=luaL_checkint(L, 1);
  int top=luaL_checkint(L,2);
  int right=luaL_checkint(L,3);
  int bottom=luaL_checkint(L,4);

  b = push_Border(L);
  b->left=left;
  b->top=top;
  b->right=right;
  b->bottom=bottom;
  return 1;
}

#define check_border(L, n)\
  (Imlib_Border*)luaL_checkudata(L, n, "imlib2.border")

/*?lua
signature "bd:__tostring()"
description [[Return a string representation such as &lt;imlib2.border left=1 
top=1 right=1 bottom=1&gt; (0x%p) where %p is the memory location]]
?*/
static int borderm_tostring(lua_State *L)
{
  Imlib_Border *b = check_border(L, 1);
  lua_pushfstring(L, "<imlib2.border left=%d top=%d right=%d bottom=%d> (%p)",
      b->left, b->top, b->right, b->bottom, b);
  return 1;
}

/*?lua
for _,v in ipairs{"left", "top", "right", "bottom"} do
  signature("bd."..v)
  description(("Get/set the %s field"):format(v))
end
?*/
static int borderm__index(lua_State *L)
{
  Imlib_Border *b = check_border(L, 1);
  const char *field = luaL_checkstring(L, 2);

  if (strcmp(field, "left") == 0)
    lua_pushinteger(L, b->left);
  else if (strcmp(field, "top") == 0)
    lua_pushinteger(L, b->top);
  else if (strcmp(field, "right") == 0)
    lua_pushinteger(L, b->right);
  else if (strcmp(field, "bottom") == 0)
    lua_pushinteger(L, b->bottom);
  else
  {
    lua_getmetatable(L, 1);
    lua_pushvalue(L, 2);
    lua_gettable(L, -2);
  }
  return 1;
}

static int borderm__newindex(lua_State *L)
{
  Imlib_Border *b = check_border(L, 1);
  const char *field = luaL_checkstring(L, 2);
  int val = luaL_checkint(L, 3);

  if(strcmp(field, "left") == 0)
    b->left = val;
  else if (strcmp(field, "top") == 0)
    b->top = val;
  else if (strcmp(field, "right") == 0)
    b->right = val;
  else if (strcmp(field, "bottom") == 0)
    b->bottom = val;

  return 0;
}

/*?lua
module "imlib2.gradient"
description [[
  Represents a gradient to which you can add colors. Each color is stored with 
  an offset.
  ]]
?*/
static Gradient push_Gradient(lua_State *L, Gradient gr)
{
  Gradient *grp = (Gradient*)lua_newuserdata(L, sizeof(Gradient));
  *grp = gr;
  luaL_getmetatable(L, "imlib2.gradient");
  lua_setmetatable(L, -2);
  return gr;
}

static Gradient check_Gradient(lua_State *L, int n)
{
  Gradient *grp, gr;
  grp = (Gradient*)luaL_checkudata(L, n, "imlib2.gradient");
  gr = *grp;
  if (gr == NULL)
    luaL_argerror(L, n, "gradient has been freed");
  return gr;
}

/*?lua
signature "gradient.new()"
description [[
  Creates a new gradient. Use grad:add_color() to add colors to it
  ]]
?*/
static int gradient_new(lua_State *L)
{
  Gradient gr = push_Gradient(L, imlib_create_color_range());
  if (gr == NULL)
    return luaL_error(L, "failed with imlib_create_color_range");
  return 1;
}

static int gradientm_gc(lua_State *L)
{
  Gradient *grp = (Gradient*)luaL_checkudata(L, 1, "imlib2.gradient");
  Gradient gr = *grp;
  if (gr)
  {
    imlib_context_set_color_range(gr);
    imlib_free_color_range();
    *grp=NULL;
  }
  return 0;
}

/*?lua
signature "grad:__tostring()"
description [[
  Returns a string representation of the gradient in the form 
  &lt;imlib2.gradient&gt; (%p) where %p is the memory location
  ]]
?*/
static int gradientm_tostring(lua_State *L)
{
  Gradient *grp = (Gradient*)luaL_checkudata(L, 1, "imlib2.gradient");
  Gradient gr = *grp;
  if (gr)
    lua_pushfstring(L, "<imlib2.gradient> (%p)", gr);
  else
    lua_pushfstring(L, "<imlib2.gradient> (freed)");
  return 1;
}

/*?lua
signature("grad:add_color(offset, color)")
description [[
  Add an imlib2.color object at the given offset. The offset parameter has no 
  meaning for the first color added
  ]]
param("offset", "the distance from the previous color")
param("color", "an imlib2.color")
?*/
static int gradientm_add_color(lua_State *L)
{
  Gradient gr = check_Gradient(L, 1);
  int offset = luaL_checkint(L, 2);
  imlib_context_set_color_range(gr);
  Imlib_Color *c = check_color(L, 3);
  set_color(c);
  imlib_add_color_to_color_range(offset);
  return 0;
}

/*?lua
module "imlib2.polygon"
description [[
  Represents a polygon object. Points are drawn in order, from the first to 
  last.
  ]]
?*/
static Polygon push_Polygon(lua_State *L, Polygon po)
{
  Polygon *pop = (Polygon*)lua_newuserdata(L, sizeof(Polygon));
  *pop = po;
  luaL_getmetatable(L, "imlib2.polygon");
  lua_setmetatable(L, -2);
  return po;
}

static Polygon check_Polygon(lua_State *L, int n)
{
  Polygon *pop, po;
  pop = (Polygon*)luaL_checkudata(L, n, "imlib2.polygon");
  po = *pop;
  if (po == NULL)
    luaL_argerror(L, n, "polygon has been freed");
  return po;
}

/*?lua
signature "polygon.new()"
description "Create a new polygon"
?*/
static int polygon_new(lua_State *L)
{
  Polygon po = push_Polygon(L, imlib_polygon_new());
  if (po == NULL)
    return luaL_error(L, "failed with imlib_polygon_new");
  return 1;
}

static int polygonm_gc(lua_State *L)
{
  Polygon *pop = (Polygon*)luaL_checkudata(L, 1, "imlib2.polygon");
  Polygon po = *pop;
  if (po)
  {
    imlib_polygon_free(po);
    *pop=NULL;
  }
  return 0;
}

/*?lua
signature "poly:__tostring()"
description [[
  Returns a string representation of the polygon of the form 
  &lt;imlib2.polygon&gt; (%p) where %p is the memory location
  ]]
?*/
static int polygonm_tostring(lua_State *L)
{
  Polygon po = check_Polygon(L, 1);
  if (po)
    lua_pushfstring(L, "<imlib2.polygon> (%p)", po);
  else
    lua_pushfstring(L, "<imlib2.polygon> (freed)");
  return 1;
}

/*?lua
signature "poly:add_point(x,y)"
description "Adds point (x,y) to the polygon."
param("x", "x coordinate in pixels")
param("y", "y coordinate in pixels")
?*/
static int polygonm_add_point(lua_State *L)
{
  Polygon po = check_Polygon(L, 1);
  int x = luaL_checkint(L, 2);
  int y = luaL_checkint(L, 3);
  imlib_polygon_add_point(po, x, y);
  return 0;
}

/*?lua
signature "poly:get_bounds()"
description [[
  Returns the four points of the rectangular bounding area for the polygon
  ]]
returns("x1", "x coordinate of the upper left corner")
returns("y1", "y coordinate of the upper left corner")
returns("x2", "x coordinate of the lower right corner")
returns("y2", "y coordinate of the lower right corner")
?*/
static int polygonm_get_bounds(lua_State *L)
{
  int i;
  Polygon po = check_Polygon(L, 1);
  int r[4] = {0, 0, 0, 0}; /* Bounding box */
  imlib_polygon_get_bounds(po, &r[0], &r[1], &r[2], &r[3]);
  for (i=0; i < 4; i++)
    lua_pushinteger(L, r[i]);
  return 4;
}

/*?lua
signature "poly:contains_point()"
description "Returns true if the polygon contains (x,y)."
?*/
static int polygonm_contains_point(lua_State *L)
{
  Polygon po = check_Polygon(L, 1);
  int x = luaL_checkint(L, 2);
  int y = luaL_checkint(L, 3);
  lua_pushboolean(L, imlib_polygon_contains_point(po, x, y));
  return 1;
}

/*?lua
module "imlib2.font"
description [[
  Represents a loaded font and provides methods for querying and modifying the 
  font paths.
  ]]
?*/

static Font push_Font(lua_State *L, Font fo)
{
  Font *fop = (Font*)lua_newuserdata(L, sizeof(Font));
  *fop = fo;
  luaL_getmetatable(L, "imlib2.font");
  lua_setmetatable(L, -2);
  return fo;
}

static Font check_Font(lua_State *L, int n)
{
  Font *fop = luaL_checkudata(L, n, "imlib2.font");
  Font fo = *fop;
  if (fo == NULL)
    luaL_argerror(L, n, "font has been freed");
  return fo;
}

/*?lua
signature "font.load(name)"
description [[
  Load a font from the front path. Returns a font object for the given font 
  name, e.g. "Vera/10". If it can not be found, returns nil, err_msg.
  ]]
?*/
static int font_load(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  Font fo = push_Font(L, imlib_load_font(path));
  if (fo == NULL)
  {
    lua_pushnil(L);
    lua_pushfstring(L, "cannot find font '%s'", path);
    return 2;
  }
  return 1;
}

/*?lua
signature "font.add_path(path)"
description [[
  Add the given path to the list of paths Imlib2 searches for font loading.
  ]]
?*/
static int font_add_path(lua_State *L)
{
  
  const char *path = luaL_checkstring(L, 1);
  /* This is safe. Imlib2 does a strdup */
  imlib_add_path_to_font_path(path);
  return 0;
}

/*?lua
signature "font.remove_path(path)"
description "Remove the given path from Imlib2's font list."
?*/
static int font_remove_path(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);

  imlib_remove_path_from_font_path(path);
  return 0;
}

/*?lua
signature "font.list_paths()"
description "Return a table containing all the font paths Imlib2 will search."
?*/
static int font_list_paths(lua_State *L)
{
  char **paths;
  int i, n;

  paths = imlib_list_font_path(&n);
  lua_createtable(L, n, 0);
  for (i=0; i<n; i++)
  {
    lua_pushstring(L, paths[i]);
    lua_rawseti(L, -2, i+1);
  }
  /* Do not free the path list */
  return 1;
}

/*?lua
signature "font.list_fonts()"
description [[
  Returns an array of all the font names Imlib2 can find in its font path.
  ]]
?*/
static int font_list_fonts(lua_State *L)
{
  char **fonts;
  int i, n;

  fonts = imlib_list_fonts(&n);
  lua_createtable(L, n, 0);
  for (i=0; i<n; i++)
  {
    lua_pushstring(L, fonts[i]);
    lua_rawseti(L, -2, i+1);
  }
  imlib_free_font_list(fonts, n);
  return 1;
}

/*?lua
signature "font.get_cache_size()"
description "Returns the font cache size in bytes"
?*/
static int font_get_cache_size(lua_State *L)
{
  lua_pushinteger(L, imlib_get_font_cache_size());
  return 1;
}

/*?lua
signature "font.set_cache_size()"
description [[
  Sets the font cache size in bytes. Whenever you set the font cache size,  
  Imlib2 will flush fonts from the -- cache until the memory used by fonts is 
  less than or equal to the font -- cache size. Setting the size to 0 frees 
  all speculatively cached fonts.
  ]]
?*/
static int font_set_cache_size(lua_State *L)
{
  int size = luaL_checkint(L, 1);

  imlib_set_font_cache_size(size);
  return 0;
}

static const char *const font_directions[] = {"right", "left", "down", "up", "angle", NULL};

/*?lua
signature "font.set_direction(dir, angle)"
description [[
  Sets the direction that text will be drawn at. Accepts 90 degree 
  orientations, or an arbitrary angle. Raises an error for invalid direction 
  names.
  ]]
param("dir", [[direction can be 'right', 'left', 'down', 'up' or 'angle'. Must
supply the second argument if 'angle' is used]])
param("angle", "Angle in radians. Only needed if direction was 'angle'")
?*/
static int font_set_direction(lua_State *L)
{
  int dir = luaL_checkoption(L, 1, NULL, font_directions);
  if (dir == IMLIB_TEXT_TO_ANGLE)
  {
    luaL_argcheck(L, lua_isnumber(L, 2), 2,
        "require a numeric angle for 'angle' direction");
    double angle = luaL_checknumber(L, 2);
    imlib_context_set_angle(angle);
  }

  imlib_context_set_direction(dir);
  return 0;
}

/*?lua
signature "font.get_direction()"
description [[
  Returns the direction that text is set to be drawn at. Returns a string if 
  the direction is "right", "left", "down" or "up".  Returns "angle" and the 
  angle in radians if text is set to be drawn at an arbitrary angle.
  ]]
?*/
static int font_get_direction(lua_State *L)
{
  int dir = imlib_context_get_direction();
  if (dir == IMLIB_TEXT_TO_ANGLE)
  {
    lua_pushstring(L, font_directions[dir]);
    lua_pushnumber(L, imlib_context_get_angle());
    return 2;
  }
  else
  {
    lua_pushstring(L, font_directions[dir]);
    return 1;
  }
}

static int fontm_gc(lua_State *L)
{
  Font *fop = (Font*)luaL_checkudata(L, 1, "imlib2.font");
  Font fo = *fop;
  if (fo)
  {
    imlib_context_set_font(fo);
    imlib_free_font();
    *fop=NULL;
  }
  return 0;
}

/*?lua
signature "fnt:__tostring()"
description [[
  Returns a string representation of the font in the format 
  &lt;imlib2.font&gt; (%p) where %p is the memory location
  ]]
?*/
static int fontm_tostring(lua_State *L)
{
  Font *fop = (Font*)luaL_checkudata(L, 1, "imlib2.font");
  Font fo = *fop;
  if (fo)
    lua_pushfstring(L, "<imlib2.font> (%p)", fo);
  else
    lua_pushfstring(L, "<imlib2.font> (freed)");
  return 1;
}

/*?lua
signature "fnt:get_size(str)"
description [[
  Get the width and height of the given string when drawn. The width and 
  height are returned in pixels.
  ]]
?*/
static int fontm_get_size(lua_State *L)
{
  Font fo = check_Font(L, 1);
  const char *text = luaL_checkstring(L, 2);

  int w, h;
  imlib_context_set_font(fo);
  imlib_get_text_size(text, &w, &h);
  lua_pushinteger(L, w);
  lua_pushinteger(L, h);
  return 2;
}

/*?lua
signature "fnt:get_advance(str)"
description [[
  Get the horizontal and vertical advance of the given string. The horizontal 
  and vertical advance are the number of pixels the next text string would 
  need to be placed at.
  ]]
?*/
static int fontm_get_advance(lua_State *L)
{
  Font fo = check_Font(L, 1);
  const char *text = luaL_checkstring(L, 2);

  int h, v;
  imlib_context_set_font(fo);
  imlib_get_text_advance(text, &h, &v);
  lua_pushinteger(L, h);
  lua_pushinteger(L, v);
  return 2;
}

/*?lua
signature "fnt:get_inset(str)"
description [[
  Returns the inset in pixels of the first character of the given string.
  ]]
?*/
static int fontm_get_inset(lua_State *L)
{
  Font fo = check_Font(L, 1);
  
  imlib_context_set_font(fo);
  lua_pushinteger(L, imlib_get_text_inset(luaL_checkstring(L, 2)));
  return 1;
}


/* NOTE:
 * imlib_text_get_location_at_index and imlib_text_get_index_and_location are 
 * no longer wrapped because I believe the underlying implementation to be 
 * buggy
 */

/*?lua
signature "fnt:get_ascent()"
description "Get the font's ascent in pixels."
?*/
static int fontm_get_ascent(lua_State *L)
{
  Font fo = check_Font(L, 1);

  imlib_context_set_font(fo);
  lua_pushinteger(L, imlib_get_font_ascent());
  return 1;
}

/*?lua
signature "fnt:get_maximum_ascent()"
description "Get the font's maximum ascent"
?*/
static int fontm_get_maximum_ascent(lua_State *L)
{
  Font fo = check_Font(L, 1);

  imlib_context_set_font(fo);
  lua_pushinteger(L, imlib_get_maximum_font_ascent());
  return 1;
}

/*?lua
signature "fnt:get_descent()"
description "Get the font's descent in pixels."
?*/
static int fontm_get_descent(lua_State *L)
{
  Font fo = check_Font(L, 1);

  imlib_context_set_font(fo);
  lua_pushinteger(L, imlib_get_font_descent());
  return 1;
}

/*?lua
signature "fnt:get_maximum_descent()"
description "Get the font's maximum descent"
?*/
static int fontm_get_maximum_descent(lua_State *L)
{
  Font fo = check_Font(L, 1);

  imlib_context_set_font(fo);
  lua_pushinteger(L, imlib_get_maximum_font_descent());
  return 1;
}


/*?lua
module "imlib2.image"
description [[
  Methods for image loading, saving and manipulation
  ]]
?*/

static Image push_Image(lua_State *L, Image im)
{
  Image *imp = (Image*)lua_newuserdata(L, sizeof(Image));
  *imp = im;
  luaL_getmetatable(L, "imlib2.image");
  lua_setmetatable(L, -2);
  return im;
}

static Image check_Image(lua_State *L, int n)
{
  Image *imp, im;
  imp = (Image*)luaL_checkudata(L, n, "imlib2.image");
  im = *imp;
  if (im == NULL)
    luaL_argerror(L, n, "image has been freed");
  return im;
}

/*?lua
signature "image.new(width, height)"
description "Create a new image of given width and height."
param("width", "width in pixels")
param("height", "height in pixels")
?*/
static int image_new(lua_State *L)
{
  int w = luaL_checkint(L, 1);
  int h = luaL_checkint(L, 2);

  Image im = push_Image(L, imlib_create_image(w, h));
  if (im == NULL)
    return luaL_error(L, "imlib_create_image failed");

  return 1;
}

/*?lua
signature "image.load(path)"
description [[
  Loads the image at the given path. Imlib2 searches its loader modules until 
  it finds one that can decode the file at the given path. The loader modules 
  available to you (and hence the range of filetypes you are able to load) 
  will depend on the options used when building the Imlib2 shared library.  
  Loaders will not decode image data until it is needed (provided this is 
  feasible for the given file format). This means it is perfectly sensible to 
  load many files in order to query their header information such as width and 
  height. If the image is present in Imlib2's cache, it won't check if the 
  file has changed and will instead just return that version of the image.
  <br/>
  If the path can not be loaded, this method will return nil followed by a 
  message describing the error
  ]]
?*/
static int image_load(lua_State *L)
{
  const char *path = luaL_checkstring(L, 1);
  Imlib_Load_Error err;
  Image im;

  im = push_Image(L, imlib_load_image_with_error_return(path, &err));
  if (err == IMLIB_LOAD_ERROR_NONE)
  {
    return 1;
  }
  else
  {
    lua_pushnil(L);
    push_load_error_str(L, err, luaL_checkstring(L, 1));
    return 2;
  }
}

static int image_fromlight(lua_State *L) {
   (void)push_Image(L,lua_touserdata(L,1));
   return 1;
}


static int image_getdata(lua_State *L) {
  Image *imp = (Image*)luaL_checkudata(L, 1, "imlib2.image");
  
  if (imp == NULL)
  {
    lua_pushfstring(L, "<imlib2.image> (freed)");
  }
  else
  {
   Image im = *imp;
   imlib_context_set_image(im);
   lua_pushlightuserdata(L,imlib_image_get_data());
  }
  return 1;
}

static int image_getdata_r(lua_State *L) {
  Image *imp = (Image*)luaL_checkudata(L, 1, "imlib2.image");
  
  if (imp == NULL)
  {
    lua_pushfstring(L, "<imlib2.image> (freed)");
  }
  else
  {
   Image im = *imp;
   imlib_context_set_image(im);
   lua_pushlightuserdata(L,imlib_image_get_data_for_reading_only());
  }
  return 1;
}


static int imagem_tolight(lua_State *L) {
  Image *imp = (Image*)luaL_checkudata(L, 1, "imlib2.image");
  if (imp == NULL)
  {
    lua_pushfstring(L, "<imlib2.image> (freed)");
  }
  else
  {
   lua_pushlightuserdata(L,*imp);
   *imp=NULL;
  }
  return 1;
}

/*?lua
signature "img:__tostring()"
description [[
  Returns a string representation of the image of the form &lt;imlib2.image 
  width=100 height=100&gt; (%p) where %p is the memory location.
  ]]
?*/
static int imagem_tostring(lua_State *L)
{
  Image *imp = (Image*)luaL_checkudata(L, 1, "imlib2.image");
  Image im = *imp;
  if (im == NULL)
  {
    lua_pushfstring(L, "<imlib2.image> (freed)");
  }
  else
  {
    imlib_context_set_image(im);
    lua_pushfstring(L, "<imlib2.image width=%d height=%d> (%p)",
        imlib_image_get_width(), imlib_image_get_height(), im);
  }
  return 1;
}

/*?lua
signature "img:free()"
description [[
  Frees the image. This does not evice the image from Imlib2's cache
  ]]
?*/
static int imagem_free(lua_State *L)
{
  Image *imp = (Image*)luaL_checkudata(L, 1, "imlib2.image");
  Image im = *imp;
  if(im)
  {
    imlib_context_set_image(im);
    imlib_free_image();
    *imp = NULL;
  }
  return 0;
}

/*?lua
signature "img:clone()"
description "Returns a clone of the image."
?*/
static int imagem_clone(lua_State *L)
{
  Image old_im = check_Image(L, 1);
  Image new_im;

  imlib_context_set_image(old_im);
  new_im = push_Image(L, imlib_clone_image());
  if (new_im == NULL)
    return luaL_error(L, "imlib_clone_image failed");
  return 1;
}

/*?lua
signature "img:get_width()"
description "Returns the width of the image in pixels."
?*/
static int imagem_get_width(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  lua_pushinteger(L, imlib_image_get_width());
  return 1;
}

/*?lua
signature "img:get_height"
description "Returns the height of the image in pixels."
?*/
static int imagem_get_height(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  lua_pushinteger(L, imlib_image_get_height());
  return 1;
}

/*?lua
signature "img:get_filename()"
description "Returns the current image filename."
?*/
static int imagem_get_filename(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  lua_pushstring(L, imlib_image_get_filename());
  return 1;
}

/*?lua
signature "img:get_format()"
description "Returns the current image format."
?*/
static int imagem_get_format(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  lua_pushstring(L, imlib_image_format());
  return 1;
}

/*?lua
signature "img:set_format(format)"
description [[
  Sets the image format to the given string. No check is made to ensure that 
  your Imlib2 can save in this format. If it can not, image:save() will fail.
  ]]
param("format", "e.g. 'png' or 'bmp'")
?*/
static int imagem_set_format(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_set_format(luaL_checkstring(L, 2));
  return 0;
}

/*?lua
signature "img:has_alpha()"
description [[
  Returns a boolean indicating whether the image has an alpha channel.
  ]]
?*/
static int imagem_has_alpha(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  lua_pushboolean(L, imlib_image_has_alpha());
  return 1;
}

/*?lua
signature "img:set_alpha()"
description [[
  Enable or disable the image's alpha channel based on the given boolean.
  ]]
?*/
static int imagem_set_alpha(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_set_has_alpha(lua_toboolean(L, 1));
  return 0;
}

/*?lua
signature "img:get_border()"
description [[
  Returns the imlib2.border object for the image. The border is the area at 
  the edge of the image that does not scale with the rest of the image when 
  resized - the borders remain constant in size.
  ]]
?*/
static int imagem_get_border(lua_State *L)
{
  Image im = check_Image(L, 1);
  Imlib_Border *b = push_Border(L);
  imlib_context_set_image(im);
  imlib_image_get_border(b);
  return 1;
}

/*?lua
signature "img:set_border(border)"
description "Sets the image border to the given imlib2.border object."
?*/
static int imagem_set_border(lua_State *L)
{
  Image im = check_Image(L, 1);
  Imlib_Border *b = (Imlib_Border*)luaL_checkudata(L, 2, "imlib2.border");
  imlib_context_set_image(im);
  imlib_image_set_border(b);
  return 0;
}

/*?lua
signature "img:get_pixel(x,y)"
description [[
  Returns the imlib2.color of the pixel at the given x,y coordinates. The 
  origin - (0, 0) is at the top left.
  ]]
?*/
static int imagem_get_pixel(lua_State *L)
{
  Image im = check_Image(L, 1);
  int x = luaL_checkint(L, 2);
  int y = luaL_checkint(L, 3);
  Imlib_Color *c = push_Color(L);
  imlib_context_set_image(im);
  imlib_image_query_pixel(x, y, c);
  return 1;
}

/*** imlib2.image metamethods which wrap functions that return copies (but are  
 * written to change self, to fit in with the rest of the api
 * ***/

/*?lua
signature "img:crop(x, y, width, height)"
description "Crops the image to the given rectangle."
param("x", "the top left x coordinate of the rectangle")
param("y", "the top left y coordinate of the rectangle")
param("width", "the width of the rectangle")
param("height", "the height of the rectangle")
?*/
static int imagem_crop(lua_State *L)
{
  Image *imp = (Image*)luaL_checkudata(L, 1, "imlib2.image");
  Image old_im;
  int x = luaL_checkint(L, 2);
  int y = luaL_checkint(L, 3);
  int w = luaL_checkint(L, 4);
  int h = luaL_checkint(L, 5);

  old_im = *imp;
  imlib_context_set_image(old_im);
  *imp = imlib_create_cropped_image(x, y, w, h);
  imlib_context_set_image(old_im);
  imlib_free_image();
  return 0;
}

/*?lua
signature [[img:crop_and_scale(source_x, source_y, source_h, source_w, dest_w, 
dest_h]]
description [[
  Crops the image to the given rectangle and also scales it. The image will be 
  scaled to the destination_width and destination_height.
  ]]
param("source_x", "the top left x coordinate of the source rectangle")
param("source_y", "the top left y coordinate of the source rectangle")
param("source_h", "the height of the source rectangle")
param("source_w", "the width of the source rectangle")
param("dest_w", "the width of the destination rectangle")
param("dest_h", "the height of the destination rectangle")
?*/
static int imagem_crop_and_scale(lua_State *L)
{
  Image *imp = (Image*)luaL_checkudata(L, 1, "imlib2.image");
  Image old_im;
  int source_x = luaL_checkint(L, 2);
  int source_y = luaL_checkint(L, 3);
  int w = luaL_checkint(L, 4);
  int h = luaL_checkint(L, 5);
  int dest_w = luaL_checkint(L, 6);
  int dest_h = luaL_checkint(L, 7);

  old_im = *imp;
  imlib_context_set_image(old_im);
  *imp =
    imlib_create_cropped_scaled_image(source_x, source_y, w, h, dest_w, dest_h);
  imlib_context_set_image(old_im);
  imlib_free_image();
  return 0;
}

/*?lua
signature "img:rotate(angle)"
description "Rotates the image by angle radians"
param("angle", "the angle of rotation in radians")
?*/
static int imagem_rotate(lua_State *L)
{
  Image *imp = (Image*)luaL_checkudata(L, 1, "imlib2.image");
  Image old_im;
  double angle = luaL_checknumber(L, 2);

  old_im = *imp;
  imlib_context_set_image(old_im);
  *imp = imlib_create_rotated_image(angle);
  imlib_context_set_image(old_im);
  imlib_free_image();
  return 0;
}

/*** End metamethods for copy-returning functions ***/

/* TODO imlib_blend_image_onto_image_at_angle */
/* TODO imlib_blend_image_onto_image_skewed */

/*?lua
signature "img:flip_horizontal()"
description "Flips the image horizontally."
?*/
static int imagem_flip_horizontal(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_flip_horizontal();
  return 1;
}

/*?lua
signature "img:flip_vertical()"
description "Flips the image vertically."
?*/
static int imagem_flip_vertical(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_flip_vertical();
  return 1;
}

/*?lua
signature "img:flip_diagonal()"
description "Flips the image diagonally."
?*/
static int imagem_flip_diagonal(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_flip_diagonal();
  return 1;
}

/*?lua
signature "img:orientate(n)"
description "Performs the given number of 90 degree rotations."
param("n", "the number of 90 degree rotations to perform")
?*/
static int imagem_orientate(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_orientate(luaL_checkint(L,2));
  return 1;
}

/*?lua
signature "img:blur(radius)"
description [[
  Blurs the image. A radius value of 0 has no effect, 1 and above determine 
  the blur matrix radius that determine how much to blur the image.
  ]]
?*/
static int imagem_blur(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_blur(luaL_checkint(L,2));
  return 1;
}

/*?lua
signature "img:sharpen(radius)"
description [[
  Sharpens the image. The radius affects how much to sharpen by.
  ]]
?*/
static int imagem_sharpen(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_sharpen(luaL_checkint(L,2));
  return 1;
}

/*?lua
signature "img:tile_horizontal()"
description "Modifies the image so it will tile seamlessly horizontally."
?*/
static int imagem_tile_horizontal(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_tile_horizontal();
  return 1;
}

/*?lua
signature "img:tile_vertical()"
description "Modifies the image so it will tile seamlessly vertically."
?*/
static int imagem_tile_vertical(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_tile_vertical();
  return 1;
}

/*?lua
signature "img:tile()"
description [[
  Modifies the image so it will tile seamlessly horizontally and vertically.
  ]]
?*/
static int imagem_tile(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_tile();
  return 1;
}

/* TODO, accept a color argument, wrapping imlib_image_clear_color */
/*?lua
signature "img:clear()"
description "Clears the contents of the image."
?*/
static int imagem_clear(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_image_clear();
  return 1;
}

/*?lua
signature "img:draw_pixel(x, y, color)"
description [[
  Draw a pixel at the specified coordinates. If no color is specified, then it 
  will use the most recent color.
  ]]
param("color", "optional. An imlib2.color object")
?*/
static int imagem_draw_pixel(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  int x = luaL_checkint(L, 2);
  int y = luaL_checkint(L, 3);
  if (lua_gettop(L)>=4) /* Given a color */
  {
    Imlib_Color *c = (Imlib_Color*)luaL_checkudata(L, 4, "imlib2.color");
    set_color(c);
  }
  imlib_image_draw_pixel(x, y, 0); /* Buggy for Imlib2 <= 1.0.5 apparently */
  return 0;
}

/*?lua
signature "img:draw_line(x1, y1, x2, y2, color)"
description [[
  Draw a line from (x1, y1) to (x2, y2). If no color is specified, then it 
  will use the most recent color.
  ]]
?*/
static int imagem_draw_line(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  int x1 = luaL_checkint(L, 2);
  int y1 = luaL_checkint(L, 3);
  int x2 = luaL_checkint(L, 4);
  int y2 = luaL_checkint(L, 5);
  if (lua_gettop(L)>=6) /* Given a color */
  {
    Imlib_Color *c = (Imlib_Color*)luaL_checkudata(L, 6, "imlib2.color");
    set_color(c);
  }
  imlib_image_draw_line(x1, y1, x2, y2, 0);
  return 0;
}

/*?lua
signature "img:draw_rectangle(x, y, width, height, color)"
description [[
  Draws the outline of a rectangle. If no color is specified, then the most 
  recent color will be used.
  ]]
?*/
static int imagem_draw_rectangle(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  int x = luaL_checkint(L, 2);
  int y = luaL_checkint(L, 3);
  int width = luaL_checkint(L, 4);
  int height = luaL_checkint(L, 5);
  if (lua_gettop(L)>=6) /* Given a color */
  {
    Imlib_Color *c = (Imlib_Color*)luaL_checkudata(L, 6, "imlib2.color");
    set_color(c);
  }
  imlib_image_draw_rectangle(x, y, width, height);
  return 0;
}

/*?lua
signature "img:fill_rectangle(x, y, width, height, color)"
description [[
  Draws a filled rectangle. If no color is specified, then the most recent 
  color will be used.
  ]]
?*/
static int imagem_fill_rectangle(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  int x = luaL_checkint(L, 2);
  int y = luaL_checkint(L, 3);
  int width = luaL_checkint(L, 4);
  int height = luaL_checkint(L, 5);
  if (lua_gettop(L)>=6) /* Given a color */
  {
    Imlib_Color *c = (Imlib_Color*)luaL_checkudata(L, 6, "imlib2.color");
    set_color(c);
  }
  imlib_image_fill_rectangle(x, y, width, height);
  return 0;
}

/* TODO imlib_copy_alpha_to_image */
/* TODO imlib_copy_alpha_rectangle_to_image */

/*?lua
signature "img:scroll_rectangle(x, y, width, height, delta_x, delta_y"
description "Scrolls the specified rectangle by the given displacement."
?*/
static int imagem_scroll_rectangle(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  int x = luaL_checkint(L, 2);
  int y = luaL_checkint(L, 3);
  int width = luaL_checkint(L, 4);
  int height = luaL_checkint(L, 5);
  int dx = luaL_checkint(L, 6);
  int dy = luaL_checkint(L, 7);
  imlib_image_scroll_rect(x, y, width, height, dx, dy);
  return 0;
}

/*?lua
signature "img:copy_rectangle(x, y, width, height, new_x, new_y)"
description "Copies the given rectangle to (new_x, new_y)"
?*/
static int imagem_copy_rectangle(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  int x = luaL_checkint(L, 2);
  int y = luaL_checkint(L, 3);
  int width = luaL_checkint(L, 4);
  int height = luaL_checkint(L, 5);
  int dest_x = luaL_checkint(L, 6);
  int dest_y = luaL_checkint(L, 7);
  imlib_image_copy_rect(x, y, width, height, dest_x, dest_y);
  return 0;
}

/*?lua
signature "img:fill_gradient(gradient, x, y, width, height, angle)"
description "Fills a rectangle with the given gradient."
param("angle", "the angle of the gradient")
?*/
static int imagem_fill_gradient(lua_State *L)
{
  Image im = check_Image(L, 1);
  Gradient gr = check_Gradient(L, 2);
  int x = luaL_checkint(L, 3);
  int y = luaL_checkint(L, 4);
  int width = luaL_checkint(L, 5);
  int height = luaL_checkint(L, 6);
  double angle = luaL_optnumber(L, 7, 0.0);

  imlib_context_set_image(im);
  imlib_context_set_color_range(gr);
  imlib_image_fill_color_range_rectangle(x, y, width, height, angle);
  return 0;
}

/*?lua
description [[
  Draws the outline of an ellipse. (xc, yc) is the centre of the ellipse, 
  which is described by the equation (x-xc)^2/a^2 + (y-yc)^2/b^2 = 1. If no 
  color is specified, then the most recent color will be used.
  ]]
param("a", "horizontal amplitude")
param("b", "vertical amplitude")
?*/
static int imagem_draw_ellipse(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  int xc = luaL_checkint(L, 2);
  int yc = luaL_checkint(L, 3);
  int a = luaL_checkint(L, 4); /* Horizontal amplitude */
  int b = luaL_checkint(L, 5); /* Vertical amplitude */
  if (lua_gettop(L)>=6) /* Given a color */
  {
    Imlib_Color *c = (Imlib_Color*)luaL_checkudata(L, 6, "imlib2.color");
    set_color(c);
  }
  imlib_image_draw_ellipse(xc, yc, a, b);
  return 0;
}

/*?lua
description [[
  Fills an ellipse (xc, yc) is the centre of the ellipse, which is described 
  by the equation (x-xc)^2/a^2 + (y-yc)^2/b^2 = 1. If no color is specified, 
  then the most recent color will be used.
  ]]
param("a", "horizontal amplitude")
param("b", "vertical amplitude")
?*/
static int imagem_fill_ellipse(lua_State *L)
{
  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  int xc = luaL_checkint(L, 2);
  int yc = luaL_checkint(L, 3);
  int a = luaL_checkint(L, 4); /* Horizontal amplitude */
  int b = luaL_checkint(L, 5); /* Vertical amplitude */
  if (lua_gettop(L)>=6) /* Given a color */
  {
    Imlib_Color *c = (Imlib_Color*)luaL_checkudata(L, 6, "imlib2.color");
    set_color(c);
  }
  imlib_image_fill_ellipse(xc, yc, a, b);
  return 0;
}

/*?lua
description [[
  Draws the outline of a polygon. Points in the imlib2.polygon are drawn first 
  to last (in the order in which they were added). The final point will be 
  drawn to the first point if closed is true. If no color is specified, then 
  the most recent color will be used.
  ]]
param("closed", "closed polygon flag")
?*/
static int imagem_draw_polygon(lua_State *L)
{
  Image im = check_Image(L, 1);
  Polygon po = check_Polygon(L, 2);
  int closed = lua_toboolean(L, 3);
  if (lua_gettop(L)>=4) /* Given a color */
  {
    Imlib_Color *c = (Imlib_Color*)luaL_checkudata(L, 4, "imlib2.color");
    set_color(c);
  }
  imlib_context_set_image(im);
  imlib_image_draw_polygon(po, closed);
  return 0;
}

/*?lua
signature "img:fill_polygon(polygon, color)"
description [[
  Fills a polygon. If no color is specified, then the most recent color will 
  be used.
  ]]
?*/
static int imagem_fill_polygon(lua_State *L)
{
  Image im = check_Image(L, 1);
  Polygon po = check_Polygon(L, 2);
  if (lua_gettop(L)>=3) /* Given a color */
  {
    Imlib_Color *c = (Imlib_Color*)luaL_checkudata(L, 4, "imlib2.color");
    set_color(c);
  }
  imlib_context_set_image(im);
  imlib_image_fill_polygon(po);
  return 0;
}

/*?lua
signature "img:draw_text(font, string, x, y, color)"
description [[
  Draws a string in the given font at (x, y). If no color is specified, then 
  the most recent color will be used.
  ]]
param("font", "an imlib2.font")
?*/
static int imagem_draw_text(lua_State *L)
{
  Image im = check_Image(L, 1);
  Font fo = check_Font(L, 2);
  const char *str = luaL_checkstring(L, 3);
  int x = luaL_checkint(L, 4);
  int y = luaL_checkint(L, 5);
  Imlib_Color *c =  (Imlib_Color*)luaL_checkudata(L, 6, "imlib2.color");
  int i;
  int r[] = {0,0,0,0};

  imlib_context_set_font(fo);
  imlib_context_set_image(im);
  set_color(c);
  imlib_text_draw_with_return_metrics(x, y, str, &r[0], &r[1], &r[2], &r[3]);
  for (i=0; i < 4; i++)
    lua_pushinteger(L, r[i]);
  return 4;
}

/* TODO: Look at imlib_filter_* */

/*?lua
signature "img:save(path)"
description [[
  Saves the image to the given path. The file extension (e.g. .png or .jpg) 
  will affect the output format (but may not override the format set by 
  img:set_format()
  <br/>
  Returns nil, err_msg if the save is not successful.
  ]]
?*/
static int imagem_save(lua_State *L)
{
  Imlib_Load_Error err;

  Image im = check_Image(L, 1);
  imlib_context_set_image(im);
  imlib_save_image_with_error_return(luaL_checkstring(L,2), &err);

  if (err == IMLIB_LOAD_ERROR_NONE)
  {
    return 0;
  }
  else
  {
    lua_pushnil(L);
    push_load_error_str(L, err, luaL_checkstring(L, 1));
    return 2;
  }
}

/*** imlib2 functions ***/
/*?lua
module "imlib2"
description [[
  Functions to modify the operation of the imlib2 library.
  ]]
?*/

/*?lua
signature "imlib2.set_anti_alias(bool)"
description "Enable or disable anti-aliasing for future drawing operations."
?*/
static int set_anti_alias(lua_State *L)
{
  luaL_checkany(L, 1);
  imlib_context_set_anti_alias(lua_toboolean(L, 1));
  return 1;
}

/*?lua
signature "imlib2.get_anti_alias(bool)"
description [[
  Returns a boolean indicating whether anti-aliasing is enabled or disabled
  ]]
?*/
static int get_anti_alias(lua_State *L)
{
  lua_pushboolean(L, imlib_context_get_anti_alias());
  return 1;
}

/*?lua
signature "imlib2.get_cache_size()"
description [[
  Return the current size of the image cache in bytes.
  ]]
?*/
static int get_cache_size(lua_State *L)
{
  lua_pushinteger(L, imlib_get_cache_size());
  return 1;
}

/*?lua
signature "imlib2.set_cache_size()"
description "Set the size of the image cache in bytes."
?*/
static int set_cache_size(lua_State *L)
{
  imlib_set_cache_size(luaL_checkint(L, 1));
  return 1;
}

/*?lua
signature "imlib2.flush_cache()"
description "Flushes the cache"
?*/
static int flush_cache(lua_State *L)
{
  int csize = imlib_get_cache_size();
  imlib_set_cache_size(0);
  imlib_set_cache_size(csize);
  return 0;
}


/*** Registration and initialization ***/

static const struct luaL_Reg border_f [] = {
  {"new", border_new},
  {NULL, NULL}
};

static const struct luaL_Reg border_m [] = {
  {"__tostring", borderm_tostring},
  {"__index", borderm__index},
  {"__newindex", borderm__newindex},
  {NULL, NULL}
};

static const struct luaL_Reg color_f [] = {
  {"new", color_new},
  {NULL, NULL}
};

static const struct luaL_Reg color_m [] = {
  {"__tostring", colorm_tostring},
  {"__index", colorm__index},
  {"__newindex", colorm__newindex},
  {NULL, NULL}
};

static const struct luaL_Reg gradient_f [] = {
  {"new", gradient_new},
  {NULL, NULL}
};

static const struct luaL_Reg gradient_m [] = {
  {"__gc", gradientm_gc},
  {"__tostring", gradientm_tostring},
  {"add_color", gradientm_add_color},
  {NULL, NULL}
};

static const struct luaL_Reg polygon_f [] = {
  {"new", polygon_new},
  {NULL, NULL}
};

static const struct luaL_Reg polygon_m [] = {
  {"__gc", polygonm_gc},
  {"__tostring", polygonm_tostring},
  {"add_point", polygonm_add_point},
  {"get_bounds", polygonm_get_bounds},
  {"contains_point", polygonm_contains_point},
  {NULL, NULL}
};

static const struct luaL_Reg font_f [] = {
  {"load", font_load},
  {"add_path", font_add_path},
  {"remove_path", font_remove_path},
  {"list_paths", font_list_paths},
  {"list_fonts", font_list_fonts},
  {"get_cache_size", font_get_cache_size},
  {"set_cache_size", font_set_cache_size},
  {"set_direction", font_set_direction},
  {"get_direction", font_get_direction},
  {NULL, NULL}
};

static const struct luaL_Reg font_m [] = {
  {"__gc", fontm_gc},
  {"__tostring", fontm_tostring},
  {"get_size", fontm_get_size},
  {"get_advance", fontm_get_advance},
  {"get_inset", fontm_get_inset},
  {"get_ascent", fontm_get_ascent},
  {"get_maximum_ascent", fontm_get_maximum_ascent},
  {"get_descent", fontm_get_descent},
  {"get_maximum_descent", fontm_get_maximum_descent},
  {NULL, NULL}
};


static const struct luaL_Reg image_f [] = {
  {"new", image_new},
  {"load", image_load},
  {"from_ptr", image_fromlight},
  {NULL, NULL}
};

static const struct luaL_Reg image_m [] = {
  {"__tostring", imagem_tostring},
  {"__gc", imagem_free},
  {"free", imagem_free},
  {"to_ptr", imagem_tolight},
  {"get_data", image_getdata},
  {"get_data_r", image_getdata_r},
  {"get_height", imagem_get_height},
  {"get_width", imagem_get_width},
  {"get_height", imagem_get_height},
  {"get_filename", imagem_get_filename},
  {"get_format", imagem_get_format},
  {"set_format", imagem_set_format},
  {"has_alpha", imagem_has_alpha},
  {"set_has_alpha", imagem_set_alpha},
  {"get_border", imagem_get_border},
  {"set_border", imagem_set_border},
  {"get_pixel", imagem_get_pixel},
  {"crop", imagem_crop},
  {"crop_and_scale", imagem_crop_and_scale},
  {"rotate", imagem_rotate},
  {"flip_horizontal", imagem_flip_horizontal},
  {"flip_vertical", imagem_flip_vertical},
  {"flip_diagonal", imagem_flip_diagonal},
  {"orientate", imagem_orientate},
  {"blur", imagem_blur},
  {"sharpen", imagem_sharpen},
  {"tile_horizontal", imagem_tile_horizontal},
  {"tile_vertical", imagem_tile_vertical},
  {"tile", imagem_tile},
  {"clear", imagem_clear},
  {"draw_pixel", imagem_draw_pixel},
  {"draw_line", imagem_draw_line},
  {"draw_rectangle", imagem_draw_rectangle},
  {"fill_rectangle", imagem_fill_rectangle},
  {"scroll_rectangle", imagem_scroll_rectangle},
  {"copy_rectangle", imagem_copy_rectangle},
  {"draw_polygon", imagem_draw_polygon},
  {"fill_polygon", imagem_fill_polygon},
  {"fill_gradient", imagem_fill_gradient},
  {"draw_ellipse", imagem_draw_ellipse},
  {"fill_ellipse", imagem_fill_ellipse},
  {"draw_text", imagem_draw_text},
  {"clone", imagem_clone},
  {"save", imagem_save},
  {NULL, NULL}
};

static const struct luaL_Reg f [] = {
  {"set_anti_alias", set_anti_alias},
  {"get_anti_alias", get_anti_alias},
  {"get_cache_size", get_cache_size},
  {"set_cache_size", set_cache_size},
  {"flush_cache", flush_cache},
  {NULL, NULL}
};

int luaopen_limlib2(lua_State *L)
{
  imlib_context_set_anti_alias(1); /* Ensure anti-alias by default */

  luaL_newmetatable(L, "imlib2.border");
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, border_m);
  luaL_register(L, "imlib2.border", border_f);

  luaL_newmetatable(L, "imlib2.color");
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, color_m);
  luaL_register(L, "imlib2.color", color_f);

  luaL_newmetatable(L, "imlib2.gradient");
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, gradient_m);
  luaL_register(L, "imlib2.gradient", gradient_f);

  luaL_newmetatable(L, "imlib2.polygon");
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, polygon_m);
  luaL_register(L, "imlib2.polygon", polygon_f);

  luaL_newmetatable(L, "imlib2.font");
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, font_m);
  luaL_register(L, "imlib2.font", font_f);

  luaL_newmetatable(L, "imlib2.image");
  lua_pushvalue(L, -1);
  lua_setfield(L, -2, "__index");
  luaL_register(L, NULL, image_m);
  luaL_register(L, "imlib2.image", image_f);

  luaL_register(L, "imlib2", f);

  return 1;
}
