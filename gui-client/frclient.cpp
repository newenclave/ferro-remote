#include "frclient.h"

namespace fr { namespace declarative {

    struct FrClient::impl {

        bool connected_;

        impl( )
            :connected_(false)
        { }
    };

    FrClient::FrClient( QObject *parent )
        :QObject(parent)
        ,impl_(new impl)
    { }

    FrClient::~FrClient( )
    {
        delete impl_;
    }

    bool FrClient::connected( ) const
    {
        return impl_->connected_;
    }

    void FrClient::connect( const QString &server )
    {

    }

    void FrClient::disconnect( )
    {

    }

}}
