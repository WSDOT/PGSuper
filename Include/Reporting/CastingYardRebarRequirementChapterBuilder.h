///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

#ifndef INCLUDED_CASTINGYARDREBARREQUIREMENTCHAPTERBUILDER_H_
#define INCLUDED_CASTINGYARDREBARREQUIREMENTCHAPTERBUILDER_H_

#include <Reporting\ReportingExp.h>
#include <Reporter\Chapter.h>
#include <Reporting\PGSuperChapterBuilder.h>


/*****************************************************************************
CLASS 
   CCastingYardRebarRequirementChapterBuilder

   Reports the details for computing the required amount of tension rebar
   so that the alternative tension stress limit can be used.


DESCRIPTION
   Reports the details for computing the required amount of tension rebar
   so that the alternative tension stress limit can be used.

LOG
   rab : 05.04.2006 : Created file
*****************************************************************************/

class REPORTINGCLASS CCastingYardRebarRequirementChapterBuilder : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   CCastingYardRebarRequirementChapterBuilder(bool bSelect = true);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const;
   

   //------------------------------------------------------------------------
   virtual rptChapter* Build(CReportSpecification* pRptSpec,Uint16 level) const;

   //------------------------------------------------------------------------
   virtual CChapterBuilder* Clone() const;

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
   CCastingYardRebarRequirementChapterBuilder(const CCastingYardRebarRequirementChapterBuilder&) = delete;
   CCastingYardRebarRequirementChapterBuilder& operator=(const CCastingYardRebarRequirementChapterBuilder&) = delete;

   void BuildTable(IBroker* pBroker,rptParagraph* pPara,const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,bool bSimpleTable) const;
   void BuildTable(IBroker* pBroker,rptParagraph* pPara,const pgsPointOfInterest& poi,IntervalIndexType intervalIdx, bool bSimpleTable) const;
   void BuildTable(IBroker* pBroker,rptParagraph* pPara,const CGirderKey& girderKey,IntervalIndexType intervalIdx, bool bSimpleTable) const;
   rptRcTable* CreateTable(IBroker* pBroker,const CGirderKey& girderKey,bool bSimpleTable,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,IntervalIndexType intervalIdx,IEAFDisplayUnits* pDisplayUnits) const;
   void FillTable(IBroker* pBroker,rptRcTable* pTable,bool bSimpleTable,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,IntervalIndexType intervalIdx,const pgsPointOfInterest& poi) const;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_CASTINGYARDREBARREQUIREMENTCHAPTERBUILDER_H_
