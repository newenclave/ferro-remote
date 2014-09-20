#ifndef CMDLIST_H
#define CMDLIST_H

#include "command-iface.h"

namespace fr { namespace cc { namespace cmd {

    namespace fs { command_sptr create( ); }
    namespace gpio { command_sptr create( ); }

}}}

#endif // CMDLIST_H
