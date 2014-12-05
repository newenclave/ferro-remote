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
            MyButton {
                id: button
                FrClientFile {
                    id: v2rFile
                    //device: true
                    client: generalClient
                    mode: "wb"
                    path: "/dev/v2r_gpio"
                }
                text: "!!!"
                width: 40
                height: 40
                radius: 40
                property int gpioId: 74
                property bool pressed: false
                property string yesCommand: "set gpio " + gpioId + " output 1"
                property string noCommand: "set gpio " + gpioId + " output 0"
                enabled: generalClient.ready
                Connections {
                    target: generalClient
                    onReadyChanged: {
                        if( value ) {
                            v2rFile.open( )
                        }
                    }
                }
                onClicked: {
                    pressed = !pressed
                    if( pressed ) {
                        v2rFile.write( yesCommand )
                        console.log( "On!" )
                    } else {
                        v2rFile.write( noCommand )
                        console.log( "Off!" )
                    }
                    if( v2rFile.failed ) {
                        console.log( v2rFile.error )
                    }
                }
            } // MyButton
            Rectangle {
                height: 40
                width: 40
                radius: 40
                border.color: "gray"
                border.width: 1
                color: button.pressed ? "red" : "gray"
            }
        } // Row
    }
}
