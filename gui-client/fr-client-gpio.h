#ifndef FRCLIENTGPIO_H
#define FRCLIENTGPIO_H


#include "fr-client-component.h"

namespace fr { namespace declarative {

    class FrClientGpio: public FrClientComponent
    {
        Q_OBJECT

        Q_PROPERTY( DirectionType direction
                    READ direction WRITE setDirection
                    NOTIFY directionChanged)

        Q_PROPERTY( EdgeType edge
                    READ edge WRITE setEdge
                    NOTIFY edgeChanged)

        Q_PROPERTY( quint32 index
                    READ index WRITE setIndex
                    NOTIFY indexChanged)

        Q_PROPERTY( quint32 value
                    READ value WRITE setValue )

        Q_PROPERTY( bool edgeSupported READ edgeSupported )

        Q_PROPERTY( bool activeLow
                    READ activeLow WRITE setActiveLow
                    NOTIFY activeLowChanged )

        Q_PROPERTY( bool events READ events
                    WRITE setEvents NOTIFY eventsChanged )

        struct  impl;
        impl   *impl_;
    public:

        enum DirectionType {
            DirectIn   = 0
           ,DirectOut  = 1
        };
        Q_ENUMS( DirectionType )

        enum EdgeType {
             EdgeNone     = 0
            ,EdgeRising   = 1
            ,EdgeFalling  = 2
            ,EdgeBoth     = 3
        };
        Q_ENUMS( EdgeType )

        enum GpioIndex { IndexInvalid = 0xFFFFFFFF };
        Q_ENUMS( GpioIndex )


        explicit FrClientGpio( QObject *parent = nullptr );
        ~FrClientGpio( );

    private:

        void on_reinit( );
        void on_ready( bool value );

        bool clientFailed( ) const;

    public:

        Q_INVOKABLE static bool supported( FrClient *client );

        DirectionType direction( ) const;
        void setDirection( DirectionType value ) const;

        bool edgeSupported( ) const;
        EdgeType edge( ) const;
        void setEdge( EdgeType value ) const;

        quint32 index( ) const;
        void setIndex( quint32 value );

        quint32 value( ) const;
        void setValue( quint32 value );

        bool activeLow( ) const;
        void setActiveLow( bool value );

        bool events( ) const;
        void setEvents( bool value );

        void makePulse( quint32 value );

    signals:

        void directionChanged( DirectionType value ) const;
        void edgeChanged( EdgeType value ) const;
        void activeLowChanged( bool value ) const;

        void indexChanged( quint32 value ) const;

        void changeEvent( quint32 value, quint64 interval ) const;

        void eventsChanged( bool value ) const;

    };

}}

#endif // FRCLIENTGPIO_H
