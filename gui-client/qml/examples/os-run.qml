import QtQuick 2.0

import QtQuick.Controls 1.1

import Fr.Client 1.0

Rectangle {

    id: mainWindow
    width: 480
    height: 120

    FrClient {
        id: generalClient
    }

    Column {
        width: mainWindow.width
        Rectangle {
            width: mainWindow.width
            height: 64
        Row {

            spacing: 5
            anchors.margins: 10
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right

            TextField {
                width: 200
                height: 22
                id: address
                text: "127.0.0.1:12345"
                //text: "192.168.3.1:12345"
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
        } // ROW
        } // Rectangle

        Rectangle {

            width: mainWindow.width
            height: 64

            FrClientOS {
                id: runner
                client: generalClient
            }

            Row {

                spacing: 5
                anchors.margins: 10
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

                TextField {
                    width: 200
                    height: 22
                    id: command
                    text: "echo \"Hello, world!\" && uname -a"
                    enabled: runner.ready
                }

                Button {
                    text: "Run"
                    enabled: runner.ready
                    onClicked: {
                        console.log( "Sending command: \"" + command.text +
                                     "\" to the server."  )
                        runner.execute( command.text )
                    }
                }
            }
        }

    } // COLUMN

}
