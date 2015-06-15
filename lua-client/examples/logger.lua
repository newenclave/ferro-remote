log = fr.client.logger

function main( )
    fr.run( )
    local lgr, err = log.add( )
    lgr:subscribe( "on_write", function( data ) fr.print( data.text, "\n" ) end )
end
