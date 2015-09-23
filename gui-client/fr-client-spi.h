#ifndef FRCLIENTSPI_H
#define FRCLIENTSPI_H

#include "fr-client-component.h"

namespace fr { namespace declarative {

    class FrClientSPI: public FrClientComponent
    {
        Q_OBJECT

        struct  impl;
        impl   *impl_;

    private:

        void on_reinit( );
        void on_ready( bool value );

        bool clientFailed( ) const;
    public:
        FrClientSPI( );
        ~FrClientSPI( );

    };

}}

#endif // FRCLIENTSPI_H
