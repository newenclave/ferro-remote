
function main( argv )
	
    local path = argv[1]
    local out  = argv[2]
    if not path then return end
    if not out then
        out = "pulled.file"
    end	

    local fs   = fr.client.fs
    local file = fs.file
    local f, e = file.open( path, "rb" )
    local out, eout = io.open( out, "wb" )
    if f and out then
        f:seek( 0, "end" )   
        print( "file size:", f:tell( ) )
        f:seek( 0 )
        local d = f:read( )
        while d do 
            out:write( d )
            d = f:read( ) 
        end
    else
        print( "remote error:", e, "local error:", eout )
	end
end

