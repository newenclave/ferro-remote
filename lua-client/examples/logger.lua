log = fr.client.logger
timer = fr.client.event_queue.timer

function main( )
    fr.run( )
    local lgr, err = log.add( )
    -- timer event. after 1 second process will send log string and exit
    timer.post( function( err, lgr ) 
        lgr:write( "INF", "Client logger: Goodbye!" ) 
        fr.exit( ) 
    end, { sec=1 }, lgr )

    -- logger event handler. 
    lgr:subscribe( "on_write", function( data ) 
        local dt = os.date( "*t", data.time // 1000000 )
        fr.print( dt.hour, ":", dt.min, ":", dt.sec, 
                  ":", data.time % 1000000, 
                  " [", data.levelstr, "] ",  
                  data.text, "\n" ) 
    end )
    lgr:write( "INF", "Client logger started." )
end

