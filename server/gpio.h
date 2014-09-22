#ifndef FR_GPIO_H
#define FR_GPIO_H

namespace fr { namespace server {

    class gpio_inst {
        struct  impl;
        impl   *impl_;
    public:
        gpio_inst( );
        ~gpio_inst(  );
    };

}}

#endif // GPIO_H
