function stop( code )
    print( "stop" )
    fr.exit( code ) -- Stop the queue. return 100
end

function main( argv )
    fr.run( ) -- запустим очередь. 
    fr.client.event_queue.post( stop, 100 ) -- post function 'stop' to the queue
                                            -- with 100 as parameter
    print( "leave 'main'" )
end

