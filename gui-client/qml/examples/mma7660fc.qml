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
    FrClientI2c {
        client: generalClient
        id: smbus
        busId: 1
        slaveAddress: 0x4c

        function getEnabled( ) {
            var data = smbus.readBytes([0x7])
            return data[7]
        }

    }



    Column {
        spacing: 5
        anchors.margins: 5
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        Rectangle {
            height: 40
            width: parent.width
            Row {
                spacing: 10
                anchors.margins: 10
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right

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
        }

        Rectangle {
            height: 40
            width: parent.width
            //color: "green"
            border.color: "white"

            Row {
                Button {
                    text: checked ? "Enabled" : "Disabled"
                    id: readyButton
                    checkable: true
                    checked: false
                    onClicked: {
                        smbus.writeBytes( {0x7: checked ? 1 : 0 } )
                        console.log( smbus.getEnabled( ) )
                    }
                    Connections {
                        target: smbus
                        onFailedChanged: {
                            console.log( "failed ", value )
                        }
                    }
                }
            }
        }
    }
}

