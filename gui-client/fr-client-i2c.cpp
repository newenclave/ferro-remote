#include <QSharedPointer>
#include <iostream>

#include "fr-client-i2c.h"

#include "client-core/interfaces/II2C.h"
#include "fr-qml-call-wrappers.h"


namespace fr { namespace declarative {

    namespace {

        namespace i2c_ns = fr::client::interfaces::i2c;
        typedef i2c_ns::iface i2c_iface;

        template <typename K, typename V>
        std::pair<K, V> make_value_pair( int k, int v )
        {
            return std::make_pair( static_cast<K>(k), static_cast<V>(v) );
        }

        template <typename Cont>
        void push_values( Cont &cont, const QVariantMap &from )
        {
            typedef typename Cont::value_type::first_type key_type;
            typedef typename Cont::value_type::second_type value_type;
            typedef QVariantMap::const_iterator mitr;

            for( mitr b(from.begin( )), e(from.end( )); b!=e; ++b ) {
                cont.push_back( make_value_pair<key_type, value_type>(
                                                    b.key( ).toInt( ),
                                                    b.value( ).toInt( ) ));
            }
        }

    }

    struct FrClientI2c::impl {

        QSharedPointer<i2c_iface> iface_;
        unsigned        bus_id_;
        quint32         slave_addr_;
        quint64         function_mask_;
        FrClientI2c    *parent_;
        impl( )
            :bus_id_(FrClientI2c::InvalidBusId)
            ,slave_addr_(FrClientI2c::InvalidSlaveAddress)
            ,function_mask_(0)
            ,parent_(nullptr)
        { }

        void reinit_iface( FrClient *client )
        {
            if( !client ) {
                return;
            }

            if( bus_id_ != FrClientI2c::InvalidBusId ) {
                iface_.reset( i2c_ns::open( client->core_client( ),
                                            bus_id_, slave_addr_ ) );
                quint64 old_mask = function_mask_;
                try {
                    function_mask_ = iface_->function_mask( );
                } catch( ... ) { /// don't care for
                    function_mask_ = 0;
                }
                if( old_mask != function_mask_ ) {
                    emit parent_->functionsSupportedChanged(
                                  static_cast<quint32>(function_mask_) );
                }
            }
        }

    };

    FrClientI2c::FrClientI2c( QObject *parent )
        :FrClientComponent(parent)
        ,impl_(new impl)
    {
        impl_->parent_ = this;
    }

    FrClientI2c::~FrClientI2c( )
    {
        delete impl_;
    }

    void FrClientI2c::on_reinit( )
    {
        if( client( ) && client( )->ready( ) ) {
            FR_QML_CALL_PROLOGUE0
            setFailed( false );
            impl_->reinit_iface( client( ) );
            FR_QML_CALL_EPILOGUE( )
        }
    }

    void FrClientI2c::on_ready( bool value )
    {
        if( value ) {
            FR_QML_CALL_PROLOGUE0
            setFailed( false );
            impl_->reinit_iface( client( ) );
            FR_QML_CALL_EPILOGUE( )
        } else {
            impl_->iface_.reset( );
        }
    }

    bool FrClientI2c::clientFailed( ) const
    {
        if( impl_->iface_.data( ) == nullptr ) {
            setError("Channel is empty.");
            setFailed( true );
            return true;
        }
        return false;
    }

    quint32 FrClientI2c::busId( ) const
    {
        return impl_->bus_id_;
    }

    void FrClientI2c::setBusId( quint32 value )
    {
        if( impl_->bus_id_ != value ) {
            if( value == InvalidBusId ) {
                impl_->iface_.reset( );
            } else {
                impl_->bus_id_ = value;
                FR_QML_CALL_PROLOGUE
                impl_->reinit_iface( client( ) );
                emit busIdChanged( impl_->bus_id_ );
                FR_QML_CALL_EPILOGUE( )
            }
        }
    }

    quint32 FrClientI2c::slaveAddress( ) const
    {
        return impl_->slave_addr_;
    }

    void FrClientI2c::setSlaveAddress( quint32 value )
    {
        if( value != impl_->slave_addr_ ) {
            impl_->slave_addr_ = value;
            FR_QML_CALL_PROLOGUE
            impl_->iface_->set_address( value );
            emit slaveAddressChanged( impl_->slave_addr_ );
            FR_QML_CALL_EPILOGUE( )
        }
    }

    quint32 FrClientI2c::functionsSupported( ) const
    {
        return static_cast<quint32>(impl_->function_mask_);
    }

    void FrClientI2c::iocontrol( unsigned code, quint64 data )
    {
        FR_QML_CALL_PROLOGUE
        impl_->iface_->ioctl( code, data );
        FR_QML_CALL_EPILOGUE( )
    }

