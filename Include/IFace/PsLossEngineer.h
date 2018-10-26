///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

#ifndef INCLUDED_IFACE_PSLOSSENGINEER_H_
#define INCLUDED_IFACE_PSLOSSENGINEER_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//

// LOCAL INCLUDES
//
#include <Reporter\Reporter.h>
#include <Details.h>

// FORWARD DECLARATIONS
//
struct IBroker;
struct IEAFDisplayUnits;

// MISCELLANEOUS
//


/*****************************************************************************
INTERFACE
   IPsLossEngineer

   Interface for computing and report prestress losses

DESCRIPTION
   Interface for computing and report prestress losses
*****************************************************************************/
// {69ABD84E-733A-4e1f-B64E-1EA888EA4935}
DEFINE_GUID(IID_IPsLossEngineer, 
0x69abd84e, 0x733a, 0x4e1f, 0xb6, 0x4e, 0x1e, 0xa8, 0x88, 0xea, 0x49, 0x35);
interface IPsLossEngineer : IUnknown
{
   //---------------------------------------------------------------------
   // Computes the prestress losses
   virtual LOSSDETAILS ComputeLosses(const pgsPointOfInterest& poi) = 0;
   
   // computes prestress losses but uses the input slab offset (the current design value)
   virtual LOSSDETAILS ComputeLossesForDesign(const pgsPointOfInterest& poi,const GDRCONFIG& config) = 0;

   //---------------------------------------------------------------------
   // Creates a detailed report of the effective flange width computation
   virtual void BuildReport(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) = 0;

   virtual void ReportFinalLosses(SpanIndexType span,GirderIndexType gdr,rptChapter* pChapter,IEAFDisplayUnits* pDisplayUnits) = 0;
};

#endif // INCLUDED_IFACE_PSLOSSENGINEER_H_

