import QtQuick 2.0

import Fr.Client 1.0

Rectangle {

    id: mainWindow
    width: 480
    height: 140

    //onWidthChanged: { console.log( width + 'x' + height ) }

    FrClient { id: generalClient }

    Column {

        spacing: 10
        anchors.margins: 10
        anchors.fill: parent

        Row {

            spacing: 10
            anchors.margins: 10

            Rectangle {

                width: 200
                height: 22

                border.color: "black"
                border.width: 1

                TextInput {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    id: address
                    anchors.leftMargin: 5
                    text: "192.168.3.1:12345"
                    //text: "127.0.0.1:12345"
                }
            }

            MyButton {

                id: connectRect
                height: 22
                width: 80
                text: "Connect"

                property bool connected: false
                onClicked: {
                    if( !connectRect.connected ) {
                        generalClient.connect( address.text )
                    } else {
                        generalClient.disconnect( )
                    }
                }
                Connections {
                    target: generalClient
                    onReadyChanged: {
                        connectRect.connected = value
                        if( value ) {
                            connectRect.text = qsTr("Disconnect")
                        } else {
                            connectRect.text = qsTr("Connect")
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

        Rectangle {
            height: 25
            width: mainWindow.width - 10
            Text {
                id: errorText
                text: qsTr("")
                color: "red"
            }
        }

        Row {
            spacing: 10

            FrClientI2c {
                id: mainI2C
                client: generalClient
                busId: 1
                slaveAddress: 0x4c
            }

            MyButton {
                id: gpioBtn
            }

            Text {
                id: i2ctext
            }

            Rectangle {
                height: 40
                width: 40
                color: "black"
                id: gpioColor
                property int val: 0
                Connections {
                    target: gpioBtn
                    onClicked: {
                        var data = mainI2C.read(7)

                        console.log( "value: ", data )
                        for (var i=0; i<data.length; i++) {
                            console.log(data[i])
                        }
                        //i2ctext.text = data
                    }
                }
            }

        } // Row
    }
}
