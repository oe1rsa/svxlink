/**
@file	 WbRxRtlTcp.h
@brief   A WBRX using RTL2832U based DVB-T tuners through rtl_tcp
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-16

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/

#ifndef WBRX_RTL_TCP_INCLUDED
#define WBRX_RTL_TCP_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <map>
#include <string>
#include <vector>
#include <complex>
#include <set>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Config;
};
class RtlTcp;
class Ddr;


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

  

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief	A WBRX using RTL2832U based DVB-T tuners through rtl_tcp
@author Tobias Blomberg / SM0SVX
@date   2014-07-16

This class handle a RTL2832U tuner through the RtlTcp class. Configuration
is read from the given section in the given configuration file.
*/
class WbRxRtlTcp
{
  public:
    typedef std::complex<float> Sample;

    static WbRxRtlTcp *instance(Async::Config &cfg, const std::string &name);

    /**
     * @brief 	Constructor
     * @param   cfg A previously initialized configuration object
     * @param   name The name of the configuration section to read config from
     */
    WbRxRtlTcp(Async::Config &cfg, const std::string &name);
  
    /**
     * @brief 	Destructor
     */
    ~WbRxRtlTcp(void);
  
    /**
     * @brief   Set the center frequency of the tuner
     * @param   fq The new center frequency, in Hz, to set
     */
    void setCenterFq(uint32_t fq);

    /**
     * @brief   Read the currently set center frequency
     * @returns Returns the currently set tuner frequency in Hz
     */
    uint32_t centerFq(void);

    /**
     * @brief   Get the currently set sample rate
     * @returns Returns the currently set sample rate in Hz
     */
    uint32_t sampleRate(void) const;

    /**
     * @brief   Register a DDR with this tuner
     * @param   ddr A pointer to the DDR object to register
     *
     * A registered DDR will receive a notification through the tunerFqChanged
     * function when the tuner frequency changes. Also, the autoplacement of
     * the tuner frequency is dependent on that all associated DDR:s are
     * registered.
     */
    void registerDdr(Ddr *ddr);

    /**
     * @brief   Unregister a DDR from this tuner
     * @param   ddr A pointer to the DDR object to unregister
     *
     * @see registerDdr
     */
    void unregisterDdr(Ddr *ddr);

    /**
     * @brief   Get the name of this tuner object
     * @returns Returns the name of this tuner object
     */
    std::string name(void) const { return m_name; }

    /**
     * @brief   A signal that is emitted when new samples have been received
     * @param   samples A vector of received samples
     *
     * Connecting to this signal is the way to get samples from the DVB-T
     * dongle. The format is a vector of complex floats (I/Q) with a range from
     * -1 to 1.
     */
    sigc::signal<void, std::vector<Sample> > iqReceived;
    
  protected:
    
  private:
    typedef std::map<std::string, WbRxRtlTcp*> InstanceMap;
    typedef std::set<Ddr*> Ddrs;

    static InstanceMap instances;

    RtlTcp *rtl;
    Ddrs ddrs;
    bool auto_tune_enabled;
    std::string m_name;

    WbRxRtlTcp(const WbRxRtlTcp&);
    WbRxRtlTcp& operator=(const WbRxRtlTcp&);
    void findBestCenterFq(void);
    
};  /* class WbRxRtlTcp */


//} /* namespace */

#endif /* WBRX_RTL_TCP_INCLUDED */



/*
 * This file has not been truncated
 */
