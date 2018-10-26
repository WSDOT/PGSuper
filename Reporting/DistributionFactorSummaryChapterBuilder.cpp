///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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
#include <Reporting\DistributionFactorSummaryChapterBuilder.h>

#include <EAF\EAFDisplayUnits.h>
#include <IFace\DistributionFactors.h>
#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CDistributionFactorSummaryChapterBuilder
****************************************************************************/

// free functions
GirderIndexType GetPierGirderCount(PierIndexType pierIdx, IBridge* pBridge)
{
   PierIndexType nPiers = pBridge->GetPierCount();

   // Number of girders at pier is max in attached spans
   if (pierIdx == 0)
   {
      return pBridge->GetGirderCountBySpan(0);
   }
   else if (pierIdx == nPiers-1)
   {
      return pBridge->GetGirderCountBySpan(pierIdx-1);
   }
   else
   {
      SpanIndexType prevSpanIdx = pierIdx-1;
      SpanIndexType nextSpanIdx = prevSpanIdx+1;
      GirderIndexType prvNgdrs = pBridge->GetGirderCountBySpan(prevSpanIdx);
      GirderIndexType nxtNgdrs = pBridge->GetGirderCountBySpan(nextSpanIdx);
      return Max(prvNgdrs, nxtNgdrs);
   }
}


void WriteSpanTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,IEAFDisplayUnits* pDisplayUnits);
void WritePierTable(rptChapter* pChapter,IBroker* pBroker,PierIndexType pier,IEAFDisplayUnits* pDisplayUnits);

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CDistributionFactorSummaryChapterBuilder::CDistributionFactorSummaryChapterBuilder(bool bSelect) :
CPGSuperChapterBuilder(bSelect)
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
LPCTSTR CDistributionFactorSummaryChapterBuilder::GetName() const
{
   return TEXT("Live Load Distribution Factor Summary");
}

rptChapter* CDistributionFactorSummaryChapterBuilder::Build(CReportSpecification* pRptSpec,Uint16 level) const
{
   USES_CONVERSION;

   CBrokerReportSpecification* pBrokerSpec = dynamic_cast<CBrokerReportSpecification*>(pRptSpec);

   // This report does not use the passd span and girder parameters
   rptChapter* pChapter = CPGSuperChapterBuilder::Build(pRptSpec,level);

   CComPtr<IBroker> pBroker;
   pBrokerSpec->GetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker,IBridge,pBridge);

   PierIndexType nPiers = pBridge->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
       WritePierTable(pChapter, pBroker, pierIdx, pDisplayUnits);

       if ( pierIdx < nPiers-1 )
       {
          SpanIndexType spanIdx = (SpanIndexType)pierIdx;
          WriteSpanTable(pChapter, pBroker, spanIdx, pDisplayUnits);
       }
   }


   return pChapter;
}

CChapterBuilder* CDistributionFactorSummaryChapterBuilder::Clone() const
{
   return new CDistributionFactorSummaryChapterBuilder;
}

void WriteSpanTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType spanIdx,IEAFDisplayUnits* pDisplayUnits)
{
   INIT_SCALAR_PROTOTYPE(rptRcScalar, df, pDisplayUnits->GetScalarFormat());
   INIT_SCALAR_PROTOTYPE(rptRcSectionScalar, dfM, pDisplayUnits->GetScalarFormat());
   INIT_SCALAR_PROTOTYPE(rptRcSectionScalar, dfV, pDisplayUnits->GetScalarFormat());

   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   bool bNegMoments = pBridge->ProcessNegativeMoments(spanIdx);

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   ColumnIndexType nCols = 4;
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      nCols += 3; // for fatigue limit state LLDF

   std::_tostringstream os;
   os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(", Distribution Factors");

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,os.str().c_str());
   *pBody << pTable;

   (*pTable)(0,0) << _T("");
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pTable->SetNumberOfHeaderRows(1);
      (*pTable)(0,1) << _T("+M");
      (*pTable)(0,2) << _T("-M");
      (*pTable)(0,3) << _T("V");
   }
   else
   {
      pTable->SetNumberOfHeaderRows(2);
      pTable->SetRowSpan(0,0,2);
      pTable->SetRowSpan(1,0,SKIP_CELL);

      pTable->SetColumnSpan(0,1,3);
      (*pTable)(0,1) << _T("Strength/Service");

      pTable->SetColumnSpan(0,2,3);
      (*pTable)(0,2) << _T("Fatigue/Single");

      pTable->SetColumnSpan(0,3,SKIP_CELL);
      pTable->SetColumnSpan(0,4,SKIP_CELL);
      pTable->SetColumnSpan(0,5,SKIP_CELL);
      pTable->SetColumnSpan(0,6,SKIP_CELL);

      (*pTable)(1,1) << _T("+M");
      (*pTable)(1,2) << _T("-M");
      (*pTable)(1,3) << _T("V");
      (*pTable)(1,4) << _T("+M");
      (*pTable)(1,5) << _T("-M");
      (*pTable)(1,6) << _T("V");
   }

   GirderIndexType nGirders = pBridge->GetGirderCountBySpan(spanIdx);

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   GroupIndexType grpIdx = pBridge->GetGirderGroupIndex(spanIdx);
   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      CSpanKey spanKey(spanIdx,gdrIdx);

      // get the length of segments within a span
      std::vector<std::pair<SegmentIndexType,Float64>> vSegments = pBridge->GetSegmentLengths(spanKey);

      // create POI at start and end of span
      SegmentIndexType segIdx = vSegments.front().first;
      pgsPointOfInterest poi_start(CSegmentKey(grpIdx,gdrIdx,segIdx),0.0);
      
      segIdx = vSegments.back().first;
      Float64 distFromStartOfSegment = vSegments.back().second;
      pgsPointOfInterest poi_end(CSegmentKey(grpIdx,gdrIdx,segIdx),distFromStartOfSegment);

      (*pTable)(row,0) << _T("Girder ") << LABEL_GIRDER(gdrIdx);

      Float64 pM, VStart, VEnd;
      Float64 nm;
      sysSectionValue nM;
      pDistFact->GetDistributionFactors(poi_start, pgsTypes::StrengthI, &pM, &nm, &VStart);
      nM.Left() = nm;

      pDistFact->GetDistributionFactors(poi_end, pgsTypes::StrengthI, &pM, &nm, &VEnd);
      nM.Right() = nm;

      (*pTable)(row,1) << df.SetValue(pM);

      if ( bNegMoments )
      {
         (*pTable)(row,2) << dfM.SetValue(nM);
      }
      else
      {
         (*pTable)(row,2) << _T("------");
      }

      if ( IsEqual(VStart,VEnd) )
      {
         (*pTable)(row,3) << dfV.SetValue(VStart);
      }
      else
      {
         (*pTable)(row,3) << dfV.SetValue(VStart) << _T(" (Left End)") << rptNewLine;
         (*pTable)(row,3) << dfV.SetValue(VEnd) << _T(" (Right End)");
      }


      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pDistFact->GetDistributionFactors(poi_start,pgsTypes::FatigueI,&pM,&nm,&VStart);
         nM.Left() = nm;

         pDistFact->GetDistributionFactors(poi_end,pgsTypes::FatigueI,&pM,&nm,&VEnd);
         nM.Right() = nm;

         (*pTable)(row,4) << df.SetValue(pM);

         if ( bNegMoments )
         {
            (*pTable)(row,5) << dfM.SetValue(nM);
         }
         else
         {
            (*pTable)(row,5) << _T("------");
         }

         if ( IsEqual(VStart,VEnd) )
         {
            (*pTable)(row,6) << dfV.SetValue(VStart);
         }
         else
         {
            (*pTable)(row,6) << dfV.SetValue(VStart) << _T(" (Left End)") << rptNewLine;
            (*pTable)(row,6) << dfV.SetValue(VEnd) << _T(" (Right End)");
         }
      }

      row++;
   }
}

