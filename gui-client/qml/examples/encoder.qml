import QtQuick 2.0
import QtQuick.Controls 1.1
import Fr.Client 1.0

Rectangle {

    color:  "white"

    Item {

        id: encoder
        property int count: 0

        FrClientGpio {
            property bool up: false
            client:     remoteClient
            index:      5
            direction:  FrClientGpio.DirectIn
            edge:       FrClientGpio.EdgeBoth
            id:         pinA
            events:     true
        }

        FrClientGpio {
            property bool up: false
            client:     remoteClient
            index:      4
            direction:  FrClientGpio.DirectIn
            edge:       FrClientGpio.EdgeBoth
            id:         pinB
            events:     true
        }

        Connections {
            target: pinB
            onChangeEvent: {
                pinB.up = (value != 0);
                if( pinB.up && pinA.up ) {
                    encoder.count = encoder.count + step.value;
                }
            }
        }

        Connections {
            target: pinA
            onChangeEvent: {
                pinA.up = (value != 0);
                if( pinB.up && pinA.up ) {
                    encoder.count = encoder.count - step.value;
                }
            }
        }

        Connections {
            target: clear
            onClicked: {
                encoder.count = 0
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
        rotation: encoder.count
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
