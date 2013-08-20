local _=require 'leda'
return _.stage 'Lua-repl' {
	handler=function()
		local repl          = require 'repl.console'
		local has_linenoise = pcall(require, 'linenoise')

		if has_linenoise then
		  repl:loadplugin 'linenoise'
		else
		  -- XXX check that we're not receiving input from a non-tty
			local has_rlwrap = os.execute('which rlwrap >/dev/null 2>/dev/null') == 0
	
			if has_rlwrap and not os.getenv 'LUA_REPL_RLWRAP' then
			   local lowest_index = -1

  			  	while arg[lowest_index] ~= nil do
  			    	lowest_index = lowest_index - 1
  			  	end
  			  	lowest_index = lowest_index + 1
  			  	os.execute(string.format('LUA_REPL_RLWRAP=1 rlwrap %q %q', arg[lowest_index], arg[0]))
  			  	return
  			end
	  end

	  repl:loadplugin 'history'
	  repl:loadplugin 'completion'
     repl:loadplugin 'autoreturn'
     repl:loadplugin 'rcfile'

	  print('Lua REPL Stage - repl version: ' .. tostring(repl.VERSION))

     leda.quit(repl:run())
	end,
	init=function()
		leda.loadlibs()
	end,
	bind=function(self,out,g)
		for s,v in pairs(g:stages()) do
			if s~=self then
				g:add(self:connect(s.name,s))
			end
		end
	end,
	autostart=true,
	serial=true
}
