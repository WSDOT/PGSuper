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

#ifndef INCLUDED_PGSEXT_HANDLINGDATA_H_
#define INCLUDED_PGSEXT_HANDLINGDATA_H_

#include <WBFLCore.h>

#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

/*****************************************************************************
CLASS 
   CHandlingData

   Utility class for lifting and transportation handling data.

DESCRIPTION
   Utility class for lifting and transportation handling data.


COPYRIGHT
   Copyright © 1997-2008
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rab : 05.27.208 : Created file
*****************************************************************************/

class PGSEXTCLASS CHandlingData
{
public:
   CHandlingData();
   CHandlingData(const CHandlingData& rOther);
   ~CHandlingData();

   CHandlingData& operator = (const CHandlingData& rOther);
   bool operator==(const CHandlingData& rOther) const;
   bool operator!=(const CHandlingData& rOther) const;

   HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
   HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   double LeftLiftPoint, RightLiftPoint;
   double LeadingSupportPoint, TrailingSupportPoint;

protected:
   void MakeCopy(const CHandlingData& rOther);
   void MakeAssignment(const CHandlingData& rOther);
};

#endif // INCLUDED_PGSEXT_HANDLINGDATA_H_
