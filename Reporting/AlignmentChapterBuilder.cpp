///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <Reporting\AlignmentChapterBuilder.h>
#include <Reporting\BridgeDescChapterBuilder.h>

#include <IFace/Tools.h>
#include <EAF/EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Alignment.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Project.h>


#include <WBFLCogo.h>

CAlignmentChapterBuilder::CAlignmentChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CAlignmentChapterBuilder::GetName() const
{
   return TEXT("Alignment");
}

rptChapter* CAlignmentChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   auto pBrokerSpec = std::dynamic_pointer_cast<const CBrokerReportSpecification>(pRptSpec);

   // This report does not use the passed span and girder parameters
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   auto pBroker = pBrokerSpec->GetBroker();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   CBridgeDescChapterBuilder::WriteAlignmentData(pBroker,pDisplayUnits,pChapter,level);
   CBridgeDescChapterBuilder::WriteProfileData(pBroker,pDisplayUnits,pChapter,level);
   CBridgeDescChapterBuilder::WriteCrownData(pBroker,pDisplayUnits,pChapter,level);

   return pChapter;
}
