/**********************************************************************/
/**********************************************************************/
/**                                                                  **/
/** Defines the class                                                **/
/**                                                                  **/
/**     SynapseEWMA                                                  **/
/**                                                                  **/
/** which computes an Exponentially Weighted Moving Average          **/
/**                                                                  **/
/** Coded from the formulae on the Wikipedia page:                   **/
/**                                                                  **/
/** http://en.wikipedia.org/wiki/Moving_average#Exponential_moving_average **/
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

#ifndef _SYNAPSE_CLICK_EWMA_ELEMENT_PA_28_III_20120618_

#define _SYNAPSE_CLICK_EWMA_ELEMENT_PA_28_III_20120618_ 1

class SynapseEWMA
   {
    private:

       double m_S_0;

       double m_fAlpha;

       double m_f_one_minus_alpha;

       bool  m_bWaitingOnFirstPrice;

    public:

       SynapseEWMA(double _falpha);

       ~SynapseEWMA(void);

       double getCurrentAverage(double _fPrice);
   };

#endif /* for ifndef _SYNAPSE_CLICK_EWMA_ELEMENT_PA_28_III_20120618_ */

//*********************************************************************
//*********************************************************************
//
//   End of file
//
//*********************************************************************
//*********************************************************************
