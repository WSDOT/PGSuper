///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

#ifndef INCLUDED_GIRDERSCHEDULECHAPTERBUILDER_H_
#define INCLUDED_GIRDERSCHEDULECHAPTERBUILDER_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#include <PgsExt\Keys.h>

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
interface IEAFDisplayUnits;

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CGirderScheduleChapterBuilder

   Hauling Check Details Chapter Builder.


DESCRIPTION
   Reports the details of the Hauling stability check
   calculation.

LOG
   rab : 11.03.1998 : Created file
*****************************************************************************/

class CGirderScheduleChapterBuilder : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   CGirderScheduleChapterBuilder(bool bSelect = true);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const override;

   //------------------------------------------------------------------------
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const override;

   //------------------------------------------------------------------------
   virtual CChapterBuilder* Clone() const override;

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE

   // Prevent accidental copying and assignment
   CGirderScheduleChapterBuilder(const CGirderScheduleChapterBuilder&) = delete;
   CGirderScheduleChapterBuilder& operator=(const CGirderScheduleChapterBuilder&) = delete;

   int GetReinforcementDetails(IBroker* pBroker,const CSegmentKey& segmentKey,CLSID& familyCLSID,Float64* pz1Spacing,Float64 *pz1Length,Float64 *pz2Spacing,Float64* pz2Length,Float64 *pz3Spacing,Float64* pz3Length) const;

   struct DebondInformation
   {
      std::vector<StrandIndexType> Strands;
      Float64 Length;
   };
   int GetDebondDetails(IBroker* pBroker,const CSegmentKey& segmentKey,std::vector<DebondInformation>& debondInfo) const;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_HAULINGCHECKCHAPTERBUILDER_H_
