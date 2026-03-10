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

#include <Reporting\SpanGirderReportSpecification.h>
#include <Reporting\ConstructabilityCheckTable.h>
#include <Reporting\FlexuralCapacityCheckTable.h>
#include <Reporting\MomentCapacityParagraphBuilder.h>
#include "TexasMomentCapacityChapterBuilder.h"

#include <IFace\Tools.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\Bridge.h>
#include <IFace\Artifact.h>
#include <IFace\Intervals.h>

CTexasMomentCapacityChapterBuilder::CTexasMomentCapacityChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CTexasMomentCapacityChapterBuilder::GetName() const
{
   return TEXT("Moment Capacity");
}

rptChapter* CTexasMomentCapacityChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGirderRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pBroker = pGirderRptSpec->GetBroker();
   const CGirderKey& girderKey(pGirderRptSpec->GetGirderKey());

   GET_IFACE2(pBroker,IArtifact,pIArtifact);
   const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);


   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   rptParagraph* p = new rptParagraph;
   bool bOverReinforced;
   *p << CFlexuralCapacityCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,liveLoadIntervalIdx,pgsTypes::StrengthI,true,&bOverReinforced) << rptNewLine;
   
   GET_IFACE2(pBroker,IBridge,pBridge);
   SpanIndexType startSpanIdx = pBridge->GetGirderGroupStartSpan(girderKey.groupIndex);
   SpanIndexType endSpanIdx   = pBridge->GetGirderGroupEndSpan(girderKey.groupIndex);
   bool bProcessNegativeMoments = false;
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      if ( pBridge->ProcessNegativeMoments(spanIdx) )
      {
         bProcessNegativeMoments = true;
         break;
      }
   }

   if ( bProcessNegativeMoments )
   {
      *p << rptNewLine;
      *p << CFlexuralCapacityCheckTable().Build(pBroker,pGirderArtifact,pDisplayUnits,liveLoadIntervalIdx,pgsTypes::StrengthI,false,&bOverReinforced) << rptNewLine;
   }

   *pChapter << p;

   return pChapter;
}