    FrClientI2c::ArrayType FrClientI2c::read( unsigned maximum ) const
    {
        FR_QML_CALL_PROLOGUE
        if( maximum ) {

            maximum = maximum > 1024 ? 1024 : maximum;

            std::vector<unsigned char> data( maximum );

            size_t r = impl_->iface_->read( &data[0], data.size( ) );

            ArrayType res;
            for( size_t i=0; i<r; ++i ) {
                res.push_back( static_cast<int>(data[i]) );
            }
            return res;
        }
        FR_QML_CALL_EPILOGUE( ArrayType( ) )
    }

    unsigned FrClientI2c::write( const FrClientI2c::ArrayType &data ) const
    {
        FR_QML_CALL_PROLOGUE
        if( data.empty( ) ) {
            return 0;
        }
        std::vector<unsigned char> wdata;
        wdata.reserve( data.size( ) );
        for( ArrayType::const_iterator b(data.constBegin( )),
                                       e(data.constEnd( )); b!=e; ++b )
        {
            wdata.push_back( static_cast<unsigned char>(*b) );
        }
        size_t r = impl_->iface_->write( &wdata[0], wdata.size( ) );
        return static_cast<unsigned>(r);
        FR_QML_CALL_EPILOGUE( 0 )
    }

    QVariantMap FrClientI2c::readBytes(const FrClientI2c::ArrayType &data) const
    {

        typedef i2c_ns::cmd_uint8_vector value_container_type;
        typedef value_container_type::const_iterator citr;

        FR_QML_CALL_PROLOGUE

        QVariantMap res;

        i2c_ns::uint8_vector inp( data.begin( ), data.end( ) );
        value_container_type values( impl_->iface_->read_bytes( inp ) );

        for( citr b(values.begin( )), e(values.end( )); b!=e; ++b ) {
            res.insert( QString::number( b->first ), b->second );
        }
        return res;
        FR_QML_CALL_EPILOGUE( QVariantMap( ) )
    }

    /// FIXIT copypaste
    QVariantMap FrClientI2c::readWords(const FrClientI2c::ArrayType &data) const
    {

        typedef i2c_ns::cmd_uint16_vector value_container_type;
        typedef value_container_type::const_iterator citr;

        FR_QML_CALL_PROLOGUE

        QVariantMap res;

        i2c_ns::uint8_vector inp( data.begin( ), data.end( ) );
        value_container_type values( impl_->iface_->read_words( inp ) );


        for( citr b(values.begin( )), e(values.end( )); b!=e; ++b ) {
            res.insert( QString::number( b->first ), b->second );
        }
        return res;
        FR_QML_CALL_EPILOGUE( QVariantMap( ) )
    }

    ///
    /// writeBytes and writeWords
    /// qml example: [{k1: v1, k2: v2}, {k3: v3}, {k4: v5, k6: v6, k7: v7} ]
    ///

    void FrClientI2c::writeBytes( const QVariantList &data ) const
    {
        typedef QVariantList::const_iterator data_iter;
        typedef i2c_ns::cmd_uint8_vector vcont_type;

        vcont_type values;
        for( data_iter b(data.begin( )), e(data.end( )); b!=e; ++b ) {
            push_values( values, b->toMap( ) );
        }
        FR_QML_CALL_PROLOGUE
        impl_->iface_->write_bytes( values );
        FR_QML_CALL_EPILOGUE( )
    }

    void FrClientI2c::writeBytes( const QVariantMap &data ) const
    {
        typedef i2c_ns::cmd_uint8_vector vcont_type;

        vcont_type values;
        push_values( values, data );

        FR_QML_CALL_PROLOGUE
        impl_->iface_->write_bytes( values );
        FR_QML_CALL_EPILOGUE( )
    }


    /// FIXIT copypaste
    void FrClientI2c::writeWords( const QVariantList &data ) const
    {
        typedef QVariantList::const_iterator data_iter;
        typedef i2c_ns::cmd_uint16_vector vcont_type;

        vcont_type values;
        for( data_iter b(data.begin( )), e(data.end( )); b!=e; ++b ) {
            push_values( values, b->toMap( ) );
        }
        FR_QML_CALL_PROLOGUE
        impl_->iface_->write_words( values );
        FR_QML_CALL_EPILOGUE( )
    }

    void FrClientI2c::writeWords( const QVariantMap &data ) const
    {
        typedef i2c_ns::cmd_uint16_vector vcont_type;

        vcont_type values;
        push_values( values, data );

        FR_QML_CALL_PROLOGUE
        impl_->iface_->write_words( values );
        FR_QML_CALL_EPILOGUE( )
    }

}}















