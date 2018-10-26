///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
//                        Bridge and Structures Office
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the Alternate Route Open Source License as 
// published by the Washington State Department of Transportation, 
// Bridge and Structures Office.
//
// This program is distributed in the hope that it will be useful, but 
// distribution is AS IS, WITHOUT ANY WARRANTY; without even the implied 
// warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See 
// the Alternate Route Open Source License for more details.
//
// You should have received a copy of the Alternate Route Open Source 
// License along with this program; if not, write to the Washington 
// State Department of Transportation, Bridge and Structures Office, 
// P.O. Box  47340, Olympia, WA 98503, USA or e-mail 
// Bridge_Support@wsdot.wa.gov
///////////////////////////////////////////////////////////////////////

#ifndef INCLUDED_IFACE_EFFFLANGERENGINEER_H_
#define INCLUDED_IFACE_EFFFLANGERENGINEER_H_

/*****************************************************************************
COPYRIGHT
   Copyright © 1997-2001
   Washington State Department Of Transportation
   All Rights Reserved
*****************************************************************************/

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//
#include <Reporter\Reporter.h>

// FORWARD DECLARATIONS
//
struct IBroker;
struct IDisplayUnits;

// MISCELLANEOUS
//


/*****************************************************************************
INTERFACE
   IEffFlangeEngineer

   Interface for computing effective flange width

DESCRIPTION
   Interface for computing effective flange width
*****************************************************************************/
// {2E33078E-94F8-4655-8FFD-A18FFEEE7192}
DEFINE_GUID(IID_IEffFlangeEngineer, 
0x2e33078e, 0x94f8, 0x4655, 0x8f, 0xfd, 0xa1, 0x8f, 0xfe, 0xee, 0x71, 0x92);
interface IEffFlangeEngineer : IUnknown
{
   //---------------------------------------------------------------------
   // Associated a broker object with this object. Call only from
   // IBeamFactory at create time.
   virtual void SetBroker(IBroker* pBroker,long agentID) = 0;

   //---------------------------------------------------------------------
   // Returns the effective flange width
   virtual double GetEffectiveFlangeWidth(Uint16 span,Uint16 gdr) = 0;

   //---------------------------------------------------------------------
   // Creates a detailed report of the effective flange width computation
   virtual void BuildReport(Uint16 span,Uint16 gdr,rptChapter* pChapter,IDisplayUnits* pDispUnit) = 0;
};

#endif // INCLUDED_IFACE_EFFFLANGERENGINEER_H_

