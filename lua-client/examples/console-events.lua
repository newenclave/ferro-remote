con = fr.client.console

function main( ) 
    local function console_reader( data )
	local f, e = load( "return "..data.data )
	if f then 
	    print( "result: ", f( ) )
	else 
	    print( "error: ", e )
	end
    end
    fr.run( )
    con.subscribe( "on_read", console_reader )
end

