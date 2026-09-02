//
// Created by jens on 18/08/2021.
//

#include "pam_oauth2_log.hpp"
#include "pam_oauth2_excpt.hpp"
#include <syslog.h>
#include <cstring>
#include <security/pam_ext.h>



pam_oauth2_log::pam_oauth2_log(pam_handle *ph, log_level_t lev) noexcept : ph_(ph), lev_(lev), log_(nullptr)
{
    if(lev == log_level_t::DEBUG || !ph)
        // TODO needs more thought
        log_ = stderr;
}


pam_oauth2_log::~pam_oauth2_log()
{
    // log_ is stderr rather than a stream this class opened: closing it would
    // take fd 2 away from the whole process.
    if(log_ && log_ != stderr && log_ != stdout)
        fclose(log_);
    log_ = nullptr;
}



//constexpr
bool
pam_oauth2_log::log_this(log_level_t severity) const noexcept
{
    // The disadvantage of closed class enums? This would be easier in later standards
    switch(lev_)
    {
        case log_level_t::DEBUG:
            if(severity == log_level_t::DEBUG)
                return true;
        case log_level_t::INFO:
            if(severity == log_level_t::INFO)
                return true;
        case log_level_t::WARN:
            if(severity == log_level_t::WARN)
                return true;
        case log_level_t::ERR:
            if(severity == log_level_t::ERR)
                return true;
        case log_level_t::OFF:
            return false;
    }
    return false;
}


//constexpr
int
pam_oauth2_log::syslog_pri(log_level_t level) const noexcept
{
    // Facility for pam modules
    int pri = LOG_AUTHPRIV;
    switch(level)
    {
        case log_level_t::DEBUG:
            pri |= LOG_DEBUG;
            break;
        case log_level_t::INFO:
            pri |= LOG_INFO;
            break;
        case log_level_t::WARN:
            pri |= LOG_WARNING;
            break;
        case log_level_t::ERR:
            pri |= LOG_ERR;
            break;
        case log_level_t::OFF:
            break; //can't happen
    }
    return pri;
}



void
pam_oauth2_log::log(BaseError const &e) noexcept
{
    if(lev_ == log_level_t::OFF)
        return;
    // Simple log
    if(ph_) {
        pam_syslog(ph_, syslog_pri(e.severity_), "%s", e.what());
	if(!e.details().empty())
	    pam_syslog(ph_, syslog_pri(e.severity_), "** %s", e.details().c_str());
    }
    if(log_)
    {
        // short message
	fprintf(log_, "[%4s] %s\n", e.type(), e.what());
        if(!e.details().empty())
        {
            // todo? make this better formatted
            fprintf(log_, "%s\n", e.details().c_str());
        }
    }
}


void
pam_oauth2_log::log(log_level_t level, const char *fmt, ...) noexcept
{
    // Exceptions are still logged unconditionally, as the header says.
    if(!log_this(level))
        return;
    va_list ap1, ap2;
    va_start(ap1, fmt);
    va_copy(ap2, ap1);		// dest, src
    if(ph_)
        pam_vsyslog(ph_, syslog_pri(level), fmt, ap1);
    if(log_) {
        vfprintf(log_, fmt, ap2);
        // syslog does not need a terminator and most callers do not supply
        // one, which runs consecutive messages together on stderr.
        size_t const len = fmt ? strlen(fmt) : 0;
        if(len == 0 || fmt[len - 1] != '\n')
            fputc('\n', log_);
    }
    va_end(ap1);
    va_end(ap2);
}


void
pam_oauth2_log::log(std::exception const &e) noexcept
{
    if(lev_ == log_level_t::OFF)
        return;
    if(ph_)
        pam_syslog(ph_, LOG_ERR, "system excpt %s", e.what());
    if(log_)
        fprintf(log_, "system exception %s\n", e.what());
}
