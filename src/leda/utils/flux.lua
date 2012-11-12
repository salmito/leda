local handler=__handler
local oldsend=leda.send

local metatable={}

function __get_meta()
	return metatable
end

metatable.__persist=function(t)
	setmetatable(t,nil)
	local new=leda.clone(t)
	setmetatable(t,metatable)
	return function() return setmetatable(new,__get_meta()) end
end

local function is_flux(t)
	return getmetatable(t)==metatable
end

__handler=function(flux,...)
	leda.newflux=function() flux=setmetatable({},metatable) end
	
	leda.setflux=function(key,val) 
		if is_flux(flux) then 
			flux[key]=val 
		else 
			error("No flux created") 
		end
	end
	
	leda.getflux=function(key) 
		if not is_flux(flux) then 
			error("No flux created") 
		end
		if key then 
			return flux[key] 
		else 
			return flux 
		end 
	end
	
	leda.send=function(key,...) 
		oldsend(key,flux,...) 
	end
	
	if is_flux(flux) then
		return handler(...)
	else
		return handler(flux,...)
	end
end

