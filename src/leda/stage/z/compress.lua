local _=require 'leda'
require 'zlib'
-----------------------------------------------------------------------------
-- Compress an event with zlib
-- param:   '...': data to be compressed
-----------------------------------------------------------------------------

local stage={}

function stage.handler(...)
	local str=leda.encode({...})
   local stream = zlib.deflate()
   local compressed_str,eof,bytes_in,bytes_out=stream(str,'full')
   leda.push(compressed_str,eof,bytes_in,bytes_out)
end

function stage.init()
	require 'zlib'
end

function stage.bind(self,out,graph)
	assert(out[1],"Default port must be connected for stage: "..tostring(self))
end

stage.name='Lossless compressor'

return _.stage(stage)
