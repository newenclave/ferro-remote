import QtQuick 2.0
import QtQuick.Controls 1.1
import Fr.Client 1.0

Rectangle {

    height: values.height
    width:  350
    color:  "white"

    FrClient {
        id: remoteClient
        Component.onCompleted: connect( "192.168.3.1:12345" )
    }

    Item {
        id:gyro
        property int address: 0x69

        property int xReg: 0
        property int yReg: 0
        property int zReg: 0
        property int tempReg: 0

        FrClientI2c {
            id: gyroDev
            client: remoteClient
            busId: 1
            slaveAddress: gyro.address

            function init( )
            {
                gyroDev.writeBytes( [
                   {0x20: 15},
                   {0x21: 0 },
                   {0x22: 8 },
                   {0x23: 0 },
                   {0x24: 0 },
                ] )
            }


            function getValues( )
            {
                var values = [ 0x28, 0x29, // X
                               0x2A, 0x2B, // Y
                               0x2C, 0x2D, // Z
                               0x26 ]      // Temp

                var res = gyroDev.readBytes( values )

                var x = 0, y = 0, z = 0, t = 0

                if( (0x28 in res) && (0x29 in res)) {
                    x = (res[0x29] << 8) | res[0x28]
                }

                if( (0x2A in res) && (0x2B in res)) {
                    y = (res[0x2B] << 8) | res[0x2A]
                }

                if( (0x2C in res) && (0x2D in res)) {
                    z = (res[0x2D] << 8) | res[0x2C]
                }

                if( 0x26 in res ) {
                    t = res[0x26]
                }

                gyro.xReg       = x
                gyro.yReg       = y
                gyro.zReg       = z
                gyro.tempReg    = t

            }

        }

        Connections {
            target: gyroDev
            onReadyChanged: {
                if( value ) {
                    gyroDev.init( )
                }
            }
        }

        Connections {
            target: shot
            onTriggered: {
                if( gyroDev.ready ) {
                    gyroDev.getValues( )
                }
            }
        }
    }

    Timer {
        id: shot
        interval: 500
        repeat:  true
        running: true
    }

    Row {
        id: values
        spacing: 10
        property int fontSize: 14
        Column {
            spacing: values.spacing
            Label {
                font.pointSize: values.fontSize
                text: " "
            }
            Label {
                font.pointSize: values.fontSize
                text: "X"
                color: "green"
            }
            Label {
                font.pointSize: values.fontSize
                text: "Y"
                color: "green"
            }
            Label {
                font.pointSize: values.fontSize
                text: "Z"
                color: "green"
            }

        }

        Column {
            spacing: values.spacing
            Label {
                id: xName
                font.pointSize: values.fontSize
                text: "Values"
                color: "green"
            }

            Label {
                font.pointSize: values.fontSize
                text: gyro.xReg
            }

            Label {
                font.pointSize: values.fontSize
                text: gyro.yReg
            }

            Label {
                font.pointSize: values.fontSize
                text: gyro.zReg
            }
        }

        Column {
            spacing: values.spacing
            Label {
                id: tempName
                font.pointSize: values.fontSize
                text: "Temperature"
                color: "green"
            }
            Label {
                id: temp
                font.pointSize: values.fontSize
                text: gyro.tempReg
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
