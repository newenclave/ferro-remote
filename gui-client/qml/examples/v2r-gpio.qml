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

            FrClientGpio {
                id: mainGpio
                index: 7
                direction: FrClientGpio.Direct_Out
                edge: FrClientGpio.Edge_Both
                client: generalClient
                activeLow: true
                events: true
            }

            Text {
                id: available
                text: "Unknown"
                Connections {
                    target: generalClient
                    onReadyChanged: {
                        if( value ) {
                            available.text = mainGpio.supported( generalClient )
                                           ? "Supported"
                                           : "Not Supported"
                        } else {
                            available.text = "Unknown"
                        }
                    }
                }
            }

            MyButton {
                id: gpioBtn
                property int val: 0
                onClicked: {
                    val = (val + 1) % 2
                    mainGpio.value = val
                }
            }

            Rectangle {
                height: 40
                width: 40
                color: "black"
                id: gpioColor
                Connections {
                    target: mainGpio
                    onChangeEvent: {
                        gpioColor.color = value ? "red" : "grey"
                    }
                }
            }

        } // Row
    }
}
