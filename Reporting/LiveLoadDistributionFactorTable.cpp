///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <EAF\EAFDisplayUnits.h>
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


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
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

//======================== OPERATORS  =======================================
CLiveLoadDistributionFactorTable& CLiveLoadDistributionFactorTable::operator= (const CLiveLoadDistributionFactorTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
void CLiveLoadDistributionFactorTable::Build(rptChapter* pChapter,
                                             IBroker* pBroker,SpanIndexType span,GirderIndexType girder,
                                             IEAFDisplayUnits* pDisplayUnits) const
{
   INIT_SCALAR_PROTOTYPE(rptRcScalar, df, pDisplayUnits->GetScalarFormat());

   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   bool bNegMoments = pBridge->ProcessNegativeMoments(span);

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   ColumnIndexType nCols = 5;
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      nCols += 4; // for fatigue limit state LLDF

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,_T("Live Load Distribution Factors"));
   *pBody << pTable;

   if ( span == ALL_SPANS )
   {
      pTable->SetColumnStyle(0,pgsReportStyleHolder::GetTableCellStyle(CB_NONE | CJ_LEFT));
      pTable->SetStripeRowColumnStyle(0,pgsReportStyleHolder::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }


   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pTable->SetNumberOfHeaderRows(1);
      (*pTable)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      (*pTable)(0,1) << _T("+M");
      (*pTable)(0,2) << _T("-M");
      (*pTable)(0,3) << _T("V");
      (*pTable)(0,4) << _T("R");
   }
   else
   {
      pTable->SetNumberOfHeaderRows(2);
      pTable->SetRowSpan(0,0,2);
      (*pTable)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDisplayUnits->GetSpanLengthUnit() );
      pTable->SetRowSpan(1,0,SKIP_CELL);

      pTable->SetColumnSpan(0,1,4);
      (*pTable)(0,1) << _T("Strength/Service");

      pTable->SetColumnSpan(0,2,4);
      (*pTable)(0,2) << _T("Fatigue/One Lane");

      pTable->SetColumnSpan(0,3,SKIP_CELL);
      pTable->SetColumnSpan(0,4,SKIP_CELL);
      pTable->SetColumnSpan(0,5,SKIP_CELL);
      pTable->SetColumnSpan(0,6,SKIP_CELL);
      pTable->SetColumnSpan(0,7,SKIP_CELL);
      pTable->SetColumnSpan(0,8,SKIP_CELL);

      (*pTable)(1,1) << _T("+M");
      (*pTable)(1,2) << _T("-M");
      (*pTable)(1,3) << _T("V");
      (*pTable)(1,4) << _T("R");
      (*pTable)(1,5) << _T("+M");
      (*pTable)(1,6) << _T("-M");
      (*pTable)(1,7) << _T("V");
      (*pTable)(1,8) << _T("R");
   }

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(span, girder, pgsTypes::BridgeSite3, POI_TABULAR);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   location.IncludeSpanAndGirder(span == ALL_SPANS);


   RowIndexType row = pTable->GetNumberOfHeaderRows();

   SpanIndexType last_span_idx = 0;
   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      pgsPointOfInterest poi = *iter;
      Float64 end_size = pBridge->GetGirderStartConnectionLength(poi.GetSpan(),poi.GetGirder());
      (*pTable)(row,0) << location.SetValue( pgsTypes::BridgeSite3, poi, end_size );

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

      if ( iter == vPoi.begin() || last_span_idx != poi.GetSpan() )
      {
         last_span_idx = poi.GetSpan();
         (*pTable)(row,4) << df.SetValue(pDistFact->GetReactionDistFactor(poi.GetSpan(),poi.GetGirder(),pgsTypes::StrengthI));
         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*pTable)(row,8) << df.SetValue(pDistFact->GetReactionDistFactor(poi.GetSpan(),poi.GetGirder(),pgsTypes::FatigueI));
         }
      }
      else
      {
         (*pTable)(row,4) << _T("");
         if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
         {
            (*pTable)(row,8) << _T("");
         }
      }

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pDistFact->GetDistributionFactors(poi,pgsTypes::FatigueI,&pM,&nM,&V);
         (*pTable)(row,5) << df.SetValue(pM);

         if ( bNegMoments )
         {
            (*pTable)(row,6) << df.SetValue(nM);
         }
         else
         {
            (*pTable)(row,6) << _T("---");
         }

         (*pTable)(row,7) << df.SetValue(V);
      }

      row++;
   }

   (*pTable)(row-1,4) << df.SetValue(pDistFact->GetReactionDistFactor(vPoi.back().GetSpan()+1,vPoi.back().GetGirder(),pgsTypes::StrengthI));
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      (*pTable)(row-1,8) << df.SetValue(pDistFact->GetReactionDistFactor(vPoi.back().GetSpan()+1,vPoi.back().GetGirder(),pgsTypes::FatigueI));
   }


   GET_IFACE2(pBroker,ILiveLoads, pLiveLoads);
   std::_tstring straction = pLiveLoads->GetLLDFSpecialActionText();
   if ( !straction.empty() )
   {
      (*pBody) << color(Red) << straction << color(Black) << rptNewLine;
   }
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CLiveLoadDistributionFactorTable::MakeCopy(const CLiveLoadDistributionFactorTable& rOther)
{
   // Add copy code here...
}

void CLiveLoadDistributionFactorTable::MakeAssignment(const CLiveLoadDistributionFactorTable& rOther)
{
   MakeCopy( rOther );
}
