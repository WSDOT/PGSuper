///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
inline GirderIndexType GetPierGirderCount(PierIndexType pierIdx, IBridge* pBridge)
{
   PierIndexType npiers = pBridge->GetPierCount();

   // Number of girders at pier is max in attached spans
   if (pierIdx==0)
   {
      return pBridge->GetGirderCount(0);
   }
   else if(pierIdx==npiers-1)
   {
      return pBridge->GetGirderCount(npiers-2);
   }
   else
   {
      GirderIndexType prvNgdrs = pBridge->GetGirderCount(pierIdx-1);
      GirderIndexType nxtNgdrs = pBridge->GetGirderCount(pierIdx);
      return max(prvNgdrs, nxtNgdrs);
   }
}


void WriteGirderTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType span,IEAFDisplayUnits* pDisplayUnits);
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
          WriteGirderTable(pChapter, pBroker, spanIdx, pDisplayUnits);
       }
   }


   return pChapter;
}

CChapterBuilder* CDistributionFactorSummaryChapterBuilder::Clone() const
{
   return new CDistributionFactorSummaryChapterBuilder;
}

void WriteGirderTable(rptChapter* pChapter,IBroker* pBroker,SpanIndexType spanIdx,IEAFDisplayUnits* pDisplayUnits)
{
   rptRcScalar df;
   df.SetFormat(sysNumericFormatTool::Fixed);
   df.SetWidth(8);
   df.SetPrecision(3); // should match format in details reports

   rptRcSectionScalar dfM;
   dfM.SetFormat(sysNumericFormatTool::Fixed);
   dfM.SetWidth(8);
   dfM.SetPrecision(3); // should match format in details reports

   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   bool bNegMoments = pBridge->ProcessNegativeMoments(spanIdx);

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   ColumnIndexType nCols = 4;
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      nCols += 3; // for fatigue limit state LLDF

   std::_tostringstream os;
   os << _T("Span ") << LABEL_SPAN(spanIdx) << _T(", Girder Distribution Factors");

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,os.str());
   *pBody << pTable;

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

   GirderIndexType ngdrs = pBridge->GetGirderCount(spanIdx);

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   for ( GirderIndexType igdr = 0; igdr<ngdrs; igdr++ )
   {
      Float64 girder_length = pBridge->GetGirderLength(spanIdx,igdr);

      (*pTable)(row,0) << _T("Girder ") << LABEL_GIRDER(igdr);

      pgsPointOfInterest poi_start(spanIdx,igdr,0.0);
      pgsPointOfInterest poi_end(spanIdx,igdr,girder_length);

      Float64 pM, V;
      Float64 nm;
      sysSectionValue nM;
      pDistFact->GetDistributionFactors(poi_start, pgsTypes::StrengthI, &pM, &nm, &V);
      nM.Left() = nm;

      pDistFact->GetDistributionFactors(poi_end, pgsTypes::StrengthI, &pM, &nm, &V);
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

      (*pTable)(row,3) << df.SetValue(V);


      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pDistFact->GetDistributionFactors(poi_start,pgsTypes::FatigueI,&pM,&nm,&V);
         nM.Left() = nm;

         pDistFact->GetDistributionFactors(poi_end,pgsTypes::FatigueI,&pM,&nm,&V);
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

         (*pTable)(row,6) << df.SetValue(V);
      }

      row++;
   }
}

void WritePierTable(rptChapter* pChapter,IBroker* pBroker,PierIndexType pierIdx,IEAFDisplayUnits* pDisplayUnits)
{
   rptRcSectionScalar dfM;
   dfM.SetFormat(sysNumericFormatTool::Fixed);
   dfM.SetWidth(8);
   dfM.SetPrecision(3); // should match format in details reports

   rptRcScalar dfV;
   dfV.SetFormat(sysNumericFormatTool::Fixed);
   dfV.SetWidth(8);
   dfV.SetPrecision(3); // should match format in details reports

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

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,os.str());
   *pBody << pTable;

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
   pgsTypes::PierFaceType face = pierIdx!=nPiers-1 ? pgsTypes::Ahead : pgsTypes::Back;
   GirderIndexType ngdrs = GetPierGirderCount(pierIdx,pBridge);

   RowIndexType row = pTable->GetNumberOfHeaderRows();


   for ( GirderIndexType igdr = 0; igdr<ngdrs; igdr++ )
   {
      (*pTable)(row,0) << _T("Girder ") << LABEL_GIRDER(igdr);

      sysSectionValue nM;
      Float64 V;
      if ( bNegMoments )
      {
         if ( pierIdx == 0 )
         {
            nM.Right() = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,igdr,pgsTypes::StrengthI,pgsTypes::Ahead);
            nM.Left() = nM.Right();
         }
         else if ( pierIdx == nPiers-1 )
         {
            nM.Left()  = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,igdr,pgsTypes::StrengthI,pgsTypes::Back);
            nM.Right() = nM.Left();
         }
         else
         {
            nM.Left()  = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,igdr,pgsTypes::StrengthI,pgsTypes::Back);
            nM.Right() = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,igdr,pgsTypes::StrengthI,pgsTypes::Ahead);
         }
         (*pTable)(row,1) << dfM.SetValue(nM);
      }
      else
      {
         (*pTable)(row,1) << _T("------");
      }

      V = pDistFact->GetReactionDistFactor(pierIdx,igdr,pgsTypes::StrengthI);
      (*pTable)(row,2) << dfV.SetValue(V);


      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         if ( bNegMoments )
         {
            if ( pierIdx == 0 )
            {
               nM.Right() = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,igdr,pgsTypes::FatigueI,pgsTypes::Ahead);
               nM.Left() = nM.Right();
            }
            else if ( pierIdx == nPiers-1 )
            {
               nM.Left()  = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,igdr,pgsTypes::FatigueI,pgsTypes::Back);
               nM.Right() = nM.Left();
            }
            else
            {
               nM.Left()  = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,igdr,pgsTypes::FatigueI,pgsTypes::Back);
               nM.Right() = pDistFact->GetNegMomentDistFactorAtPier(pierIdx,igdr,pgsTypes::FatigueI,pgsTypes::Ahead);
            }
            (*pTable)(row,3) << dfM.SetValue(nM);
         }
         else
         {
            (*pTable)(row,3) << _T("------");
         }

         V = pDistFact->GetReactionDistFactor(pierIdx,igdr,pgsTypes::FatigueI);
         (*pTable)(row,4) << dfV.SetValue(V);
      }

      row++;
   }
}

