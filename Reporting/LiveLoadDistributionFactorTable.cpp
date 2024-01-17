///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#include <IFace\DocumentType.h>
#include <IFace\DistributionFactors.h>
#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\ReportOptions.h>

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

   ColumnIndexType nCols = 4;
   if ( WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      nCols += 3; // for fatigue limit state LLDF
   }

   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols,_T("Moment and Shear"));
   (*pBody) << pTable;

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      pTable->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   ColumnIndexType col = 0;
   if ( WBFL::LRFD::BDSManager::GetEdition() < WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims )
   {
      pTable->SetNumberOfHeaderRows(1);
      (*pTable)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(0, col++) << _T("+M");
      (*pTable)(0, col++) << _T("-M");
      (*pTable)(0, col++) << _T("V");
   }
   else
   {
      pTable->SetNumberOfHeaderRows(2);
      pTable->SetRowSpan(0, col, 2);
      (*pTable)(0, col++) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );

      pTable->SetColumnSpan(0, col, 3);
      (*pTable)(0, col) << _T("Strength/Service");
      (*pTable)(1, col++) << _T("+M");
      (*pTable)(1, col++) << _T("-M");
      (*pTable)(1, col++) << _T("V");

      pTable->SetColumnSpan(0, col, 3);
      (*pTable)(0, col) << _T("Fatigue/Special Permit Rating");
      (*pTable)(1, col++) << _T("+M");
      (*pTable)(1, col++) << _T("-M");
      (*pTable)(1, col++) << _T("V");
   }

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   PoiList vPoi;
   pIPoi->GetPointsOfInterest(CSegmentKey(girderKey, ALL_SEGMENTS), POI_SPAN, &vPoi);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );

   GET_IFACE2(pBroker,IReportOptions,pReportOptions);
   location.IncludeSpanAndGirder(pReportOptions->IncludeSpanAndGirder4Pois(girderKey));

   RowIndexType row = pTable->GetNumberOfHeaderRows();

   for (const pgsPointOfInterest& poi : vPoi)
   {
      col = 0;
      (*pTable)(row, col++) << location.SetValue( POI_SPAN, poi );

      Float64 pM, nM, V;
      pDistFact->GetDistributionFactors(poi,pgsTypes::StrengthI,&pM,&nM,&V);
      (*pTable)(row, col++) << df.SetValue(pM);

      if ( bNegMoments )
      {
         (*pTable)(row, col++) << df.SetValue(nM);
      }
      else
      {
         (*pTable)(row, col++) << _T("---");
      }
      (*pTable)(row, col++) << df.SetValue(V);

      if ( WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims <= WBFL::LRFD::BDSManager::GetEdition() )
      {
         pDistFact->GetDistributionFactors(poi,pgsTypes::FatigueI,&pM,&nM,&V);
         (*pTable)(row, col++) << df.SetValue(pM);

         if ( bNegMoments )
         {
            (*pTable)(row, col++) << df.SetValue(nM);
         }
         else
         {
            (*pTable)(row, col++) << _T("---");
         }

         (*pTable)(row, col++) << df.SetValue(V);
      }

      row++;
   }

   nCols = 2;
   if ( WBFL::LRFD::BDSManager::Edition::FourthEditionWith2009Interims <= WBFL::LRFD::BDSManager::GetEdition() )
   {
      nCols++;
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
