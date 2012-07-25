-----------------------------------------------------------------------------
-- Leda's debug API
-- Author: Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura
-----------------------------------------------------------------------------
-----------------------------------------------------------------------------
-- Declare module and import dependencies
-----------------------------------------------------------------------------
local string = string
local io=io
module("leda.debug")

-----------------------------------------------------------------------------
-- Function to get a debug handler
-----------------------------------------------------------------------------
function get_debug(prefix,sufix)
   return function () end --comment this line to enable debugging
--[[   sufix=sufix or "\n"
   return function (fmt, ...)
      io.stderr:write(prefix)
      io.stderr:write(string.format(fmt, ...))
      io.stderr:write(sufix)
   end
   --]]
end

