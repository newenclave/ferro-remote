import QtQuick 2.0
import QtQuick.Controls 1.1

import Fr.Client 1.0


Rectangle {

    id: mainWindow
    width: 480
    height: 200

    FrClient {
        id: generalClient
    }

    function call( )
    {
        var t = frComponents.newOs( generalClient )
        t.execute( command.text )
        return t
    }

    function makeListModel( path )
    {
        var dirs  = [ ]
        var files = [ ]
        dirPath.failed = false
        var i = dirPath.begin( path )
        while( !i.end ) {

            //res.push( i.name )

            if( i.info.directory  ) {
                dirs.push( { "name": i.name, "is_dir": true } )
            } else {
                files.push( { "name": i.name, "is_dir": false } )
            }

            i.next( )
        }
        dirs.sort( )
        files.sort( )
        return dirs.concat( files )
    }

    Column {
        anchors.margins: 10
        spacing: 10
        anchors.fill: parent

        FrClientFs {
            id: dirPath
            path: "/home/data"
            client: generalClient
        }

        FrClientOS {
            id: osIface
            client: generalClient
        }

        Row {
            spacing: 10

            TextField {
                id: address
                text: "192.168.3.1:12345"
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

            FrClientI2c {
                id: i2c
                busId: 1
                slaveAddress: 0x4C
                client: generalClient
            }

            Button {
                enabled: false
                id: run
                text: qsTr("Run")

                onClicked: {
                    if( i2c.failed ) {
                        throw i2c.error
                    }
                    //i2c.write( [ 0x7, 1 ] )
                    var data = i2c.read( 7 );
                    console.log( data )
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

        Text {
            id: file
        }

        Text {
            id: dataFile
        }

        Component {
            id: nameDelegate
            Text {
                text: setText( )
                font.pixelSize: 20
                function setText(  ) {
                    if( is_dir ) {
                        return "[" + name + "]"
                    } else {
                        return name
                    }
                }
            }
        }

    }
}
