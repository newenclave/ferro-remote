function stop( code )
    print( "stop" )
    fr.exit( code ) 
end

function timer( err, hello )
    if err then 
        print( "timer cancelled" )
    else 
        print( hello )	
        fr.client.event_queue.timer.post( timer, 1, hello.."." )
    end
end

function main( argv )
    fr.run( ) 
    fr.client.event_queue.timer.post( stop, 10, 100 )
    timer( nil, "." ) 
    print( "leave 'main'" )
end

