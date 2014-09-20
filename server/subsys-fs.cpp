
#include "subsys-fs.h"


namespace fr { namespace server { namespace subsys {

    namespace {

        const std::string subsys_name( "fs" );

    }

    struct fs::impl {

    };


    fs::fs( application *app )
        :impl_(new impl)
    { }

    fs::~fs( )
    {
        delete impl_;
    }

    /// static
    vtrc::shared_ptr<fs> fs::create( application *app )
    {
        vtrc::shared_ptr<fs> new_inst(new fs(app));
        return new_inst;
    }

    const std::string &fs::name( )  const
    {
        return subsys_name;
    }

    void fs::init( )
    {

    }

    void fs::start( )
    {

    }

    void fs::stop( )
    {

    }


}}}

    
