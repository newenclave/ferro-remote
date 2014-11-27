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
                width: 200
            }

            Button {
                text: qsTr("Connect")
                onClicked: {
                    generalClient.connect( address.text )
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
                    onChannelReady: { enabled = true }
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
