#ifndef IASYNCOPERATION_H
#define IASYNCOPERATION_H

#include <string>
#include "vtrc-function.h"

namespace fr { namespace client {

    ///
    /// void call( error_code, data )
    ///
    typedef void (async_op_callback_sign)( unsigned,
                                           const std::string &,
                                           uint64_t );
    typedef vtrc::function<async_op_callback_sign> async_op_callback_type;

}}

#endif // IASYNCOPERATION_H
