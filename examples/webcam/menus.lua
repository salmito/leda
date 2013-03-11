-- ======================================================================
-- menus.lua - Copyright (C) 2005-2006 Varol Kaptan
-- see LICENSE for more information
-- ======================================================================
-- vim: set ts=3 et:

-- everything related to menu management is contained in the
-- luaglut.menus table. Integer indices (1..) represent the menus
-- themselves, uidmap is a an array of unique ids used to translate
-- values between the luaglut layer and lua user layer

local uidmap = { next_free = -1 }
local menu = { uidmap = uidmap }
luaglut.menu = menu

local function get_uid(entry)
   local uid
   if uidmap.next_free < 0 then
      -- all taken, create a new one
      uid = - uidmap.next_free
      uidmap[uid] = entry
      uidmap.next_free = uidmap.next_free - 1
   else
      -- reuse uid
      uid = uidmap.next_free
      uidmap.next_free = uidmap[uid]
      uidmap[uid] = entry
   end
   return uid
end

local function release_uid(uid)
   uidmap.next_free, uidmap[uid] = uid, uidmap.next_free
end

-- These are the original glut functions, we will override
-- them here with versions which will perform some extra
-- book-keeping

local _glutCreateMenu         = glutCreateMenu
local _glutDestroyMenu        = glutDestroyMenu
local _glutAddMenuEntry       = glutAddMenuEntry
local _glutAddSubMenu         = glutAddSubMenu
local _glutChangeToMenuEntry  = glutChangeToMenuEntry
local _glutChangeToSubMenu    = glutChangeToSubMenu
local _glutRemoveMenuItem     = glutRemoveMenuItem

function glutCreateMenu(callback)
   local id = _glutCreateMenu()
   menu[id] = { callback = callback, id = id, entries = { } }
   return id
end

function glutDestroyMenu(id)
   _glutDestroyMenu(id)
   for k, v in menu[id].entries do
      if v.value then release_uid(v.uid) end
   end
   menu[id] = nil
end

function glutAddMenuEntry(name, value)
   local id = glutGetMenu()
   local entry = { name = name, value = value, callback = menu[id].callback }
   entry.uid = get_uid(entry)
   table.insert(menu[id].entries, entry)
   _glutAddMenuEntry(name, entry.uid)
end

function glutAddSubMenu(name, child)
   local id = glutGetMenu()
   local submenu = { name = name, child = child, }
   table.insert(menu[id].entries, submenu)
   _glutAddSubMenu(name, child)
end

function glutChangeToMenuEntry(index, name, value)
   local id = glutGetMenu()
   local entry = menu[id].entries[index]
   if entry.value then
      -- just update an entry
      entry.name  = name
      entry.value = value
   else
      -- change to entry from a submenu
      entry.name     = name
      entry.value    = value
      entry.callback = menu[id].callback
      entry.uid      = get_uid(entry)
      entry.child    = nil
   end
   _glutChangeToMenuEntry(index, name, entry.uid)
end

function glutChangeToSubMenu(index, name, child)
   local id = glutGetMenu()
   local entry = menu[id].entries[index]
   if entry.value then
      -- change from an entry to submenu
      entry.name     = name
      entry.child    = child
      entry.value    = nil
      entry.callback = nil
      release_uid(entry.uid)
      entry.uid      = nil
   else
      -- just update a submenu
      entry.name     = name
      entry.child    = child
   end
   _glutChangeToSubMenu(index, name, child)
end

function glutRemoveMenuItem(index)
   local id = glutGetMenu()
   local entry = menu[id].entries[index]
   _glutRemoveMenuItem(index)
   if entry.uid then
      release_uid(entry.uid)
   end
   table.remove(menu[id].entries, index)
end
