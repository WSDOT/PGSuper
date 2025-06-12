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
#include <Reporting\DistributionFactorDetailsChapterBuilder.h>
#include <Reporting\LiveLoadDistributionFactorTable.h>
#include <EAF\EAFUtilities.h>

#include <IFace/Tools.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\DistributionFactors.h>
#include <IFace\Bridge.h>
#include <IFace\DocumentType.h>



// free functions
rptRcTable* BuildDfTable(const WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& G1, const WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& G2, bool isExterior);
void FillRow(int row, rptRcTable* pTable, const std::_tstring& rowtit, const WBFL::LRFD::ILiveLoadDistributionFactor::DFResult& res, bool isExterior);


CDistributionFactorDetailsChapterBuilder::CDistributionFactorDetailsChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

LPCTSTR CDistributionFactorDetailsChapterBuilder::GetName() const
{
   return TEXT("Live Load Distribution Factor Details");
}

rptChapter* CDistributionFactorDetailsChapterBuilder::Build(const std::shared_ptr<const WBFL::Reporting::ReportSpecification>& pRptSpec,Uint16 level) const
{
   auto pGdrRptSpec = std::dynamic_pointer_cast<const CGirderReportSpecification>(pRptSpec);
   auto pGdrLineRptSpec = std::dynamic_pointer_cast<const CGirderLineReportSpecification>(pRptSpec);

   std::shared_ptr<WBFL::EAF::Broker> pBroker;
   CGirderKey girderKey;

   if ( pGdrRptSpec )
   {
      pBroker = pGdrRptSpec->GetBroker();
      girderKey = pGdrRptSpec->GetGirderKey();
   }
   else
   {
      pBroker = pGdrLineRptSpec->GetBroker();
      girderKey = pGdrLineRptSpec->GetGirderKey();
   }

   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   GET_IFACE2(pBroker, IDocumentType, pDocType);
   bool isPGSuper = pDocType->IsPGSuperDocument();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx  = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType firstGirderIdx = Min(nGirders-1,(girderKey.girderIndex == ALL_GIRDERS ? 0 : girderKey.girderIndex));
      GirderIndexType lastGirderIdx  = Min(nGirders-1,(girderKey.girderIndex == ALL_GIRDERS ? nGirders-1 : firstGirderIdx));
      for ( GirderIndexType gdrIdx = firstGirderIdx; gdrIdx <= lastGirderIdx; gdrIdx++ )
      {
         rptParagraph* pPara;
         if ( girderKey.groupIndex == ALL_GROUPS || girderKey.girderIndex == ALL_GIRDERS )
         {
            pPara = new rptParagraph(rptStyleManager::GetHeadingStyle());
            *pChapter << pPara;
            std::_tostringstream os;
            if (isPGSuper)
            {
               os << _T("Span ") << LABEL_SPAN(grpIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx);
            }
            else
            {
               os << _T("Group ") << LABEL_GROUP(grpIdx) << _T(" Girder ") << LABEL_GIRDER(gdrIdx);
            }

            pPara->SetName( os.str().c_str() );
            (*pPara) << pPara->GetName() << rptNewLine;
         }

         CGirderKey thisGirderKey(grpIdx,gdrIdx);
         CLiveLoadDistributionFactorTable().Build(pChapter,pBroker,thisGirderKey,pDisplayUnits);
         pDistFact->ReportDistributionFactors(thisGirderKey,pChapter,pDisplayUnits);
      }
   }
   return pChapter;
}
