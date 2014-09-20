#ifndef FR_COMMAND_IFACE_H
#define FR_COMMAND_IFACE_H

#include "vtrc-memory.h"

namespace boost { namespace program_options {

    class variables_map;
    class options_description;

}}

namespace fr { namespace cc {

    struct command_iface {

        virtual ~command_iface( ) { }

        virtual const char *name( ) const = 0;

        virtual void exec( boost::program_options::variables_map &vm ) = 0;

        virtual void add_options(
                boost::program_options::options_description &desc ) = 0;

        virtual std::string help( ) const = 0;
        virtual std::string desc( ) const = 0;

    };

    typedef vtrc::shared_ptr<command_iface> command_sptr;

}}

#endif // FR_COMMAND_IFACE_H
