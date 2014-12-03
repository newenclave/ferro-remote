import QtQuick 2.0

import Fr.Client 1.0

Rectangle {

    id: mainWindow
    width: 480
    height: 42

    FrClient {
        id: generalClient
    }

    Row {

        spacing: 10
        anchors.margins: 10
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

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
                text: "127.0.0.1:12345"
            }
        }

        Rectangle {

            id: connectRect

            height: 22
            width: 80

            border.color: "black"
            border.width: 1

            property bool connected: false

            Text {
                id: connectButton
                anchors.centerIn: parent
                text: qsTr("Connect")
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if( !connectRect.connected ) {
                        generalClient.connect( address.text )
                    } else {
                        generalClient.disconnect( )
                    }
                }
            }

            Connections {
                target: generalClient
                onReadyChanged: {
                    connectRect.connected = value
                    if( value ) {
                        connectButton.text = qsTr("Disconnect")
                    } else {
                        connectButton.text = qsTr("Connect")
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
}
