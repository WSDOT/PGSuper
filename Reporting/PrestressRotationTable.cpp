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
#include <Reporting\PrestressRotationTable.h>
#include <Reporting\ReactionInterfaceAdapters.h>

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Intervals.h>
#include <IFace\Project.h>

#include <PgsExt\PrecastSegmentData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//======================== LIFECYCLE  =======================================
CPrestressRotationTable::CPrestressRotationTable()
{
}

CPrestressRotationTable::CPrestressRotationTable(const CPrestressRotationTable& rOther)
{
   MakeCopy(rOther);
}

CPrestressRotationTable::~CPrestressRotationTable()
{
}

//======================== OPERATORS  =======================================
CPrestressRotationTable& CPrestressRotationTable::operator= (const CPrestressRotationTable& rOther)
{
   if( this != &rOther )
   {
      MakeAssignment(rOther);
   }

   return *this;
}

//======================== OPERATIONS =======================================
rptRcTable* CPrestressRotationTable::Build(IBroker* pBroker,const CGirderKey& girderKey, pgsTypes::AnalysisType analysisType, IntervalIndexType intervalIdx, IEAFDisplayUnits* pDisplayUnits) const
{
   // Build table
   INIT_UV_PROTOTYPE( rptLengthUnitValue, location, pDisplayUnits->GetSpanLengthUnit(), false );
   INIT_UV_PROTOTYPE( rptAngleUnitValue, rotation, pDisplayUnits->GetRadAngleUnit(), false );

   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2(pBroker, IProductLoads, pProductLoads);
   GET_IFACE2(pBroker, IPointOfInterest, pPOI);
   GET_IFACE2(pBroker, IProductForces, pProdForces);

   GET_IFACE2_NOCHECK(pBroker, ICamber, pCamber);

   GET_IFACE2(pBroker, IGirderTendonGeometry, pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   ColumnIndexType nCols = 2;
   if (0 < nDucts)
   {
      nCols++;
   }

   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   bool bTimeStep = (pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? true : false);
   if (bTimeStep)
   {
      nCols += 3;
   }
   else
   {
      nCols++;
   }



   rptRcTable* pTable = rptStyleManager::CreateDefaultTable(nCols, _T("Prestress and Time-Dependent Rotations"));

   pTable->SetColumnStyle(0, rptStyleManager::GetTableCellStyle(CB_NONE | CJ_LEFT));
   pTable->SetStripeRowColumnStyle(0, rptStyleManager::GetTableStripeRowCellStyle(CB_NONE | CJ_LEFT));

   ColumnIndexType col = 0;
   (*pTable)(0, col++) << _T("");
   (*pTable)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftPretension), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   if (0 < nDucts)
   {
      (*pTable)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftPostTensioning), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   }
   if (bTimeStep)
   {
      (*pTable)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftCreep), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
      (*pTable)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftShrinkage), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
      (*pTable)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftRelaxation), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   }
   else
   {
      (*pTable)(0, col++) << COLHDR(pProductLoads->GetProductLoadName(pgsTypes::pftCreep), rptAngleUnitTag, pDisplayUnits->GetRadAngleUnit());
   }


   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(startGroupIdx);

   pgsTypes::BridgeAnalysisType maxBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Maximize);
   pgsTypes::BridgeAnalysisType minBAT = pProdForces->GetBridgeAnalysisType(analysisType,pgsTypes::Minimize);

   // get poi at start and end of each segment in the girder
   PoiList vPoi;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      GirderIndexType gdrIdx = Min( girderKey.girderIndex, pBridge->GetGirderCount(grpIdx)-1 );

      // don't report girders that don't exist on bridge
      SegmentIndexType nSegments = pBridge->GetSegmentCount(CGirderKey(grpIdx,gdrIdx));
      CSegmentKey segmentKey(grpIdx, gdrIdx, 0);
      PoiList vSegPoi;
      pPOI->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSegPoi);
      ATLASSERT(vSegPoi.size() == 1);
      vPoi.insert(vPoi.end(), vSegPoi.begin(), vSegPoi.end());

      segmentKey.segmentIndex = nSegments-1;
      vSegPoi.clear();
      pPOI->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSegPoi);
      ATLASSERT(vSegPoi.size() == 1);
      vPoi.insert(vPoi.end(), vSegPoi.begin(), vSegPoi.end());
   }

   GET_IFACE2(pBroker,IBridgeDescription, pIBridgeDesc);
   PierIndexType nPiers = pBridge->GetPierCount();

   GET_IFACE2(pBroker, IIntervals, pIntervals);
   RowIndexType row = pTable->GetNumberOfHeaderRows();
   for(const pgsPointOfInterest& poi : vPoi)
   {
      ColumnIndexType col = 0;

      const CSegmentKey& segmentKey(poi.GetSegmentKey());
      const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      const CPierData2* pPier;
      const CTemporarySupportData* pTS;
      pgsTypes::MemberEndType endType = (poi.IsTenthPoint(POI_ERECTED_SEGMENT) == 1 ? pgsTypes::metStart : pgsTypes::metEnd);
      pSegment->GetSupport(endType, &pPier, &pTS);
      ATLASSERT(pTS == nullptr);

      std::_tstring strName(pPier->GetIndex() == 0 || pPier->GetIndex() == nPiers - 1 ? _T("Abutment") : _T("Pier"));
      std::_tstring strFace(endType == pgsTypes::metStart ? _T("Ahead") : _T("Back"));
      (*pTable)(row, col++) << strName << _T(" ") << LABEL_PIER(pPier->GetIndex()) << _T(" - ") << strFace;

      (*pTable)(row, col++) << rotation.SetValue(pProdForces->GetRotation(releaseIntervalIdx, pgsTypes::pftPretension, poi, maxBAT, rtCumulative, false));
      if (0 < nDucts)
      {
         (*pTable)(row, col++) << rotation.SetValue(pProdForces->GetRotation(intervalIdx, pgsTypes::pftPostTensioning, poi, maxBAT, rtCumulative, false));
      }

      if (bTimeStep)
      {
         (*pTable)(row, col++) << rotation.SetValue(pProdForces->GetRotation(intervalIdx, pgsTypes::pftCreep, poi, maxBAT, rtCumulative, false));
         (*pTable)(row, col++) << rotation.SetValue(pProdForces->GetRotation(intervalIdx, pgsTypes::pftShrinkage, poi, maxBAT, rtCumulative, false));
         (*pTable)(row, col++) << rotation.SetValue(pProdForces->GetRotation(intervalIdx, pgsTypes::pftRelaxation, poi, maxBAT, rtCumulative, false));
      }
      else
      {
         Float64 Dcreep1, Rcreep1;
         pCamber->GetCreepDeflection(poi, ICamber::cpReleaseToDeck, pgsTypes::CreepTime::Max, pgsTypes::pddErected, nullptr, &Dcreep1, &Rcreep1);

         //Float64 Dcreep2, Rcreep2;
         //pCamber->GetCreepDeflection(poi, ICamber::cpDiaphragmToDeck, pgsTypes::CreepTime::Max, pgsTypes::pddErected, nullptr, &Dcreep2, &Rcreep2);
         (*pTable)(row, col++) << rotation.SetValue(Rcreep1 /*+ Rcreep2*/);
      }

      row++;
   }

   return pTable;
}

//======================== ACCESS     =======================================
//======================== INQUIRY    =======================================

////////////////////////// PROTECTED  ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CPrestressRotationTable::MakeCopy(const CPrestressRotationTable& rOther)
{
   // Add copy code here...
}

void CPrestressRotationTable::MakeAssignment(const CPrestressRotationTable& rOther)
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
