import QtQuick 2.0
import QtQuick.Controls 1.1
import Fr.Client 1.0

Rectangle {

    height: 40
    width:  350
    color:  "white"

    Item {

        id:sonic
        property int tick: 0
        property double last_distance: 0.0

        FrClientGpio {
            id:         trigger
            index:      4
            client:     remoteClient
            direction:  FrClientGpio.DirectOut
        }

        FrClientGpio {
            id:         echo
            index:      5
            client:     remoteClient
            direction:  FrClientGpio.DirectIn
            edge:       FrClientGpio.EdgeBoth
            events:     true
        }

        Connections {
            target: echo
            onChangeEvent: {
                var inter = interval % 1000000
                if( sonic.tick != 0 ) {
                    if( value == 1 ) {
                        /// signal was sent
                    } else {
                        sonic.last_distance = (inter - sonic.tick) / 58
                    }
                }
                sonic.tick = inter
            }
        }

        Connections {
            target: shot
            onTriggered: {
                sonic.tick = 0;
                trigger.makePulse( 70 )
            }
        }
    }

    Timer {
        id: shot
        interval: 500
        repeat: true
        running: true
    }

    FrClient {
        id: remoteClient
        Component.onCompleted: connect( "192.168.3.1:12345" )
    }

    Label {
        font.pointSize: 21
        text: sonic.last_distance.toString( )
    }

    Connections {
        target: remoteClient
        onTryConnect:   console.log( "remoteClient: I'm trying!!!" )
        onConnected:    console.log( "remoteClient: connected" )
        onChannelReady: console.log( "remoteClient: channel ready" )
        onDisconnected: console.log( "remoteClient: disconnected " )
        onInitError:    console.log( "remoteClient: init error " + message )
    }
}
