#include <QSharedPointer>
#include <iostream>

#include "fr-client-spi.h"

#include "client-core/interfaces/ISPI.h"
#include "fr-qml-call-wrappers.h"


namespace fr { namespace declarative {

    namespace {
        namespace spi_ns = fr::client::interfaces::spi;
        typedef spi_ns::iface spi_iface;
    }

    struct FrClientSPI::impl {

        QSharedPointer<spi_iface> iface_;
        FrClientSPI *parent_;
        impl( )
            :parent_(nullptr)
        { }

    };

    FrClientSPI::FrClientSPI( )
        :impl_(new impl)
    {
        impl_->parent_ = this;
    }

    FrClientSPI::~FrClientSPI( )
    {
        delete impl_;
    }

    void FrClientSPI::on_reinit( )
    {

    }

    void FrClientSPI::on_ready( bool value )
    {

    }

    bool FrClientSPI::clientFailed( ) const
    {

    }

}}

