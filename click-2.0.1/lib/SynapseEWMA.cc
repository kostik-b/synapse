/**********************************************************************/
/**********************************************************************/
/**                                                                  **/
/** Implements methods in the class                                  **/
/**                                                                  **/
/**     SynapseEWMA                                                  **/
/**                                                                  **/
/** which computes an Exponentially Weighted Moving Average          **/
/**                                                                  **/
/** Developers/Modifiers:  Charles J Gillan, Ivor Spence             **/
/**                                                                  **/
/** Location:                                                        **/
/**            The Institute for Electronics Communications and      **/
/**                Information Technology (ECIT)                     **/
/**            The Queen's University of Belfast                     **/
/**            The Northern Ireland Science Park                     **/
/**            Queen's Road, Queen's Island, Belfast                 **/
/**            Northern Ireland BT3 9DT, UK                          **/
/**                                                                  **/
/**  Copyright (c) 2012  The Queen's University of Belfast           **/
/**  All rights reserved                                             **/
/**                                                                  **/
/**-+----1----+----2----+----3----+----4----+----5----+----6----+----**/
/**                                                                  **/
/**  Modifications within project Synapse                            **/
/**  -------------                                                   **/
/**                                                                  **/
/**  June 18th 2012  CJG  created source file                        **/
/**                                                                  **/
/**********************************************************************/
/**********************************************************************/

#include <click/SynapseEWMA.hh>

/**
 *  Constructor
 *
 */
SynapseEWMA::SynapseEWMA(double _falpha)
   {
    this->m_S_0 = 0.0e+00;

    this->m_bWaitingOnFirstPrice = true;

    this->m_fAlpha = _falpha;

    this->m_f_one_minus_alpha = 1.0e+00 - _falpha;

    return;
   }

/**
 *  Destructor
 *
 */
SynapseEWMA::~SynapseEWMA(void)
   {
    return;
   }

double SynapseEWMA::getCurrentAverage(double _fPrice)
  {
   //
   //---- Compute current moving average "S_1"
   //
   //     This depends on
   //
   //         1. Current price
   //         2. Previous value of moving average
   //
   //
   //     Note that we have to deal with the initialization
   //     where there is no previous price.
   //

   double fRc;

   if(this->m_bWaitingOnFirstPrice)
     {
      fRc = _fPrice;

      this->m_bWaitingOnFirstPrice = false;
     }
   else
     {
      fRc  = this->m_fAlpha * _fPrice;

      fRc += this->m_f_one_minus_alpha * this->m_S_0;
     }

   //
   //---- In preparation for the next price, we set
   //     the "current" moving average into S_0
   //

   this->m_S_0 = fRc;

   //
   //---- Ok, we're done - let's return the current moving average
   //

   return(fRc);
  }
   // End of method SynapseEWMA::getNextAverage()

//*********************************************************************
//*********************************************************************
//
//  End of file
//
//*********************************************************************
//*********************************************************************
