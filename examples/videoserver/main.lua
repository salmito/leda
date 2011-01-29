#!/usr/bin/lua

require("leda")

local mancode=require("manager")
local sencode=require("sender")

local man=leda.createStage(mancode)
local man=leda.createStage(sender)

leda.start()
