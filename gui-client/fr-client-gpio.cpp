#include "fr-client-gpio.h"

#include "client-core/interfaces/IGPIO.h"

#include <QSharedPointer>

#include "fr-qml-call-wrappers.h"

#include <functional>

namespace fr { namespace declarative {


    namespace gpio_ns = fr::client::interfaces::gpio;
    typedef gpio_ns::iface gpio_iface;
    typedef QSharedPointer<gpio_iface> gpio_qsptr;

    const unsigned invalid_gpio = FrClientGpio::IndexInvalid;



    struct FrClientGpio::impl {
        gpio_qsptr iface_;
        FrClientGpio::DirectionType dir_;
        FrClientGpio::EdgeType      edge_;
        unsigned                    id_;
        bool                        events_;

        gpio_ns::value_change_interval_callback event_cb_;

        FrClientGpio                *parent_;

        impl( )
            :dir_(FrClientGpio::Direct_Out)
            ,edge_(FrClientGpio::Edge_None)
            ,id_(invalid_gpio)
            ,events_(false)
        {
            event_cb_ = std::bind( &impl::event_handler, this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    std::placeholders::_3 );
        }

        void reset_iface( FrClient *client )
        {
            iface_.reset( ( dir_ == Direct_In )
                    ? gpio_ns::create_input( client->core_client( ), id_ )
                    : gpio_ns::create_output( client->core_client( ), id_ )
                    );


            if( iface_->edge_supported( ) ) {
                iface_->set_edge( gpio_ns::edge_val2enum( edge_ ) );
                if( events_ ) {
                    register_events( events_ );
                }
            }
        }

        void register_events( bool value )
        {
            if( iface_.data( ) != nullptr ) {
                if( value ) {
                    iface_->register_for_change_int( event_cb_ );
                } else {
                    iface_->unregister( );
                }
            }
        }

        void event_handler( unsigned err, unsigned val, quint64 last_inter )
        {
            Q_UNUSED( err )
            emit parent_->changeEvent( val, last_inter );
        }

    };

    FrClientGpio::FrClientGpio(QObject *parent)
        :FrClientComponent(parent)
        ,impl_(new impl)
    {
        impl_->parent_ = this;
    }

    FrClientGpio::~FrClientGpio( )
    {
        delete impl_;
    }

    void FrClientGpio::on_reinit( )
    {
        if( client( ) && client( )->ready( ) ) {
            FR_QML_CALL_PROLOGUE0
            impl_->reset_iface( client( ) );
            FR_QML_CALL_EPILOGUE( )
        }
    }

    void FrClientGpio::on_ready( bool value )
    {
        FR_QML_CALL_PROLOGUE0
        if( value && impl_->id_ != invalid_gpio ) {
            setFailed( false );
            impl_->reset_iface( client( ) );
        } else {
            impl_->iface_.reset( );
        }
        FR_QML_CALL_EPILOGUE( )
    }

    bool FrClientGpio::clientFailed( ) const
    {
        if( impl_->iface_.data( ) == nullptr ) {
            setError("Channel is empty.");
            setFailed( true );
            return true;
        }
        return false;
    }


    FrClientGpio::DirectionType FrClientGpio::direction( ) const
    {
        return impl_->dir_;
    }

    void FrClientGpio::setDirection( DirectionType value ) const
    {
        if( value != impl_->dir_ ) {
            FR_QML_CALL_PROLOGUE

            impl_->iface_->set_direction( gpio_ns::direction_val2enum( value ));
            impl_->dir_ = value;
            emit directionChanged( impl_->dir_ );

            FR_QML_CALL_EPILOGUE( )
        }
    }

    FrClientGpio::EdgeType FrClientGpio::edge( ) const
    {
        return impl_->edge_;
    }

    void FrClientGpio::setEdge( EdgeType value ) const
    {
        if( value != impl_->edge_ ) {
            impl_->edge_ = value;

            FR_QML_CALL_PROLOGUE
            if( edgeSupport( ) ) {
                impl_->iface_->set_edge( gpio_ns::edge_val2enum( value ) );
            }
            emit edgeChanged( value );
            FR_QML_CALL_EPILOGUE( )
        }
    }


    quint32 FrClientGpio::index( ) const
    {
        return impl_->id_;
    }

    void FrClientGpio::setIndex( quint32 value )
    {
        if( value != impl_->id_ ) {
            impl_->id_ = value;

            FR_QML_CALL_PROLOGUE

            impl_->reset_iface( client( ) );
            emit indexChanged( value );

            FR_QML_CALL_EPILOGUE( )

        }
    }

    quint32 FrClientGpio::value( ) const
    {
        FR_QML_CALL_PROLOGUE

        return impl_->iface_->value( );

        FR_QML_CALL_EPILOGUE( 0 )
    }

    void FrClientGpio::setValue( quint32 value )
    {
        FR_QML_CALL_PROLOGUE

        impl_->iface_->set_value( value );

        FR_QML_CALL_EPILOGUE( )
    }

    bool FrClientGpio::edgeSupport( ) const
    {
        FR_QML_CALL_PROLOGUE

        return impl_->iface_->edge_supported( );

        FR_QML_CALL_EPILOGUE( false )
    }


    bool FrClientGpio::events( ) const
    {
        return impl_->events_;
    }

    void FrClientGpio::setEvents( bool value )
    {
        if( value != impl_->events_ ) {
            impl_->events_ = value;
            FR_QML_CALL_PROLOGUE
            impl_->register_events( value );
            emit eventsChanged( value );
            FR_QML_CALL_EPILOGUE(  )
        }
    }

}}
