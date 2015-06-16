log = fr.client.logger
timer = fr.client.event_queue.timer

function main( )
    fr.run( )
    local lgr, err = log.add( )
    -- timer event. after 1 second process will send log string and exit
    local function lcall( err, lgr, id )
        lgr:write( "INF", "Cient log "..tostring(id).."!" )
        timer.post( lcall, { millii = 100 }, lgr, id + 1 )
    end

    lcall( nil, lgr, 0 )    

    -- logger event handler. 
    lgr:subscribe( "on_write", function( data ) 
        local dt = os.date( "%H:%M:%S", data.time.sec )
        fr.print( dt, 
                  ":", data.time.micro, 
                  " [", data.levelstr, "] ",  
                  data.text, "\n" ) 
    end )
    lgr:write( "INF", "Client logger started." )
end

