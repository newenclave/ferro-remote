--- makes system( "command" ) on the target system

function main( argv )
    for i, v in pairs(argv) do
	fr.print( "Call system '", v, "' ... " )
	local res, err = fr.client.os.system( v )
	if err then 
	    fr.print( "error: ", err )
	else 
	    fr.print( "return ", res )
	end
	print( ) -- new line
    end
end

