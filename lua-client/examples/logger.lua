log = fr.client.logger

function main( )
    fr.run( )
    local lgr, err = log.add( )
    lgr:subscribe( "on_write", function( data ) 
        local dt = os.date( "*t", data.time // 1000000 )
        local levels = { "ERR", "WRN", "INF", "DBG" }
        fr.print( dt.hour, ":", dt.min, ":", dt.sec, 
                  ":", data.time % 1000000, 
                  " [", levels[data.level], "] ",  
                  data.text, "\n" ) 
    end )
    lgr:write( "INF", "Client logger started." )
end

