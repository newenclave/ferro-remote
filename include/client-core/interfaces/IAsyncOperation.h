#ifndef IASYNCOPERATION_H
#define IASYNCOPERATION_H

#include <string>
#include "vtrc-function.h"
#include "vtrc-stdint.h"

namespace fr { namespace client {

    ///
    /// void call( error_code, data )
    ///
    typedef vtrc::function<
        void ( unsigned, const std::string & )
    > async_op_callback_type;

}}

#endif // IASYNCOPERATION_H
