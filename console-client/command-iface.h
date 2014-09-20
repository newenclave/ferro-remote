#ifndef FR_COMMAND_IFACE_H
#define FR_COMMAND_IFACE_H

namespace boost { namespace program_options {

    class variables_map;
    class options_description;

}}

namespace fr { namespace console_client {

    struct command_iface {

        virtual ~command_iface( ) { }

        virtual const char *name( ) const = 0;

        virtual void exec( boost::program_options::variables_map &vm ) = 0;

        virtual void add_options(
                boost::program_options::options_description &desc ) = 0;

    };

}}

#endif // FR_COMMAND_IFACE_H
