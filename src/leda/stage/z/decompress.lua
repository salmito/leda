local _=require 'leda'
require 'zlib'
-----------------------------------------------------------------------------
-- Decompress an event with zlib
-----------------------------------------------------------------------------

local stage={}

function stage.handler(compressed_str,eof,bytes_in,bytes_out)
   local stream = zlib.inflate()
   local decompressed_str,eof_2,bytes_in_2,bytes_out_2=stream(str);
   leda.send('data',unpack(leda.decode(decompressed_str)))
end

function stage.init()
	require 'zlib'
end

function stage.bind(self,out,graph)
	assert(out.data,"Data port must be connected for stage: "..tostring(self))
end

stage.name='Lossless Decompressor'

return _.stage(stage)
