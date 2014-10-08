#ifndef CMDLIST_H
#define CMDLIST_H

#include "command-iface.h"

#include "ferro-remote-config.h"

namespace fr { namespace cc { namespace cmd {

    namespace fs   { command_sptr create( ); }
    namespace gpio { command_sptr create( ); }
    namespace os   { command_sptr create( ); }
#if FR_WITH_LUA
    namespace lua  { command_sptr create( ); }
#endif

}}}

#endif // CMDLIST_H