void WritePierTable(rptChapter* pChapter,IBroker* pBroker,PierIndexType pierIdx,IEAFDisplayUnits* pDisplayUnits)
{
   INIT_SCALAR_PROTOTYPE(rptRcSectionScalar, dfM, pDisplayUnits->GetScalarFormat());
   INIT_SCALAR_PROTOTYPE(rptRcSectionScalar, dfV, pDisplayUnits->GetScalarFormat());

   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   bool bContinuousLeft,bContinuousRight;
   pBridge->IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);

   bool bIntegralLeft,bIntegralRight;
   pBridge->IsIntegralAtPier(pierIdx,&bIntegralLeft,&bIntegralRight);

   bool bNegMoments = (bContinuousLeft || bContinuousRight || bIntegralLeft || bIntegralRight);

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   ColumnIndexType nCols = 3;
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      nCols += 2; // for fatigue limit state LLDF

   std::_tostringstream os;
   os << _T("Pier ") << LABEL_PIER(pierIdx) << _T(", Distribution Factors");

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,os.str().c_str());
   *pBody << pTable;

   (*pTable)(0,0) << _T("");
   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pTable->SetNumberOfHeaderRows(1);
      (*pTable)(0,1) << _T("-M");
      (*pTable)(0,2) << _T("R");
   }
   else
   {
      pTable->SetNumberOfHeaderRows(2);
      pTable->SetRowSpan(0,0,2);
      pTable->SetRowSpan(1,0,SKIP_CELL);

      pTable->SetColumnSpan(0,1,2);
      (*pTable)(0,1) << _T("Strength/Service");

      pTable->SetColumnSpan(0,2,2);
      (*pTable)(0,2) << _T("Fatigue/Single");

      pTable->SetColumnSpan(0,3,SKIP_CELL);
      pTable->SetColumnSpan(0,4,SKIP_CELL);

      (*pTable)(1,1) << _T("-M");
      (*pTable)(1,2) << _T("R");
      (*pTable)(1,3) << _T("-M");
      (*pTable)(1,4) << _T("R");
   }

   // report ahead face of pier except at end
   PierIndexType nPiers = pBridge->GetPierCount();
   pgsTypes::PierFaceType face = (pierIdx != nPiers-1 ? pgsTypes::Ahead : pgsTypes::Back);
   GirderIndexType nGirders = GetPierGirderCount(pierIdx,pBridge);

   RowIndexType row = pTable->GetNumberOfHeaderRows();


   for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
   {
      (*pTable)(row,0) << _T("Girder ") << LABEL_GIRDER(gdrIdx);

      sysSectionValue nM;
      Float64 V;
      if ( bNegMoments )
      {
         if ( pierIdx == 0 )
         {
            nM.Right() = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Ahead);
            nM.Left() = nM.Right();
         }
         else if ( pierIdx == nPiers-1 )
         {
            nM.Left()  = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Back);
            nM.Right() = nM.Left();
         }
         else
         {
            nM.Left()  = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Back);
            nM.Right() = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::StrengthI,pgsTypes::Ahead);
         }
         (*pTable)(row,1) << dfM.SetValue(nM);
      }
      else
      {
         (*pTable)(row,1) << _T("------");
      }

      V = pDistFact->GetReactionDistFactor(pierIdx,gdrIdx,pgsTypes::StrengthI);
      (*pTable)(row,2) << dfV.SetValue(V);


      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         if ( bNegMoments )
         {
            if ( pierIdx == 0 )
            {
               nM.Right() = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::FatigueI,pgsTypes::Ahead);
               nM.Left() = nM.Right();
            }
            else if ( pierIdx == nPiers-1 )
            {
               nM.Left()  = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::FatigueI,pgsTypes::Back);
               nM.Right() = nM.Left();
            }
            else
            {
               nM.Left()  = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::FatigueI,pgsTypes::Back);
               nM.Right() = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,gdrIdx,pgsTypes::FatigueI,pgsTypes::Ahead);
            }
            (*pTable)(row,3) << dfM.SetValue(nM);
         }
         else
         {
            (*pTable)(row,3) << _T("------");
         }

         V = pDistFact->GetReactionDistFactor(pierIdx,gdrIdx,pgsTypes::FatigueI);
         (*pTable)(row,4) << dfV.SetValue(V);
      }

      row++;
   }
}

