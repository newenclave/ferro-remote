#ifndef FRQMLCALLWRAPPERS_H
#define FRQMLCALLWRAPPERS_H

#include <sstream>
#include "vtrc-common/vtrc-exception.h"

#define FR_QML_CALL_PROLOGUE0                       \
    try {                                           \

#define FR_QML_CALL_PROLOGUE                        \
    if( prologueCall( ) ) try {                     \


#define FR_QML_CALL_EPILOGUE( DefResult )                       \
    } catch( const vtrc::common::exception &ex ) {              \
        std::ostringstream qts;                                 \
        qts << ex.what( );                                      \
        if( ex.additional( ) && *ex.additional( ) != '\0' ) {   \
            qts << ": " <<  ex.additional( );                   \
        }                                                       \
        setError( qts.str( ).c_str( ) );                        \
        setFailed( true );                                      \
    } catch( const std::exception &ex ) {                       \
        setError( ex.what( ) );                                 \
        setFailed( true );                                      \
    }                                                           \
    return DefResult;

#endif // FRQMLCALLWRAPPERS_H
