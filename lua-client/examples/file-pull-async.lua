file = fr.client.fs.file

function file_get( from, to, maximum_block )

	local eq = fr.client.event_queue	

	local function impl( input, output )
		local d = input:read( maximum_block )	
		if d then 
			print( "got", string.len(d), "bytes" )
			output:write( d )
			eq.post( impl, input, output )
		else
			fr.exit( )
		end
	end	

    local f, e = file.open( from, "rb" )
    local out, eout = io.open( to, "wb" )
	
	if f and out then 
		print( "file: ", f, out )
		impl( f, out )
	else 
        print( "remote error:", e, "local error:", eout )
	end
	
end

function main( argv )

	fr.run( )	
	
    local path = argv[1]
    local out  = argv[2]
    if not path then return end
    if not out then
        out = "pulled.file"
    end	
	
	file_get( path, out )

end
