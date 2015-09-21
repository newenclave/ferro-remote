fs = fr.client.fs
file = fs.file

function main( )
    fr.run( ) 
    local f = assert(file.open( "/dev/pts/0" ))
    assert(f:subscribe( "on_pollin", function( data ) print(data.data) end, f ))
end

