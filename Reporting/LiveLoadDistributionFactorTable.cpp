///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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
#include <IFace\DisplayUnits.h>
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
                                             IDisplayUnits* pDispUnits) const
{
   rptRcScalar df;
   df.SetFormat(sysNumericFormatTool::Fixed);
   df.SetWidth(8);
   df.SetPrecision(3); // should match format in details reports

   GET_IFACE2(pBroker,ILiveLoadDistributionFactors,pDistFact);
   GET_IFACE2(pBroker,IBridge,pBridge);

   PierIndexType startPier = span;
   PierIndexType endPier   = span+1;

   bool bContinuousLeft, bContinuousRight;
   bool bIntegralLeft, bIntegralRight;
   pBridge->IsContinuousAtPier(startPier,&bContinuousLeft,&bContinuousRight);
   pBridge->IsIntegralAtPier(startPier,&bIntegralLeft,&bIntegralRight);
   bool bNegMomentAtStart = (bContinuousRight || bIntegralRight);

   pBridge->IsContinuousAtPier(endPier,&bContinuousLeft,&bContinuousRight);
   pBridge->IsIntegralAtPier(endPier,&bIntegralLeft,&bIntegralRight);
   bool bNegMomentAtEnd = (bContinuousLeft || bIntegralLeft);

   bool bNegMomentDF = (bNegMomentAtStart || bNegMomentAtEnd);

   rptParagraph* pBody = new rptParagraph;
   *pChapter << pBody;

   ColumnIndexType nCols = 5;
   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      nCols += 4; // for fatigue limit state LLDF

   rptRcTable* pTable = pgsReportStyleHolder::CreateDefaultTable(nCols,"Live Load Distribution Factors");
   *pBody << pTable;


   if ( lrfdVersionMgr::GetVersion() < lrfdVersionMgr::FourthEditionWith2009Interims )
   {
      pTable->SetNumberOfHeaderRows(1);
      (*pTable)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDispUnits->GetSpanLengthUnit() );
      (*pTable)(0,1) << "+M";
      (*pTable)(0,2) << "-M";
      (*pTable)(0,3) << "V";
      (*pTable)(0,4) << "R";
   }
   else
   {
      pTable->SetNumberOfHeaderRows(3);
      pTable->SetRowSpan(0,0,3);
      (*pTable)(0,0) << COLHDR(RPT_LFT_SUPPORT_LOCATION,   rptLengthUnitTag, pDispUnits->GetSpanLengthUnit() );
      pTable->SetRowSpan(1,0,-1);
      pTable->SetRowSpan(2,0,-1);

      pTable->SetColumnSpan(0,1,8);
      (*pTable)(0,1) << "Limit State";
      pTable->SetColumnSpan(0,2,-1);
      pTable->SetColumnSpan(0,3,-1);
      pTable->SetColumnSpan(0,4,-1);
      pTable->SetColumnSpan(0,5,-1);
      pTable->SetColumnSpan(0,6,-1);
      pTable->SetColumnSpan(0,7,-1);
      pTable->SetColumnSpan(0,8,-1);

      pTable->SetColumnSpan(1,1,4);
      (*pTable)(1,1) << "Strength/Service";

      pTable->SetColumnSpan(1,2,4);
      (*pTable)(1,2) << "Fatigue";

      pTable->SetColumnSpan(1,3,-1);
      pTable->SetColumnSpan(1,4,-1);
      pTable->SetColumnSpan(1,5,-1);
      pTable->SetColumnSpan(1,6,-1);
      pTable->SetColumnSpan(1,7,-1);
      pTable->SetColumnSpan(1,8,-1);

      (*pTable)(2,1) << "+M";
      (*pTable)(2,2) << "-M";
      (*pTable)(2,3) << "V";
      (*pTable)(2,4) << "R";
      (*pTable)(2,5) << "+M";
      (*pTable)(2,6) << "-M";
      (*pTable)(2,7) << "V";
      (*pTable)(2,8) << "R";
   }

   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);

   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest(pgsTypes::BridgeSite3,span,girder, POI_TABULAR);
   Float64 end_size = pBridge->GetGirderStartConnectionLength(span,girder);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDispUnits->GetSpanLengthUnit(), false );


   RowIndexType row = pTable->GetNumberOfHeaderRows();

   std::vector<pgsPointOfInterest>::iterator iter;
   for ( iter = vPoi.begin(); iter != vPoi.end(); iter++ )
   {
      pgsPointOfInterest poi = *iter;
      (*pTable)(row,0) << location.SetValue( poi, end_size );

      double pM, nM, V;
      pDistFact->GetDistributionFactors(poi,pgsTypes::StrengthI,&pM,&nM,&V);
      (*pTable)(row,1) << df.SetValue(pM);

      if ( bNegMomentDF )
      {
         (*pTable)(row,2) << df.SetValue(nM);
      }
      else
      {
         (*pTable)(row,2) << "---";
      }
      (*pTable)(row,3) << df.SetValue(V);
      (*pTable)(row,4) << "";

      if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
      {
         pDistFact->GetDistributionFactors(poi,pgsTypes::FatigueI,&pM,&nM,&V);
         (*pTable)(row,5) << df.SetValue(pM);

         if ( bNegMomentDF )
         {
            (*pTable)(row,6) << df.SetValue(nM);
         }
         else
         {
            (*pTable)(row,6) << "---";
         }

         (*pTable)(row,7) << df.SetValue(V);
         (*pTable)(row,8) << "";
      }

      row++;
   }

   row = pTable->GetNumberOfHeaderRows();
   (*pTable)(row,4)               << df.SetValue(pDistFact->GetReactionDistFactor(startPier,girder,pgsTypes::StrengthI));
   (*pTable)(row+vPoi.size()-1,4) << df.SetValue(pDistFact->GetReactionDistFactor(endPier,  girder,pgsTypes::StrengthI));

   if ( lrfdVersionMgr::FourthEditionWith2009Interims <= lrfdVersionMgr::GetVersion() )
   {
      (*pTable)(row,8)               << df.SetValue(pDistFact->GetReactionDistFactor(startPier,girder,pgsTypes::FatigueI));
      (*pTable)(row+vPoi.size()-1,8) << df.SetValue(pDistFact->GetReactionDistFactor(endPier,  girder,pgsTypes::FatigueI));
   }


   GET_IFACE2(pBroker,ILiveLoads, pLiveLoads);
   std::string straction = pLiveLoads->GetLLDFSpecialActionText();
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
