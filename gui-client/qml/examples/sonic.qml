import QtQuick 2.0
import QtQuick.Controls 1.1
import Fr.Client 1.0

Rectangle {

    height: 40
    width:  350
    color:  "white"

    Item {

        id:sonic
        property real tick: 0
        property real lastDistance: 0.0

        property int triggerId: 4
        property int echoId:    5

        FrClientGpio {
            id:         trigger
            index:      sonic.triggerId
            client:     remoteClient
            direction:  FrClientGpio.DirectOut
        }

        FrClientGpio {
            id:         echo
            index:      sonic.echoId
            client:     remoteClient
            direction:  FrClientGpio.DirectIn
            edge:       FrClientGpio.EdgeBoth
            events:     true
        }

        Connections {
            target: echo
            onChangeEvent: {
                //console.log( sonic.tick )
                if( value == 0 ) {
                    sonic.lastDistance = (interval - sonic.tick) / 58
                }
                sonic.tick = interval
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
        id: output
        font.pointSize: 21
        text: sonic.lastDistance.toString( )
        Connections {
            target: sonic
            onLastDistanceChanged: {
                if( sonic.lastDistance < 0 || sonic.lastDistance > 400 ) {
                    output.color = "red"
                } else {
                    output.color = "black"
                }
            }
        }
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
