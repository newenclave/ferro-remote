import QtQuick 2.0

import Fr.Client 1.0

Rectangle {

    id: mainWindow
    width: 480
    height: 640

    FrClient {
        id: generalClient
    }

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
                    text: "127.0.0.1:12345"
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

        Row {

            spacing: 10

            Rectangle {
                height: 22
                width: 200
                color: "lightgray"
                TextInput {
                    anchors.fill: parent
                    id: dirPath
                    text: "/dev"
                }
            }

            FrClientFs {
                id: dirInst
                path: ""
                client: generalClient
            }

            MyButton {
                id: dirRefresh

                height: 22
                width: 80

                text: qsTr("Refresh")

                enabled: false
            }
        }

        ListView {

            id: dirView
            model: dirModel
            delegate: dirDelegate
            height: mainWindow.height - dirView.y - 22
            width: mainWindow.width

            ListModel {
                id: dirModel
                function refresh( path ) {
                    var dirs  = [ ]
                    var files = [ ]
                    dirInst.failed = false // drop fail-state for instance
                    var i = dirInst.begin( path )
                    while( !i.end ) {
                        if( i.info.directory  ) {
                            dirs.push( { "name": i.name, "is_dir": true } )
                        } else {
                            files.push( { "name": i.name, "is_dir": false } )
                        }
                        i.next( )
                    }
                    dirs.sort( )
                    files.sort( )
                    dirModel.clear( )
                    dirModel.append( dirs.concat( files ) )
                }
            }
            Connections {
                target: dirRefresh
                onClicked: {
                    dirModel.refresh( dirPath.text )
                }
            }
            Component {
                id: dirDelegate
                Rectangle {
                    height: 22
                    border.color: "black"
                    border.width: 1
                    Text {
                        text: name
                    }
                }
            }
        }
    }
}
