#include "logger.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <queue>
#include <mutex>

#include "boost/date_time/posix_time/posix_time.hpp"
#include "boost/asio/strand.hpp"
#include "boost/algorithm/string.hpp"

#include "application.h"

namespace fr { namespace agent {

    namespace bpt = boost::posix_time;
    namespace ba  = boost::asio;

    struct queue_element_info {
        log_record_info info;
        std::string data;
        bool split;
    };

    typedef std::unique_ptr<queue_element_info> queue_element_ptr;
    typedef std::queue<queue_element_ptr> queue_type;

    struct logger::impl {
        ba::io_service::strand   dispatcher_;
        const char              *split_string_;
        queue_type               queue_;
        std::mutex               queue_lock_;

        impl(ba::io_service &ios, const char *split_string)
            :dispatcher_(ios)
            ,split_string_(split_string)
        { }
    };

    logger::logger( boost::asio::io_service &ios, level lvl,
                    const char *split_string )
        :common::logger( lvl )
        ,impl_(new impl(ios, split_string))
    { }

    logger::~logger( )
    {
        //while( impl_->dispatcher_.get_io_service( ).poll_one( ) );
        delete impl_;
    }

    boost::asio::io_service &logger::get_io_service()
    {
        return impl_->dispatcher_.get_io_service( );
    }

    void logger::dispatch( std::function<void ( )> call )
    {
        impl_->dispatcher_.post( call );
    }

    bool logger::empty( ) const
    {
        std::lock_guard<std::mutex> l(impl_->queue_lock_);
        return impl_->queue_.empty( );
    }

    size_t logger::drop_all( )
    {
        size_t res = 0;
        while( !empty( ) ) {
            do_write( );
            ++res;
        }
        return res;
    }

    void fill_record_info( log_record_info &info,
                           logger::level lvl, const std::string &name )
    {
        info.level   = static_cast<int>(lvl);
        info.name    = name;
        info.when    = bpt::microsec_clock::local_time( );
        info.tid     = std::this_thread::get_id( );
        info.tprefix = application::thread_prefix( );
    }

    void logger::send_data( level lev, const std::string &name,
                                       const std::string &data )
    {
        //static const bpt::ptime epoch( bpt::ptime::date_type(1970, 1, 1) );
        emplace_element( lev, name, data, true );
        impl_->dispatcher_.post( std::bind( &logger::do_write, this ) );
    }

    void logger::send_data_nosplit( level lev, const std::string &name,
                                    const std::string &data )
    {
        emplace_element( lev, name, data, false );
        impl_->dispatcher_.post( std::bind( &logger::do_write, this ) );
    }

    void logger::emplace_element( logger::level lev,
                                  const std::string &name,
                                  const std::string &data,
                                  bool split )
    {
#if __cplusplus > 201103L
        auto qi = std::make_unique<queue_element_info>( );
#else
        queue_element_ptr qi(new queue_element_info);
#endif
        fill_record_info( qi->info, lev, name );
        qi->split = split;
        qi->data  = data;
        {
            std::lock_guard<std::mutex> l(impl_->queue_lock_);
            impl_->queue_.emplace( std::move(qi) );
        }
    }

    void logger::do_write( ) NOEXCEPT
    {

        queue_element_ptr ptr;
        {
            std::lock_guard<std::mutex> l(impl_->queue_lock_);
            ptr.reset(impl_->queue_.front( ).release( ));
            impl_->queue_.pop( );
        }

        logger_data_type all;
        if( ptr->split ) {
            boost::split( all, ptr->data,
                          boost::is_any_of(impl_->split_string_) );
        } else {
            all.push_back( ptr->data );
        }
        on_write_( ptr->info, all );
    }

}}
