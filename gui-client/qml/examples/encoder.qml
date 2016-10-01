import QtQuick 2.0
import QtQuick.Controls 1.1
import Fr.Client 1.0

Rectangle {

    color:  "white"

    Item {

        id: encoder
        property int step: step.value
        property int state: 0
        property bool l_state: false
        property bool r_state: false

        FrClientGpio {
            client:     remoteClient
            index:      5
            direction:  FrClientGpio.DirectIn
            edge:       FrClientGpio.EdgeBoth
            id:         left
            events:     true
        }

        FrClientGpio {
            client:     remoteClient
            index:      4
            direction:  FrClientGpio.DirectIn
            edge:       FrClientGpio.EdgeBoth
            id:         right
            events:     true
        }

        Connections {
            target: right
            onChangeEvent: {
                encoder.r_state = (value != 0);
                if( encoder.r_state && encoder.l_state ) {
                    encoder.state = encoder.state + encoder.step;
                }
            }
        }

        Connections {
            target: left
            onChangeEvent: {
                encoder.l_state = (value != 0);
                if( encoder.l_state && encoder.r_state ) {
                    encoder.state = encoder.state - encoder.step;
                }
            }
        }

        Connections {
            target: clear
            onClicked: {
                encoder.state = 0
            }
        }
    }

    FrClient {
        id: remoteClient
        Component.onCompleted: connect( "192.168.3.1:12345" )
    }

    Image {
        id: compas
        source: "compas.png"
        rotation: encoder.state
        Component.onCompleted: {
            parent.height = compas.height
            parent.width  = compas.width
        }
    }

    SpinBox {
        id: step
        minimumValue: -359
        maximumValue:  359
        value:         1
        anchors {
            left:   parent.left
            bottom: parent.bottom
        }
    }

    Button {
        id: clear
        text: "Drop"
        anchors {
            right: parent.right
            bottom: parent.bottom
        }
    }

    Connections {
        target: remoteClient
        onTryConnect:   console.log( "local: I'm trying!!!" )
        onConnected:    console.log( "local connected" )
        onChannelReady: console.log( "local channel ready" )
        onDisconnected: console.log( "local disconnected" )
        onInitError:    console.log( "local init error " + message )
    }
}
