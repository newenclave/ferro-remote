import QtQuick 2.0
import QtQuick.Controls 1.1

import fr.client 1.0

Rectangle {

    width: 480
    height: 640

    FrClient {
        id: generalClient
    }

    Column {
        anchors.margins: 10
        spacing: 10
        FrClientOS {
            id: osIface
            client: generalClient
        }
        Row {
            anchors.margins: 10
            spacing: 10
            TextField {
                id: address
                text: "127.0.0.1:12345"
                width: 200
            }

            Button {

                text: qsTr("Connect")
                id: connectButton
                property bool connected: false

                onClicked: {
                    if( !connected ) {
                        generalClient.connect( address.text )
                    } else {
                        generalClient.disconnect( )
                    }
                }

                Connections {
                    target: generalClient
                    onReadyChanged: {
                        connectButton.connected = value
                        if( value ) {
                            connectButton.text = qsTr("Disconnect")
                        } else {
                            connectButton.text = qsTr("Connect")
                        }
                    }
                }
            }
        }
        Row {
            anchors.margins: 10
            spacing: 10
            TextField {
                id: command
                width: 200
            }

            Button {
                enabled: false
                id: run
                text: qsTr("Run")
                onClicked: {
                    osIface.execute( command.text )
                }
                Connections {
                    target: generalClient
                    onReadyChanged: { run.enabled = value }
                }
            }
        }
        Text {
            id: status
            text: qsTr("wait")
            Connections {
               target: generalClient
               onChannelReady: {
                   status.text = qsTr("ready")
               }
               onConnected: {
                   status.text = qsTr("connected")
               }
               onDisconnected: {
                   status.text = qsTr("disconnected")
               }
               onInitError: {
                   status.text = qsTr("init error: ") + message
               }
            }
        }
    }
}
