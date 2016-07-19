import QtQuick 2.0

import QtQuick.Controls 1.1
import QtQuick.Dialogs 1.0

import Fr.Client 1.0

Rectangle {

    id: mainWindow
    width:  480
    height: 200

    FrClient {
        id: generalClient
    }

    Rectangle {
        id: lcdDevice
        width: 0
        height: 0

        property bool ready: txtDev.ready && rgbDev.ready

        FrClientI2c {
            id: txtDev
            client: generalClient
            busId: 1
            slaveAddress: 0x3E
        }

        FrClientI2c {
            id: rgbDev
            client: generalClient
            busId: 1
            slaveAddress: 0x62
        }

        Connections {
            target: colorDialog
            function fix( val )
            {
                return Math.floor(val === 1.0 ? 255 : val * 256.0)
            }
            onAccepted: {
                var r = fix(colorDialog.currentColor.r)
                var g = fix(colorDialog.currentColor.g)
                var b = fix(colorDialog.currentColor.b)

                lcdDevice.set_color( r, g, b )
            }
            onCurrentColorChanged: {
                var r = fix(colorDialog.currentColor.r)
                var g = fix(colorDialog.currentColor.g)
                var b = fix(colorDialog.currentColor.b)

                lcdDevice.set_color( r, g, b )
            }
        }

        onReadyChanged: {
            if( ready ) {
                clear( )
                set_color( 0, 0, 0 )
                //set_text( "Hola,", "raspberrypi.ru" )
            }
        }

        Connections {
            target: lcdText
            onTextChanged: {
                lcdDevice.set_text( lcdText.text, lcdText2.text )
            }
        }
        Connections {
            target: lcdText2
            onTextChanged: {
                lcdDevice.set_text( lcdText.text, lcdText2.text )
            }
        }

        function set_color( r, g, b )
        {
            rgbDev.writeBytes( { 0x00: 0, 0x01: 0, 0x08: 0xAA,
                                 0x04: r, 0x03: g, 0x02: b } )
        }

        function clear( )
        {
            txtDev.writeBytes( [{ 0x80: 0x01 }] )
        }

        function set_text( txt, txt2 )
        {
            //txtDev.writeBytes( [{ 0x80: 0x01 },
            //                    { 0x80: 0x08 | 0x4 },
            //                    { 0x80: 0x28 }] )

            txtDev.writeBytes( { 0x80: 0x01 } )
            txtDev.writeBytes( { 0x80: 0x08 | 0x4 } )
            txtDev.writeBytes( { 0x80: 0x28 } )

            var txt_value = []

            for( var i = 0; i < txt.length; i++ ) {
                txt_value = txt_value.concat( [{ 0x40: txt.charCodeAt(i) }] )
            }
            txt_value = txt_value.concat({ 0x80: 0xC0 })
            for( i = 0; i < txt2.length; i++ ) {
                txt_value = txt_value.concat( [{ 0x40: txt2.charCodeAt(i) }] )
            }
            txtDev.writeBytes(txt_value)
        }
    }

    ColorDialog {
        id: colorDialog
        title: "Please choose a color"
        onRejected: {
            console.log("Canceled")
        }
        Component.onCompleted: visible = false
    }

    Column {

        spacing: 10
        anchors.margins: 10
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.fill: parent

        Row {

            spacing: 10
            anchors.margins: 10

            TextField {
                width: 200
                height: 22
                id: address
                text: "192.168.3.1:12345"
            }

            Button {

                text: qsTr("Connect")
                id: connectBtn
                property bool connected: false

                onClicked: {
                    connected
                        ? generalClient.disconnect(  )
                        : generalClient.connect( address.text )
                }

                Connections {
                    target: generalClient
                    onReadyChanged: {
                        connectBtn.connected = value
                        if( value ) {
                            connectBtn.text = qsTr("Disconnect")
                        } else {
                            connectBtn.text = qsTr("Connect")
                        }
                    }
                }
            }

            Text {
                id: status
                text: qsTr("wait")
                color: "black"
                Connections {
                   target: generalClient
                   onChannelReady: {
                       status.text = qsTr("ready")
                       status.color = "green"
                   }
                   onConnected: {
                       status.text = qsTr("connected")
                       status.color = "yellow"
                   }
                   onDisconnected: {
                       status.text = qsTr("disconnected")
                       status.color = "black"
                   }
                   onInitError: {
                       status.text = qsTr("init error: ") + message
                       status.color = "red"
                   }
                }
            }
        }

        Row {
            spacing: 10
            anchors.margins: 10
            Rectangle {
                id: lcdColor
                height: 22
                width: 22
                color: "gray"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        colorDialog.open( )
                    }
                }
                Connections {
                    target: generalClient
                    onConnected: {
                        lcdColor.color = colorDialog.color
                    }
                    onDisconnected: {
                        lcdColor.color = "gray"
                    }
                }
                Connections {
                    target: colorDialog
                    onColorChanged: {
                        lcdColor.color = colorDialog.color
                    }
                }
            }
        }

        Row {
            spacing: 10
            anchors.margins: 10
            TextField {
                id: lcdText
                width: 200
                height: 22
                text: ""
                enabled: false
                maximumLength: 16
                Connections {
                    target: generalClient
                    onDisconnected: {
                        lcdText.enabled = false
                    }
                    onConnected: {
                        lcdText.enabled = true
                    }
                }
            }
        }

        Row {
            spacing: 10
            anchors.margins: 10
            TextField {
                id: lcdText2
                width: 200
                height: 22
                text: ""
                enabled: false
                maximumLength: 16
                Connections {
                    target: generalClient
                    onDisconnected: {
                        lcdText2.enabled = false
                    }
                    onConnected: {
                        lcdText2.enabled = true
                    }
                }
            }
        }
    }
}
