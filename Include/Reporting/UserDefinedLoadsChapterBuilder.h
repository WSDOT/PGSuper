///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#ifndef INCLUDED_USERDEFINEDLOADSCHAPTERBUILDER_H_
#define INCLUDED_USERDEFINEDLOADSCHAPTERBUILDER_H_

#include <Reporting\ReportingExp.h>
#include <Reporting\PGSuperChapterBuilder.h>

interface IEAFDisplayUnits;

/*****************************************************************************
CLASS 
   CUserDefinedLoadsChapterBuilder

   Report user-defined load data


DESCRIPTION
   Report user-defined load data

LOG
   rdp : 06.01.1999 : Created file
*****************************************************************************/

class REPORTINGCLASS CUserDefinedLoadsChapterBuilder : public CPGSuperChapterBuilder
{
public:
   // GROUP: LIFECYCLE
   CUserDefinedLoadsChapterBuilder(bool bSelect = true, bool SimplifiedVersion=false);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   virtual LPCTSTR GetName() const override;
   

   //------------------------------------------------------------------------
   virtual rptChapter* Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const override;

   //------------------------------------------------------------------------
   virtual std::unique_ptr<WBFL::Reporting::ChapterBuilder> Clone() const override;

   static rptParagraph* CreatePointLoadTable(IBroker* pBroker,
                           const CSpanKey& spanKey,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level, bool bSimplifiedVersion);

   static rptParagraph* CreateDistributedLoadTable(IBroker* pBroker,
                           const CSpanKey& spanKey,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level, bool bSimplifiedVersion);

   static rptParagraph* CreateMomentLoadTable(IBroker* pBroker,
                           const CSpanKey& spanKey,
                           IEAFDisplayUnits* pDisplayUnits,
                           Uint16 level, bool bSimplifiedVersion);

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
   bool m_bSimplifiedVersion;

   // GROUP: LIFECYCLE

   // Prevent accidental copying and assignment
   CUserDefinedLoadsChapterBuilder(const CUserDefinedLoadsChapterBuilder&) = delete;
   CUserDefinedLoadsChapterBuilder& operator=(const CUserDefinedLoadsChapterBuilder&) = delete;

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_UserDefinedLoadsCHAPTERBUILDER_H_
