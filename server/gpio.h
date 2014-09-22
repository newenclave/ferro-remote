#ifndef FR_GPIO_H
#define FR_GPIO_H

#include <string>

namespace fr { namespace server {

    class gpio_inst {
        struct  impl;
        impl   *impl_;
    public:
        gpio_inst( unsigned id );
        ~gpio_inst(  );

        bool exists( ) const;
        unsigned id( ) const;

        void exp( ) const;
        void unexp( ) const;

    };

}}

#endif // GPIO_H
