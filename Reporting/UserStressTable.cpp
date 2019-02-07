///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <Reporting\UserStressTable.h>
#include <Reporting\UserMomentsTable.h>
#include <Reporting\ReportNotes.h>

#include <PgsExt\ReportPointOfInterest.h>

#include <IFace\Bridge.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/****************************************************************************
CLASS
   CUserStressTable
****************************************************************************/


////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CUserStressTable::CUserStressTable()
{
}

CUserStressTable::CUserStressTable(const CUserStressTable& rOther)
{
   MakeCopy(rOther);
}

CUserStressTable::~CUserStressTable()
{
}

//======================== OPERATORS  =======================================
CUserStressTable& CUserStressTable::operator= (const CUserStressTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CUserStressTable::Build(IBroker* pBroker,const CGirderKey& girderKey,pgsTypes::AnalysisType analysisType,IntervalIndexType intervalIdx,
                                      IEAFDisplayUnits* pDisplayUnits,bool bGirderStresses) const
{
   pgsTypes::StressLocation topLocation = (bGirderStresses ? pgsTypes::TopGirder    : pgsTypes::TopDeck);
   pgsTypes::StressLocation botLocation = (bGirderStresses ? pgsTypes::BottomGirder : pgsTypes::BottomDeck);

   INIT_UV_PROTOTYPE( rptPointOfInterest, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptStressUnitValue, stress, pDisplayUnits->GetStressUnit(), false );

   location.IncludeSpanAndGirder(girderKey.groupIndex == ALL_GROUPS ? true : false);

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   CString strTitle;
   strTitle.Format(_T("%s Stresses due to User Defined Loads in Interval %d: %s"),(bGirderStresses ? _T("Girder") : _T("Deck")),LABEL_INTERVAL(intervalIdx),pIntervals->GetDescription(intervalIdx));
   rptRcTable* p_table = CreateUserLoadHeading<rptStressUnitTag,unitmgtStressData>(strTitle.GetBuffer(),false,analysisType,intervalIdx,pDisplayUnits,pDisplayUnits->GetStressUnit());

   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      p_table->SetColumnStyle(0,rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
      p_table->SetStripeRowColumnStyle(0,rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));
   }

   // Get the interface pointers we need
   GET_IFACE2(pBroker,IPointOfInterest,pIPoi);
   GET_IFACE2(pBroker,IProductForces2,pForces2);
   GET_IFACE2(pBroker,IBridge,pBridge);

   GET_IFACE2(pBroker,IProductForces,pForces);
   pgsTypes::BridgeAnalysisType maxBAT = pForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   RowIndexType row = p_table->GetNumberOfHeaderRows();
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = (nGirders <= girderKey.girderIndex ? nGirders-1 : girderKey.girderIndex);

      PoiList vPoi;
      pIPoi->GetPointsOfInterest(CSegmentKey(grpIdx, gdrIdx, ALL_SEGMENTS), POI_ERECTED_SEGMENT, &vPoi);

      std::vector<Float64> dummy;
      std::vector<Float64> fTopMaxDC, fBotMaxDC;
      std::vector<Float64> fTopMinDC, fBotMinDC;
      std::vector<Float64> fTopMaxDW, fBotMaxDW;
      std::vector<Float64> fTopMinDW, fBotMinDW;
      std::vector<Float64> fTopMaxLLIM, fBotMaxLLIM;
      std::vector<Float64> fTopMinLLIM, fBotMinLLIM;

      pForces2->GetStress(intervalIdx, pgsTypes::pftUserDC, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopMaxDC, &fBotMaxDC);
      pForces2->GetStress(intervalIdx, pgsTypes::pftUserDC, vPoi, minBAT, rtIncremental, topLocation, botLocation, &fTopMinDC, &fBotMinDC);

      pForces2->GetStress(intervalIdx, pgsTypes::pftUserDW, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopMaxDW, &fBotMaxDW);
      pForces2->GetStress(intervalIdx, pgsTypes::pftUserDW, vPoi, minBAT, rtIncremental, topLocation, botLocation, &fTopMinDW, &fBotMinDW);

      pForces2->GetStress(intervalIdx, pgsTypes::pftUserLLIM, vPoi, maxBAT, rtIncremental, topLocation, botLocation, &fTopMaxLLIM, &fBotMaxLLIM);
      pForces2->GetStress(intervalIdx, pgsTypes::pftUserLLIM, vPoi, minBAT, rtIncremental, topLocation, botLocation, &fTopMinLLIM, &fBotMinLLIM);

      IndexType index = 0;
      for(const pgsPointOfInterest& poi : vPoi)
      {
         ColumnIndexType col = 0;

         (*p_table)(row,col++) << location.SetValue( POI_SPAN, poi );

         if ( analysisType == pgsTypes::Envelope )
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDC[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDC[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMinDC[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMinDC[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDW[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDW[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMinDW[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMinDW[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxLLIM[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxLLIM[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMinLLIM[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMinLLIM[index] );
         }
         else
         {
            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDC[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDC[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxDW[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxDW[index] );

            (*p_table)(row,col  ) << RPT_FTOP << _T(" = ") << stress.SetValue( fTopMaxLLIM[index] ) << rptNewLine;
            (*p_table)(row,col++) << RPT_FBOT << _T(" = ") << stress.SetValue( fBotMaxLLIM[index] );
         }

         row++;
         index++;
      }
   }

   return p_table;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CUserStressTable::MakeCopy(const CUserStressTable& rOther)
{
   // Add copy code here...
}

void CUserStressTable::MakeAssignment(const CUserStressTable& rOther)
{
   MakeCopy( rOther );
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PRIVATE    ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
//======================== ACCESS     =======================================
//======================== INQUERY    =======================================
