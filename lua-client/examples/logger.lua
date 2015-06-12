log = fr.client.logger

function main( )
    fr.print( log, "\n" )
    local lgr, err = log.add( )
    print( lgr )
    print(lgr:write( 1, "test" ))
    print(log.set_level( 4 ))
    lgr:write( 2, "test" )
    lgr:write( 3, "test" )
    lgr:write( "DBG", "test" )
    lgr:subscribe( "on_write", function( data ) end )
    lgr:write( 2, "sadsf" )
    local lgr2 = log.add( )
    lgr:subscribe( "on_write" )
    lgr2:write( 3, "sadsf2" )
    fr.print( lgr:events( ), "\n" )
    fr.print( lgr2:events( ) )
end
