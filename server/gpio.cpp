#include "gpio.h"

#include "boost/filesystem.hpp"

namespace fr { namespace server {

    namespace {
        const std::string gpio_sysfs_path( "/sys/class/gpio" );
    }

    struct gpio_inst::impl {
        unsigned id_;
        impl( unsigned id )
            :id_(id)
        { }
    };

    gpio_inst::gpio_inst( unsigned id )
        :impl_(new impl(id))
    {

    }

    gpio_inst::~gpio_inst( )
    {
        delete impl_;
    }

}}

