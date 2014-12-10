## Classes

#### Common properties

```JS

Q_PROPERTY( bool failed READ failed WRITE setFailed NOTIFY failedChanged )
Q_PROPERTY( QString error READ error )

```


### FrClient

#### properties:

```JS
Q_PROPERTY( bool ready READ ready NOTIFY readyChanged )
Q_PROPERTY( QString sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged )
Q_PROPERTY( QString sessionKey READ sessionKey WRITE setSessionKey NOTIFY sessionKeyChanged )
```

#### signals:

```cpp
    void connected( );
    void disconnected( );
```

#### public slots:

```cpp
    void connect( const QString &server ); /// x.x.x.x:xxxx
    void disconnect( );
```

### FrClientOS

#### public:

```cpp
    Q_INVOKABLE int execute( const QString &cmd ) const; // command for system("")
```

### FrClientFs

For directory access

#### properties:
```JS
    Q_PROPERTY( QString path READ path WRITE setPath NOTIFY pathChanged)
```

### FrClientFile
### FrClientGpio


