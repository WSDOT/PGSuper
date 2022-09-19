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

#include "StdAfx.h"

#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\ConstructabilityCheckTable.h>
#include <Reporting\MultiGirderHaunchGeometryChapterBuilder.h>
#include <EAF\EAFDisplayUnits.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CMultiGirderHaunchGeometryChapterBuilder
****************************************************************************/
////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CMultiGirderHaunchGeometryChapterBuilder::CMultiGirderHaunchGeometryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CMultiGirderHaunchGeometryChapterBuilder::GetName() const
{
   return TEXT("Haunch Geometry");
}

rptChapter* CMultiGirderHaunchGeometryChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   // This can be called for multi or single girders
   std::vector<CGirderKey> girder_list;

   CComPtr<IBroker> pBroker;

   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   if (pGirderRptSpec!=nullptr)
   {
      pGirderRptSpec->GetBroker(&pBroker);
      girder_list.push_back( pGirderRptSpec->GetGirderKey() );
   }
   else
   {
      auto pReportSpec = std::dynamic_pointer_cast<const CMultiGirderReportSpecification>(pRptSpec);
      pReportSpec->GetBroker(&pBroker);

      girder_list = pReportSpec->GetGirderKeys();
   }
   ATLASSERT(!girder_list.empty());

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   // Constructability check
   CConstructabilityCheckTable().BuildSlabOffsetTable(pChapter,pBroker,girder_list,pDisplayUnits);

   // Min Haunch at bearing centerlines check
   CConstructabilityCheckTable().BuildMinimumHaunchCLCheck(pChapter,pBroker,girder_list,pDisplayUnits);

   // Fillet Check
   CConstructabilityCheckTable().BuildMinimumFilletCheck(pChapter,pBroker,girder_list,pDisplayUnits);

   // Haunch Geometry Check
   CConstructabilityCheckTable().BuildHaunchGeometryComplianceCheck(pChapter,pBroker,girder_list,pDisplayUnits);

   return pChapter;
}

std::unique_ptr<WBFL::Reporting::ChapterBuilder> CMultiGirderHaunchGeometryChapterBuilder::Clone() const
{
   return std::make_unique<CMultiGirderHaunchGeometryChapterBuilder>();
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
