function stop( code )
    print( "stop" )
    fr.exit( code ) -- завершим очередь, выйдем, вернем 100
end

function main( argv )
    fr.run( ) -- запустим очередь. 
    fr.client.event_queue.post( stop, 100 ) -- отправили на исполнение функцию stop 
                                            -- с параметром 100
    print( "leave 'main'" )
end

