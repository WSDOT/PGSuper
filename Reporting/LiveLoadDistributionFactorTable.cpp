///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <Reporting\LiveLoadDistributionFactorTable.h>
#include <Reporting\ReportNotes.h>

#include <IFace\DistributionFactors.h>

#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PsgLib\SpecLibraryEntry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CLiveLoadDistributionFactorTable
****************************************************************************/

CLiveLoadDistributionFactorTable::CLiveLoadDistributionFactorTable()
{
}

CLiveLoadDistributionFactorTable::CLiveLoadDistributionFactorTable(const CLiveLoadDistributionFactorTable& rOther)
{
   MakeCopy(rOther);
}

CLiveLoadDistributionFactorTable::~CLiveLoadDistributionFactorTable()
{
}

CLiveLoadDistributionFactorTable& CLiveLoadDistributionFactorTable::operator= (const CLiveLoadDistributionFactorTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

void CLiveLoadDistributionFactorTable::Build(rptChapter* pChapter,
                                             IBroker* pBroker,const CGirderKey& girderKey,
                                             IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_SCALAR_PROTOTYPE(rptRcScalar, df, pDisplayUnits->GetScalarFormat());

   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   SpanIndexType startSpanIdx, endSpanIdx;
   pBridge->GetGirderGroupSpans(girderKey.groupIndex,&startSpanIdx,&endSpanIdx);

   bool bNegMoments = false;
   for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
   {
      if ( pBridge->ProcessNegativeMoments(spanIdx) )
      {
         bNegMoments = true;
         break;
      }
   }

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   rptRcTable* pMasterTable = rptStyleManager::CreateLayoutTable(2,_T("Live Load Distribution Factors"));
   *pBody << pMasterTable;

   ColumnIndexType nCols = 4;
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      nCols += 3; // for fatigue limit state LLDF
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T("Moment and Shear"));
   (*pMasterTable)(0,0) << pTable;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }


   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pTable->SetNumberOfHeaderRows(1);
      (*pTable)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(0,1) << _T("+M");
      (*pTable)(0,2) << _T("-M");
      (*pTable)(0,3) << _T("V");
   }
   else
   {
      pTable->SetNumberOfHeaderRows(2);
      pTable->SetRowSpan(0,0,2);
      (*pTable)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      pTable->SetRowSpan(1,0,SKIP_CELL);

      pTable->SetColumnSpan(0,1,3);
      (*pTable)(0,1) << _T("Strength/Service");

      pTable->SetColumnSpan(0,2,3);
      (*pTable)(0,2) << _T("Fatigue/One Lane");

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

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest(CSegmentKey(girderKey,ALL_SEGMENTS),POI_SPAN) );

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS);

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest& poi( *iter );

      (*pTable)(row,0) << location.SetValue( POI_SPAN, poi );

      Float64 pM, nM, V;
      pDistFact->GetDistributionFactors(poi,pgsTypes::StrengthI,&pM,&nM,&V);
      (*pTable)(row,1) << df.SetValue(pM);

      if ( bNegMoments )
      {
         (*pTable)(row,2) << df.SetValue(nM);
      }
      else
      {
         (*pTable)(row,2) << _T("---");
      }
      (*pTable)(row,3) << df.SetValue(V);

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pDistFact->GetDistributionFactors(poi,pgsTypes::FatigueI,&pM,&nM,&V);
         (*pTable)(row,4) << df.SetValue(pM);

         if ( bNegMoments )
         {
            (*pTable)(row,5) << df.SetValue(nM);
         }
         else
         {
            (*pTable)(row,5) << _T("---");
         }

         (*pTable)(row,6) << df.SetValue(V);
      }

      row++;
   }

   nCols = 2;
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      nCols++;
   }

   rptRcTable* pTable2 = rptStyleManager::CreateDefaultTable(nCols,_T("Reaction"));
   (*pMasterTable)(0,1) << pTable2;
   (*pTable2)(0,0) << _T("Pier");
   (*pTable2)(0,1) << _T("Strength/Service");

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      (*pTable2)(0,2) << _T("Fatigue/One Lane");
   }

   row = pTable2->GetNumberOfHeaderRows();
   PierIndexType nPiers = pBridge->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++, row++ )
   {
      (*pTable2)(row,0) << LABEL_PIER(pierIdx);
      (*pTable2)(row,1) << df.SetValue(pDistFact->GetReactionDistFactor(pierIdx,girderKey.girderIndex,pgsTypes::StrengthI));
      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         (*pTable2)(row,2) << df.SetValue(pDistFact->GetReactionDistFactor(pierIdx,girderKey.girderIndex,pgsTypes::FatigueI));
      }
   }

   GET_IFACE2(pBroker,ILiveLoads, pLiveLoads);
   std::_tstring strSpecialAction = pLiveLoads->GetLLDFSpecialActionText();
   if ( !strSpecialAction.empty() )
   {
      (*pBody) << color(Red) << strSpecialAction << color(Black) << rptNewLine;
   }
}

void CLiveLoadDistributionFactorTable::MakeCopy(const CLiveLoadDistributionFactorTable& rOther)
{
   // Add copy code here...
}

void CLiveLoadDistributionFactorTable::MakeAssignment(const CLiveLoadDistributionFactorTable& rOther)
{
   MakeCopy( rOther );
}
