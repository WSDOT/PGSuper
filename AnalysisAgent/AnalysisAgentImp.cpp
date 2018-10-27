///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

// AnalysisAgentImp.cpp : Implementation of CAnalysisAgent
#include "stdafx.h"
#include "AnalysisAgent.h"
#include "AnalysisAgentImp.h"
#include "StatusItems.h"
#include "..\PGSuperException.h"

#include <IFace\Intervals.h>
#include <IFace\GirderHandling.h>

#include <WBFLSTL.h>
#include <MathEx.h>

#include <PgsExt\LoadFactors.h>
#include <PgsExt\DebondUtil.h>
#include <PgsExt\GirderModelFactory.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderMaterial.h>
#include <PgsExt\StrandData.h>
#include <PgsExt\ClosureJointData.h>

#include <PgsExt\GirderLabel.h>

#include <iterator>
#include <algorithm>

#pragma Reminder("NOTES: Possible New Features") 
// 1) IExternalLoading: Add CreateDistributedLoad. 
//    Distributed load is the more general version and it is supported by the FEM model...

//////////////////////////////////////////////////////////////////////////////////
// NOTES:
//
// The pedestrian live load distribution factor is the pedestrian load.

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// FEM Loading IDs
const LoadCaseIDType g_lcidGirder           = 1;
const LoadCaseIDType g_lcidStraightStrandX  = 2; // equiv. straight strand loading for moments about the x axis
const LoadCaseIDType g_lcidStraightStrandY  = 3; // equiv. straight strand loading for moments about the y axis
const LoadCaseIDType g_lcidHarpedStrandX    = 4; // equiv. harped strand loading for moments about the x axis
const LoadCaseIDType g_lcidHarpedStrandY    = 5; // equiv. harped strand loading for moments about the y axis
const LoadCaseIDType g_lcidTemporaryStrandX = 6; // equiv. temporary strand loading for moments about the x axis
const LoadCaseIDType g_lcidTemporaryStrandY = 7; // equiv. temporary strand loading for moments about the y axis

const LoadCaseIDType g_lcidDesignSlab       = 2;
const LoadCaseIDType g_lcidDesignSlabPad    = 3;

void GetPrestressLoadCaseIDs(pgsTypes::StrandType strandType, LoadCaseIDType* plcidX, LoadCaseIDType* plcidY)
{
   switch (strandType)
   {
   case pgsTypes::Straight:
      *plcidX = g_lcidStraightStrandX;
      *plcidY = g_lcidStraightStrandY;
      break;

   case pgsTypes::Harped:
      *plcidX = g_lcidHarpedStrandX;
      *plcidY = g_lcidHarpedStrandY;
      break;

   case pgsTypes::Temporary:
      *plcidX = g_lcidTemporaryStrandX;
      *plcidY = g_lcidTemporaryStrandY;
      break;

   default:
      ATLASSERT(false);
      *plcidX = INVALID_ID;
      *plcidY = INVALID_ID;
   }
}

#define TOP 0
#define BOT 1

CAnalysisAgentImp::CAnalysisAgentImp()
{
   m_Level = 0;
}

UINT CAnalysisAgentImp::DeleteSegmentModelManager(LPVOID pParam)
{
   WATCH(_T("Begin: DeleteSegmentModelManager"));

   CSegmentModelManager* pModelMgr = (CSegmentModelManager*)(pParam);
   pModelMgr->Clear();
   delete pModelMgr;

   WATCH(_T("End: DeleteSegmentModelManager"));

   return 0; // success... returning terminates the thread
}

UINT CAnalysisAgentImp::DeleteGirderModelManager(LPVOID pParam)
{
   WATCH(_T("Begin: DeleteGirderModelManager"));

   CGirderModelManager* pModelMgr = (CGirderModelManager*)(pParam);
   pModelMgr->Clear();
   delete pModelMgr;

   WATCH(_T("End: DeleteGirderModelManager"));

   return 0; // success... returning terminates the thread
}

HRESULT CAnalysisAgentImp::FinalConstruct()
{
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// CAnalysisAgent
void CAnalysisAgentImp::Invalidate(bool clearStatus)
{
   // use a worker thread to delete the models
   CSegmentModelManager* pOldSegmentModels = m_pSegmentModelManager.release();
   m_pSegmentModelManager = std::make_unique<CSegmentModelManager>(LOGGER,m_pBroker);

   CGirderModelManager* pOldGirderModels = m_pGirderModelManager.release();
   m_pGirderModelManager = std::make_unique<CGirderModelManager>(LOGGER,m_pBroker,m_StatusGroupID);

#if defined _USE_MULTITHREADING
   m_ThreadManager.CreateThread(CAnalysisAgentImp::DeleteSegmentModelManager,(LPVOID)(pOldSegmentModels));
   m_ThreadManager.CreateThread(CAnalysisAgentImp::DeleteGirderModelManager, (LPVOID)(pOldGirderModels));
#else
   CAnalysisAgentImp::DeleteSegmentModelManager((LPVOID)pOldSegmentModels);
   CAnalysisAgentImp::DeleteGirderModelManager((LPVOID)pOldGirderModels);
#endif

   InvalidateCache();

   InvalidateCamberModels();

   InValidateSlabOffsetDesignModel();

   for ( int i = 0; i < 6; i++ )
   {
      m_CreepCoefficientDetails[CREEP_MINTIME][i].clear();
      m_CreepCoefficientDetails[CREEP_MAXTIME][i].clear();
   }

   if (clearStatus)
   {
      GET_IFACE(IEAFStatusCenter,pStatusCenter);
      pStatusCenter->RemoveByStatusGroupID(m_StatusGroupID);
   }

   m_ExternalLoadState.clear();
}

void CAnalysisAgentImp::InvalidateCamberModels()
{
   m_PrestressDeflectionModels.clear();
   m_InitialTempPrestressDeflectionModels.clear();
   m_ReleaseTempPrestressDeflectionModels.clear();
}

void CAnalysisAgentImp::ValidateCamberModels(const CSegmentKey& segmentKey) const
{
   CamberModelData camber_model_data = BuildCamberModel(segmentKey);

   m_PrestressDeflectionModels.insert( std::make_pair(segmentKey,camber_model_data) );

   CamberModelData initial_temp_beam;
   CamberModelData release_temp_beam;
   BuildTempCamberModel(segmentKey,nullptr,&initial_temp_beam,&release_temp_beam);
   m_InitialTempPrestressDeflectionModels.insert( std::make_pair(segmentKey,initial_temp_beam) );
   m_ReleaseTempPrestressDeflectionModels.insert( std::make_pair(segmentKey,release_temp_beam) );
}

void CAnalysisAgentImp::ValidateCamberModels(const GDRCONFIG* pConfig) const
{
   ATLASSERT(pConfig != nullptr);
   if ( *pConfig != m_CacheConfig )
   {
      m_CacheConfig_PrestressDeflectionModel.Model.Release();
      m_CacheConfig_PrestressDeflectionModel.PoiMap.Clear();

      m_CacheConfig_InitialTempPrestressDeflectionModels.Model.Release();
      m_CacheConfig_InitialTempPrestressDeflectionModels.PoiMap.Clear();

      m_CacheConfig_ReleaseTempPrestressDeflectionModels.Model.Release();
      m_CacheConfig_ReleaseTempPrestressDeflectionModels.PoiMap.Clear();

      const CSegmentKey& segmentKey = pConfig->SegmentKey;
      ASSERT_SEGMENT_KEY(segmentKey);
      m_CacheConfig_PrestressDeflectionModel = BuildCamberModel(segmentKey,pConfig);
      BuildTempCamberModel(segmentKey,pConfig,&m_CacheConfig_InitialTempPrestressDeflectionModels,&m_CacheConfig_ReleaseTempPrestressDeflectionModels);

      m_CacheConfig = *pConfig;
   }
}

void CAnalysisAgentImp::InvalidateCache()
{
   GDRCONFIG defaultConfig;
   m_CacheConfig = defaultConfig;

   CamberModelData defaultModelData;
   m_CacheConfig_PrestressDeflectionModel = defaultModelData;
   m_CacheConfig_InitialTempPrestressDeflectionModels = defaultModelData;
   m_CacheConfig_ReleaseTempPrestressDeflectionModels = defaultModelData;
}

CAnalysisAgentImp::CamberModelData CAnalysisAgentImp::GetPrestressDeflectionModel(const CSegmentKey& segmentKey,CamberModels& models) const
{
   auto found = models.find( segmentKey );

   if ( found == models.end() )
   {
      ValidateCamberModels(segmentKey);
      found = models.find( segmentKey );
   }

   // Model should have already been created in ValidateCamberModels
   ATLASSERT( found != models.cend() );

   return (*found).second;
}

std::vector<EquivPretensionLoad> CAnalysisAgentImp::GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType, const GDRCONFIG* pConfig,bool bTempStrandInstallation,IntervalIndexType intervalIdx) const
{
   std::vector<EquivPretensionLoad> equivLoads;
   equivLoads.reserve(12);

   Float64 Mslx;  // Concentrated x-moments at straight strand bond locations (left)
   Float64 Msly;  // Concentrated y-moments at straight strand bond locations (left)
   Float64 Msrx;  // Concentrated x-moments at straight strand bond locations (right)
   Float64 Msry;  // Concentrated y-moments at straight strand bond locations (right)
   Float64 Mhlx;  // Concentrated x-moments at ends of beam for eccentric prestress forces from harped strands (left)
   Float64 Mhly;  // Concentrated y-moments at ends of beam for eccentric prestress forces from harped strands (left)
   Float64 Mhrx;  // Concentrated x-moments at ends of beam for eccentric prestress forces from harped strands (right)
   Float64 Mhry;  // Concentrated y-moments at ends of beam for eccentric prestress forces from harped strands (right)
   Float64 Mtlx;  // Concentrated x-moments at temporary straight strand bond locations (left)
   Float64 Mtly;  // Concentrated y-moments at temporary straight strand bond locations (left)
   Float64 Mtrx;  // Concentrated x-moments at temporary straight strand bond locations (right)
   Float64 Mtry;  // Concentrated y-moments at temporary straight strand bond locations (right)
   Float64 Nl;    // Vertical loads at left harping point
   Float64 Nr;    // Vertical loads at right harping point
   Float64 wy;    // Vertical distributed load associated with precamber and "straight" (which are actually curved) strands that follow the precamber shape
   Float64 PsStart;  // Force in straight strands (varies with location due to debonding)
   Float64 PsEnd;    // Force in straight strands (varies with location due to debonding)
   Float64 Ph;    // Force in harped strands
   Float64 Pt;    // Force in temporary strands
   Float64 ecc_x_harped_start; // x eccentricity of harped strands at end of girder
   Float64 ecc_y_harped_start; // y eccentricity of harped strands at end of girder
   Float64 ecc_x_harped_end;  // x eccentricity of harped strands at end of girder
   Float64 ecc_y_harped_end;  // y eccentricity of harped strands at end of girder
   Float64 ecc_y_harped_hp1;  // y eccentricity of harped strand at harping point (left)
   Float64 ecc_y_harped_hp2;  // y eccentricity of harped strand at harping point (right)
   Float64 ecc_x_straight_start;  // x eccentricity of straight strands (left)
   Float64 ecc_x_straight_end;    // x eccentricity of straight strands (right)
   Float64 ecc_y_straight_start;  // y eccentricity of straight strands (left)
   Float64 ecc_y_straight_end;    // y eccentricity of straight strands (right)
   Float64 ecc_x_temporary_start; // x eccentricity of temporary strands (left)
   Float64 ecc_y_temporary_start; // y eccentricity of temporary strands (left)
   Float64 ecc_x_temporary_end;   // x eccentricity of temporary strands (right)
   Float64 ecc_y_temporary_end;   // y xccentricity of temporary strands (right)
   Float64 hp1; // Location of left harping point
   Float64 hp2; // Location of right harping point
   Float64 Ls;  // Length of segment
   Float64 Ybl; // Ybottom at left end of girder
   Float64 Ybm; // Ybottom at middle of girder
   Float64 Ybr; // Ybottom at right end of girder

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType liftingIntervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   if ( intervalIdx == INVALID_INDEX )
   {
      intervalIdx = releaseIntervalIdx;
   }

   GET_IFACE(IBridge, pBridge);
   Ls = pBridge->GetSegmentLength(segmentKey);

   GET_IFACE(IGirder, pGirder);
   Float64 precamber = pGirder->GetPrecamber(segmentKey);

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);

   GET_IFACE(IPointOfInterest, pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_5L | POI_10L | POI_RELEASED_SEGMENT, &vPoi);
   ATLASSERT( vPoi.size() == 3 );
   const pgsPointOfInterest& poiStart(vPoi[0]);
   const pgsPointOfInterest& poiMiddle(vPoi[1]);
   const pgsPointOfInterest& poiEnd(vPoi[2]);

   GET_IFACE(ISectionProperties, pSectProps);
   Ybl = pSectProps->GetY(releaseIntervalIdx, poiStart,  pgsTypes::BottomGirder);
   Ybm = pSectProps->GetY(releaseIntervalIdx, poiMiddle, pgsTypes::BottomGirder);
   Ybr = pSectProps->GetY(releaseIntervalIdx, poiEnd,    pgsTypes::BottomGirder);

#if defined _DEBUG
   ATLASSERT( poiMiddle.IsMidSpan(POI_RELEASED_SEGMENT) == true );
#endif

   GET_IFACE(IStrandGeometry, pStrandGeom);
   StrandIndexType Ns = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Straight, pConfig);
   StrandIndexType Nh = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Harped, pConfig);
   StrandIndexType Nt = pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary, pConfig);

   if ( strandType == pgsTypes::Harped && 0 < Nh )
   {
      hp1 = 0;
      hp2 = 0;
      Nl  = 0;
      Nr  = 0;
      Mhlx = 0;
      Mhly = 0;
      Mhrx = 0;
      Mhry = 0;

      // Determine the prestress force
      GET_IFACE(IPretensionForce,pPrestressForce);
      Ph = pPrestressForce->GetPrestressForce(poiMiddle, pgsTypes::Harped, releaseIntervalIdx, pgsTypes::End, pConfig);

      // get harping point locations
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_HARPINGPOINT, &vPoi);
      ATLASSERT( 0 <= vPoi.size() && vPoi.size() <= 2 );
      pgsPointOfInterest hp1_poi;
      pgsPointOfInterest hp2_poi;
      if ( vPoi.size() == 0 )
      {
         hp1_poi.SetSegmentKey(segmentKey);
         hp1_poi.SetDistFromStart(0.0);
         hp2_poi.SetSegmentKey(segmentKey);
         hp2_poi.SetDistFromStart(0.0);
      }
      else if ( vPoi.size() == 1 )
      { 
         hp1_poi = vPoi.front();
         hp2_poi = hp1_poi;
      }
      else
      {
         hp1_poi = vPoi.front();
         hp2_poi = vPoi.back();
      }

      hp1 = hp1_poi.GetDistFromStart();
      hp2 = hp2_poi.GetDistFromStart();

      // Determine eccentricity of harped strands at end and harp point
      // (assumes eccentricities are the same at each harp point - which they are because
      // of the way the input is defined)
      Float64 nHs_effective;

      pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Harped, pConfig, &nHs_effective, &ecc_x_harped_start, &ecc_y_harped_start);
      ecc_y_harped_hp1 = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp1_poi,  pgsTypes::Harped, pConfig, &nHs_effective);
      ecc_y_harped_hp2 = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp2_poi,  pgsTypes::Harped, pConfig, &nHs_effective);
      pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   pgsTypes::Harped, pConfig, &nHs_effective,&ecc_x_harped_end,&ecc_y_harped_end);

      // precamber at the harp points
      Float64 pc1 = pGirder->GetPrecamber(hp1_poi);
      Float64 pc2 = pGirder->GetPrecamber(hp2_poi);

      // Ybottom at the harp points
      Float64 Yb1 = pSectProps->GetY(releaseIntervalIdx, hp1_poi, pgsTypes::BottomGirder);
      Float64 Yb2 = pSectProps->GetY(releaseIntervalIdx, hp2_poi, pgsTypes::BottomGirder);

      // Determine equivalent loads

      // moment
      if (pSegment->TopFlangeThickeningType == pgsTypes::tftNone)
      {
         Mhlx = Ph*(ecc_y_harped_start + 2 * precamber / 3);
         Mhrx = Ph*(ecc_y_harped_end + 2 * precamber / 3);
      }
      else
      {
         Mhlx = Ph*(ecc_y_harped_start + 2 * (Ybm - Ybl + precamber) / 3);
         Mhrx = Ph*(ecc_y_harped_end + 2 * (Ybm - Ybr + precamber) / 3);
      }
      Mhly = Ph*ecc_x_harped_start;
      Mhry = Ph*ecc_x_harped_end;

      // upward force
      Float64 e_prime_start, e_prime_end;
      if (pSegment->TopFlangeThickeningType == pgsTypes::tftNone)
      {
         e_prime_start = (ecc_y_harped_hp1 - ecc_y_harped_start) - pc1;
         e_prime_end = (ecc_y_harped_hp2 - ecc_y_harped_end) - pc2;
      }
      else
      {
         e_prime_start = (ecc_y_harped_hp1 - ecc_y_harped_start) + (Ybl - Yb1) - pc1;
         e_prime_end = (ecc_y_harped_hp2 - ecc_y_harped_end) + (Ybr - Yb2) - pc2;
      }
      e_prime_start = IsZero(e_prime_start) ? 0 : e_prime_start;
      e_prime_end = IsZero(e_prime_end) ? 0 : e_prime_end;

      Nl = IsZero(hp1)    ? 0 : Ph*e_prime_start/hp1;
      Nr = IsZero(Ls-hp2) ? 0 : Ph*e_prime_end/(Ls-hp2);

      EquivPretensionLoad startMoment;
      startMoment.Xs = 0;
      startMoment.P  = Ph;
      startMoment.Ybe = Ybl;
      startMoment.Ybm = Ybm;
      startMoment.ex = ecc_x_harped_start;
      startMoment.eye = ecc_y_harped_start;
      startMoment.Precamber = precamber;
      startMoment.Mx = Mhlx;
      startMoment.My = Mhly;

      EquivPretensionLoad leftHpLoad;
      leftHpLoad.Xs = hp1;
      leftHpLoad.P = Ph;
      leftHpLoad.b = hp1;
      leftHpLoad.Ybe = Ybl;
      leftHpLoad.Ybh = Yb1;
      leftHpLoad.eye = ecc_y_harped_start;
      leftHpLoad.eyh = ecc_y_harped_hp1;
      leftHpLoad.eprime = e_prime_start;
      leftHpLoad.PrecamberAtHarpPoint = pc1;
      leftHpLoad.Precamber = precamber;
      leftHpLoad.N  = Nl;

      EquivPretensionLoad rightHpLoad;
      rightHpLoad.Xs = hp2;
      rightHpLoad.P = Ph;
      rightHpLoad.b = Ls-hp2;
      rightHpLoad.Ybe = Ybr;
      rightHpLoad.Ybh = Yb2;
      rightHpLoad.eye = ecc_y_harped_end;
      rightHpLoad.eyh = ecc_y_harped_hp2;
      rightHpLoad.eprime = e_prime_end;
      rightHpLoad.PrecamberAtHarpPoint = pc2;
      rightHpLoad.Precamber = precamber;
      rightHpLoad.N = Nr;

      EquivPretensionLoad endMoment;
      endMoment.Xs = Ls;
      endMoment.P  = -Ph;
      endMoment.Ybe = Ybr;
      endMoment.Ybm = Ybm;
      endMoment.ex = ecc_x_harped_end;
      endMoment.eye = ecc_y_harped_end;
      endMoment.Precamber = precamber;
      endMoment.Mx = -Mhrx;
      endMoment.My = -Mhry;

      equivLoads.push_back(startMoment);
      equivLoads.push_back(leftHpLoad);
      equivLoads.push_back(rightHpLoad);
      equivLoads.push_back(endMoment);
   }
   else if ( strandType == pgsTypes::Straight && 0 < Ns )
   {
      GET_IFACE(IPretensionForce,pPrestressForce);

      Float64 nSsEffective;
      Float64 Ps = pPrestressForce->GetPrestressForce(poiMiddle,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End,pConfig);

      pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Straight, pConfig, &nSsEffective, &ecc_x_straight_start, &ecc_y_straight_start);
      PsStart = Ps*nSsEffective/Ns;

      pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd, pgsTypes::Straight, pConfig, &nSsEffective, &ecc_x_straight_end, &ecc_y_straight_end);
      PsEnd = Ps*nSsEffective/Ns;

      if (pSegment->TopFlangeThickeningType == pgsTypes::tftNone)
      {
         Mslx = PsStart*(ecc_y_straight_start + 2 * precamber / 3);
         Msrx = PsEnd*(ecc_y_straight_end + 2 * precamber / 3);
      }
      else
      {
         Mslx = PsStart*(ecc_y_straight_start + 2 * (Ybm - Ybl + precamber) / 3);
         Msrx = PsEnd*(ecc_y_straight_end + 2 * (Ybm - Ybr + precamber) / 3);
      }
      Msly = PsStart*ecc_x_straight_start;
      Msry = PsEnd*ecc_x_straight_end;

      wy = -8 * PsStart*precamber / (Ls*Ls);

      EquivPretensionLoad startMoment;
      startMoment.Xs = 0;
      startMoment.Xe = Ls;
      startMoment.P = PsStart;
      startMoment.Ybe = Ybl;
      startMoment.Ybm = Ybm;
      startMoment.ex = ecc_x_straight_start;
      startMoment.eye = ecc_y_straight_start;
      startMoment.PrecamberAtHarpPoint = 0;
      startMoment.Precamber = precamber;
      startMoment.Ls = Ls;
      startMoment.N = 0;
      startMoment.Mx = Mslx;
      startMoment.My = Msly;
      startMoment.wy = wy;

      equivLoads.push_back(startMoment);

      EquivPretensionLoad endMoment;
      endMoment.Xs = Ls;
      endMoment.P = -PsEnd;
      endMoment.Ybe = Ybr;
      endMoment.Ybm = Ybm;
      endMoment.ex = ecc_x_straight_end;
      endMoment.eye = ecc_y_straight_end;
      endMoment.PrecamberAtHarpPoint = 0;
      endMoment.Precamber = precamber;
      endMoment.Ls = Ls;
      endMoment.N = 0;
      endMoment.Mx = -Msrx;
      endMoment.My = -Msry;

      equivLoads.push_back(endMoment);

      // debonding
      std::vector<RowIndexType> vRows = pStrandGeom->GetRowsWithDebonding(segmentKey, pgsTypes::Straight, pConfig);
      for (const auto& rowIdx : vRows)
      {
         IndexType nDebondConfigsInThisRow = pStrandGeom->GetDebondConfigurationCountByRow(segmentKey, pgsTypes::Straight, rowIdx, pConfig);
         for (IndexType configIdx = 0; configIdx < nDebondConfigsInThisRow; configIdx++)
         {
            Float64 Xstart, Lstrand;
            StrandIndexType nStrands;
            pStrandGeom->GetDebondConfigurationByRow(segmentKey, pgsTypes::Straight, rowIdx, configIdx, pConfig, &Xstart, &Lstrand, &nStrands);
            Float64 Xend = Xstart + Lstrand;

            Float64 PsDebond = nStrands*Ps / Ns;
            pgsPointOfInterest startPoi(pPoi->GetPointOfInterest(segmentKey, Xstart));
            ATLASSERT(pConfig == nullptr ? startPoi.GetID() != INVALID_ID : true);
            ATLASSERT(pConfig == nullptr ? startPoi.HasAttribute(POI_DEBOND) : true);

            pgsPointOfInterest endPoi(pPoi->GetPointOfInterest(segmentKey, Xend));
            ATLASSERT(pConfig == nullptr ? endPoi.GetID() != INVALID_ID : true);
            ATLASSERT(pConfig == nullptr ? endPoi.HasAttribute(POI_DEBOND) : true);

            Float64 nSsEffective;
            Float64 ecc_x_start, ecc_y_start;
            pStrandGeom->GetEccentricity(releaseIntervalIdx, startPoi, pgsTypes::Straight, pConfig, &nSsEffective, &ecc_x_start, &ecc_y_start);

            Float64 ecc_x_end, ecc_y_end;
            pStrandGeom->GetEccentricity(releaseIntervalIdx, endPoi, pgsTypes::Straight, pConfig, &nSsEffective, &ecc_x_end, &ecc_y_end);

            Float64 Msx = PsDebond*ecc_y_start;
            Float64 Msy = PsDebond*ecc_x_start;

            Float64 Mex = PsDebond*ecc_y_end;
            Float64 Mey = PsDebond*ecc_x_end;

            wy = -8 * PsDebond*precamber / (Ls*Ls);

            EquivPretensionLoad startLoad;
            startLoad.Xs = Xstart;
            startLoad.Xe = Xend;
            startLoad.P = PsDebond;
            startLoad.ex = ecc_x_start;
            startLoad.eye = ecc_y_start;
            startLoad.Precamber = precamber;
            startLoad.Ls = Ls;
            startLoad.N = 0;
            startLoad.Mx = Msx;
            startLoad.My = Msy;
            startLoad.wy = wy;

            equivLoads.push_back(startLoad);

            EquivPretensionLoad endLoad;
            endLoad.Xs = Xend;
            endLoad.P = -PsDebond;
            endLoad.ex = ecc_x_end;
            endLoad.eye = ecc_y_end;
            endLoad.Precamber = precamber;
            endLoad.Ls = Ls;
            endLoad.N = 0;
            endLoad.Mx = -Mex;
            endLoad.My = -Mey;

            equivLoads.push_back(endLoad);
         }
      }
   }
   else if ( strandType == pgsTypes::Temporary && 0 < Nt )
   {
      GET_IFACE(IPretensionForce,pPrestressForce);

      IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
      IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
      ATLASSERT(tsInstallationIntervalIdx != INVALID_INDEX);
      ATLASSERT(tsRemovalIntervalIdx != INVALID_INDEX);

      pgsTypes::TTSUsage tempStrandUsage = pStrandGeom->GetTemporaryStrandUsage(segmentKey,pConfig);

      if (bTempStrandInstallation)
      {
         Pt = pPrestressForce->GetPrestressForce(poiMiddle, pgsTypes::Temporary, tsInstallationIntervalIdx, pgsTypes::End, pConfig);
      }
      else
      {
         Pt = -1*pPrestressForce->GetPrestressForce(poiMiddle, pgsTypes::Temporary, tsRemovalIntervalIdx, pgsTypes::Start, pConfig);
      }

      Float64 nTsEffective;
      pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Temporary, pConfig, &nTsEffective, &ecc_x_temporary_start, &ecc_y_temporary_start);
      pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   pgsTypes::Temporary, pConfig, &nTsEffective, &ecc_x_temporary_end,   &ecc_y_temporary_end);

      Pt *= nTsEffective/Nt;

      if (pSegment->TopFlangeThickeningType == pgsTypes::tftNone)
      {
         Mtlx = Pt*(ecc_y_temporary_start + 2 * precamber / 3);
         Mtrx = Pt*(ecc_y_temporary_end + 2 * precamber / 3);
      }
      else
      {
         Mtlx = Pt*(ecc_y_temporary_start + 2 * (Ybm - Ybl + precamber) / 3);
         Mtrx = Pt*(ecc_y_temporary_end + 2 * (Ybm - Ybr + precamber) / 3);
      }

      Mtly = Pt*ecc_x_temporary_start;
      Mtry = Pt*ecc_x_temporary_end;

      wy = 0;
      if (tempStrandUsage != pgsTypes::ttsPretensioned)
      {
         // post
         wy = -8 * Pt * precamber / (Ls*Ls);
      }

      EquivPretensionLoad startMoment;
      startMoment.Xs = 0;
      startMoment.Xe = Ls;
      startMoment.P  = Pt;
      startMoment.N  = 0;
      startMoment.Ybe = Ybl;
      startMoment.Ybm = Ybm;
      startMoment.ex = ecc_x_temporary_start;
      startMoment.eye = ecc_y_temporary_start;
      startMoment.PrecamberAtHarpPoint = 0;
      startMoment.Precamber = precamber;
      startMoment.Ls = Ls;
      startMoment.Mx = Mtlx;
      startMoment.My = Mtly;
      startMoment.wy = wy;
      equivLoads.push_back(startMoment);

      EquivPretensionLoad endMoment;
      endMoment.Xs = Ls;
      endMoment.P  = -Pt;
      endMoment.Ybm = Ybm;
      endMoment.ex = ecc_x_temporary_end;
      endMoment.eye = ecc_y_temporary_end;
      endMoment.PrecamberAtHarpPoint = 0;
      endMoment.Precamber = precamber;
      endMoment.Ls = Ls;
      endMoment.N  = 0;
      endMoment.Mx = -Mtrx;
      endMoment.My = -Mtry;

      equivLoads.push_back(endMoment);
   }
   else
   {
      ATLASSERT(strandType != pgsTypes::Permanent); // can't be permanent
   }

   return equivLoads;
}


CAnalysisAgentImp::CamberModelData CAnalysisAgentImp::BuildCamberModel(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig) const
{
   // These are the interfaces we will be using
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   Float64 E = pMaterial->GetSegmentEc(segmentKey,releaseIntervalIdx, pgsTypes::Middle, pConfig);

   //
   // Create the FEM model (includes girder dead load)
   //
   Float64 Ls = pBridge->GetSegmentLength(segmentKey); // segment length

   PoiList vPoi;
   vPoi.reserve(30);
   pPoi->GetPointsOfInterest(segmentKey, &vPoi);
   vPoi.erase(std::remove_if(std::begin(vPoi), std::end(vPoi), [Ls](const pgsPointOfInterest& poi) {return !InRange(0.0, poi.GetDistFromStart(), Ls);}), std::end(vPoi)); // remove all poi's that are not on the segment

   // don't clear vPoi.... using it again will cause the new POIs to be appended
   GET_IFACE(ISegmentLiftingPointsOfInterest,pLiftPOI);
   pLiftPOI->GetLiftingPointsOfInterest(segmentKey, 0, &vPoi);

   GET_IFACE(ISegmentHaulingPointsOfInterest,pHaulPOI);
   pHaulPOI->GetHaulingPointsOfInterest(segmentKey, 0, &vPoi);

   pPoi->SortPoiList(&vPoi);

   CamberModelData modelData;

   pgsGirderModelFactory().CreateGirderModel(m_pBroker,releaseIntervalIdx,segmentKey,0.0,Ls,Ls,E,g_lcidGirder,false,false,vPoi,&modelData.Model,&modelData.PoiMap);

   CComPtr<IFem2dLoadingCollection> loadings;
   modelData.Model->get_Loadings(&loadings);

   // add pretension forces for permanent (straight and harped) strands
   for ( int i = 0; i < 2; i++ )
   {
      pgsTypes::StrandType strandType = (pgsTypes::StrandType)i;
      std::vector<EquivPretensionLoad> vLoads = GetEquivPretensionLoads(segmentKey,strandType,pConfig);

      CComPtr<IFem2dLoading> loadingX, loadingY;
      CComPtr<IFem2dPointLoadCollection> pointLoadsX, pointLoadsY;
      CComPtr<IFem2dDistributedLoadCollection> distLoadsX;

      LoadCaseIDType lcidX, lcidY;
      GetPrestressLoadCaseIDs(strandType, &lcidX, &lcidY);

      loadings->Create(lcidX, &loadingX);
      loadingX->get_PointLoads(&pointLoadsX);
      loadingX->get_DistributedLoads(&distLoadsX);

      loadings->Create(lcidY, &loadingY);
      loadingY->get_PointLoads(&pointLoadsY);

      LoadIDType ptLoadIDX;
      pointLoadsX->get_Count((CollectionIndexType*)&ptLoadIDX);

      LoadIDType distLoadIDX;
      distLoadsX->get_Count((CollectionIndexType*)&distLoadIDX);

      LoadIDType ptLoadIDY;
      pointLoadsY->get_Count((CollectionIndexType*)&ptLoadIDY);

      for(const auto& equivLoad : vLoads)
      {
         CComPtr<IFem2dPointLoad> ptLoad;
         MemberIDType mbrIDStart, mbrIDEnd;
         Float64 Xs, Xe;
         pgsGirderModelFactory::FindMember(modelData.Model, equivLoad.Xs, &mbrIDStart, &Xs);
         pgsGirderModelFactory::FindMember(modelData.Model, equivLoad.Xe, &mbrIDEnd, &Xe);


         // loading that causes bending about the x-axis (vertical deflections)
         pointLoadsX->Create(ptLoadIDX++,mbrIDStart,Xs,equivLoad.P,equivLoad.N,equivLoad.Mx,lotGlobal,&ptLoad);

         if (!IsZero(equivLoad.wy))
         {
            if (mbrIDStart == mbrIDEnd)
            {
               CComPtr<IFem2dDistributedLoad> distLoad;
               distLoadsX->Create(distLoadIDX++, mbrIDStart, loadDirFy, Xs, Xe, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
            }
            else
            {
               CComPtr<IFem2dDistributedLoad> distLoad;
               distLoadsX->Create(distLoadIDX++, mbrIDStart, loadDirFy, Xs, -1, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
               for (MemberIDType mbrID = mbrIDStart + 1; mbrID < mbrIDEnd; mbrID++)
               {
                  distLoad.Release();
                  distLoadsX->Create(distLoadIDX++, mbrID, loadDirFy, 0, -1, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
               }
               distLoad.Release();
               distLoadsX->Create(distLoadIDX++, mbrIDEnd, loadDirFy, 0, Xe, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
            }
         }

         // loading that causes bending about the y-axis (lateral deflections)
         if (!IsZero(equivLoad.My)) // no need to create the loading if it is zero
         {
            ptLoad.Release();
            pointLoadsY->Create(ptLoadIDY++, mbrIDStart, Xs, 0, 0, equivLoad.My, lotGlobal, &ptLoad);
         }
      }
   }

   return modelData;
}

void CAnalysisAgentImp::BuildTempCamberModel(const CSegmentKey& segmentKey,const GDRCONFIG* pConfig,CamberModelData* pInitialModelData,CamberModelData* pReleaseModelData) const
{
   PoiList vPoi; // Vector of points of interest
   vPoi.reserve(20);

   // These are the interfaces we will be using
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   ATLASSERT(tsInstallationIntervalIdx != INVALID_INDEX);
   ATLASSERT(tsRemovalIntervalIdx != INVALID_INDEX);

   // Build models
   Float64 Ls = pBridge->GetSegmentLength(segmentKey);

   Float64 Eci = pMaterial->GetSegmentEc(segmentKey, tsInstallationIntervalIdx, pgsTypes::Middle, pConfig);
   Float64 Ec  = pMaterial->GetSegmentEc(segmentKey, tsRemovalIntervalIdx, pgsTypes::Middle, pConfig);

   pPoi->GetPointsOfInterest(segmentKey,&vPoi);
   vPoi.erase(std::remove_if(std::begin(vPoi), std::end(vPoi), [Ls](const pgsPointOfInterest& poi) {return !InRange(0.0, poi.GetDistFromStart(), Ls); }), std::end(vPoi)); // remove all poi that are not on the segment

   GET_IFACE(ISegmentLiftingPointsOfInterest,pLiftPOI);
   pLiftPOI->GetLiftingPointsOfInterest(segmentKey, 0, &vPoi);

   GET_IFACE(ISegmentHaulingPointsOfInterest,pHaulPOI);
   pHaulPOI->GetHaulingPointsOfInterest(segmentKey, 0, &vPoi);

   pPoi->SortPoiList(&vPoi);

   pgsGirderModelFactory().CreateGirderModel(m_pBroker,tsInstallationIntervalIdx, segmentKey,0.0,Ls,Ls,Eci,g_lcidGirder,false,false,vPoi,&pInitialModelData->Model,&pInitialModelData->PoiMap);
   pgsGirderModelFactory().CreateGirderModel(m_pBroker,tsRemovalIntervalIdx,      segmentKey,0.0,Ls,Ls,Ec, g_lcidGirder,false,false,vPoi,&pReleaseModelData->Model,&pReleaseModelData->PoiMap);

   std::vector<EquivPretensionLoad> vInstallationLoads = GetEquivPretensionLoads(segmentKey,pgsTypes::Temporary,pConfig,true);
   std::vector<EquivPretensionLoad> vRemovalLoads      = GetEquivPretensionLoads(segmentKey,pgsTypes::Temporary,pConfig,false);

   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loadingX, loadingY;
   pInitialModelData->Model->get_Loadings(&loadings);

   LoadCaseIDType lcidX, lcidY;
   GetPrestressLoadCaseIDs(pgsTypes::Temporary, &lcidX, &lcidY);
   loadings->Create(lcidX, &loadingX);
   loadings->Create(lcidY, &loadingY);

   CComPtr<IFem2dPointLoadCollection> pointLoadsX, pointLoadsY;
   loadingX->get_PointLoads(&pointLoadsX);
   loadingY->get_PointLoads(&pointLoadsY);

   CComPtr<IFem2dDistributedLoadCollection> distLoadsX;
   loadingX->get_DistributedLoads(&distLoadsX);

   LoadIDType ptLoadIDX;
   pointLoadsX->get_Count((CollectionIndexType*)&ptLoadIDX);

   LoadIDType ptLoadIDY;
   pointLoadsY->get_Count((CollectionIndexType*)&ptLoadIDY);

   LoadIDType distLoadIDX;
   distLoadsX->get_Count(((CollectionIndexType*)&distLoadIDX));

   for( const auto& equivLoad : vInstallationLoads)
   {
      CComPtr<IFem2dPointLoad> ptLoad;
      MemberIDType mbrIDStart, mbrIDEnd;
      Float64 Xs, Xe;
      pgsGirderModelFactory::FindMember(pInitialModelData->Model, equivLoad.Xs, &mbrIDStart, &Xs);
      pgsGirderModelFactory::FindMember(pInitialModelData->Model, equivLoad.Xe, &mbrIDEnd, &Xe);

      pointLoadsX->Create(ptLoadIDX++,mbrIDStart,Xs,equivLoad.P,equivLoad.N,equivLoad.Mx,lotGlobal,&ptLoad);

      if (!IsZero(equivLoad.My))
      {
         ptLoad.Release();
         pointLoadsY->Create(ptLoadIDY++, mbrIDStart, Xs, 0, 0, equivLoad.My, lotGlobal, &ptLoad);
      }

      if (!IsZero(equivLoad.wy))
      {
         if (mbrIDStart == mbrIDEnd)
         {
            CComPtr<IFem2dDistributedLoad> distLoad;
            distLoadsX->Create(distLoadIDX++, mbrIDStart, loadDirFy, Xs, Xe, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
         }
         else
         {
            CComPtr<IFem2dDistributedLoad> distLoad;
            distLoadsX->Create(distLoadIDX++, mbrIDStart, loadDirFy, Xs, -1, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
            for (MemberIDType mbrID = mbrIDStart + 1; mbrID < mbrIDEnd; mbrID++)
            {
               distLoad.Release();
               distLoadsX->Create(distLoadIDX++, mbrID, loadDirFy, 0, -1, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
            }
            distLoad.Release();
            distLoadsX->Create(distLoadIDX++, mbrIDEnd, loadDirFy, 0, Xe, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
         }
      }
   }


   loadings.Release();
   loadingX.Release();
   loadingY.Release();
   
   pReleaseModelData->Model->get_Loadings(&loadings);

   loadings->Create(lcidX, &loadingX);
   loadings->Create(lcidY, &loadingY);

   pointLoadsX.Release();
   loadingX->get_PointLoads(&pointLoadsX);
   pointLoadsX->get_Count((CollectionIndexType*)&ptLoadIDX);

   pointLoadsY.Release();
   loadingY->get_PointLoads(&pointLoadsY);
   pointLoadsY->get_Count((CollectionIndexType*)&ptLoadIDY);

   distLoadsX.Release();
   loadingX->get_DistributedLoads(&distLoadsX);
   distLoadsX->get_Count((CollectionIndexType*)&distLoadIDX);

   for( const auto& equivLoad : vRemovalLoads)
   {
      CComPtr<IFem2dPointLoad> ptLoad;
      MemberIDType mbrIDStart, mbrIDEnd;
      Float64 Xs, Xe;
      pgsGirderModelFactory::FindMember(pReleaseModelData->Model,equivLoad.Xs,&mbrIDStart,&Xs);
      pgsGirderModelFactory::FindMember(pReleaseModelData->Model, equivLoad.Xe, &mbrIDEnd, &Xe);

      pointLoadsX->Create(ptLoadIDX++,mbrIDStart,Xs,equivLoad.P, equivLoad.N, equivLoad.Mx,lotGlobal,&ptLoad);

      if (!IsZero(equivLoad.My))
      {
         ptLoad.Release();
         pointLoadsY->Create(ptLoadIDY++, mbrIDStart, Xs, 0, 0, equivLoad.My, lotGlobal, &ptLoad);
      }

      if (!IsZero(equivLoad.wy))
      {
         if (mbrIDStart == mbrIDEnd)
         {
            CComPtr<IFem2dDistributedLoad> distLoad;
            distLoadsX->Create(distLoadIDX++, mbrIDStart, loadDirFy, Xs, Xe, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
         }
         else
         {
            CComPtr<IFem2dDistributedLoad> distLoad;
            distLoadsX->Create(distLoadIDX++, mbrIDStart, loadDirFy, Xs, -1, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
            for (MemberIDType mbrID = mbrIDStart + 1; mbrID < mbrIDEnd; mbrID++)
            {
               distLoad.Release();
               distLoadsX->Create(distLoadIDX++, mbrID, loadDirFy, 0, -1, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
            }
            distLoad.Release();
            distLoadsX->Create(distLoadIDX++, mbrIDEnd, loadDirFy, 0, Xe, equivLoad.wy, equivLoad.wy, lotGlobal, &distLoad);
         }
      }
   }
}

void CAnalysisAgentImp::InValidateSlabOffsetDesignModel()
{
   m_CacheConfig_SlabOffsetDesignModel.Model.Release();
   m_CacheConfig_SlabOffsetDesignModel.PoiMap.Clear();
}

void CAnalysisAgentImp::ValidateSlabOffsetDesignModel( const GDRCONFIG* pConfig) const
{
   if ( *pConfig != m_SlabOffsetDesignCacheConfig || !m_CacheConfig_SlabOffsetDesignModel.Model)
   {
      m_CacheConfig_SlabOffsetDesignModel.Model.Release();

      const CSegmentKey& segmentKey = pConfig->SegmentKey;
      ASSERT_SEGMENT_KEY(segmentKey);
      BuildSlabOffsetDesignModel(segmentKey,pConfig,&m_CacheConfig_SlabOffsetDesignModel);

      m_SlabOffsetDesignCacheConfig = *pConfig;
   }
}

void CAnalysisAgentImp::BuildSlabOffsetDesignModel(const CSegmentKey& segmentKey, const GDRCONFIG* pConfig,CamberModelData* pModelData) const
{
   Float64 Ls;  // Length of segment

   // These are the interfaces we will be using
   GET_IFACE(IPointOfInterest,pPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   // Create the FEM model
   Float64 E = pConfig->Ec;
   Float64 left_support_loc = pBridge->GetSegmentStartEndDistance(segmentKey);
   Ls = pBridge->GetSegmentLength(segmentKey);
   Float64 end_end_size   = pBridge->GetSegmentEndEndDistance(segmentKey);
   Float64 right_support_loc = Ls - end_end_size;

   PoiList vPOI;
   pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT, &vPOI);

   // Differential slab load between design and original models
   std::vector<SlabLoad> slabLoads;
   GetDesignMainSpanSlabLoadAdjustment(segmentKey, pConfig->SlabOffset[pgsTypes::metStart], pConfig->SlabOffset[pgsTypes::metEnd], pConfig->AssExcessCamber, &slabLoads);

   // model autogenerates loadings for delta (design) slab and haunch
   bool bModelLeftCantilever,bModelRightCantilever;
   pBridge->ModelCantilevers(segmentKey,&bModelLeftCantilever,&bModelRightCantilever);
   pgsDesignHaunchLoadGirderModelFactory factory(slabLoads, g_lcidDesignSlab, g_lcidDesignSlabPad);
   factory.CreateGirderModel(m_pBroker,castDeckIntervalIdx,segmentKey,left_support_loc,right_support_loc,Ls,E,g_lcidGirder,bModelLeftCantilever,bModelRightCantilever,vPOI,&pModelData->Model,&pModelData->PoiMap);
}

/////////////////////////////////////////////////////////////////////////////
// IAgent
//
STDMETHODIMP CAnalysisAgentImp::SetBroker(IBroker* pBroker)
{
   EAF_AGENT_SET_BROKER(pBroker);

   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::RegInterfaces()
{
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);

   pBrokerInit->RegInterface( IID_IProductLoads,             this );
   pBrokerInit->RegInterface( IID_IProductForces,            this );
   pBrokerInit->RegInterface( IID_IProductForces2,           this );
   pBrokerInit->RegInterface( IID_ICombinedForces,           this );
   pBrokerInit->RegInterface( IID_ICombinedForces2,          this );
   pBrokerInit->RegInterface( IID_ILimitStateForces,         this );
   pBrokerInit->RegInterface( IID_ILimitStateForces2,        this );
   pBrokerInit->RegInterface( IID_IExternalLoading,          this );
   pBrokerInit->RegInterface( IID_IPretensionStresses,       this );
   pBrokerInit->RegInterface( IID_ICamber,                   this );
   pBrokerInit->RegInterface( IID_IContraflexurePoints,      this );
   pBrokerInit->RegInterface( IID_IContinuity,               this );
   pBrokerInit->RegInterface( IID_IBearingDesign,            this );
   pBrokerInit->RegInterface( IID_IPrecompressedTensileZone, this );
   pBrokerInit->RegInterface( IID_IReactions,                this );

   return S_OK;
};

STDMETHODIMP CAnalysisAgentImp::Init()
{
   CREATE_LOGFILE("AnalysisAgent");

   EAF_AGENT_INIT;

   // Register status callbacks that we want to use
   m_scidVSRatio = pStatusCenter->RegisterCallback(new pgsVSRatioStatusCallback(m_pBroker));

   m_pSegmentModelManager = std::make_unique<CSegmentModelManager>(LOGGER,m_pBroker);
   m_pGirderModelManager  = std::make_unique<CGirderModelManager>(LOGGER,m_pBroker,m_StatusGroupID);

   return AGENT_S_SECONDPASSINIT;
}

STDMETHODIMP CAnalysisAgentImp::Init2()
{
   //
   // Attach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   // Connection point for the bridge description
   hr = pBrokerInit->FindConnectionPoint( IID_IBridgeDescriptionEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwBridgeDescCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the specification
   hr = pBrokerInit->FindConnectionPoint( IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwSpecCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the rating specification
   hr = pBrokerInit->FindConnectionPoint( IID_IRatingSpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwRatingSpecCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the load modifiers
   hr = pBrokerInit->FindConnectionPoint( IID_ILoadModifiersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwLoadModifierCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   // Connection point for the loss parameters
   hr = pBrokerInit->FindConnectionPoint( IID_ILossParametersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Advise( GetUnknown(), &m_dwLossParametersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the IConnectionPoint smart pointer so we can use it again.

   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::GetClassID(CLSID* pCLSID)
{
   *pCLSID = CLSID_AnalysisAgent;
   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::Reset()
{
   LOG("Reset");
   Invalidate(true);
   return S_OK;
}

STDMETHODIMP CAnalysisAgentImp::ShutDown()
{
   LOG("AnalysisAgent Log Closed");

   //
   // Detach to connection points
   //
   CComQIPtr<IBrokerInitEx2,&IID_IBrokerInitEx2> pBrokerInit(m_pBroker);
   CComPtr<IConnectionPoint> pCP;
   HRESULT hr = S_OK;

   hr = pBrokerInit->FindConnectionPoint(IID_IBridgeDescriptionEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwBridgeDescCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ISpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwSpecCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_IRatingSpecificationEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwRatingSpecCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ILoadModifiersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLoadModifierCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   hr = pBrokerInit->FindConnectionPoint(IID_ILossParametersEventSink, &pCP );
   ATLASSERT( SUCCEEDED(hr) );
   hr = pCP->Unadvise( m_dwLossParametersCookie );
   ATLASSERT( SUCCEEDED(hr) );
   pCP.Release(); // Recycle the connection point

   EAF_AGENT_CLEAR_INTERFACE_CACHE;
   CLOSE_LOGFILE;

   return S_OK;
}

void CAnalysisAgentImp::GetSidewalkLoadFraction(const CSegmentKey& segmentKey,Float64* pSidewalkLoad,Float64* pFraLeft,Float64* pFraRight) const
{
   m_pGirderModelManager->GetSidewalkLoadFraction(segmentKey,pSidewalkLoad,pFraLeft,pFraRight);
}

void CAnalysisAgentImp::GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad,Float64* pFraExtLeft, Float64* pFraIntLeft,Float64* pFraExtRight,Float64* pFraIntRight) const
{
   m_pGirderModelManager->GetTrafficBarrierLoadFraction(segmentKey,pBarrierLoad,pFraExtLeft,pFraIntLeft,pFraExtRight,pFraIntRight);
}

PoiIDPairType CAnalysisAgentImp::AddPointOfInterest(CamberModelData& models,const pgsPointOfInterest& poi) const
{
   PoiIDPairType femPoiID = pgsGirderModelFactory::AddPointOfInterest(models.Model,poi);
   models.PoiMap.AddMap( poi, femPoiID );
   return femPoiID;
}

/////////////////////////////////////////////////////////////////////////////
// IProductForces
//
pgsTypes::BridgeAnalysisType CAnalysisAgentImp::GetBridgeAnalysisType(pgsTypes::AnalysisType analysisType,pgsTypes::OptimizationType optimization) const
{
   pgsTypes::BridgeAnalysisType bat;
   if ( analysisType == pgsTypes::Simple )
   {
      bat = pgsTypes::SimpleSpan;
   }
   else if ( analysisType == pgsTypes::Continuous )
   {
      bat = pgsTypes::ContinuousSpan;
   }
   else
   {
      if ( optimization == pgsTypes::Maximize )
      {
         bat = pgsTypes::MaxSimpleContinuousEnvelope;
      }
      else
      {
         bat = pgsTypes::MinSimpleContinuousEnvelope;
      }
   }

   return bat;
}

pgsTypes::BridgeAnalysisType CAnalysisAgentImp::GetBridgeAnalysisType(pgsTypes::OptimizationType optimization) const
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   return GetBridgeAnalysisType(analysisType,optimization);
}


Float64 CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetAxial(intervalIdx,pfType,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

sysSectionValue CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<sysSectionValue> results = GetShear(intervalIdx,pfType,vPoi,bat,resultsType);
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetMoment(intervalIdx,pfType,vPoi,bat,resultsType);
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment, bool bIncludePrecamber) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetDeflection(intervalIdx,pfType,vPoi,bat,resultsType,bIncludeElevationAdjustment,bIncludePrecamber);
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetXDeflection(intervalIdx, pfType, vPoi, bat, resultsType);
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment,bool bIncludePrecamber) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetRotation(intervalIdx,pfType,vPoi,bat,resultsType,bIncludeSlopeAdjustment,bIncludePrecamber);
   ATLASSERT(results.size() == 1);
   return results.front();
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop,fBot;
   GetStress(intervalIdx,pfType,vPoi,bat,resultsType,topLocation,botLocation,&fTop,&fBot);

   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);

   *pfTop = fTop.front();
   *pfBot = fBot.front();
}

////////////////////////////////////////////////////////////////////////////////////////////
// IProductLoads
LPCTSTR CAnalysisAgentImp::GetProductLoadName(pgsTypes::ProductForceType pfType) const
{
   // these are the names that are displayed to the user in the UI and reports
   // this must be in the same order as the pgsTypes::ProductForceType enum
   static LPCTSTR strNames[] =
   {
      _T("Girder"),
      _T("Construction"),
      _T("Slab"),
      _T("Haunch"),
      _T("Slab Panel"),
      _T("Diaphragm"),
      _T("Overlay"),
      _T("Sidewalk"),
      _T("Railing System"),
      _T("User DC"),
      _T("User DW"),
      _T("User LLIM"),
      _T("Shear Key"),
      _T("Longitudinal Joint"),
      _T("Pre-tensioning"),
      _T("Post-tensioning"),
      _T("Secondary Effects"),
      _T("Creep"),
      _T("Shrinkage"),
      _T("Relaxation"),
      _T("Total Post-tensioning"),
      _T("Overlay (rating)")
   };

   // the direct lookup in the array is faster, however if the enum changes (number of values or order of values)
   // it isn't easily detectable... the switch/case below is slower but it can detect errors that result
   // from changing the enum
#if defined _DEBUG
   std::_tstring strName;
   switch (pfType)
   {
   case pgsTypes::pftGirder:
      strName = _T("Girder");
      break;

   case pgsTypes::pftConstruction:
      strName = _T("Construction");
      break;

   case pgsTypes::pftSlab:
      strName = _T("Slab");
      break;

   case pgsTypes::pftSlabPad:
      strName = _T("Haunch");
      break;

   case pgsTypes::pftSlabPanel:
      strName = _T("Slab Panel");
      break;

   case pgsTypes::pftDiaphragm:
      strName = _T("Diaphragm");
      break;

   case pgsTypes::pftOverlay:
      strName = _T("Overlay");
      break;

   case pgsTypes::pftSidewalk:
      strName = _T("Sidewalk");
      break;

   case pgsTypes::pftTrafficBarrier:
      strName = _T("Railing System");
      break;

   case pgsTypes::pftUserDC:
      strName = _T("User DC");
      break;

   case pgsTypes::pftUserDW:
      strName = _T("User DW");
      break;

   case pgsTypes::pftUserLLIM:
      strName = _T("User LLIM");
      break;

   case pgsTypes::pftShearKey:
      strName = _T("Shear Key");
      break;

   case pgsTypes::pftLongitudinalJoint:
      strName = _T("Longitudinal Joint");
      break;

   case pgsTypes::pftPretension:
      strName = _T("Pre-tensioning");
      break;

   case pgsTypes::pftPostTensioning:
      strName = _T("Post-tensioning");
      break;

   case pgsTypes::pftSecondaryEffects:
      strName = _T("Secondary Effects");
      break;

   case pgsTypes::pftCreep:
      strName = _T("Creep");
      break;

   case pgsTypes::pftShrinkage:
      strName = _T("Shrinkage");
      break;

   case pgsTypes::pftRelaxation:
      strName = _T("Relaxation");
      break;

   case pgsTypes::pftOverlayRating:
      strName = _T("Overlay (rating)");
      break;

   default:
      ATLASSERT(false); // is there a new type?
      strName = _T("");
      break;
   }

   ATLASSERT(strName == std::_tstring(strNames[pfType]));
#endif

   if (pfType == pgsTypes::pftSlab || pfType == pgsTypes::pftSlabPad)
   {
      GET_IFACE(IBridge, pBridge);
      pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
      if (deckType == pgsTypes::sdtNonstructuralOverlay)
      {
         return (pfType == pgsTypes::pftSlab) ? _T("Nonstructural Overlay") : _T("Nonstructural Haunch");
      }
      else if (deckType == pgsTypes::sdtCompositeOverlay)
      {
         return (pfType == pgsTypes::pftSlab) ? _T("Composite Overlay") : _T("Haunch");
      }
      else
      {
         return strNames[pfType];
      }
   }
   else
   {
      return strNames[pfType];
   }
}

LPCTSTR CAnalysisAgentImp::GetLoadCombinationName(LoadingCombinationType loadCombo) const
{
   // these are the names that are displayed to the user in the UI and reports
   // this must be in the same order as the LoadingCombinationType enum
   static LPCTSTR strNames[] =
   {
   _T("DC"), 
   _T("DW"), 
   _T("DWRating"), 
   _T("DWp"), 
   _T("DWf"), 
   _T("CR"), 
   _T("SH"), 
   _T("RE"), 
   _T("PS")
   };

   // the direct lookup in the array is faster, however if the enum changes (number of values or order of values)
   // it isn't easily detectable... the switch/case below is slower but it can detect errors that result
   // from changing the enum
#if defined _DEBUG
   std::_tstring strName;
   switch(loadCombo)
   {
   case lcDC:
      strName = _T("DC");
      break; 
   
   case lcDW:
      strName = _T("DW");
      break;

   case lcDWRating:
      strName = _T("DWRating");
      break;

   case lcDWp:
      strName = _T("DWp");
      break;

   case lcDWf:
      strName = _T("DWf");
      break;

   case lcCR:
      strName = _T("CR");
      break;

   case lcSH:
      strName = _T("SH");
      break;

   case lcRE:
      strName = _T("RE");
      break;

   case lcPS:
      strName = _T("PS");
      break;

   default:
      ATLASSERT(false);
   }

   ATLASSERT(strName == std::_tstring(strNames[loadCombo]));
#endif

   return strNames[loadCombo];
}

bool CAnalysisAgentImp::ReportAxialResults() const
{
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   PierIndexType nPiers = pBridgeDesc->GetPierCount();
   for ( PierIndexType pierIdx = 0; pierIdx < nPiers; pierIdx++ )
   {
      const CPierData2* pPier = pBridgeDesc->GetPier(pierIdx);
      if ( pPier->GetPierModelType() == pgsTypes::pmtPhysical )
      {
         return true;
      }
   }

   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
      {
         const CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         if ( 0 < pGirder->GetPostTensioning()->GetDuctCount() )
         {
            return true;
         }
      }
   }

   return false;
}

void CAnalysisAgentImp::GetSegmentSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<SegmentLoad>* pSegmentLoads,std::vector<DiaphragmLoad>* pDiaphragmLoads,std::vector<ClosureJointLoad>* pClosureJointLoads) const
{
   m_pGirderModelManager->GetSegmentSelfWeightLoad(segmentKey,pSegmentLoads,pDiaphragmLoads,pClosureJointLoads);
}

std::vector<EquivPretensionLoad> CAnalysisAgentImp::GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType,bool bTempStrandInstallation,IntervalIndexType intervalIdx) const
{
   return GetEquivPretensionLoads(segmentKey,strandType,nullptr,bTempStrandInstallation,intervalIdx);
}

Float64 CAnalysisAgentImp::GetTrafficBarrierLoad(const CSegmentKey& segmentKey) const
{
   return m_pGirderModelManager->GetTrafficBarrierLoad(segmentKey);
}

Float64 CAnalysisAgentImp::GetSidewalkLoad(const CSegmentKey& segmentKey) const
{
   return m_pGirderModelManager->GetSidewalkLoad(segmentKey);
}

void CAnalysisAgentImp::GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads) const
{
   m_pGirderModelManager->GetOverlayLoad(segmentKey,pOverlayLoads);
}

bool  CAnalysisAgentImp::HasConstructionLoad(const CGirderKey& girderKey) const
{
   return m_pGirderModelManager->HasConstructionLoad(girderKey);
}

void CAnalysisAgentImp::GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads) const
{
   m_pGirderModelManager->GetConstructionLoad(segmentKey,pConstructionLoads);
}

void CAnalysisAgentImp::GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads) const
{
   m_pGirderModelManager->GetMainSpanSlabLoad(segmentKey,pSlabLoads);
}

void CAnalysisAgentImp::GetDesignMainSpanSlabLoadAdjustment(const CSegmentKey& segmentKey, Float64 Astart, Float64 Aend, Float64 AssExcessCamber, std::vector<SlabLoad>* pSlabLoads) const
{
   m_pGirderModelManager->GetDesignMainSpanSlabLoadAdjustment(segmentKey,Astart,Aend,AssExcessCamber,pSlabLoads);
}

bool CAnalysisAgentImp::HasShearKeyLoad(const CGirderKey& girderKey) const
{
   return m_pGirderModelManager->HasShearKeyLoad(girderKey);
}

void CAnalysisAgentImp::GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads) const
{
   m_pGirderModelManager->GetShearKeyLoad(segmentKey,pLoads);
}

bool CAnalysisAgentImp::HasLongitudinalJointLoad() const
{
   return m_pGirderModelManager->HasLongitudinalJointLoad();
}

void CAnalysisAgentImp::GetLongitudinalJointLoad(const CSegmentKey& segmentKey, std::vector<LongitudinalJointLoad>* pLoads) const
{
   return m_pGirderModelManager->GetLongitudinalJointLoad(segmentKey, pLoads);
}

bool CAnalysisAgentImp::HasPedestrianLoad() const
{
   return m_pGirderModelManager->HasPedestrianLoad();
}

bool CAnalysisAgentImp::HasSidewalkLoad(const CGirderKey& girderKey) const
{
   return m_pGirderModelManager->HasSidewalkLoad(girderKey);
}

bool CAnalysisAgentImp::HasPedestrianLoad(const CGirderKey& girderKey) const
{
   return m_pGirderModelManager->HasPedestrianLoad(girderKey);
}

Float64 CAnalysisAgentImp::GetPedestrianLoad(const CSegmentKey& segmentKey) const
{
   return m_pGirderModelManager->GetPedestrianLoad(segmentKey);
}

Float64 CAnalysisAgentImp::GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation) const
{
   return m_pGirderModelManager->GetPedestrianLoadPerSidewalk(orientation);
}

void CAnalysisAgentImp::GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const
{
   m_pGirderModelManager->GetCantileverSlabLoad(segmentKey,pP1,pM1,pP2,pM2);
}

void CAnalysisAgentImp::GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2) const
{
   m_pGirderModelManager->GetCantileverSlabPadLoad(segmentKey,pP1,pM1,pP2,pM2);
}

void CAnalysisAgentImp::GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads) const
{
   m_pGirderModelManager->GetPrecastDiaphragmLoads(segmentKey,pLoads);
}

void CAnalysisAgentImp::GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads) const
{
   m_pGirderModelManager->GetIntermediateDiaphragmLoads(spanKey,pLoads);
}

void CAnalysisAgentImp::GetPierDiaphragmLoads( PierIndexType pierIdx, GirderIndexType gdrIdx, Float64* pPback, Float64 *pMback, Float64* pPahead, Float64* pMahead) const
{
   m_pGirderModelManager->GetPierDiaphragmLoads(pierIdx,gdrIdx,pPback,pMback,pPahead,pMahead);
}

Float64 CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const
{
   Float64 dyStorage,rzStorage;
   Float64 dyErected,rzErected;
   Float64 dy_inc,rz_inc;
   GetGirderDeflectionForCamber(poi,pConfig,&dyStorage,&rzStorage,&dyErected,&rzErected,&dy_inc,&rz_inc);
   return dyStorage; // by definition we want deflection during storage
}

void CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDyStorage,Float64* pRzStorage,Float64* pDyErected,Float64* pRzErected,Float64* pDyInc,Float64* pRzInc) const
{
   // this is the deflection during storage based on the concrete properties input by the user

   Float64 dStorage, rStorage;
   Float64 dErected, rErected;
   Float64 d_inc, r_inc;
   // NOTE: This function assumes that deflection due to girder self-weight is the only loading
   // that goes into the camber calculations
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // we want the largest downward deflection. With (+) being up and (-) being down
   // we want the minimum (most negative) deflection
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   // The initial camber occurs while the girder is sitting in storage
   // get the deflection while in storage
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   dStorage = GetDeflection(storageIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative, false,true/*include precamber*/);
   rStorage = GetRotation(storageIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative, false,true/*include precamber*/);

   // This is the girder deflection after it has been erected
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   dErected = GetDeflection(erectionIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative, false, true/*include precamber*/);
   rErected = GetRotation(erectionIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative, false, true/*include precamber*/);

   // If the storage support locations and the support locations after the girder is erected aren't at the same
   // location there is an incremental change in moment which causes an additional deflection increment
   //
   // We must use the actual loading, "Girder_Incremental", to get the incremental deformation due to the
   // change in support location. Using the product load type pftGirder gives use both the incremental deformation
   // and a deflection adjustment because of the change in the deformation measurement datum. During storage
   // deformations are measured relative to the storage supports and after erection, deformations are measured
   // relative to the final bearing locations. This translation between measurement datums is not subject to
   // creep and camber.
   d_inc = GetDeflection(erectionIntervalIdx, _T("Girder_Incremental"), poi, bat, rtCumulative, false);
   r_inc = GetRotation(erectionIntervalIdx, _T("Girder_Incremental"), poi, bat, rtCumulative, false);

   ATLASSERT(IsEqual(dStorage+d_inc,dErected,0.001)); // dStorage+d_inc also needs to deal with support datum change
   //ATLASSERT(IsEqual(rStorage+r_inc,rErected));

   if (pConfig != nullptr)
   {
      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      // we need to adjust the deflections for the concrete properties in config
      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

      GET_IFACE(IMaterials, pMaterial);

      // get Eci used to compute delta and rotation
      Float64 Eci_original = pMaterial->GetSegmentEc(segmentKey, releaseIntervalIdx);

      // get Eci for the concrete properties in the config object
      Float64 Eci_config;
      if (pConfig->bUserEci)
      {
         Eci_config = pConfig->Eci;
      }
      else
      {
         Eci_config = pMaterial->GetEconc(pConfig->Fci, pMaterial->GetSegmentStrengthDensity(segmentKey),
            pMaterial->GetSegmentEccK1(segmentKey),
            pMaterial->GetSegmentEccK2(segmentKey));
      }

      // adjust the deflections
      // deltaOriginal = K/Ix*Eoriginal
      // deltaConfig   = K/Ix*Econfig = (K/Ix*Eoriginal)(Eoriginal/Econfig) = deltaOriginal*(Eoriginal/Econfig)
      dStorage *= (Eci_original / Eci_config);
      rStorage *= (Eci_original / Eci_config);

      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

      // get Ec used to compute delta and rotation
      Float64 Ec_original = pMaterial->GetSegmentEc(segmentKey, erectionIntervalIdx);

      // get Ec for the concrete properties in the config object
      Float64 Ec_config;
      if (pConfig->bUserEc)
      {
         Ec_config = pConfig->Ec;
      }
      else
      {
         Ec_config = pMaterial->GetEconc(pConfig->Fc, pMaterial->GetSegmentStrengthDensity(segmentKey),
            pMaterial->GetSegmentEccK1(segmentKey),
            pMaterial->GetSegmentEccK2(segmentKey));
      }

      d_inc *= (Ec_original / Ec_config);
      r_inc *= (Ec_original / Ec_config);

      dErected = dStorage + d_inc;
      rErected = rStorage + r_inc;
   }

   *pDyStorage = dStorage;
   *pRzStorage = rStorage;
   *pDyErected = dErected;
   *pRzErected = rErected;
   *pDyInc = d_inc;
   *pRzInc = r_inc;
}

void CAnalysisAgentImp::GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck) const
{
   m_pGirderModelManager->GetLiveLoadAxial(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck) const
{
   return m_pGirderModelManager->GetLiveLoadShear(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck) const
{
   m_pGirderModelManager->GetLiveLoadMoment(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const
{
   m_pGirderModelManager->GetLiveLoadDeflection(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const
{
   return m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const
{
   m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,pierIdx,girderKey,pierFace,bat,bIncludeImpact,bIncludeLLDF,pTmin,pTmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig) const
{
   m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,pierIdx,girderKey,pierFace,bat,bIncludeImpact,bIncludeLLDF,pTmin,pTmax,pRmin,pRmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig,VehicleIndexType* pTopMaxConfig,VehicleIndexType* pBotMinConfig,VehicleIndexType* pBotMaxConfig) const
{
   m_pGirderModelManager->GetLiveLoadStress(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pTopMinConfig,pTopMaxConfig,pBotMinConfig,pBotMaxConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig) const
{
   m_pGirderModelManager->GetVehicularLiveLoadAxial(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,AxleConfiguration* pMinLeftAxleConfig,AxleConfiguration* pMinRightAxleConfig,AxleConfiguration* pMaxLeftAxleConfig,AxleConfiguration* pMaxRightAxleConfig) const
{
   m_pGirderModelManager->GetVehicularLiveLoadShear(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMinLeftAxleConfig,pMinRightAxleConfig,pMaxLeftAxleConfig,pMaxRightAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig) const
{
   m_pGirderModelManager->GetVehicularLiveLoadMoment(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig) const
{
   m_pGirderModelManager->GetVehicularLiveLoadDeflection(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig) const
{
   m_pGirderModelManager->GetVehicularLiveLoadRotation(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop,AxleConfiguration* pMaxAxleConfigTop,AxleConfiguration* pMinAxleConfigBot,AxleConfiguration* pMaxAxleConfigBot) const
{
   m_pGirderModelManager->GetVehicularLiveLoadStress(intervalIdx,llType,vehicleIdx,poi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pMinAxleConfigTop,pMaxAxleConfigTop,pMinAxleConfigBot,pMaxAxleConfigBot);
}

void CAnalysisAgentImp::GetDeflLiveLoadDeflection(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const
{
   m_pGirderModelManager->GetDeflLiveLoadDeflection(type,poi,bat,pDmin,pDmax);
}

Float64 CAnalysisAgentImp::GetDesignSlabMomentAdjustment(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const
{
   return GetDesignMomentAdjustment(g_lcidDesignSlab, poi, pConfig);
}

Float64 CAnalysisAgentImp::GetDesignMomentAdjustment(LoadCaseIDType lcid, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const
{
   ATLASSERT(pConfig != nullptr);

   // returns the difference in moment between the slab moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   if (deckType == pgsTypes::sdtNone)
   {
      return 0.0; // no deck, no load
   }

   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      return 0.0;
   }

   // make sure model data is up to date
   ValidateSlabOffsetDesignModel(pConfig);

   PoiIDPairType femPoiID = m_CacheConfig_SlabOffsetDesignModel.PoiMap.GetModelPoi(poi);
   if ( femPoiID.first == INVALID_ID )
   {
      pgsPointOfInterest thePOI;
      if ( poi.GetID() == INVALID_ID )
      {
         const CSegmentKey& segmentKey = poi.GetSegmentKey();

         GET_IFACE(IPointOfInterest,pPOI);
         thePOI = pPOI->GetPointOfInterest(segmentKey,poi.GetDistFromStart());
      }
      else
      {
         thePOI = poi;
      }

      femPoiID = AddPointOfInterest(m_CacheConfig_SlabOffsetDesignModel,thePOI);
      ATLASSERT( 0 <= femPoiID.first );
   }

   CComQIPtr<IFem2dModelResults> results(m_CacheConfig_SlabOffsetDesignModel.Model);

   Float64 fxRight, fyRight, mzRight;
   HRESULT hr = results->ComputePOIForces(lcid,femPoiID.first,mftRight,lotMember,&fxRight,&fyRight,&mzRight);
   ATLASSERT( SUCCEEDED(hr) );

   return mzRight;
}

void CAnalysisAgentImp::GetDesignDeflectionAdjustment(LoadCaseIDType lcid, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const
{
   *pDy = 0.0;
   *pRz = 0.0;

   if (pConfig == nullptr)
      return; // no design adjustment


   // returns the difference in moment between the slab moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   if (deckType == pgsTypes::sdtNone)
   {
      *pDy = 0.0;
      *pRz = 0.0; // no deck, no load
      return;
   }

   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      *pDy = 0.0;
      *pRz = 0.0;
   }
   else
   {
      // make sure model data is up to date
      ValidateSlabOffsetDesignModel(pConfig);

      PoiIDPairType femPoiID = m_CacheConfig_SlabOffsetDesignModel.PoiMap.GetModelPoi(poi);
      if ( femPoiID.first == INVALID_ID )
      {
         pgsPointOfInterest thePOI;
         if ( poi.GetID() == INVALID_ID )
         {
            const CSegmentKey& segmentKey = poi.GetSegmentKey();

            GET_IFACE(IPointOfInterest,pPOI);
            thePOI = pPOI->GetPointOfInterest(segmentKey,poi.GetDistFromStart());
         }
         else
         {
            thePOI = poi;
         }

         femPoiID = AddPointOfInterest(m_CacheConfig_SlabOffsetDesignModel,thePOI);
         ATLASSERT( 0 <= femPoiID.first );
      }

      CComQIPtr<IFem2dModelResults> results(m_CacheConfig_SlabOffsetDesignModel.Model);

      Float64 Dx;
      HRESULT hr = results->ComputePOIDeflections(lcid,femPoiID.first,lotGlobal,&Dx,pDy,pRz);
      ATLASSERT( SUCCEEDED(hr) );
   }
}

Float64 CAnalysisAgentImp::GetDesignSlabDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const
{
   Float64 dy,rz;
   GetDesignSlabDeflectionAdjustment(poi,pConfig,&dy,&rz);
   return dy;
}

void  CAnalysisAgentImp::GetDesignSlabStressAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pfTop,Float64* pfBot) const
{
   ATLASSERT(pConfig != nullptr);

   // returns the difference in top and bottom girder stress between the stresses caused by the current slab
   // and the input value.
   Float64 M = GetDesignSlabMomentAdjustment(poi,pConfig);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   GET_IFACE(ISectionProperties, pSectProp);
   Float64 Cat, Cbt;
   pSectProp->GetStressCoefficients(castDeckIntervalIdx, poi, pgsTypes::TopGirder, pConfig, &Cat, &Cbt);

   Float64 Cab, Cbb;
   pSectProp->GetStressCoefficients(castDeckIntervalIdx, poi, pgsTypes::BottomGirder, pConfig, &Cab, &Cbb);

   *pfTop = Cbt*M;
   *pfBot = Cbb*M;
}

Float64 CAnalysisAgentImp::GetDesignSlabPadMomentAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const
{
   ATLASSERT(pConfig != nullptr);
   return GetDesignMomentAdjustment(g_lcidDesignSlabPad, poi, pConfig);
}

Float64 CAnalysisAgentImp::GetDesignSlabPadDeflectionAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const
{
   ATLASSERT(pConfig != nullptr);
   Float64 dy,rz;
   GetDesignSlabPadDeflectionAdjustment(poi,pConfig,&dy,&rz);
   return dy;
}

void CAnalysisAgentImp::GetDesignSlabPadStressAdjustment(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pfTop,Float64* pfBot) const
{
   ATLASSERT(pConfig != nullptr);

   // returns the difference in top and bottom girder stress between the stresses caused by the current slab pad
   // and the input value.
   Float64 M = GetDesignSlabPadMomentAdjustment(poi,pConfig);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   GET_IFACE(ISectionProperties, pSectProp);
   Float64 Cat, Cbt;
   pSectProp->GetStressCoefficients(castDeckIntervalIdx, poi, pgsTypes::TopGirder, pConfig, &Cat, &Cbt);

   Float64 Cab, Cbb;
   pSectProp->GetStressCoefficients(castDeckIntervalIdx, poi, pgsTypes::BottomGirder, pConfig, &Cab, &Cbb);

   *pfTop = Cbt*M;
   *pfBot = Cbb*M;
}

void CAnalysisAgentImp::DumpAnalysisModels(GirderIndexType gdrIdx) const
{
   m_pSegmentModelManager->DumpAnalysisModels(gdrIdx);
   m_pGirderModelManager->DumpAnalysisModels(gdrIdx);
}

void CAnalysisAgentImp::GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64* pftop,Float64* pfbot) const
{
   m_pGirderModelManager->GetDeckShrinkageStresses(poi,pftop,pfbot);
}

void CAnalysisAgentImp::GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64 fcGdr,Float64* pftop,Float64* pfbot) const
{
   m_pGirderModelManager->GetDeckShrinkageStresses(poi,fcGdr,pftop,pfbot);
}

std::vector<Float64> CAnalysisAgentImp::GetTimeStepPrestressAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning || pfType == pgsTypes::pftSecondaryEffects);
   ATLASSERT(bat == pgsTypes::ContinuousSpan);

   std::vector<Float64> P;
   P.reserve(vPoi.size());

   GET_IFACE(ILosses,pILosses);
   for (const auto& poi : vPoi)
   {
      const LOSSDETAILS* pLossDetails = pILosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails = pLossDetails->TimeStepDetails[intervalIdx];

      Float64 dP = 0;
      if ( resultsType == rtIncremental )
      {
         dP = tsDetails.dPi[pfType]; // incremental
      }
      else
      {
         dP = tsDetails.Pi[pfType]; // cumulative
      }
      P.push_back(dP);
   }

   return P;
}

std::vector<Float64> CAnalysisAgentImp::GetTimeStepPrestressMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   ATLASSERT(pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning || pfType == pgsTypes::pftSecondaryEffects);
   ATLASSERT(bat == pgsTypes::ContinuousSpan);

   std::vector<Float64> M;
   M.reserve(vPoi.size());


   GET_IFACE(ILosses,pILosses);

   for (const auto& poi : vPoi)
   {
      const LOSSDETAILS* pLossDetails = pILosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails = pLossDetails->TimeStepDetails[intervalIdx];

      Float64 dM = 0;
      if ( resultsType == rtIncremental )
      {
         dM = tsDetails.dMi[pfType]; // incremental
      }
      else
      {
         dM = tsDetails.Mi[pfType]; // cumulative
      }
      M.push_back(dM);
   }

   return M;
}

void CAnalysisAgentImp::ApplyPrecamberElevationAdjustment(IntervalIndexType intervalIdx, const PoiList& vPoi, std::vector<Float64>* pDeflection1, std::vector<Float64>* pDeflection2) const
{
   GET_IFACE(IGirder, pGirder);
   std::vector<Float64> vAdjustments;
   vAdjustments.reserve(vPoi.size());
   for (const auto& poi : vPoi)
   {
      Float64 precamber = pGirder->GetPrecamber(poi);
      vAdjustments.push_back(precamber);
   }

   if (pDeflection1)
   {
      std::transform(pDeflection1->cbegin(), pDeflection1->cend(), vAdjustments.cbegin(), pDeflection1->begin(), [](const auto& a, const auto& b) {return a + b;});
   }

   if (pDeflection2)
   {
      std::transform(pDeflection2->cbegin(), pDeflection2->cend(), vAdjustments.cbegin(), pDeflection2->begin(), [](const auto& a, const auto& b) {return a + b;});
   }
}

void CAnalysisAgentImp::ApplyPrecamberRotationAdjustment(IntervalIndexType intervalIdx, const PoiList& vPoi, std::vector<Float64>* pRotation1, std::vector<Float64>* pRotation2) const
{
   GET_IFACE(IGirder, pGirder);
   std::vector<Float64> vAdjustments;
   vAdjustments.reserve(vPoi.size());
   for (const auto& poi : vPoi)
   {
      Float64 precamber = pGirder->GetPrecamberSlope(poi);
      vAdjustments.push_back(precamber);
   }

   if (pRotation1)
   {
      std::transform(pRotation1->cbegin(), pRotation1->cend(), vAdjustments.cbegin(), pRotation1->begin(), [](const auto& a, const auto& b) {return a + b;});
   }

   if (pRotation2)
   {
      std::transform(pRotation2->cbegin(), pRotation2->cend(), vAdjustments.cbegin(), pRotation2->begin(), [](const auto& a, const auto& b) {return a + b;});
   }
}

void CAnalysisAgentImp::ApplyElevationAdjustment(IntervalIndexType intervalIdx,const PoiList& vPoi,std::vector<Float64>* pDeflection1,std::vector<Float64>* pDeflection2) const
{
   GET_IFACE(IBridge,pBridge);
   std::vector<Float64> vAdjustments;
   vAdjustments.reserve(vPoi.size());
   for (const auto& poi : vPoi)
   {
      Float64 elevAdj = pBridge->GetElevationAdjustment(intervalIdx,poi);
      vAdjustments.push_back(elevAdj);
   }

   if ( pDeflection1 )
   {
      std::transform(pDeflection1->cbegin(), pDeflection1->cend(), vAdjustments.cbegin(), pDeflection1->begin(), [](const auto& a, const auto& b) {return a + b;});
   }

   if ( pDeflection2 )
   {
      std::transform(pDeflection2->cbegin(),pDeflection2->cend(),vAdjustments.cbegin(),pDeflection2->begin(), [](const auto& a, const auto& b) {return a + b;});
   }
}

void CAnalysisAgentImp::ApplyRotationAdjustment(IntervalIndexType intervalIdx,const PoiList& vPoi,std::vector<Float64>* pRotation1,std::vector<Float64>* pRotation2) const
{
   GET_IFACE(IBridge,pBridge);
   std::vector<Float64> vAdjustments;
   vAdjustments.reserve(vPoi.size());
   for (const auto& poi : vPoi)
   {
      Float64 slopeAdj = pBridge->GetRotationAdjustment(intervalIdx,poi);
      vAdjustments.push_back(slopeAdj);
   }

   if ( pRotation1 )
   {
      std::transform(pRotation1->cbegin(),pRotation1->cend(),vAdjustments.cbegin(),pRotation1->begin(),[](const auto& a, const auto& b) {return a + b;});
   }

   if ( pRotation2 )
   {
      std::transform(pRotation2->cbegin(),pRotation2->cend(),vAdjustments.cbegin(),pRotation2->begin(),[](const auto& a, const auto& b) {return a + b;});
   }
}

std::vector<std::_tstring> CAnalysisAgentImp::GetVehicleNames(pgsTypes::LiveLoadType llType, const CGirderKey& girderKey) const
{
   return m_pGirderModelManager->GetVehicleNames(llType, girderKey);
}

std::_tstring CAnalysisAgentImp::GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const
{
   return m_pGirderModelManager->GetLiveLoadName(llType,vehicleIdx);
}

pgsTypes::LiveLoadApplicabilityType CAnalysisAgentImp::GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const
{
   return m_pGirderModelManager->GetLiveLoadApplicability(llType,vehicleIdx);
}

VehicleIndexType CAnalysisAgentImp::GetVehicleCount(pgsTypes::LiveLoadType llType) const
{
   return m_pGirderModelManager->GetVehicleCount(llType);
}

Float64 CAnalysisAgentImp::GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx) const
{
   return m_pGirderModelManager->GetVehicleWeight(llType,vehicleIdx);
}

/////////////////////////////////////////////////////////////////////////////
// IProductForces2
//
std::vector<Float64> CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   if ( pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning )
   {
      GET_IFACE( ILossParameters, pLossParams);
      if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         return GetTimeStepPrestressAxial(intervalIdx,pfType,vPoi,bat,resultsType);
      }
      else
      {
#if defined _DEBUG
         GET_IFACE(IPointOfInterest,pPoi);
         std::vector<CSegmentKey> vSegmentKeys;
         pPoi->GetSegmentKeys(vPoi, &vSegmentKeys);
         ATLASSERT(vSegmentKeys.size() == 1); // this method assumes all the poi are for the same segment
#endif
         // This is not time-step analysis.
         // The pretension effects are handled in the segment and girder models for
         // elastic analysis... we want to use the code further down in this method.
         if ( pfType == pgsTypes::pftPretension )
         {
            // for elastic analysis, force effects due to pretensioning are always those at release
            if ( resultsType == rtCumulative )
            {
               CSegmentKey segmentKey = vPoi.front().get().GetSegmentKey();
               GET_IFACE(IIntervals,pIntervals);
               IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
               intervalIdx = releaseIntervalIdx;
            }
         }

         // Post-tensioning is not modeled for linear elastic analysis, so the
         // results are zero
         if ( pfType == pgsTypes::pftPostTensioning || pfType == pgsTypes::pftSecondaryEffects )
         {
            results.resize(vPoi.size(),0.0);
            return results;
         }
      }
   }

   if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      GET_IFACE( ILossParameters, pLossParams);
      if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
         ComputeTimeDependentEffects(girderKey,intervalIdx);
         
         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),0);
            GET_IFACE(ILosses,pLosses);
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
                  std::vector<Float64> fx = GetAxial(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.cbegin(),results.cend(),fx.cbegin(),results.begin(),[](const auto& a, const auto& b) {return a + b;});
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               GET_IFACE(ILosses,pLosses);
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,pfType - pgsTypes::pftCreep);
               results = GetAxial(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),0);
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),0.0);
      }
      return results;
   }

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetAxial(intervalIdx,pfType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Aprev = GetAxial(intervalIdx-1,pfType,vPoi,bat,rtCumulative);
         std::vector<Float64> Athis = GetAxial(intervalIdx,  pfType,vPoi,bat,rtCumulative);
         std::transform(Athis.cbegin(),Athis.cend(),Aprev.cbegin(),std::back_inserter(results),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         results = m_pGirderModelManager->GetAxial(intervalIdx,pfType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   return results;
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<sysSectionValue> results;
   results.reserve(vPoi.size());

   if ( pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning )
   {
      // pre and post-tensioning don't cause external shear forces.
      // There is a vertical component of prestress, Vp, that is used
      // in shear analysis, but that isn't this...
      results.resize(vPoi.size(),sysSectionValue(0,0));
      return results;
   }


   if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      GET_IFACE( ILossParameters, pLossParams);
      if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         ComputeTimeDependentEffects(vPoi.front().get().GetSegmentKey(),intervalIdx);
         
         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),sysSectionValue(0,0));
            GET_IFACE(ILosses,pLosses);
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(vPoi.front().get().GetSegmentKey());
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
                  std::vector<sysSectionValue> fy = GetShear(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.cbegin(),results.cend(),fy.cbegin(),results.begin(),[](const auto& a, const auto& b) {return a + b;});
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               GET_IFACE(ILosses,pLosses);
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,pfType - pgsTypes::pftCreep);
               results = GetShear(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),sysSectionValue(0,0));
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),sysSectionValue(0,0));
      }
      return results;
   }

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetShear(intervalIdx,pfType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<sysSectionValue> Vprev = GetShear(intervalIdx-1,pfType,vPoi,bat,rtCumulative);
         std::vector<sysSectionValue> Vthis = GetShear(intervalIdx,  pfType,vPoi,bat,rtCumulative);
         std::transform(Vthis.cbegin(),Vthis.cend(),Vprev.cbegin(),std::back_inserter(results),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         // after erection - results are in the girder models
         results = m_pGirderModelManager->GetShear(intervalIdx,pfType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   if ( pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning )
   {
      GET_IFACE( ILossParameters, pLossParams);
      if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         return GetTimeStepPrestressMoment(intervalIdx,pfType,vPoi,bat,resultsType);
      }
      else
      {
         // Post-tensioning is not modeled for linear elastic analysis, so the
         // results are zero
         if ( pfType == pgsTypes::pftPostTensioning || pfType == pgsTypes::pftSecondaryEffects )
         {
            results.resize(vPoi.size(),0.0);
            return results;
         }
         else
         {
            // Pretension moment is just P*e... if we use the Segment or Girder Models
            // we'll get the moments for the equivalent loads. If there is precamber, then
            // the is a uniform load and it doesn't give us the correct results... consider
            // a precambered girder with only straight strands. The moment we want is P*e but
            // the moment we'll get from the model is P*e + wL^2/8
            GET_IFACE(IPretensionForce, pPSForce);
            GET_IFACE(IStrandGeometry, pStrandGeom);
            for (const auto& poi : vPoi)
            {
               Float64 fpes = pPSForce->GetEffectivePrestress(poi, pgsTypes::Straight, intervalIdx, pgsTypes::End);
               Float64 Ns;
               Float64 es = pStrandGeom->GetEccentricity(intervalIdx, poi, pgsTypes::Straight, &Ns);
               Float64 As = pStrandGeom->GetStrandArea(poi, intervalIdx, pgsTypes::Straight);

               Float64 fpeh = pPSForce->GetEffectivePrestress(poi, pgsTypes::Harped, intervalIdx, pgsTypes::End);
               Float64 Nh;
               Float64 eh = pStrandGeom->GetEccentricity(intervalIdx, poi, pgsTypes::Harped, &Nh);
               Float64 Ah = pStrandGeom->GetStrandArea(poi, intervalIdx, pgsTypes::Harped);

               Float64 fpet = pPSForce->GetEffectivePrestress(poi, pgsTypes::Temporary, intervalIdx, pgsTypes::End);
               Float64 Nt;
               Float64 et = pStrandGeom->GetEccentricity(intervalIdx, poi, pgsTypes::Temporary, &Nt);
               Float64 At = pStrandGeom->GetStrandArea(poi, intervalIdx, pgsTypes::Temporary);

               Float64 Ps = -fpes*As;
               Float64 Ph = -fpeh*Ah;
               Float64 Pt = -fpet*At;

               Float64 M = Ps*es + Ph*eh + Pt*et;

               results.push_back(M);
            }
            return results;
         }
      }
   }

   if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      GET_IFACE( ILossParameters, pLossParams);
      if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
         ComputeTimeDependentEffects(girderKey,intervalIdx);
      
         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),0);
            GET_IFACE_NOCHECK(ILosses,pLosses);
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
                  std::vector<Float64> mz = GetMoment(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.cbegin(),results.cend(),mz.cbegin(),results.begin(),[](const auto& a, const auto& b) {return a + b;});
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               GET_IFACE(ILosses,pLosses);
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,pfType - pgsTypes::pftCreep);
               results = GetMoment(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),0.0);
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),0.0);
      }
      return results;
   }

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetMoment(intervalIdx,pfType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Mprev = GetMoment(intervalIdx-1,pfType,vPoi,bat,rtCumulative);
         std::vector<Float64> Mthis = GetMoment(intervalIdx,  pfType,vPoi,bat,rtCumulative);
         std::transform(Mthis.cbegin(),Mthis.cend(),Mprev.cbegin(),std::back_inserter(results),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         results = m_pGirderModelManager->GetMoment(intervalIdx,pfType,vPoi,bat,resultsType);
      }

   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment, bool bIncludePrecamber) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> deflections;
   deflections.reserve(vPoi.size());
   if ( pfType == pgsTypes::pftSecondaryEffects )
   {
      deflections.resize(vPoi.size(),0.0);
   }
   else if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      CGirderKey girderKey(vPoi.front().get().GetSegmentKey());

      ComputeTimeDependentEffects(girderKey,intervalIdx);

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
      IntervalIndexType beforeErectionFirstIntervalIdx, beforeErectionLastIntervalIdx;
      IntervalIndexType afterErectionFirstIntervalIdx, afterErectionLastIntervalIdx;
      if ( resultsType == rtIncremental )
      {
         if ( intervalIdx <= erectionIntervalIdx )
         {
            beforeErectionFirstIntervalIdx = intervalIdx;
            beforeErectionLastIntervalIdx = intervalIdx;
            afterErectionFirstIntervalIdx = 1;
            afterErectionLastIntervalIdx = 0;
         }
         else
         {
            // if the interval is after erection, we don't want to go through the before erection intervals
            beforeErectionFirstIntervalIdx = 1;
            beforeErectionLastIntervalIdx  = 0;
            afterErectionFirstIntervalIdx = intervalIdx;
            afterErectionLastIntervalIdx = intervalIdx;
         }
      }
      else
      {
         beforeErectionFirstIntervalIdx = releaseIntervalIdx;
         beforeErectionLastIntervalIdx = Min(erectionIntervalIdx,intervalIdx);
         afterErectionFirstIntervalIdx = erectionIntervalIdx+1;
         afterErectionLastIntervalIdx = intervalIdx;
      }

      GET_IFACE_NOCHECK(ILosses,pLosses);
      GET_IFACE_NOCHECK(IPointOfInterest,pPoi);
      deflections.resize(vPoi.size(),0);

      // add up all deflections that occur before and at erection taking into
      // account that the deflection measurement datum changes because support locations
      // can change.
      for ( IntervalIndexType iIdx = beforeErectionFirstIntervalIdx; iIdx <= beforeErectionLastIntervalIdx; iIdx++ )
      {
         Float64 intervalDuration = pIntervals->GetDuration(iIdx);
         if ( ::IsGT(0.0,intervalDuration) )
         {
            CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
            std::vector<Float64> dy = GetDeflection(iIdx,strLoadingName,vPoi,bat,rtIncremental,false);
            std::transform(deflections.cbegin(),deflections.cend(),dy.cbegin(),deflections.begin(),[](const auto& a, const auto& b) {return a + b;});
         }

         // adjust for change in support locations
         if ( releaseIntervalIdx < iIdx && ::IsZero(intervalDuration) )
         {
            // changing support locations is considered to be a sudden change in loading conditions
            // this only happens in zero duration intervals
            std::vector<Float64>::iterator deflIter = deflections.begin();

            std::list<PoiList> lPoi;
            pPoi->GroupBySegment(vPoi, &lPoi);
            for ( const auto& vSegmentPoi : lPoi)
            {
               CSegmentKey segmentKey(vSegmentPoi.front().get().GetSegmentKey());
               std::vector<std::pair<pgsPointOfInterest,IntervalIndexType>> vSupportPoi = GetSegmentPreviousSupportLocations(segmentKey,iIdx);

               Float64 Dy1 = GetDeflection(vSupportPoi.front().second,pfType,vSupportPoi.front().first,bat,rtCumulative,false);
               Float64 Dy2 = GetDeflection(vSupportPoi.back().second, pfType,vSupportPoi.back().first, bat,rtCumulative,false);

               if ( vSupportPoi.front().second == iIdx )
               {
                  Dy1 *= -1;
               }

               if ( vSupportPoi.back().second == iIdx )
               {
                  Dy2 *= -1;
               }

               Float64 Xg1 = pPoi->ConvertPoiToGirderPathCoordinate(vSupportPoi.front().first);
               Float64 Xg2 = pPoi->ConvertPoiToGirderPathCoordinate(vSupportPoi.back().first);

               for( const auto& poi : vSegmentPoi)
               {
                  Float64 Xg = pPoi->ConvertPoiToGirderPathCoordinate(poi);
                  Float64 d = ::LinInterp(Xg-Xg1,Dy1,Dy2,Xg2-Xg1);

                  *deflIter -= d;
                  deflIter++;
               }
            }
         }
      } // next interval

      // add deflection after erection. the support locations no longer change
      for ( IntervalIndexType iIdx = afterErectionFirstIntervalIdx; iIdx <= afterErectionLastIntervalIdx; iIdx++ )
      {
         if ( 0 < pIntervals->GetDuration(iIdx) )
         {
            CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
            std::vector<Float64> dy = GetDeflection(iIdx,strLoadingName,vPoi,bat,rtIncremental,false);
            std::transform(deflections.cbegin(),deflections.cend(),dy.cbegin(),deflections.begin(),[](const auto& a, const auto& b) {return a + b;});
         }
      }
   }
   else
   {
      try
      {
         IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

         if ( intervalIdx < erectionIntervalIdx || pfType == pgsTypes::pftPretension )
         {
            // before erection - results are in the segment models
            // also... pretension erections are always computed from the segment models
            deflections = m_pSegmentModelManager->GetDeflection(intervalIdx,pfType,vPoi,resultsType);
         }
         else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
         {
            // the incremental result at the time of erection is being requested. this is when
            // we switch between segment models and girder models. the incremental results
            // is the cumulative result this interval minus the cumulative result in the previous interval
            std::vector<Float64> Dprev = GetDeflection(intervalIdx-1,pfType,vPoi,bat,rtCumulative,false);
            std::vector<Float64> Dthis = GetDeflection(intervalIdx,  pfType,vPoi,bat,rtCumulative,false);
            std::transform(Dthis.cbegin(),Dthis.cend(),Dprev.cbegin(),std::back_inserter(deflections),[](const auto& a, const auto& b) {return a - b;});
         }
         else
         {
            deflections = m_pGirderModelManager->GetDeflection(intervalIdx,pfType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
         throw;
      }
   }

   if (pfType == pgsTypes::pftGirder && bIncludePrecamber)
   {
      ApplyPrecamberElevationAdjustment(intervalIdx, vPoi, &deflections, nullptr);
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,&deflections,nullptr);
   }

   return deflections;
}

std::vector<Float64> CAnalysisAgentImp::GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::ProductForceType pfType, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   // assume there aren't any lateral deflections once the girder is composite in the bridge system
   std::vector<Float64> vDelta;
   vDelta.reserve(vPoi.size());

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();
   if (resultsType == rtIncremental && compositeIntervalIdx <= intervalIdx)
   {
      // want incremental results after composite... since we've assumed there aren't any lateral deflections for
      // this case, the increment deflections are zero
      vDelta.resize(vPoi.size(), 0.0);
   }
   else
   {
      if (pfType == pgsTypes::pftPretension)
      {
         // pretensioning is handled a little differently since there can be loads in both the vertical and lateral directions
         // (all other load types are just vertical/gravity loads).... if the CG of the prestressing is laterally offset from the CG of the section
         // lateral moments and deflections result
         vDelta = m_pSegmentModelManager->GetPretensionXDeflection(intervalIdx, vPoi, resultsType);
      }
      else
      {
         // lateral deflections are a function of vertical deflections... 
         // we don't want vertical deflections after the girder is composite
         // so limit the interval index up to, but not including, the composite interval index
         intervalIdx = Min(intervalIdx, compositeIntervalIdx - 1);

         // Y deflections are based on mid-span properties
         GET_IFACE(IPointOfInterest, pPoi);
         PoiList vMyPoi;
         pPoi->GetPointsOfInterest(vPoi.front().get().GetSegmentKey(), POI_5L | POI_RELEASED_SEGMENT, &vMyPoi);
         ATLASSERT(vMyPoi.size() == 1);
         const pgsPointOfInterest& spPoi = vMyPoi.front();
         ATLASSERT(spPoi.IsMidSpan(POI_RELEASED_SEGMENT));

         // we have to scale the vertical deflections by (-Ixy/Iyy) to get the lateral deflection
         // we must use the section properties from the same interval used to create the analysis model
         // that is why we need to loop over intervals
         IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().get().GetSegmentKey());
         IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

         IntervalIndexType startIntervalIdx = (resultsType == rtIncremental ? intervalIdx : releaseIntervalIdx);

         vDelta.resize(vPoi.size(), 0.0);

         GET_IFACE(ISectionProperties, pSectProp);
         GET_IFACE(IBridge, pBridge);
         if (pBridge->HasTiltedGirders() && intervalIdx == erectionIntervalIdx)
         {
            // account for the girder being erected in a tilted orientation
            // vertical deflection deltaY was computed using Ixx... we want a lateral deflection using Iyy
            // also, vertical deflection is based on full weight of the girder going downward, we need to
            // adjust the deflection for a component of the vertical gravity load being applied at the girder orientation slope
            // this is the dead load component causing lateral deflection

            std::vector<Float64> deltaY = GetDeflection(intervalIdx, pfType, vPoi, bat, rtCumulative, false); // this is the deflection based on Eci at release

            Float64 Ixx = pSectProp->GetIxx(intervalIdx, spPoi);
            Float64 Iyy = pSectProp->GetIyy(intervalIdx, spPoi);

            // Deflection due to tilting occurs at erection so we need to use the erection material properties
            // deltaY is based on Eci at release... 
            GET_IFACE(IMaterials, pMaterials);
            Float64 Eci = pMaterials->GetSegmentEc(spPoi.GetSegmentKey(), releaseIntervalIdx);
            Float64 Ec = pMaterials->GetSegmentEc(spPoi.GetSegmentKey(), erectionIntervalIdx);

            GET_IFACE(IGirder, pGirder);
            Float64 girder_orientation = pGirder->GetOrientation(spPoi.GetSegmentKey());

            // girder orientation adjusts vertical girder self-weight load, to a self-weight load component transverse to the girder
            // if the girder is plumb, girder_orietation = 0
            Float64 D = -girder_orientation*Eci*Ixx /(Ec*Iyy);
            std::transform(deltaY.cbegin(), deltaY.cend(), vDelta.cbegin(), vDelta.begin(), [&D](const auto& dY, auto& cummDx) {return cummDx + dY*D;});
         }
         else
         {
            for (IntervalIndexType iIdx = startIntervalIdx; iIdx <= intervalIdx; iIdx++)
            {
               Float64 Iyy = pSectProp->GetIyy(iIdx, spPoi);
               Float64 Ixy = pSectProp->GetIxy(iIdx, spPoi);

               std::vector<Float64> deltaY = GetDeflection(iIdx, pfType, vPoi, bat, rtIncremental, false); // these are the incremental vertical deflections in interval iIdx

               // lateral deflections are (delta Y)*(-Ixy/Iyy)
               // scale deltaY by D and add to the running sum
               Float64 D = -Ixy / Iyy;
               std::transform(deltaY.cbegin(), deltaY.cend(), vDelta.cbegin(), vDelta.begin(), [&D](const auto& dY, auto& cummDx) {return cummDx + dY*D;});
            }
         }
      }
   }

   return vDelta;
}

std::vector<Float64> CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment, bool bIncludePrecamber) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> rotations;
   rotations.reserve(vPoi.size());

   if ( pfType == pgsTypes::pftSecondaryEffects )
   {
      rotations.resize(vPoi.size(),0.0);
      return rotations;
   }
   else if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
      ComputeTimeDependentEffects(girderKey,intervalIdx);

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
      IntervalIndexType beforeErectionFirstIntervalIdx, beforeErectionLastIntervalIdx;
      IntervalIndexType afterErectionFirstIntervalIdx, afterErectionLastIntervalIdx;
      if ( resultsType == rtIncremental )
      {
         if ( intervalIdx <= erectionIntervalIdx )
         {
            beforeErectionFirstIntervalIdx = intervalIdx;
            beforeErectionLastIntervalIdx = intervalIdx;
            afterErectionFirstIntervalIdx = 1;
            afterErectionLastIntervalIdx = 0;
         }
         else
         {
            // if the interval is after erection, we don't want to go through the before erection intervals
            beforeErectionFirstIntervalIdx = 1;
            beforeErectionLastIntervalIdx  = 0;
            afterErectionFirstIntervalIdx = intervalIdx;
            afterErectionLastIntervalIdx = intervalIdx;
         }
      }
      else
      {
         beforeErectionFirstIntervalIdx = releaseIntervalIdx;
         beforeErectionLastIntervalIdx = Min(erectionIntervalIdx,intervalIdx);
         afterErectionFirstIntervalIdx = erectionIntervalIdx+1;
         afterErectionLastIntervalIdx = intervalIdx;
      }
      GET_IFACE_NOCHECK(ILosses,pLosses);
      GET_IFACE_NOCHECK(IPointOfInterest,pPoi);
      rotations.resize(vPoi.size(),0);

      // add up all deflections that occur before and at erection taking into
      // account that the deflection measurement datum changes because support locations
      // can change.
      for ( IntervalIndexType iIdx = beforeErectionFirstIntervalIdx; iIdx <= beforeErectionLastIntervalIdx; iIdx++ )
      {
         Float64 intervalDuration = pIntervals->GetDuration(iIdx);
         if ( ::IsGT(0.0,intervalDuration) )
         {
            CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
            std::vector<Float64> rz = GetRotation(iIdx,strLoadingName,vPoi,bat,rtIncremental,false);
            std::transform(rotations.cbegin(),rotations.cend(),rz.cbegin(),rotations.begin(),[](const auto& a, const auto& b) {return a + b;});
         }

         // adjust for change in support locations
         if ( releaseIntervalIdx < iIdx && ::IsZero(intervalDuration) )
         {
            // changing support locations is considered to be a sudden change in loading conditions
            // this only happens in zero duration intervals
            std::vector<Float64>::iterator rotIter = rotations.begin();

            std::list<PoiList> lPoi;
            pPoi->GroupBySegment(vPoi,&lPoi);
            for (const auto& vSegmentPoi : lPoi)
            {
               CSegmentKey segmentKey(vSegmentPoi.front().get().GetSegmentKey());
               std::vector<std::pair<pgsPointOfInterest,IntervalIndexType>> vSupportPoi = GetSegmentPreviousSupportLocations(segmentKey,iIdx);

               Float64 Dy1 = GetDeflection(vSupportPoi.front().second,pfType,vSupportPoi.front().first,bat,rtCumulative,false);
               Float64 Dy2 = GetDeflection(vSupportPoi.back().second, pfType,vSupportPoi.back().first, bat,rtCumulative,false);

               if ( vSupportPoi.front().second == iIdx )
               {
                  Dy1 *= -1;
               }

               if ( vSupportPoi.back().second == iIdx )
               {
                  Dy2 *= -1;
               }

               Float64 Xg1 = pPoi->ConvertPoiToGirderPathCoordinate(vSupportPoi.front().first);
               Float64 Xg2 = pPoi->ConvertPoiToGirderPathCoordinate(vSupportPoi.back().first);

               Float64 r = (Dy2 - Dy1)/(Xg2 - Xg1);

               for( const auto& poi : vSegmentPoi)
               {
                  *rotIter -= r;
                  rotIter++;
               }
            }
         }
      } // next interval

      // add deflection after erection. the support locations no longer change
      for ( IntervalIndexType iIdx = afterErectionFirstIntervalIdx; iIdx <= afterErectionLastIntervalIdx; iIdx++ )
      {
         if ( 0 < pIntervals->GetDuration(iIdx) )
         {
            CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
            std::vector<Float64> rz = GetRotation(iIdx,strLoadingName,vPoi,bat,rtIncremental,false);
            std::transform(rotations.cbegin(),rotations.cend(),rz.cbegin(),rotations.begin(),[](const auto& a, const auto& b) {return a + b;});
         }
      }
   }
   else
   {
      try
      {
         IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

         if ( intervalIdx < erectionIntervalIdx || pfType == pgsTypes::pftPretension )
         {
            // before erection - results are in the segment models
            rotations = m_pSegmentModelManager->GetRotation(intervalIdx,pfType,vPoi,resultsType);
         }
         else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
         {
            // the incremental result at the time of erection is being requested. this is when
            // we switch between segment models and girder models. the incremental results
            // is the cumulative result this interval minus the cumulative result in the previous interval
            std::vector<Float64> Rprev = GetRotation(intervalIdx-1,pfType,vPoi,bat,rtCumulative,false);
            std::vector<Float64> Rthis = GetRotation(intervalIdx,  pfType,vPoi,bat,rtCumulative,false);
            std::transform(Rthis.cbegin(),Rthis.cend(),Rprev.cbegin(),std::back_inserter(rotations),[](const auto& a, const auto& b) {return a - b;});
         }
         else
         {
            rotations = m_pGirderModelManager->GetRotation(intervalIdx,pfType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
         throw;
      }
   }

   if (pfType == pgsTypes::pftGirder && bIncludePrecamber)
   {
      ApplyPrecamberRotationAdjustment(intervalIdx, vPoi, &rotations, nullptr);
   }

   if ( bIncludeSlopeAdjustment )
   {
      ApplyRotationAdjustment(intervalIdx,vPoi,&rotations,nullptr);
   }

   return rotations;
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      GetTimeStepStress(intervalIdx,pfType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
   }
   else
   {
      GetElasticStress(intervalIdx,pfType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
   }
}

void CAnalysisAgentImp::GetLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck,std::vector<VehicleIndexType>* pMmaxTruck) const
{
   m_pGirderModelManager->GetLiveLoadAxial(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<VehicleIndexType>* pMinTruck,std::vector<VehicleIndexType>* pMaxTruck) const
{
   m_pGirderModelManager->GetLiveLoadShear(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMinTruck,pMaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck,std::vector<VehicleIndexType>* pMmaxTruck) const
{
   m_pGirderModelManager->GetLiveLoadMoment(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig) const
{
   m_pGirderModelManager->GetLiveLoadDeflection(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig) const
{
   m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex,std::vector<VehicleIndexType>* pTopMaxIndex,std::vector<VehicleIndexType>* pBotMinIndex,std::vector<VehicleIndexType>* pBotMaxIndex) const
{
   m_pGirderModelManager->GetLiveLoadStress(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pTopMinIndex,pTopMaxIndex,pBotMinIndex,pBotMaxIndex);
}

void CAnalysisAgentImp::GetVehicularLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig) const
{
   m_pGirderModelManager->GetVehicularLiveLoadAxial(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<AxleConfiguration>* pMinLeftAxleConfig,std::vector<AxleConfiguration>* pMinRightAxleConfig,std::vector<AxleConfiguration>* pMaxLeftAxleConfig,std::vector<AxleConfiguration>* pMaxRightAxleConfig) const
{
   m_pGirderModelManager->GetVehicularLiveLoadShear(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMinLeftAxleConfig,pMinRightAxleConfig,pMaxLeftAxleConfig,pMaxRightAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig) const
{
   m_pGirderModelManager->GetVehicularLiveLoadMoment(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigTop,std::vector<AxleConfiguration>* pMaxAxleConfigTop,std::vector<AxleConfiguration>* pMinAxleConfigBot,std::vector<AxleConfiguration>* pMaxAxleConfigBot) const
{
   m_pGirderModelManager->GetVehicularLiveLoadStress(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pMinAxleConfigTop,pMaxAxleConfigTop,pMinAxleConfigBot,pMaxAxleConfigBot);
}

void CAnalysisAgentImp::GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig) const
{
   m_pGirderModelManager->GetVehicularLiveLoadDeflection(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig) const
{
   m_pGirderModelManager->GetVehicularLiveLoadRotation(intervalIdx,llType,vehicleIdx,vPoi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinAxleConfig,pMaxAxleConfig);
}

/////////////////////////////////////////////////////////////////////////////
// ICombinedForces
//
Float64 CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetAxial(intervalIdx,comboType,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

sysSectionValue CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> V( GetShear(intervalIdx,comboType,vPoi,bat,resultsType) );

   ATLASSERT(V.size() == 1);

   return V.front();
}

Float64 CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetMoment(intervalIdx,comboType,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx, LoadingCombinationType comboType, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType, bool bIncludeElevationAdjustment, bool bIncludePrecamber) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results(GetDeflection(intervalIdx, comboType, vPoi, bat, resultsType, bIncludeElevationAdjustment,bIncludePrecamber));
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetXDeflection(IntervalIndexType intervalIdx, LoadingCombinationType comboType, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results(GetXDeflection(intervalIdx, comboType, vPoi, bat, resultsType));
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment, bool bIncludePrecamber) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetRotation(intervalIdx,comboType,vPoi,bat,resultsType,bIncludeSlopeAdjustment,bIncludePrecamber) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop, fBot;
   GetStress(intervalIdx,comboType,vPoi,bat,resultsType,topLocation,botLocation,&fTop,&fBot);
   
   ATLASSERT(fTop.size() == 1 && fBot.size() == 1);

   *pfTop = fTop.front();
   *pfBot = fBot.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Pmin, Pmax;
   GetCombinedLiveLoadAxial(intervalIdx,llType,vPoi,bat,&Pmin,&Pmax);

   ATLASSERT(Pmin.size() == 1 && Pmax.size() == 1);
   *pMin = Pmin.front();
   *pMax = Pmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,sysSectionValue* pVmin,sysSectionValue* pVmax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   GetCombinedLiveLoadShear(intervalIdx,llType,vPoi,bat,bIncludeImpact,&Vmin,&Vmax);

   ATLASSERT( Vmin.size() == 1 && Vmax.size() == 1 );
   *pVmin = Vmin.front();
   *pVmax = Vmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   GetCombinedLiveLoadMoment(intervalIdx,llType,vPoi,bat,&Mmin,&Mmax);

   ATLASSERT(Mmin.size() == 1 && Mmax.size() == 1);
   *pMin = Mmin.front();
   *pMax = Mmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetCombinedLiveLoadDeflection(intervalIdx,llType,vPoi,bat,&Dmin,&Dmax);

   ATLASSERT( Dmin.size() == 1 && Dmax.size() == 1 );
   *pDmin = Dmin.front();
   *pDmax = Dmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Rmin, Rmax;
   GetCombinedLiveLoadRotation(intervalIdx,llType,vPoi,bat,&Rmin,&Rmax);

   ATLASSERT( Rmin.size() == 1 && Rmax.size() == 1 );
   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
   GetCombinedLiveLoadStress(intervalIdx,llType,vPoi,bat,topLocation,botLocation,&fTopMin,&fTopMax,&fBotMin,&fBotMax);

   ATLASSERT( fTopMin.size() == 1 && fTopMax.size() == 1 && fBotMin.size() == 1 && fBotMax.size() == 1 );

   *pfTopMin = fTopMin.front();
   *pfTopMax = fTopMax.front();
   *pfBotMin = fBotMin.front();
   *pfBotMax = fBotMax.front();
}

/////////////////////////////////////////////////////////////////////////////
// ICombinedForces2
//
std::vector<Float64> CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   if ( comboType == lcPS )
   {
      // secondary effects were requested... the LBAM doesn't have secondary effects... get the product load
      // effects that feed into lcPS
      results.resize(vPoi.size(),0.0); // initilize the results vector with 0.0
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      std::vector<pgsTypes::ProductForceType>::iterator pfIter(pfTypes.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(pfTypes.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         pgsTypes::ProductForceType pfType = *pfIter;
         std::vector<Float64> A = GetAxial(intervalIdx,pfType,vPoi,bat,resultsType);

         // add A to results and assign answer to results
         std::transform(A.cbegin(),A.cend(),results.cbegin(),results.begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      return results;
   }

   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      GET_IFACE(ILossParameters,pLossParameters);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
         ComputeTimeDependentEffects(girderKey,intervalIdx);

         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),0);

            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  GET_IFACE(ILosses,pLosses);
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,comboType - lcCR);
                  std::vector<Float64> fx = GetAxial(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.cbegin(),results.cend(),fx.cbegin(),results.begin(),[](const auto& a, const auto& b) {return a + b;});
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               GET_IFACE(ILosses,pLosses);
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,comboType - lcCR);
               results = GetAxial(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),0.0);
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),0.0);
      }
      return results;
   }

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if (intervalIdx < erectionIntervalIdx )
      {
         results = m_pSegmentModelManager->GetAxial(intervalIdx,comboType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Aprev = GetAxial(intervalIdx-1,comboType,vPoi,bat,rtCumulative);
         std::vector<Float64> Athis = GetAxial(intervalIdx,  comboType,vPoi,bat,rtCumulative);
         std::transform(Athis.cbegin(),Athis.cend(),Aprev.cbegin(),std::back_inserter(results),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         results = m_pGirderModelManager->GetAxial(intervalIdx,comboType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   return results;
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<sysSectionValue> results;
   results.reserve(vPoi.size());

   if ( comboType == lcPS )
   {
      // secondary effects were requested... the LBAM doesn't have secondary effects... get the product load
      // effects that feed into lcPS
      results.resize(vPoi.size(),0.0); // initilize the results vector with 0.0
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      std::vector<pgsTypes::ProductForceType>::iterator pfIter(pfTypes.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(pfTypes.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         pgsTypes::ProductForceType pfType = *pfIter;
         std::vector<sysSectionValue> V = GetShear(intervalIdx,pfType,vPoi,bat,resultsType);

         // add V to results and assign answer to results
         std::transform(V.cbegin(),V.cend(),results.cbegin(),results.begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      return results;
   }

   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      GET_IFACE(ILossParameters,pLossParameters);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
         ComputeTimeDependentEffects(girderKey,intervalIdx);
         
         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),sysSectionValue(0,0));
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  GET_IFACE(ILosses,pLosses);
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,comboType - lcCR);
                  std::vector<sysSectionValue> fy = GetShear(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.cbegin(),results.cend(),fy.cbegin(),results.begin(),[](const auto& a, const auto& b) {return a + b;});
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               GET_IFACE(ILosses,pLosses);
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,comboType - lcCR);
               results = GetShear(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),sysSectionValue(0,0));
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),sysSectionValue(0,0));
      }
      return results;
   }

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         results =  m_pSegmentModelManager->GetShear(intervalIdx,comboType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<sysSectionValue> Vprev = GetShear(intervalIdx-1,comboType,vPoi,bat,rtCumulative);
         std::vector<sysSectionValue> Vthis = GetShear(intervalIdx,  comboType,vPoi,bat,rtCumulative);
         std::transform(Vthis.cbegin(),Vthis.cend(),Vprev.cbegin(),std::back_inserter(results),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         results = m_pGirderModelManager->GetShear(intervalIdx,comboType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   if ( comboType == lcPS )
   {
      // secondary effects were requested... the LBAM doesn't have secondary effects... get the product load
      // effects that feed into lcPS
      results.resize(vPoi.size(),0.0); // initilize the results vector with 0.0
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker, comboType);
      for ( const auto& pfType : pfTypes)
      {
         std::vector<Float64> M = GetMoment(intervalIdx,pfType,vPoi,bat,resultsType);

         // add M to results and assign answer to results
         std::transform(M.cbegin(),M.cend(),results.cbegin(),results.begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      return results;
   }

   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      GET_IFACE(ILossParameters,pLossParameters);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
         ComputeTimeDependentEffects(girderKey,intervalIdx);

         if ( resultsType == rtCumulative )
         {
            results.resize(vPoi.size(),0);
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetFirstPrestressReleaseInterval(girderKey);
            for ( IntervalIndexType iIdx = releaseIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  GET_IFACE(ILosses,pLosses);
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,comboType - lcCR);
                  std::vector<Float64> mz = GetMoment(iIdx,strLoadingName,vPoi,bat,rtIncremental);
                  std::transform(results.cbegin(),results.cend(),mz.cbegin(),results.begin(),[](const auto& a, const auto& b) {return a + b;});
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               GET_IFACE(ILosses,pLosses);
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,comboType - lcCR);
               results = GetMoment(intervalIdx,strLoadingName,vPoi,bat,resultsType);
            }
            else
            {
               results.resize(vPoi.size(),0.0);
            }
         }
      }
      else
      {
         results.resize(vPoi.size(),0.0);
      }
      return results;
   }

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if (intervalIdx < erectionIntervalIdx )
      {
         results = m_pSegmentModelManager->GetMoment(intervalIdx,comboType,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Mprev = GetMoment(intervalIdx-1,comboType,vPoi,bat,rtCumulative);
         std::vector<Float64> Mthis = GetMoment(intervalIdx,  comboType,vPoi,bat,rtCumulative);
         std::transform(Mthis.cbegin(),Mthis.cend(),Mprev.cbegin(),std::back_inserter(results),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         results = m_pGirderModelManager->GetMoment(intervalIdx,comboType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment, bool bIncludePrecamber) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> deflection;
   deflection.reserve(vPoi.size());

   if ( comboType == lcPS )
   {
      // secondary effects aren't directly computed. get the individual product forces
      // and sum them up here
      deflection.resize(vPoi.size(),0.0);
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      for( const auto& pfType : pfTypes)
      {
         std::vector<Float64> delta = GetDeflection(intervalIdx,pfType,vPoi,bat,resultsType,false);
         std::transform(delta.cbegin(),delta.cend(),deflection.cbegin(),deflection.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }
   else if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
#if defined _DEBUG
      // must be time-step analysis
      GET_IFACE( ILossParameters, pLossParams);
      ATLASSERT( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP );
#endif

      pgsTypes::ProductForceType pfType;
      switch(comboType)
      {
      case lcCR: pfType = pgsTypes::pftCreep; break;
      case lcSH: pfType = pgsTypes::pftShrinkage; break;
      case lcRE: pfType = pgsTypes::pftRelaxation; break;
      default: ATLASSERT(false); // should not happen
      }

      deflection = GetDeflection(intervalIdx,pfType,vPoi,bat,resultsType,false);
   }
   else
   {
      try
      {
         IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

         if ( intervalIdx < erectionIntervalIdx )
         {
            deflection = m_pSegmentModelManager->GetDeflection(intervalIdx,comboType,vPoi,resultsType);
         }
         else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
         {
            // the incremental result at the time of erection is being requested. this is when
            // we switch between segment models and girder models. the incremental results
            // is the cumulative result this interval minus the cumulative result in the previous interval
            std::vector<Float64> Dprev = GetDeflection(intervalIdx-1,comboType,vPoi,bat,rtCumulative,false);
            std::vector<Float64> Dthis = GetDeflection(intervalIdx,  comboType,vPoi,bat,rtCumulative,false);
            std::transform(Dthis.cbegin(),Dthis.cend(),Dprev.cbegin(),std::back_inserter(deflection),[](const auto& a, const auto& b) {return a - b;});
         }
         else
         {
            deflection = m_pGirderModelManager->GetDeflection(intervalIdx,comboType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
         throw;
      }
   }

   if (comboType == lcDC && bIncludePrecamber)
   {
      ApplyPrecamberElevationAdjustment(intervalIdx, vPoi, &deflection, nullptr);
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,&deflection,nullptr);
   }

   return deflection;
}

std::vector<Float64> CAnalysisAgentImp::GetXDeflection(IntervalIndexType intervalIdx, LoadingCombinationType comboType, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> deflection;
   deflection.reserve(vPoi.size());

   if (comboType == lcPS)
   {
      // secondary effects aren't directly computed. get the individual product forces
      // and sum them up here
      deflection.resize(vPoi.size(), 0.0);
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker, comboType);
      for (const auto& pfType : pfTypes)
      {
         std::vector<Float64> delta = GetXDeflection(intervalIdx, pfType, vPoi, bat, resultsType);
         std::transform(delta.cbegin(), delta.cend(), deflection.cbegin(), deflection.begin(), [](const auto& a, const auto& b) {return a + b;});
      }
   }
   else if (comboType == lcCR || comboType == lcSH || comboType == lcRE)
   {
#if defined _DEBUG
      // must be time-step analysis
      GET_IFACE(ILossParameters, pLossParams);
      ATLASSERT(pLossParams->GetLossMethod() == pgsTypes::TIME_STEP);
#endif

      pgsTypes::ProductForceType pfType;
      switch (comboType)
      {
      case lcCR: pfType = pgsTypes::pftCreep; break;
      case lcSH: pfType = pgsTypes::pftShrinkage; break;
      case lcRE: pfType = pgsTypes::pftRelaxation; break;
      default: ATLASSERT(false); // should not happen
      }

      deflection = GetXDeflection(intervalIdx, pfType, vPoi, bat, resultsType);
   }
   else
   {
      try
      {
         IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

         if (intervalIdx < erectionIntervalIdx)
         {
            deflection.resize(vPoi.size(), 0.0);
            GET_IFACE(IIntervals, pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(vPoi.front().get().GetSegmentKey());
            IntervalIndexType startIntervalIdx = (resultsType == rtIncremental ? intervalIdx : releaseIntervalIdx);

            // deflections are computed using mid-span section properties (see model builders)
            GET_IFACE(ISectionProperties, pSectProp);
            GET_IFACE(IPointOfInterest, pPoi);
            PoiList vMyPoi;
            pPoi->GetPointsOfInterest(vPoi.front().get().GetSegmentKey(), POI_5L | POI_RELEASED_SEGMENT, &vMyPoi);
            ATLASSERT(vMyPoi.size() == 1);
            const pgsPointOfInterest& spPoi = vMyPoi.front();
            ATLASSERT(spPoi.IsMidSpan(POI_RELEASED_SEGMENT));

            for (IntervalIndexType iIdx = startIntervalIdx; iIdx <= intervalIdx; iIdx++)
            {
               std::vector<Float64> deltaY = m_pSegmentModelManager->GetDeflection(iIdx, comboType, vPoi, rtIncremental); // these are the vertical deflections

               Float64 Iyy = pSectProp->GetIyy(iIdx, spPoi);
               Float64 Ixy = pSectProp->GetIxy(iIdx, spPoi);

               // lateral deflections are (delta Y)*(-Ixy/Iyy)
               // scale deltaY by D and add to the running sum
               Float64 D = -Ixy / Iyy;
               std::transform(deltaY.cbegin(), deltaY.cend(), deflection.cbegin(), deflection.begin(), [&D](const auto& dY, auto& cummDx) {return cummDx + dY*D;});
            }
         }
         else if (intervalIdx == erectionIntervalIdx && resultsType == rtIncremental)
         {
            // the incremental result at the time of erection is being requested. this is when
            // we switch between segment models and girder models. the incremental results
            // is the cumulative result this interval minus the cumulative result in the previous interval
            ATLASSERT(intervalIdx != 0);
            std::vector<Float64> Dprev = GetXDeflection(intervalIdx - 1, comboType, vPoi, bat, rtCumulative);
            std::vector<Float64> Dthis = GetXDeflection(intervalIdx, comboType, vPoi, bat, rtCumulative);
            std::transform(Dthis.cbegin(), Dthis.cend(), Dprev.cbegin(), std::back_inserter(deflection), [](const auto& a, const auto& b) {return a - b;});
         }
         else
         {
            GET_IFACE(IIntervals, pIntervals);
            IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();
            // we assume there is no additional lateral deflection after girders become composite in the bridge system
            if (resultsType == rtIncremental && compositeIntervalIdx <= intervalIdx)
            {
               // the bridge is now composite and we want incremental results.... no deflection = 0.0
               deflection.resize(vPoi.size(), 0.0);
            }
            else
            {
               // the interval is either before the girder is composite in the bridge system, or we want
               // cumulative results. for cumulative results, don't use the composite interval or any 
               // interval thereafter
               intervalIdx = Min(intervalIdx, compositeIntervalIdx - 1);

               IntervalIndexType startIntervalIdx = (resultsType == rtIncremental ? intervalIdx : erectionIntervalIdx);

               // deflections are computed using mid-span section properties (see model builders)
               GET_IFACE(IPointOfInterest, pPoi);
               PoiList vMyPoi;
               pPoi->GetPointsOfInterest(vPoi.front().get().GetSegmentKey(), POI_5L | POI_RELEASED_SEGMENT, &vMyPoi);
               ATLASSERT(vMyPoi.size() == 1);
               const pgsPointOfInterest& spPoi = vMyPoi.front();
               ATLASSERT(spPoi.IsMidSpan(POI_RELEASED_SEGMENT));

               GET_IFACE(ISectionProperties, pSectProp);

               deflection.resize(vPoi.size(), 0.0);
               for (IntervalIndexType iIdx = startIntervalIdx; iIdx <= intervalIdx; iIdx++)
               {
                  Float64 Iyy = pSectProp->GetIyy(iIdx, spPoi);
                  Float64 Ixy = pSectProp->GetIxy(iIdx, spPoi);

                  std::vector<Float64> deltaY = m_pGirderModelManager->GetDeflection(iIdx, comboType, vPoi, bat, rtIncremental); // these are the incremental vertical deflections in interval iIdx

                  // lateral deflections are (delta Y)*(-Ixy/Iyy)
                  // scale deltaY by D and add to the running sum
                  Float64 D = -Ixy / Iyy;
                  std::transform(deltaY.cbegin(), deltaY.cend(), deflection.cbegin(), deflection.begin(), [&D](const auto& dY, auto& cummDx) {return cummDx + dY*D;});

                  if (comboType == lcDC && iIdx == erectionIntervalIdx)
                  {
                     // vertical deflection deltaY was computed using Ixx... we want a lateral deflection using Iyy
                     // also, vertical deflection is based on full weight of the girder going downward, we need to
                     // adjust the deflection for a component of the vertical gravity load being applied at the girder orientation slope
                     // this is the dead load component causing lateral deflection
                     Float64 Ixx = pSectProp->GetIxx(iIdx, spPoi);

                     GET_IFACE(IGirder, pGirder);
                     Float64 girder_orientation = pGirder->GetOrientation(spPoi.GetSegmentKey());

                     // sin of girder orientation adjusts vertical girder self-weight load, to a self-weight load component transverse to the girder
                     // if the girder is plumb, sin(girder_orietation) = 0
                     Float64 D = sin(-girder_orientation)*Ixx / Iyy;
                     std::transform(deltaY.cbegin(), deltaY.cend(), deflection.cbegin(), deflection.begin(), [&D](const auto& dY, auto& cummDx) {return cummDx + dY*D;});
                  }
               }
            }
         }
      }
      catch (...)
      {
         // reset all of our data.
         const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
         throw;
      }
   }

   return deflection;
}

std::vector<Float64> CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment, bool bIncludePrecamber) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> rotation;
   rotation.reserve(vPoi.size());

   if ( comboType == lcPS )
   {
      // secondary effects aren't directly computed. get the individual product forces
      // and sum them up here
      rotation.resize(vPoi.size(),0.0);
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      for (const auto& pfType : pfTypes)
      {
         std::vector<Float64> delta = GetRotation(intervalIdx,pfType,vPoi,bat,resultsType,false);
         std::transform(delta.cbegin(),delta.cend(),rotation.cbegin(),rotation.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }
   else if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
#if defined _DEBUG
      // must be time-step analysis
      GET_IFACE( ILossParameters, pLossParams);
      ATLASSERT( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP );
#endif

      pgsTypes::ProductForceType pfType;
      switch(comboType)
      {
      case lcCR: pfType = pgsTypes::pftCreep; break;
      case lcSH: pfType = pgsTypes::pftShrinkage; break;
      case lcRE: pfType = pgsTypes::pftRelaxation; break;
      default: ATLASSERT(false); // should not happen
      }

      rotation = GetRotation(intervalIdx,pfType,vPoi,bat,resultsType,false);
   }
   else
   {
      try
      {
         IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

         if ( intervalIdx < erectionIntervalIdx )
         {
            rotation = m_pSegmentModelManager->GetRotation(intervalIdx,comboType,vPoi,resultsType);
         }
         else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
         {
            // the incremental result at the time of erection is being requested. this is when
            // we switch between segment models and girder models. the incremental results
            // is the cumulative result this interval minus the cumulative result in the previous interval
            std::vector<Float64> Dprev = GetRotation(intervalIdx-1,comboType,vPoi,bat,rtCumulative,false);
            std::vector<Float64> Dthis = GetRotation(intervalIdx,  comboType,vPoi,bat,rtCumulative,false);
            std::transform(Dthis.cbegin(),Dthis.cend(),Dprev.cbegin(),std::back_inserter(rotation),[](const auto& a, const auto& b) {return a - b;});
         }
         else
         {
            rotation = m_pGirderModelManager->GetRotation(intervalIdx,comboType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
         throw;
      }
   }

   if (comboType == lcDC && bIncludePrecamber)
   {
      ApplyPrecamberRotationAdjustment(intervalIdx, vPoi, &rotation, nullptr);
   }

   if ( bIncludeSlopeAdjustment )
   {
      ApplyRotationAdjustment(intervalIdx,vPoi,&rotation,nullptr);
   }

   return rotation;
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      GetTimeStepStress(intervalIdx,comboType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
   }
   else
   {
      GetElasticStress(intervalIdx,comboType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
   }
}

void CAnalysisAgentImp::GetCombinedLiveLoadAxial(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) const
{
   m_pGirderModelManager->GetCombinedLiveLoadAxial(intervalIdx,llType,vPoi,bat,pMmin,pMmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax) const
{
   m_pGirderModelManager->GetCombinedLiveLoadShear(intervalIdx,llType,vPoi,bat,bIncludeImpact,pVmin,pVmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax) const
{
   m_pGirderModelManager->GetCombinedLiveLoadMoment(intervalIdx,llType,vPoi,bat,pMmin,pMmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax) const
{
   m_pGirderModelManager->GetCombinedLiveLoadDeflection(intervalIdx,llType,vPoi,bat,pDmin,pDmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const
{
   m_pGirderModelManager->GetCombinedLiveLoadRotation(intervalIdx,llType,vPoi,bat,pRmin,pRmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax) const
{
   m_pGirderModelManager->GetCombinedLiveLoadStress(intervalIdx,llType,vPoi,bat,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax);
}

/////////////////////////////////////////////////////////////////////////////
// ILimitStateForces
//
void CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Pmin, Pmax;
   GetAxial(intervalIdx,limitState,vPoi,bat,&Pmin,&Pmax);

   ATLASSERT(Pmin.size() == 1);
   ATLASSERT(Pmax.size() == 1);

   *pMin = Pmin.front();
   *pMax = Pmax.front();
}

void CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   GetShear(intervalIdx,limitState,vPoi,bat,&Vmin,&Vmax);

   ATLASSERT(Vmin.size() == 1);
   ATLASSERT(Vmax.size() == 1);

   *pMin = Vmin.front();
   *pMax = Vmax.front();
}

void CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   GetMoment(intervalIdx,limitState,vPoi,bat,&Mmin,&Mmax);

   ATLASSERT(Mmin.size() == 1);
   ATLASSERT(Mmax.size() == 1);

   *pMin = Mmin.front();
   *pMax = Mmax.front();
}

void CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncPrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,bool bIncludePrecamber,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetDeflection(intervalIdx,limitState,vPoi,bat,bIncPrestress,bIncludeLiveLoad,bIncludeElevationAdjustment,bIncludePrecamber,&Dmin,&Dmax);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);

   *pMin = Dmin.front();
   *pMax = Dmax.front();
}

void CAnalysisAgentImp::GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, Float64* pMin, Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetXDeflection(intervalIdx, limitState, vPoi, bat, bIncludePrestress, &Dmin, &Dmax);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);

   *pMin = Dmin.front();
   *pMax = Dmax.front();
}

void CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment, bool bIncludePrecamber, Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Rmin, Rmax;
   GetRotation(intervalIdx,limitState,vPoi,bat,bIncludePrestress,bIncludeLiveLoad,bIncludeSlopeAdjustment,bIncludePrecamber,&Rmin,&Rmax);

   ATLASSERT(Rmin.size() == 1);
   ATLASSERT(Rmax.size() == 1);

   *pMin = Rmin.front();
   *pMax = Rmax.front();
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,Float64* pMin,Float64* pMax) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Fmin, Fmax;
   GetStress(intervalIdx,limitState,vPoi,bat,bIncludePrestress,loc,&Fmin,&Fmax);

   ATLASSERT(Fmin.size() == 1);
   ATLASSERT(Fmax.size() == 1);

   *pMin = Fmin.front();
   *pMax = Fmax.front();
}

Float64 CAnalysisAgentImp::GetSlabDesignMoment(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat) const
{
   PoiList vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> M = GetSlabDesignMoment(limitState,vPoi,bat);

   ATLASSERT(M.size() == vPoi.size());

   return M.front();
}

bool CAnalysisAgentImp::IsStrengthIIApplicable(const CGirderKey& girderKey) const
{
   // If we have permit truck, we're golden
   GET_IFACE(ILiveLoads,pLiveLoads);
   bool bPermit = pLiveLoads->IsLiveLoadDefined(pgsTypes::lltPermit);
   if (bPermit)
   {
      return true;
   }
   else
   {
      // last chance is if this girder has pedestrian load and it is turned on for permit states
      ILiveLoads::PedestrianLoadApplicationType PermitPedLoad = pLiveLoads->GetPedestrianLoadApplication(pgsTypes::lltPermit);
      if (ILiveLoads::PedDontApply != PermitPedLoad)
      {
         return HasPedestrianLoad(girderKey);
      }
   }

   return false;
}

/////////////////////////////////////////////////////////////////////////////
// ILimitStateForces2
//
void CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   CGirderKey girderKey(vPoi.front().get().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(girderKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if (intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetAxial(intervalIdx,limitState,vPoi,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetAxial(intervalIdx,limitState,vPoi,bat,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      Float64 gCRMax;
      Float64 gCRMin;
      Float64 gSHMax;
      Float64 gSHMin;
      Float64 gREMax;
      Float64 gREMin;
      if ( IsRatingLimitState(limitState) )
      {
         GET_IFACE(IRatingSpecification,pRatingSpec);
         gCRMax = pRatingSpec->GetCreepFactor(limitState);
         gSHMax = pRatingSpec->GetShrinkageFactor(limitState);
         gREMax = pRatingSpec->GetRelaxationFactor(limitState);
         
         gCRMin = gCRMax;
         gSHMin = gSHMax;
         gREMin = gREMax;
      }
      else
      {
         GET_IFACE(ILoadFactors,pILoadFactors);
         const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
         pLoadFactors->GetCR(limitState, &gCRMin, &gCRMax);
         pLoadFactors->GetSH(limitState, &gSHMin, &gSHMax);
         pLoadFactors->GetRE(limitState, &gREMin, &gREMax);
      }

      std::vector<Float64> vPcr = GetAxial(intervalIdx,lcCR,vPoi,bat,rtCumulative);
      std::vector<Float64> vPsh = GetAxial(intervalIdx,lcSH,vPoi,bat,rtCumulative);
      std::vector<Float64> vPre = GetAxial(intervalIdx,lcRE,vPoi,bat,rtCumulative);

      if ( !IsEqual(gCRMin,1.0) )
      {
         std::vector<Float64> vPcrMin;
         vPcrMin.reserve(vPcr.size());
         std::transform(vPcr.cbegin(), vPcr.cend(), std::back_inserter(vPcrMin), [&gCRMin](const Float64& value){return value*gCRMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vPcrMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMin->cbegin(),pMin->cend(),vPcr.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gCRMax,1.0) )
      {
         std::vector<Float64> vPcrMax;
         vPcrMax.reserve(vPcr.size());
         std::transform(vPcr.cbegin(),vPcr.cend(),std::back_inserter(vPcrMax), [&gCRMax](const Float64& value) {return value*gCRMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vPcrMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMax->cbegin(),pMax->cend(),vPcr.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gSHMin,1.0) )
      {
         std::vector<Float64> vPshMin;
         vPshMin.reserve(vPsh.size());
         std::transform(vPsh.cbegin(),vPsh.cend(),std::back_inserter(vPshMin), [&gSHMin](const Float64& value) {return value*gSHMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vPshMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMin->cbegin(),pMin->cend(),vPsh.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gSHMax,1.0) )
      {
         std::vector<Float64> vPshMax;
         vPshMax.reserve(vPsh.size());
         std::transform(vPsh.cbegin(),vPsh.cend(),std::back_inserter(vPshMax), [&gSHMax](const Float64& value) {return value*gSHMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vPshMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMax->cbegin(),pMax->cend(),vPsh.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gREMin,1.0) )
      {
         std::vector<Float64> vPreMin;
         vPreMin.reserve(vPre.size());
         std::transform(vPre.cbegin(),vPre.cend(),std::back_inserter(vPreMin), [&gREMin](const Float64& value) {return value*gREMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vPreMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMin->cbegin(),pMin->cend(),vPre.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gREMax,1.0) )
      {
         std::vector<Float64> vPreMax;
         vPreMax.reserve(vPre.size());
         std::transform(vPre.cbegin(),vPre.cend(),std::back_inserter(vPreMax), [&gREMax](const Float64& value) {return value*gREMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vPreMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMax->cbegin(),pMax->cend(),vPre.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }
}

void CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   CGirderKey girderKey(vPoi.front().get().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(girderKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetShear(intervalIdx,limitState,vPoi,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetShear(intervalIdx,limitState,vPoi,bat,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      Float64 gCRMax;
      Float64 gCRMin;
      Float64 gSHMax;
      Float64 gSHMin;
      Float64 gREMax;
      Float64 gREMin;
      if ( IsRatingLimitState(limitState) )
      {
         GET_IFACE(IRatingSpecification,pRatingSpec);
         gCRMax = pRatingSpec->GetCreepFactor(limitState);
         gSHMax = pRatingSpec->GetShrinkageFactor(limitState);
         gREMax = pRatingSpec->GetRelaxationFactor(limitState);
         
         gCRMin = gCRMax;
         gSHMin = gSHMax;
         gREMin = gREMax;
      }
      else
      {
         GET_IFACE(ILoadFactors,pILoadFactors);
         const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
         pLoadFactors->GetCR(limitState, &gCRMin, &gCRMax);
         pLoadFactors->GetSH(limitState, &gSHMin, &gSHMax);
         pLoadFactors->GetRE(limitState, &gREMin, &gREMax);
      }

      std::vector<sysSectionValue> vVcr = GetShear(intervalIdx,lcCR,vPoi,bat,rtCumulative);
      std::vector<sysSectionValue> vVsh = GetShear(intervalIdx,lcSH,vPoi,bat,rtCumulative);
      std::vector<sysSectionValue> vVre = GetShear(intervalIdx,lcRE,vPoi,bat,rtCumulative);

      if ( !IsEqual(gCRMin,1.0) )
      {
         std::vector<sysSectionValue> vVcrMin;
         vVcrMin.reserve(vVcr.size());
         std::transform(vVcr.cbegin(),vVcr.cend(),std::back_inserter(vVcrMin), [&gCRMin](const auto& value) {return value*gCRMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vVcrMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMin->cbegin(),pMin->cend(),vVcr.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gCRMax,1.0) )
      {
         std::vector<sysSectionValue> vVcrMax;
         vVcrMax.reserve(vVcr.size());
         std::transform(vVcr.cbegin(),vVcr.cend(),std::back_inserter(vVcrMax), [&gCRMax](const auto& value) {return value*gCRMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vVcrMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMax->cbegin(),pMax->cend(),vVcr.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gSHMin,1.0) )
      {
         std::vector<sysSectionValue> vVshMin;
         vVshMin.reserve(vVsh.size());
         std::transform(vVsh.cbegin(),vVsh.cend(),std::back_inserter(vVshMin), [&gSHMin](const auto& value) {return value*gSHMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vVshMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMin->cbegin(),pMin->cend(),vVsh.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gSHMax,1.0) )
      {
         std::vector<sysSectionValue> vVshMax;
         vVshMax.reserve(vVsh.size());
         std::transform(vVsh.cbegin(),vVsh.cend(),std::back_inserter(vVshMax), [&gSHMax](const auto& value) {return value*gSHMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vVshMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMax->cbegin(),pMax->cend(),vVsh.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gREMin,1.0) )
      {
         std::vector<sysSectionValue> vVreMin;
         vVreMin.reserve(vVre.size());
         std::transform(vVre.cbegin(),vVre.cend(),std::back_inserter(vVreMin), [&gREMin](const auto& value) {return value*gREMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vVreMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMin->cbegin(),pMin->cend(),vVre.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gREMax,1.0) )
      {
         std::vector<sysSectionValue> vVreMax;
         vVreMax.reserve(vVre.size());
         std::transform(vVre.cbegin(),vVre.cend(),std::back_inserter(vVreMax), [&gREMax](const auto& value) {return value*gREMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vVreMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMax->cbegin(),pMax->cend(),vVre.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }
}

void CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   CGirderKey girderKey(vPoi.front().get().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(girderKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if (intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetMoment(intervalIdx,limitState,vPoi,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetMoment(intervalIdx,limitState,vPoi,bat,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      Float64 gCRMax;
      Float64 gCRMin;
      Float64 gSHMax;
      Float64 gSHMin;
      Float64 gREMax;
      Float64 gREMin;
      if ( IsRatingLimitState(limitState) )
      {
         GET_IFACE(IRatingSpecification,pRatingSpec);
         gCRMax = pRatingSpec->GetCreepFactor(limitState);
         gSHMax = pRatingSpec->GetShrinkageFactor(limitState);
         gREMax = pRatingSpec->GetRelaxationFactor(limitState);
         
         gCRMin = gCRMax;
         gSHMin = gSHMax;
         gREMin = gREMax;
      }
      else
      {
         GET_IFACE(ILoadFactors,pILoadFactors);
         const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
         pLoadFactors->GetCR(limitState, &gCRMin, &gCRMax);
         pLoadFactors->GetSH(limitState, &gSHMin, &gSHMax);
         pLoadFactors->GetRE(limitState, &gREMin, &gREMax);
      }

      std::vector<Float64> vMcr = GetMoment(intervalIdx,lcCR,vPoi,bat,rtCumulative);
      std::vector<Float64> vMsh = GetMoment(intervalIdx,lcSH,vPoi,bat,rtCumulative);
      std::vector<Float64> vMre = GetMoment(intervalIdx,lcRE,vPoi,bat,rtCumulative);

      if ( !IsEqual(gCRMin,1.0) )
      {
         std::vector<Float64> vMcrMin;
         vMcrMin.reserve(vMcr.size());
         std::transform(vMcr.cbegin(),vMcr.cend(),std::back_inserter(vMcrMin), [&gCRMin](const Float64& value) {return value*gCRMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vMcrMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMin->cbegin(),pMin->cend(),vMcr.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gCRMax,1.0) )
      {
         std::vector<Float64> vMcrMax;
         vMcrMax.reserve(vMcr.size());
         std::transform(vMcr.cbegin(),vMcr.cend(),std::back_inserter(vMcrMax), [&gCRMax](const Float64& value) {return value*gCRMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vMcrMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMax->cbegin(),pMax->cend(),vMcr.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gSHMin,1.0) )
      {
         std::vector<Float64> vMshMin;
         vMshMin.reserve(vMsh.size());
         std::transform(vMsh.cbegin(),vMsh.cend(),std::back_inserter(vMshMin), [&gSHMin](const Float64& value) {return value*gSHMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vMshMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMin->cbegin(),pMin->cend(),vMsh.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gSHMax,1.0) )
      {
         std::vector<Float64> vMshMax;
         vMshMax.reserve(vMsh.size());
         std::transform(vMsh.cbegin(),vMsh.cend(),std::back_inserter(vMshMax), [&gSHMax](const Float64& value) {return value*gSHMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vMshMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMax->cbegin(),pMax->cend(),vMsh.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gREMin,1.0) )
      {
         std::vector<Float64> vMreMin;
         vMreMin.reserve(vMre.size());
         std::transform(vMre.cbegin(),vMre.cend(),std::back_inserter(vMreMin), [&gREMin](const Float64& value) {return value*gREMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vMreMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMin->cbegin(),pMin->cend(),vMre.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( !IsEqual(gREMax,1.0) )
      {
         std::vector<Float64> vMreMax;
         vMreMax.reserve(vMre.size());
         std::transform(vMre.cbegin(),vMre.cend(),std::back_inserter(vMreMax), [&gREMax](const Float64& value) {return value*gREMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vMreMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::transform(pMax->cbegin(),pMax->cend(),vMre.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }
}

std::vector<Float64> CAnalysisAgentImp::GetSlabDesignMoment(pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat) const
{
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys;
   pPoi->GetGirderKeys(vPoi, &vGirderKeys);
   ATLASSERT(vGirderKeys.size() == 1); // this method assumes all the poi are for the same girder
#endif

   std::vector<Float64> Msd;
   Msd.reserve(vPoi.size());
   try
   {
      // need to do the time-step analysis because it adds loads to the LBAM
      CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
      ComputeTimeDependentEffects(girderKey,INVALID_INDEX);

      Msd = m_pGirderModelManager->GetSlabDesignMoment(limitState,vPoi,bat);
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   return Msd;
}

void CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,bool bIncludePrecamber,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   CGirderKey girderKey(vPoi.front().get().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(girderKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetDeflection(intervalIdx,limitState,vPoi,bIncludePrestress,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetDeflection(intervalIdx,limitState,vPoi,bat,bIncludePrestress,bIncludeLiveLoad,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   // The deformations that come from the model managers are just the elastic response.
   // Here we add in the time-dependent deformations
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      Float64 gCRMax;
      Float64 gCRMin;
      Float64 gSHMax;
      Float64 gSHMin;
      Float64 gREMax;
      Float64 gREMin;
      if ( IsRatingLimitState(limitState) )
      {
         GET_IFACE(IRatingSpecification,pRatingSpec);
         gCRMax = pRatingSpec->GetCreepFactor(limitState);
         gSHMax = pRatingSpec->GetShrinkageFactor(limitState);
         gREMax = pRatingSpec->GetRelaxationFactor(limitState);
         
         gCRMin = gCRMax;
         gSHMin = gSHMax;
         gREMin = gREMax;
      }
      else
      {
         GET_IFACE(ILoadFactors,pILoadFactors);
         const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
         pLoadFactors->GetCR(limitState, &gCRMin, &gCRMax);
         pLoadFactors->GetSH(limitState, &gSHMin, &gSHMax);
         pLoadFactors->GetRE(limitState, &gREMin, &gREMax);
      }

      std::vector<Float64> vDcr = GetDeflection(intervalIdx,lcCR,vPoi,bat,rtCumulative,false);
      std::vector<Float64> vDsh = GetDeflection(intervalIdx,lcSH,vPoi,bat,rtCumulative,false);
      std::vector<Float64> vDre = GetDeflection(intervalIdx,lcRE,vPoi,bat,rtCumulative,false);

      if ( IsEqual(gCRMin,1.0) )
      {
         // if the load factor is 1.0, just add the results into the total
         // there is no sense multiplying a bunch of numbers by 1.0
         std::transform(pMin->cbegin(),pMin->cend(),vDcr.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         // multiple the values by the load factor and then add into the total.
         std::vector<Float64> vDcrMin;
         vDcrMin.reserve(vDcr.size());
         std::transform(vDcr.cbegin(),vDcr.cend(),std::back_inserter(vDcrMin), [&gCRMin](const Float64& value) {return value*gCRMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vDcrMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( IsEqual(gCRMax,1.0) )
      {
         std::transform(pMax->cbegin(),pMax->cend(),vDcr.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::vector<Float64> vDcrMax;
         vDcrMax.reserve(vDcr.size());
         std::transform(vDcr.cbegin(),vDcr.cend(),std::back_inserter(vDcrMax), [&gCRMax](const Float64& value) {return value*gCRMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vDcrMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( IsEqual(gSHMin,1.0) )
      {
         std::transform(pMin->cbegin(),pMin->cend(),vDsh.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::vector<Float64> vDshMin;
         vDshMin.reserve(vDsh.size());
         std::transform(vDsh.cbegin(),vDsh.cend(),std::back_inserter(vDshMin), [&gSHMin](const Float64& value) {return value*gSHMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vDshMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( IsEqual(gSHMax,1.0) )
      {
         std::transform(pMax->cbegin(),pMax->cend(),vDsh.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::vector<Float64> vDshMax;
         vDshMax.reserve(vDsh.size());
         std::transform(vDsh.cbegin(),vDsh.cend(),std::back_inserter(vDshMax), [&gSHMax](const Float64& value) {return value*gSHMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vDshMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( IsEqual(gREMin,1.0) )
      {
         std::transform(pMin->cbegin(),pMin->cend(),vDre.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::vector<Float64> vDreMin;
         vDreMin.reserve(vDre.size());
         std::transform(vDre.cbegin(),vDre.cend(),std::back_inserter(vDreMin), [&gREMin](const Float64& value) {return value*gREMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vDreMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( IsEqual(gREMax,1.0) )
      {
         std::transform(pMax->cbegin(),pMax->cend(),vDre.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::vector<Float64> vDreMax;
         vDreMax.reserve(vDre.size());
         std::transform(vDre.cbegin(),vDre.cend(),std::back_inserter(vDreMax), [&gREMax](const Float64& value) {return value*gREMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vDreMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }
   else
   {
      GET_IFACE(IIntervals,pIntervals);
      const CSegmentKey& segmentKey(vPoi.front().get().GetSegmentKey());
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType haulingIntervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();

      GET_IFACE(IBridge,pBridge);
      pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

      pgsTypes::PrestressDeflectionDatum supportDatum;
      std::vector<ICamber::CreepPeriod> vCreepPeriods;
      if ( intervalIdx < storageIntervalIdx )
      {
         // creep has not yet occured
      }
      else if ( storageIntervalIdx < intervalIdx && intervalIdx < erectionIntervalIdx )
      {
         vCreepPeriods.push_back(ICamber::cpReleaseToDiaphragm);
         supportDatum = (intervalIdx == haulingIntervalIdx ? pgsTypes::pddHauling : pgsTypes::pddStorage);
      }
      else if ( erectionIntervalIdx <= intervalIdx && intervalIdx < compositeIntervalIdx -1 )
      {
         if ( deckType == pgsTypes::sdtNone )
         {
            vCreepPeriods.push_back(ICamber::cpReleaseToFinal);
         }
         else
         {
            vCreepPeriods.push_back(ICamber::cpReleaseToDiaphragm);
         }
         supportDatum = pgsTypes::pddErected;
      }
      else if (compositeIntervalIdx -1 <= intervalIdx )
      {
         if ( deckType == pgsTypes::sdtNone )
         {
            vCreepPeriods.push_back(ICamber::cpReleaseToFinal);
            vCreepPeriods.push_back(ICamber::cpDiaphragmToFinal);
            vCreepPeriods.push_back(ICamber::cpDeckToFinal);
         }
         else
         {
            vCreepPeriods.push_back(ICamber::cpReleaseToDiaphragm);
            vCreepPeriods.push_back(ICamber::cpDiaphragmToDeck);
         }
         supportDatum = pgsTypes::pddErected;
      }

      for ( const auto& creepPeriod : vCreepPeriods)
      {
         int i = 0;
         for ( const pgsPointOfInterest& poi : vPoi)
         {
            Float64 Dcr = GetCreepDeflection(poi,creepPeriod,CREEP_MAXTIME,supportDatum);
            (*pMin)[i] += Dcr;
            (*pMax)[i] += Dcr;
            i++;
         }
      }
   }

   if (bIncludePrecamber)
   {
      ApplyPrecamberElevationAdjustment(intervalIdx, vPoi, pMin, pMax);
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,pMin,pMax);
   }
}

void CAnalysisAgentImp::GetXDeflection(IntervalIndexType intervalIdx, pgsTypes::LimitState limitState, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, bool bIncludePrestress, std::vector<Float64>* pMin, std::vector<Float64>* pMax) const
{
   ATLASSERT(IsDesignLimitState(limitState)); // only supporting design limit states right now

   // Lateral deflections only occur through erection interval. After erection it is
   // assumed that girders are laterally braced and there is no additional X deflection
   // The only loading the occurs at erection or earlier is DC and Prestressing
   GET_IFACE(ILoadFactors, pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 DCmin, DCmax;
   pLoadFactors->GetDC(limitState, &DCmin, &DCmax);

   const CGirderKey& girderKey(vPoi.front().get().GetSegmentKey()); // assumes all POI are for the same girder
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);

   std::vector<Float64> dxDC = GetXDeflection(Min(intervalIdx,erectionIntervalIdx), lcDC, vPoi, bat, rtCumulative);

   std::vector<Float64> dxPS;
   dxPS.reserve(vPoi.size());

   if (bIncludePrestress)
   {
      dxPS = GetXDeflection(Min(intervalIdx, erectionIntervalIdx), pgsTypes::pftPretension, vPoi, bat, rtCumulative);
   }
   else
   {
      dxPS.resize(vPoi.size(), 0.0);
   }

   pMin->clear();
   std::transform(dxDC.cbegin(), dxDC.cend(), dxPS.cbegin(), std::back_inserter(*pMin), [DCmin](const auto& dc, const auto& ps) {return DCmin*dc + ps;});

   pMax->clear();
   std::transform(dxDC.cbegin(), dxDC.cend(), dxPS.cbegin(), std::back_inserter(*pMax), [DCmax](const auto& dc, const auto& ps) {return DCmax*dc + ps;});
}

void CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,bool bIncludePrecamber,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   CGirderKey girderKey(vPoi.front().get().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(girderKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetRotation(intervalIdx,limitState,vPoi,bIncludePrestress,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetRotation(intervalIdx,limitState,vPoi,bat,bIncludePrestress,bIncludeLiveLoad,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   // The deformations that come from the model managers are just the elastic response.
   // Here we add in the time-dependent deformations
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      Float64 gCRMax;
      Float64 gCRMin;
      Float64 gSHMax;
      Float64 gSHMin;
      Float64 gREMax;
      Float64 gREMin;
      if ( IsRatingLimitState(limitState) )
      {
         GET_IFACE(IRatingSpecification,pRatingSpec);
         gCRMax = pRatingSpec->GetCreepFactor(limitState);
         gSHMax = pRatingSpec->GetShrinkageFactor(limitState);
         gREMax = pRatingSpec->GetRelaxationFactor(limitState);
         
         gCRMin = gCRMax;
         gSHMin = gSHMax;
         gREMin = gREMax;
      }
      else
      {
         GET_IFACE(ILoadFactors,pILoadFactors);
         const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
         pLoadFactors->GetCR(limitState, &gCRMin, &gCRMax);
         pLoadFactors->GetSH(limitState, &gSHMin, &gSHMax);
         pLoadFactors->GetRE(limitState, &gREMin, &gREMax);
      }

      std::vector<Float64> vRcr = GetRotation(intervalIdx,lcCR,vPoi,bat,rtCumulative,false);
      std::vector<Float64> vRsh = GetRotation(intervalIdx,lcSH,vPoi,bat,rtCumulative,false);
      std::vector<Float64> vRre = GetRotation(intervalIdx,lcRE,vPoi,bat,rtCumulative,false);

      if ( IsEqual(gCRMin,1.0) )
      {
         std::transform(pMin->cbegin(),pMin->cend(),vRcr.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::vector<Float64> vRcrMin;
         vRcrMin.reserve(vRcr.size());
         std::transform(vRcr.cbegin(),vRcr.cend(),std::back_inserter(vRcrMin), [&gCRMin](const Float64& value) {return value*gCRMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vRcrMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( IsEqual(gCRMax,1.0) )
      {
         std::transform(pMax->cbegin(),pMax->cend(),vRcr.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::vector<Float64> vRcrMax;
         vRcrMax.reserve(vRcr.size());
         std::transform(vRcr.cbegin(),vRcr.cend(),std::back_inserter(vRcrMax), [&gCRMax](const Float64& value) {return value*gCRMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vRcrMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( IsEqual(gSHMin,1.0) )
      {
         std::transform(pMin->cbegin(),pMin->cend(),vRsh.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::vector<Float64> vRshMin;
         vRshMin.reserve(vRsh.size());
         std::transform(vRsh.cbegin(),vRsh.cend(),std::back_inserter(vRshMin), [&gSHMin](const Float64& value) {return value*gSHMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vRshMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( IsEqual(gSHMax,1.0) )
      {
         std::transform(pMax->cbegin(),pMax->cend(),vRsh.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::vector<Float64> vRshMax;
         vRshMax.reserve(vRsh.size());
         std::transform(vRsh.cbegin(),vRsh.cend(),std::back_inserter(vRshMax), [&gSHMax](const Float64& value) {return value*gSHMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vRshMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( IsEqual(gREMin,1.0) )
      {
         std::transform(pMin->cbegin(),pMin->cend(),vRre.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::vector<Float64> vRreMin;
         vRreMin.reserve(vRre.size());
         std::transform(vRre.cbegin(),vRre.cend(),std::back_inserter(vRreMin), [&gREMin](const Float64& value) {return value*gREMin;});
         std::transform(pMin->cbegin(),pMin->cend(),vRreMin.cbegin(),pMin->begin(),[](const auto& a, const auto& b) {return a + b;});
      }

      if ( IsEqual(gREMax,1.0) )
      {
         std::transform(pMax->cbegin(),pMax->cend(),vRre.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      else
      {
         std::vector<Float64> vRreMax;
         vRreMax.reserve(vRre.size());
         std::transform(vRre.cbegin(),vRre.cend(),std::back_inserter(vRreMax), [&gREMax](const Float64& value) {return value*gREMax;});
         std::transform(pMax->cbegin(),pMax->cend(),vRreMax.cbegin(),pMax->begin(),[](const auto& a, const auto& b) {return a + b;});
      }
   }
   else
   {
#pragma Reminder("UPDATE: need to add in creep rotations")
   }

   if (bIncludePrecamber)
   {
      ApplyPrecamberRotationAdjustment(intervalIdx, vPoi, pMin, pMax);
   }

   if (bIncludeSlopeAdjustment)
   {
      ApplyRotationAdjustment(intervalIdx, vPoi, pMin, pMax);
   }
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      GetTimeStepStress(intervalIdx,limitState,vPoi,bat,bIncludePrestress,loc,pMin,pMax);
   }
   else
   {
      GetElasticStress(intervalIdx,limitState,vPoi,bat,bIncludePrestress,loc,pMin,pMax);
   }
}

void CAnalysisAgentImp::GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax) const
{
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      GET_IFACE(IBridge,pBridge);

      CSegmentKey segmentKey = pBridge->GetSegmentAtPier(pierIdx,girderKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetReaction(intervalIdx,limitState,pierIdx,girderKey,bIncludeImpact,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetReaction(intervalIdx,limitState,pierIdx,girderKey,bat,bIncludeImpact,pMin,pMax);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }
}

///////////////////////////////////////////////////////
// IExternalLoading
bool CAnalysisAgentImp::CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName)
{
   if ( !m_pSegmentModelManager->CreateLoading(girderLineIdx,strLoadingName) )
   {
      ATLASSERT(false);
      return false;
   }

   if ( !m_pGirderModelManager->CreateLoading(girderLineIdx,strLoadingName) )
   {
      ATLASSERT(false);
      return false;
   }

   return true;
}

bool CAnalysisAgentImp::AddLoadingToLoadCombination(GirderIndexType girderLineIdx,LPCTSTR strLoadingName,LoadingCombinationType lcCombo)
{
   if ( !m_pSegmentModelManager->AddLoadingToLoadCombination(girderLineIdx,strLoadingName,lcCombo) )
   {
      return false;
   }

   if ( !m_pGirderModelManager->AddLoadingToLoadCombination(girderLineIdx,strLoadingName,lcCombo) )
   {
      return false;
   }

   return true;
}

bool CAnalysisAgentImp::CreateConcentratedLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   if ( IsZero(Fx) && IsZero(Fy) && IsZero(Mz) )
   {
      // don't add zero load
      return true;
   }

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateConcentratedLoad(intervalIdx,strLoadingName,poi,Fx,Fy,Mz);
   }
   else
   {
      return m_pGirderModelManager->CreateConcentratedLoad(intervalIdx,strLoadingName,poi,Fx,Fy,Mz);
   }
}

bool CAnalysisAgentImp::CreateConcentratedLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
{
   if ( IsZero(Fx) && IsZero(Fy) && IsZero(Mz) )
   {
      // don't add zero load
      return true;
   }

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateConcentratedLoad(intervalIdx,pfType,poi,Fx,Fy,Mz);
   }
   else
   {
      return m_pGirderModelManager->CreateConcentratedLoad(intervalIdx,pfType,poi,Fx,Fy,Mz);
   }
}

bool CAnalysisAgentImp::CreateUniformLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   if ( IsZero(wx) && IsZero(wy) )
   {
      // don't add zero load
      return true;
   }

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi1.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateUniformLoad(intervalIdx,strLoadingName,poi1,poi2,wx,wy);
   }
   else
   {
      return m_pGirderModelManager->CreateUniformLoad(intervalIdx,strLoadingName,poi1,poi2,wx,wy);
   }
}

bool CAnalysisAgentImp::CreateUniformLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
   if ( IsZero(wx) && IsZero(wy) )
   {
      // don't add zero load
      return true;
   }

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi1.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateUniformLoad(intervalIdx,pfType,poi1,poi2,wx,wy);
   }
   else
   {
      return m_pGirderModelManager->CreateUniformLoad(intervalIdx,pfType,poi1,poi2,wx,wy);
   }
}

bool CAnalysisAgentImp::CreateInitialStrainLoad(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
#if defined _DEBUG
   // POI's must belong to the same girder
   const CSegmentKey& segmentKey1(poi1.GetSegmentKey());
   const CSegmentKey& segmentKey2(poi2.GetSegmentKey());
   ATLASSERT(segmentKey1.groupIndex  == segmentKey2.groupIndex);
   ATLASSERT(segmentKey1.girderIndex == segmentKey2.girderIndex);
#endif

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi1.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateInitialStrainLoad(intervalIdx,strLoadingName,poi1,poi2,e,r);
   }
   else
   {
      return m_pGirderModelManager->CreateInitialStrainLoad(intervalIdx,strLoadingName,poi1,poi2,e,r);
   }
}

bool CAnalysisAgentImp::CreateInitialStrainLoad(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
{
#if defined _DEBUG
   // POI's must belong to the same girder
   const CSegmentKey& segmentKey1(poi1.GetSegmentKey());
   const CSegmentKey& segmentKey2(poi2.GetSegmentKey());
   ATLASSERT(segmentKey1.groupIndex  == segmentKey2.groupIndex);
   ATLASSERT(segmentKey1.girderIndex == segmentKey2.girderIndex);
#endif

   GET_IFACE(IIntervals,pIntervals);
   const CSegmentKey& segmentKey(poi1.GetSegmentKey());
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->CreateInitialStrainLoad(intervalIdx,pfType,poi1,poi2,e,r);
   }
   else
   {
      return m_pGirderModelManager->CreateInitialStrainLoad(intervalIdx,pfType,poi1,poi2,e,r);
   }
}

Float64 CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetAxial(intervalIdx,strLoadingName,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

sysSectionValue CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<sysSectionValue> results( GetShear(intervalIdx,strLoadingName,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetMoment(intervalIdx,strLoadingName,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetDeflection(intervalIdx,strLoadingName,vPoi,bat,resultsType,bIncludeElevationAdjustment) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetXDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const pgsPointOfInterest& poi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results(GetXDeflection(intervalIdx, strLoadingName, vPoi, bat, resultsType));
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetRotation(intervalIdx,strLoadingName,vPoi,bat,resultsType,bIncludeSlopeAdjustment) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot) const
{
   PoiList vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> fTop,fBot;
   GetStress(intervalIdx,strLoadingName,vPoi,bat,resultsType,topLocation,botLocation,&fTop,&fBot);
   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);
   *pfTop = fTop.front();
   *pfBot = fBot.front();
}

std::vector<Float64> CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetAxial(intervalIdx,strLoadingName,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Aprev = GetAxial(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative);
         std::vector<Float64> Athis = GetAxial(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative);
         std::transform(Athis.cbegin(),Athis.cend(),Aprev.cbegin(),std::back_inserter(results),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         results = m_pGirderModelManager->GetAxial(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   return results;
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<sysSectionValue> results;
   results.reserve(vPoi.size());

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetShear(intervalIdx,strLoadingName,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<sysSectionValue> Vprev = GetShear(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative);
         std::vector<sysSectionValue> Vthis = GetShear(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative);
         std::transform(Vthis.cbegin(),Vthis.cend(),Vprev.cbegin(),std::back_inserter(results),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         results = m_pGirderModelManager->GetShear(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetMoment(intervalIdx,strLoadingName,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Mprev = GetMoment(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative);
         std::vector<Float64> Mthis = GetMoment(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative);
         std::transform(Mthis.cbegin(),Mthis.cend(),Mprev.cbegin(),std::back_inserter(results),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         results = m_pGirderModelManager->GetMoment(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetDeflection(intervalIdx,strLoadingName,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Dprev = GetDeflection(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative,false);
         std::vector<Float64> Dthis = GetDeflection(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative,false);
         std::transform(Dthis.cbegin(),Dthis.cend(),Dprev.cbegin(),std::back_inserter(results),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         results = m_pGirderModelManager->GetDeflection(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,&results,nullptr);
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetXDeflection(IntervalIndexType intervalIdx, LPCTSTR strLoadingName, const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   // assume there aren't any lateral deflections once the girder is composite in the bridge system
   std::vector<Float64> vDelta;
   vDelta.reserve(vPoi.size());

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType compositeIntervalIdx = pIntervals->GetLastCompositeInterval();
   if (resultsType == rtIncremental && compositeIntervalIdx <= intervalIdx)
   {
      // want incremental results after composite... since we've assumed there aren't any lateral deflections for
      // this case, the increment deflections are zero
      vDelta.resize(vPoi.size(), 0.0);
   }
   else
   {
      // lateral deflections are a function of vertical deflections... 
      // we don't want vertical deflections after the girder is composite
      // so limit the interval index up to, but not including, the composite interval index
      intervalIdx = Min(intervalIdx, compositeIntervalIdx - 1);

      // Y deflections are based on mid-span properties
      GET_IFACE(IPointOfInterest, pPoi);
      PoiList vMyPoi;
      pPoi->GetPointsOfInterest(vPoi.front().get().GetSegmentKey(), POI_5L | POI_RELEASED_SEGMENT, &vMyPoi);
      ATLASSERT(vMyPoi.size() == 1);
      const pgsPointOfInterest& spPoi = vMyPoi.front();
      ATLASSERT(spPoi.IsMidSpan(POI_RELEASED_SEGMENT));

      // we have to scale the vertical deflections by (-Ixy/Ixx) to get the lateral deflection
      // we must use the section properties from the same interval used to create the analysis model
      // that is why we need to loop over intervals
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().get().GetSegmentKey());
      IntervalIndexType startIntervalIdx = (resultsType == rtIncremental ? intervalIdx : erectionIntervalIdx);

      GET_IFACE(ISectionProperties, pSectProp);
      vDelta.resize(vPoi.size(), 0.0);
      for (IntervalIndexType iIdx = startIntervalIdx; iIdx <= intervalIdx; iIdx++)
      {
         Float64 Iyy = pSectProp->GetIyy(iIdx, spPoi);
         Float64 Ixy = pSectProp->GetIxy(iIdx, spPoi);

         std::vector<Float64> deltaY = GetDeflection(iIdx, strLoadingName, vPoi, bat, rtIncremental, false); // these are the incremental vertical deflections in interval iIdx

         // lateral deflections are (delta Y)*(-Ixy/Iyy)
         // scale deltaY by D and add to the running sum
         Float64 D = -Ixy / Iyy;
         std::transform(deltaY.cbegin(), deltaY.cend(), vDelta.cbegin(), vDelta.begin(), [&D](const auto& dY, auto& cummDx) {return cummDx + dY*D;});
      }
   }

   return vDelta;
}

std::vector<Float64> CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   std::vector<Float64> results;
   results.reserve(vPoi.size());

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         results = m_pSegmentModelManager->GetRotation(intervalIdx,strLoadingName,vPoi,resultsType);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> Rprev = GetRotation(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative,false);
         std::vector<Float64> Rthis = GetRotation(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative,false);
         std::transform(Rthis.cbegin(),Rthis.cend(),Rprev.cbegin(),std::back_inserter(results),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         results = m_pGirderModelManager->GetRotation(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }

   if ( bIncludeSlopeAdjustment )
   {
      ApplyRotationAdjustment(intervalIdx,vPoi,&results,nullptr);
   }

   return results;
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   USES_CONVERSION;
   InitializeAnalysis(vPoi);

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);

      if ( intervalIdx < erectionIntervalIdx )
      {
         // before erection - results are in the segment models
         m_pSegmentModelManager->GetStress(intervalIdx,strLoadingName,vPoi,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> fTopPrev, fBotPrev;
         GetStress(intervalIdx-1,strLoadingName,vPoi,bat,resultsType,topLocation,botLocation,&fTopPrev,&fBotPrev);

         std::vector<Float64> fTopThis, fBotThis;
         GetStress(intervalIdx,strLoadingName,vPoi,bat,resultsType,topLocation,botLocation,&fTopThis,&fBotThis);

         std::vector<Float64> Mprev = GetMoment(intervalIdx-1,strLoadingName,vPoi,bat,rtCumulative);
         std::vector<Float64> Mthis = GetMoment(intervalIdx,  strLoadingName,vPoi,bat,rtCumulative);
         std::transform(fTopThis.cbegin(),fTopThis.cend(),fTopPrev.cbegin(),std::back_inserter(*pfTop),[](const auto& a, const auto& b) {return a - b;});
         std::transform(fBotThis.cbegin(),fBotThis.cend(),fBotPrev.cbegin(),std::back_inserter(*pfBot),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         m_pGirderModelManager->GetStress(intervalIdx,strLoadingName,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const
{
   std::vector<CSegmentKey> vSegmentKeys;
   vSegmentKeys.push_back(segmentKey);
   std::vector<Float64> vFyLeft, vFyRight;
   GetSegmentReactions(vSegmentKeys,intervalIdx,strLoadingName,bat,resultsType,&vFyLeft,&vFyRight);
   *pRleft = vFyLeft.front();
   *pRright = vFyRight.front();
}

void CAnalysisAgentImp::GetSegmentReactions(const std::vector<CSegmentKey>& vSegmentKeys,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const
{
   pRleft->reserve(vSegmentKeys.size());
   pRright->reserve(vSegmentKeys.size());

   GET_IFACE(IIntervals,pIntervals);
   std::vector<CSegmentKey>::const_iterator segKeyIter(vSegmentKeys.begin());
   std::vector<CSegmentKey>::const_iterator segKeyIterEnd(vSegmentKeys.end());
   for ( ; segKeyIter != segKeyIterEnd; segKeyIter++ )
   {
      const CSegmentKey& segmentKey = *segKeyIter;

      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      Float64 Rleft(0), Rright(0);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetReaction(segmentKey,intervalIdx,strLoadingName,bat,resultsType,&Rleft,&Rright);
      }
      pRleft->push_back(Rleft);
      pRright->push_back(Rright);
   }
}

REACTION CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   vSupports.emplace_back(supportIdx,supportType);
   std::vector<REACTION> vReactions = GetReaction(girderKey,vSupports,intervalIdx,strLoadingName,bat,resultsType);
   return vReactions.front();
}

std::vector<REACTION> CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LPCTSTR strLoadingName,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      std::vector<REACTION> reactions;
      reactions.resize(vSupports.size());
      return reactions;
   }
   else
   {
      if ( resultsType == rtCumulative )
      {
         std::vector<REACTION> reactions;
         reactions.resize(vSupports.size());
         for ( IntervalIndexType iIdx = erectionIntervalIdx; iIdx <= intervalIdx; iIdx++)
         {
            std::vector<REACTION> vReactions = m_pGirderModelManager->GetReaction(girderKey,vSupports,iIdx,strLoadingName,bat,rtIncremental);
            std::transform(reactions.cbegin(),reactions.cend(),vReactions.cbegin(),reactions.begin(),[](const auto& a, const auto& b) {return a + b;});
         }
         return reactions;
      }
      else
      {
         return m_pGirderModelManager->GetReaction(girderKey,vSupports,intervalIdx,strLoadingName,bat,rtIncremental);
      }
   }
}

///////////////////////////
void CAnalysisAgentImp::GetDesignStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc, const GDRCONFIG* pConfig,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax) const
{
   // Computes design-time stresses due to external loads
   ATLASSERT(IsGirderStressLocation(loc)); // expecting stress location to be on the girder

   // figure out which stage the girder loading is applied
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx           = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx      = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();
   IntervalIndexType castLongitudinalJointIntervalIdx = pIntervals->GetCastLongitudinalJointInterval();
   IntervalIndexType castShearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();
   IntervalIndexType castDeckIntervalIdx          = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType compositeIntervalIdx     = pIntervals->GetLastCompositeInterval();
   IntervalIndexType trafficBarrierIntervalIdx    = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx           = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx          = pIntervals->GetLiveLoadInterval();
   IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();

   if ( intervalIdx == releaseIntervalIdx )
   {
      GetStress(intervalIdx,limitState,poi,bat,false,loc,pMin,pMax);
      *pMax = (IsZero(*pMax) ? 0 : *pMax);
      *pMin = (IsZero(*pMin) ? 0 : *pMin);
      return;
   }

   // can't use this method with strength limit states
   ATLASSERT( !IsStrengthLimitState(limitState) );

   GET_IFACE(ISectionProperties,pSectProp);

   // Top of girder
   // original stress coefficients
   Float64 Cat_original, Cbt_original;
   pSectProp->GetStressCoefficients(compositeIntervalIdx, poi, pgsTypes::TopGirder, nullptr, &Cat_original, &Cbt_original);

   // new stress coefficients using design f'c
   Float64 Cat_new, Cbt_new;
   pSectProp->GetStressCoefficients(compositeIntervalIdx, poi, pgsTypes::TopGirder, pConfig, &Cat_new, &Cbt_new);

   // scale factor that converts top stress computed with the original stress coefficients to top stress
   // computed with the new stress coefficients
   Float64 k_top = Cbt_new / Cbt_original;

   // Bottom of girder
   // original stress coefficients
   Float64 Cab_original, Cbb_original;
   pSectProp->GetStressCoefficients(compositeIntervalIdx, poi, pgsTypes::BottomGirder, nullptr, &Cab_original, &Cbb_original);

   // new stress coefficients using design f'c
   Float64 Cab_new, Cbb_new;
   pSectProp->GetStressCoefficients(compositeIntervalIdx, poi, pgsTypes::BottomGirder, pConfig, &Cab_new, &Cbb_new);

   // scale factor that converts bottom stress computed with the original stress coefficients to bottom stress
   // computed with the new stress coefficients
   Float64 k_bot = Cbb_new / Cbb_original;

   Float64 ftop1, ftop2, ftop3Min, ftop3Max;
   Float64 fbot1, fbot2, fbot3Min, fbot3Max;

   ftop1 = ftop2 = ftop3Min = ftop3Max = 0;
   fbot1 = fbot2 = fbot3Min = fbot3Max = 0;

   // Load Factors
   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();
   ATLASSERT(IsDesignLimitState(limitState));
   Float64 dc = pLoadFactors->GetDCMax(limitState);
   Float64 dw = pLoadFactors->GetDWMax(limitState);
   Float64 ll = pLoadFactors->GetLLIMMax(limitState);

   Float64 ft,fb;

   // Erect Segment (also covers temporary strand removal and diaphragm placement)
   if (erectSegmentIntervalIdx <= intervalIdx)
   {
      GetStress(erectSegmentIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative, pgsTypes::TopGirder, pgsTypes::BottomGirder, &ft, &fb);
      ftop1 = dc*ft;
      fbot1 = dc*fb;
   }

   if (castDiaphragmIntervalIdx <= intervalIdx)
   {
      GetStress(castDiaphragmIntervalIdx,pgsTypes::pftDiaphragm,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;
   }

   if (castLongitudinalJointIntervalIdx != INVALID_INDEX && castLongitudinalJointIntervalIdx <= intervalIdx)
   {
      GetStress(castLongitudinalJointIntervalIdx, pgsTypes::pftLongitudinalJoint, poi, bat, rtIncremental, pgsTypes::TopGirder, pgsTypes::BottomGirder, &ft, &fb);
      ftop1 += dc*ft;
      fbot1 += dc*fb;
   }

   // Casting Deck (non-composite girder carrying deck dead load)
   if (castDeckIntervalIdx != INVALID_INDEX && castDeckIntervalIdx <= intervalIdx)
   {
      GetStress(castDeckIntervalIdx, pgsTypes::pftConstruction, poi, bat, rtIncremental, pgsTypes::TopGirder, pgsTypes::BottomGirder, &ft, &fb);
      ftop1 += dc*ft;
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtIncremental, pgsTypes::TopGirder, pgsTypes::BottomGirder, &ft, &fb);
      ftop1 += dc*ft;
      fbot1 += dc*fb;

      GetDesignSlabStressAdjustment(poi, pConfig, &ft, &fb);
      ftop1 += dc*ft;
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, bat, rtIncremental, pgsTypes::TopGirder, pgsTypes::BottomGirder, &ft, &fb);
      ftop1 += dc*ft;
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx, pgsTypes::pftSlabPanel, poi, bat, rtIncremental, pgsTypes::TopGirder, pgsTypes::BottomGirder, &ft, &fb);
      ftop1 += dc*ft;
      fbot1 += dc*fb;

      GetDesignSlabPadStressAdjustment(poi, pConfig, &ft, &fb);
      ftop1 += dc*ft;
      fbot1 += dc*fb;
   }

   if (castShearKeyIntervalIdx != INVALID_INDEX && castShearKeyIntervalIdx <= intervalIdx)
   {
      GetStress(castShearKeyIntervalIdx, pgsTypes::pftShearKey, poi, bat, rtIncremental, pgsTypes::TopGirder, pgsTypes::BottomGirder, &ft, &fb);
      ftop1 += dc*ft;
      fbot1 += dc*fb;
   }

   if ( noncompositeUserLoadIntervalIdx != INVALID_INDEX && noncompositeUserLoadIntervalIdx <= intervalIdx)
   {
      GetStress(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDC,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(noncompositeUserLoadIntervalIdx,pgsTypes::pftUserDW,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dw*ft;   
      fbot1 += dw*fb;
   }

   // Composite Section Carrying Superimposed Dead Loads
   if (trafficBarrierIntervalIdx <= intervalIdx)
   {
      GetStress(trafficBarrierIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, pgsTypes::TopGirder, pgsTypes::BottomGirder, &ft, &fb);
      ftop2 = dc*k_top*ft;
      fbot2 = dc*k_bot*fb;

      GetStress(trafficBarrierIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental, pgsTypes::TopGirder, pgsTypes::BottomGirder, &ft, &fb);
      ftop2 += dc*k_top*ft;
      fbot2 += dc*k_bot*fb;

      if (overlayIntervalIdx != INVALID_INDEX && overlayIntervalIdx <= intervalIdx)
      {
         GetStress(overlayIntervalIdx, pgsTypes::pftOverlay, poi, bat, rtIncremental, pgsTypes::TopGirder, pgsTypes::BottomGirder, &ft, &fb);
         ftop2 += dw*k_top*ft;
         fbot2 += dw*k_bot*fb;
      }
   }

   if ( compositeDeckIntervalIdx != INVALID_INDEX && compositeDeckIntervalIdx <= intervalIdx )
   {
      // slab shrinkage stresses
      Float64 ft_ss, fb_ss;
      GetDeckShrinkageStresses(poi, pConfig->Fc, &ft_ss, &fb_ss);
      ftop2 += dc*ft_ss;
      fbot2 += dc*fb_ss;
   }

   if (compositeUserLoadIntervalIdx != INVALID_INDEX && compositeUserLoadIntervalIdx <= intervalIdx)
   {
      GetStress(compositeUserLoadIntervalIdx,pgsTypes::pftUserDC,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop2 += dc*k_top*ft;   
      fbot2 += dc*k_bot*fb;

      GetStress(compositeUserLoadIntervalIdx,pgsTypes::pftUserDW,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop2 += dw*k_top*ft;   
      fbot2 += dw*k_bot*fb;
   }

   // Open to traffic, carrying live load
   if ( liveLoadIntervalIdx <= intervalIdx )
   {
      ftop3Min = ftop3Max = 0.0;   
      fbot3Min = fbot3Max = 0.0;   

      // When we get the LL stress below, it is per girder (includes LLDF). However, the LLDF is based on the original bridge model, not the design.
      // We have to adjust the stresses by removing the original LLDF and applying the LLDF based on current design values
      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pGirderMaterial = pSegmentData->GetSegmentMaterial(segmentKey);

      Float64 fc_lldf = pConfig->Fc ;
      if ( pGirderMaterial->Concrete.bUserEc )
      {
         fc_lldf = lrfdConcreteUtil::FcFromEc( pGirderMaterial->Concrete.Ec, pGirderMaterial->Concrete.StrengthDensity );
      }

      GET_IFACE(ILiveLoadDistributionFactors,pLLDF);
      Float64 gV1, gpM1, gnM1;
      Float64 gV2, gpM2, gnM2;
      pLLDF->GetDistributionFactors(poi,limitState,fc_lldf,&gpM1,&gnM1,&gV1);
      pLLDF->GetDistributionFactors(poi,limitState,&gpM2,&gnM2,&gV2);
      Float64 lldf_adj = gpM1/gpM2; // multiply by design LLDF and divide out the original LLDF

      Float64 ftMin,ftMax,fbMin,fbMax;
      if ( limitState == pgsTypes::FatigueI )
      {
         GetLiveLoadStress(liveLoadIntervalIdx,pgsTypes::lltFatigue, poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);
      }
      else
      {
         GetLiveLoadStress(liveLoadIntervalIdx,pgsTypes::lltDesign,  poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);
      }

      ftop3Min += ll*lldf_adj*k_top*ftMin;   
      fbot3Min += ll*lldf_adj*k_bot*fbMin;

      ftop3Max += ll*lldf_adj*k_top*ftMax;   
      fbot3Max += ll*lldf_adj*k_bot*fbMax;

      GetLiveLoadStress(liveLoadIntervalIdx,pgsTypes::lltPedestrian,  poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);

      ftop3Min += ll*k_top*ftMin;   
      fbot3Min += ll*k_bot*fbMin;

      ftop3Max += ll*k_top*ftMax;   
      fbot3Max += ll*k_bot*fbMax;

      GetStress(liveLoadIntervalIdx,pgsTypes::pftUserLLIM,poi,bat,rtCumulative,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop3Min += ll*k_top*ft;   
      fbot3Min += ll*k_bot*fb;

      ftop3Max += ll*k_top*ft;   
      fbot3Max += ll*k_bot*fb;
   }

   if ( loc == pgsTypes::TopGirder )
   {
      *pMin = ftop1 + ftop2 + ftop3Min;
      *pMax = ftop1 + ftop2 + ftop3Max;
   }
   else
   {
      *pMin = fbot1 + fbot2 + fbot3Min;
      *pMax = fbot1 + fbot2 + fbot3Max;
   }

   if (*pMax < *pMin )
   {
      std::swap(*pMin,*pMax);
   }

   *pMax = (IsZero(*pMax) ? 0 : *pMax);
   *pMin = (IsZero(*pMin) ? 0 : *pMin);
}

//////////////////////////////////////////////////////////////////////
void CAnalysisAgentImp::GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax) const
{
   try
   {
      m_pGirderModelManager->GetConcurrentShear(intervalIdx,limitState,poi,bat,pMin,pMax);
   }
   catch(...)
   {
      // reset all of our data.
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax) const
{
   m_pGirderModelManager->GetViMmax(intervalIdx,limitState,poi,bat,pVi,pMmax);
}

/////////////////////////////////////////////////////////////////////////////
// ICamber
//
Uint32 CAnalysisAgentImp::GetCreepMethod() const
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   return pSpecEntry->GetCreepMethod();
}

Float64 CAnalysisAgentImp::GetCreepCoefficient(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate, const GDRCONFIG* pConfig) const
{
   CREEPCOEFFICIENTDETAILS details = GetCreepCoefficientDetails(segmentKey,creepPeriod,constructionRate,pConfig);
   return details.Ct;
}

CREEPCOEFFICIENTDETAILS CAnalysisAgentImp::GetCreepCoefficientDetails(const CSegmentKey& segmentKey,CreepPeriod creepPeriod, Int16 constructionRate,const GDRCONFIG* pConfig) const
{
   if (pConfig == nullptr)
   {
      // check to see if the configuration was already cached
      auto found = m_CreepCoefficientDetails[constructionRate][creepPeriod].find(segmentKey);
      if (found != m_CreepCoefficientDetails[constructionRate][creepPeriod].end())
      {
         // yep... return it
         return (*found).second;
      }
   }

   CREEPCOEFFICIENTDETAILS details;

   GET_IFACE(IEnvironment,pEnvironment);

   GET_IFACE(ISectionProperties,pSectProp);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   // if fc < 0 use current fc girder
   LoadingEvent le = GetLoadingEvent(creepPeriod);
#pragma Reminder("UPDATE - make this a generic call to GetConcreteStrengthAtTimeOfLoading")
   // fc = GetConcreteStrengthAtTimeOfLoading(segmentKey,le,pConfig); // problem is, this will change results from previous versions... be careful and do this in isolation of all other changes
   Float64 fc;
   if (pConfig)
   {
      fc = GetConcreteStrengthAtTimeOfLoading(*pConfig, le);
   }
   else
   {
      // this looks totally wrong, but it how things worked before making
      // this method work with pConfig as an actual config or null pointer
      GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
      fc = GetConcreteStrengthAtTimeOfLoading(config, le);
   }

   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   details.Method = pSpecEntry->GetCreepMethod();
   ATLASSERT(details.Method == CREEP_LRFD); // only supporting LRFD method... the old WSDOT method is out

   details.Spec = (pSpecEntry->GetSpecificationType() <= lrfdVersionMgr::ThirdEdition2004 ) ? CREEP_SPEC_PRE_2005 : CREEP_SPEC_2005;

   if ( details.Spec == CREEP_SPEC_PRE_2005 )
   {
      //  LRFD 3rd edition and earlier
      try
      {
         lrfdCreepCoefficient cc;
         cc.SetRelHumidity( pEnvironment->GetRelHumidity() );
         cc.SetSurfaceArea( pSectProp->GetSegmentSurfaceArea(segmentKey) );
         cc.SetVolume( pSectProp->GetSegmentVolume(segmentKey) );
         cc.SetFc(fc);
         cc.SetCuringMethodTimeAdjustmentFactor(pSpecEntry->GetCuringMethodTimeAdjustmentFactor());

         switch( creepPeriod )
         {
            case cpReleaseToDiaphragm:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient::Accelerated : lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max()) - cc.GetAdjustedInitialAge() );
               break;

            case cpReleaseToDeck:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient::Accelerated : lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               break;

            case cpReleaseToFinal:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient::Accelerated : lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            case cpDiaphragmToDeck:
               cc.SetCuringMethod( lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               cc.SetMaturity(   constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               break;

            case cpDiaphragmToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            case cpDeckToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
               break;

            default:
               ATLASSERT(false);
         }

         details.ti           = cc.GetAdjustedInitialAge();
         details.t            = cc.GetMaturity();
         details.Fc           = cc.GetFc();
         details.H            = cc.GetRelHumidity();
         details.kf           = cc.GetKf();
         details.kc           = cc.GetKc();
         details.VSratio      = cc.GetVolume() / cc.GetSurfaceArea();
         details.CuringMethod = cc.GetCuringMethod();

         details.Ct           = cc.GetCreepCoefficient();
      }
#if defined _DEBUG
      catch( lrfdXCreepCoefficient& ex )
#else
      catch( lrfdXCreepCoefficient& /*ex*/ )
#endif // _DEBUG
      {
         ATLASSERT( ex.GetReason() == lrfdXCreepCoefficient::VSRatio );

         std::_tstring strMsg(_T("V/S Ratio exceeds maximum value per C5.4.2.3.2. Use a different method for estimating creep"));
      
         pgsVSRatioStatusItem* pStatusItem = new pgsVSRatioStatusItem(segmentKey,m_StatusGroupID,m_scidVSRatio,strMsg.c_str());

         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         pStatusCenter->Add(pStatusItem);
      
         THROW_UNWIND(strMsg.c_str(),-1);
      }
   }
   else
   {
      // LRFD 3rd edition with 2005 interims and later
      try
      {
         lrfdCreepCoefficient2005 cc;
         cc.SetRelHumidity( pEnvironment->GetRelHumidity() );
         cc.SetSurfaceArea( pSectProp->GetSegmentSurfaceArea(segmentKey) );
         cc.SetVolume( pSectProp->GetSegmentVolume(segmentKey) );
         cc.SetFc(fc);

         cc.SetCuringMethodTimeAdjustmentFactor(pSpecEntry->GetCuringMethodTimeAdjustmentFactor());

         GET_IFACE(IMaterials,pMaterial);
         cc.SetK1( pMaterial->GetSegmentCreepK1(segmentKey) );
         cc.SetK2( pMaterial->GetSegmentCreepK2(segmentKey) );


         switch( creepPeriod )
         {
            case cpReleaseToDiaphragm:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient2005::Accelerated : lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max()) - cc.GetAdjustedInitialAge() );
               break;

            case cpReleaseToDeck:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient2005::Accelerated : lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max()) - cc.GetAdjustedInitialAge() );
               break;

            case cpReleaseToFinal:
               cc.SetCuringMethod( pSpecEntry->GetCuringMethod() == CURING_ACCELERATED ? lrfdCreepCoefficient2005::Accelerated : lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( pSpecEntry->GetXferTime() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() - cc.GetAdjustedInitialAge() );
               break;

            case cpDiaphragmToDeck:
               cc.SetCuringMethod( lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max() );
               cc.SetMaturity(  (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max()) - cc.GetAdjustedInitialAge());
               break;

            case cpDiaphragmToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max()) );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() - cc.GetAdjustedInitialAge() );
               break;

            case cpDeckToFinal:
               cc.SetCuringMethod( lrfdCreepCoefficient2005::Normal );
               cc.SetInitialAge( constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration2Min() : pSpecEntry->GetCreepDuration2Max() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() - cc.GetAdjustedInitialAge() );
               break;

            default:
               ATLASSERT(false);
         }

         details.ti           = cc.GetAdjustedInitialAge();
         details.t            = cc.GetMaturity();
         details.Fc           = cc.GetFc();
         details.H            = cc.GetRelHumidity();
         details.kvs          = cc.GetKvs();
         details.khc          = cc.GetKhc();
         details.kf           = cc.GetKf();
         details.ktd          = cc.GetKtd();
         details.VSratio      = cc.GetVolume() / cc.GetSurfaceArea();
         details.CuringMethod = cc.GetCuringMethod();
         details.K1           = cc.GetK1();
         details.K2           = cc.GetK2();

         details.Ct           = cc.GetCreepCoefficient();
      }
#if defined _DEBUG
      catch( lrfdXCreepCoefficient& ex )
#else
      catch( lrfdXCreepCoefficient& /*ex*/ )
#endif // _DEBUG
      {
         ATLASSERT( ex.GetReason() == lrfdXCreepCoefficient::VSRatio );

         std::_tstring strMsg(_T("V/S Ratio exceeds maximum value per C5.4.2.3.2. Use a different method for estimating creep"));
      
         pgsVSRatioStatusItem* pStatusItem = new pgsVSRatioStatusItem(segmentKey,m_StatusGroupID,m_scidVSRatio,strMsg.c_str());

         GET_IFACE(IEAFStatusCenter,pStatusCenter);
         pStatusCenter->Add(pStatusItem);
      
         THROW_UNWIND(strMsg.c_str(),-1);
      }
   }

   if (pConfig == nullptr)
   {
      // cache the results
      m_CreepCoefficientDetails[constructionRate][creepPeriod].insert(std::make_pair(segmentKey, details));
   }

   return details;
}

Float64 CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig) const
{
   Float64 dx, dy,rz;
   GetPermanentPrestressDeflection(poi,datum,pConfig,&dx,&dy,&rz);
   return dy;
}

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig,Float64* pDx,Float64* pDy) const
{
   Float64 rz;
   GetPermanentPrestressDeflection(poi, datum, pConfig, pDx, pDy, &rz);
}

Float64 CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,const GDRCONFIG* pConfig) const
{
   Float64 dx,dy,rz;
   GetInitialTempPrestressDeflection(poi,datum,pConfig,&dx,&dy,&rz);
   return dy;
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig, Float64* pDx,Float64* pDy) const
{
   Float64 rz;
   GetInitialTempPrestressDeflection(poi, datum, pConfig, pDx, pDy, &rz);
}

Float64 CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const
{
   Float64 dx, dy, rz;
   GetReleaseTempPrestressDeflection(poi, pConfig, &dx, &dy, &rz);
   return dy;
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDx,Float64* pDy) const
{
   Float64 rz;
   GetReleaseTempPrestressDeflection(poi, pConfig, pDx, pDy, &rz);
}

Float64 CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig) const
{
   Float64 dy,rz;
   GetCreepDeflection(poi,creepPeriod,constructionRate,datum,pConfig,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetXCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig) const
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   Float64 delta = 0;
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   Float64 DXpsStorage;
   if (pConfig == nullptr)
   {
      DXpsStorage = GetXDeflection(storageIntervalIdx, pgsTypes::pftPretension, poi, bat, rtCumulative);
   }
   else
   {
      Float64 dx, dy, rz;
      GetPermanentPrestressDeflection(poi, pgsTypes::pddStorage, pConfig, &dx, &dy, &rz);
      DXpsStorage = dx;
   }

   Float64 DXgdrStorage = GetXDeflection(storageIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative);

   Float64 Ct = GetCreepCoefficient(segmentKey, creepPeriod, constructionRate, pConfig);

   delta = Ct*(DXpsStorage + DXgdrStorage);

   if (datum != pgsTypes::pddStorage)
   {
      // get POI at final bearing locations.... 
      // we want to deduct the deformation relative to the storage supports at these locations from the storage deformations
      // to make the deformation relative to the final bearings
      GET_IFACE(IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 2);
      const pgsPointOfInterest& poiLeft(vPoi.front());
      const pgsPointOfInterest& poiRight(vPoi.back());
      ATLASSERT(poiLeft.IsTenthPoint(POI_ERECTED_SEGMENT) == 1);
      ATLASSERT(poiRight.IsTenthPoint(POI_ERECTED_SEGMENT) == 11);
      ATLASSERT(poiLeft.GetID() != INVALID_ID && poiRight.GetID() != INVALID_ID);

      // get x creep deformations during storage at location of final bearings
      Float64 dxLeft  = GetXCreepDeflection(poiLeft,  creepPeriod, constructionRate, pgsTypes::pddStorage, pConfig);
      Float64 dxRight = GetXCreepDeflection(poiRight, creepPeriod, constructionRate, pgsTypes::pddStorage, pConfig);
      Float64 dx = ::LinInterp(poi.GetDistFromStart(), dxLeft, dxRight, poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());

      ATLASSERT(datum != pgsTypes::pddLifting);
      ATLASSERT(datum != pgsTypes::pddHauling);
      delta -= dx;
   }

   return delta;
}

Float64 CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const
{
   Float64 dsy,rsz,dspy,rspz;
   GetDeckDeflection(poi,pConfig,&dsy,&rsz,&dspy,&rspz);
   return dsy + dspy;
}

Float64 CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const
{
   Float64 dy,rz;
   GetDeckPanelDeflection(poi,pConfig,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const
{
   Float64 dy,rz;
   GetShearKeyDeflection(poi,pConfig,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const
{
   Float64 dy,rz;
   GetConstructionLoadDeflection(poi,pConfig,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const
{
   Float64 dy,rz;
   GetDiaphragmDeflection(poi,pConfig,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const
{
   Float64 dy,rz;
   GetUserLoadDeflection(intervalIdx,poi,pConfig,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const
{
   Float64 dy,rz;
   GetSlabBarrierOverlayDeflection(poi,pConfig,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const
{
   Float64 dy,rz;
   GetScreedCamberEx(poi,pConfig,true,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetScreedCamberUnfactored(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig) const
{
   Float64 dy,rz;
   GetScreedCamberEx(poi,pConfig,false,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig) const
{
   return GetExcessCamberEx(poi, time, pConfig, true);
}

Float64 CAnalysisAgentImp::GetExcessCamberUnfactored(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig) const
{
   return GetExcessCamberEx(poi, time, pConfig, false);
}

Float64 CAnalysisAgentImp::GetExcessCamberEx(const pgsPointOfInterest& poi,Int16 time,const GDRCONFIG* pConfig, bool applyFactors) const
{
   GET_IFACE(ILossParameters, pLossParams);
   if (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP)
   {
      ATLASSERT(pConfig == nullptr);
#if defined _DEBUG
      // this is slower, but use it as a check on the more direct method used below
      Float64 D = GetDCamberForGirderSchedule(poi, time);
      Float64 C = GetScreedCamber(poi);
      Float64 excess = D - C;
#endif

      // excess camber is the camber that remains in the girder when the bridge is open
      // to traffic. we simply need to get the deflection when the bridge goes into
      // service.
      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      Float64 Dmin, Dmax;
      GetDeflection(liveLoadIntervalIdx, pgsTypes::ServiceI, poi, bat, true, false/*exclude live load deflection*/, true, true, &Dmin, &Dmax);
      ATLASSERT(IsEqual(Dmin, Dmax)); // no live load so these should be the same

      ATLASSERT(IsEqual(Dmin, excess));

      return Dmin;
   }
   else
   {
      if (pConfig == nullptr)
      {
         const CSegmentKey& segmentKey = poi.GetSegmentKey();

         CamberModelData model = GetPrestressDeflectionModel(segmentKey, m_PrestressDeflectionModels);
         CamberModelData initTempModel = GetPrestressDeflectionModel(segmentKey, m_InitialTempPrestressDeflectionModels);
         CamberModelData relsTempModel = GetPrestressDeflectionModel(segmentKey, m_ReleaseTempPrestressDeflectionModels);

         Float64 Dy, Rz;
         GetExcessCamberEx2(poi, nullptr, model, initTempModel, relsTempModel, time, applyFactors, &Dy, &Rz);
         return Dy;
      }
      else
      {
         ValidateCamberModels(pConfig);
         Float64 Dy, Rz;
         GetExcessCamberEx2(poi, pConfig, m_CacheConfig_PrestressDeflectionModel, m_CacheConfig_InitialTempPrestressDeflectionModels, m_CacheConfig_ReleaseTempPrestressDeflectionModels, time, applyFactors, &Dy, &Rz);
         return Dy;
      }
   }
}

Float64 CAnalysisAgentImp::GetExcessCamberRotation(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig) const
{
   return GetExcessCamberRotationEx(poi, time, pConfig, true);
}

Float64 CAnalysisAgentImp::GetExcessCamberRotationUnfactored(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig) const
{
   return GetExcessCamberRotationEx(poi, time, pConfig, false);
}

Float64 CAnalysisAgentImp::GetExcessCamberRotationEx(const pgsPointOfInterest& poi,Int16 time,const GDRCONFIG* pConfig, bool applyFactors) const
{
   if (pConfig == nullptr)
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CamberModelData model = GetPrestressDeflectionModel(segmentKey, m_PrestressDeflectionModels);
      CamberModelData initTempModel = GetPrestressDeflectionModel(segmentKey, m_InitialTempPrestressDeflectionModels);
      CamberModelData relsTempModel = GetPrestressDeflectionModel(segmentKey, m_ReleaseTempPrestressDeflectionModels);

      Float64 dy, rz;
      GetExcessCamberEx2(poi, nullptr, model, initTempModel, relsTempModel, time, applyFactors, &dy, &rz);
      return rz;
   }
   else
   {
      ValidateCamberModels(pConfig);

      Float64 dy, rz;
      GetExcessCamberEx2(poi, pConfig, m_CacheConfig_PrestressDeflectionModel, m_CacheConfig_InitialTempPrestressDeflectionModels, m_CacheConfig_ReleaseTempPrestressDeflectionModels, time, applyFactors, &dy, &rz);
      return rz;
   }
}

Float64 CAnalysisAgentImp::GetSidlDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig) const
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType nIntervals               = pIntervals->GetIntervalCount();

   // NOTE: No need to validate camber models
   Float64 delta_trafficbarrier  = 0;
   Float64 delta_sidewalk        = 0;
   Float64 delta_overlay         = 0;
   Float64 delta_diaphragm       = 0;
   Float64 delta_user            = 0;

   // adjustment factor for fcgdr that is different that current value
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,pConfig,compositeDeckIntervalIdx);

   delta_diaphragm      = GetDiaphragmDeflection(poi,pConfig);
   delta_trafficbarrier = k2*GetDeflection(compositeDeckIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false);
   delta_sidewalk       = k2*GetDeflection(compositeDeckIntervalIdx, pgsTypes::pftSidewalk,       poi, bat, rtIncremental, false);

   for ( IntervalIndexType intervalIdx = compositeDeckIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 dUser = GetUserLoadDeflection(intervalIdx,poi,pConfig);
      delta_user += dUser;
   }

   if ( !pBridge->IsFutureOverlay() && overlayIntervalIdx != INVALID_INDEX )
   {
      delta_overlay = k2*GetDeflection(overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental, false);
   }

   GET_IFACE(IStrandGeometry, pStrandGeom);
   bool bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary, pConfig) && pStrandGeom->GetTemporaryStrandUsage(segmentKey, pConfig) != pgsTypes::ttsPTBeforeShipping);

   Float64 dSIDL;
   if ( bTempStrands )
   {
      dSIDL = delta_trafficbarrier + delta_sidewalk + delta_overlay + delta_user;
   }
   else
   {
      // for SIP decks, diaphagms are applied before the cast portion of the slab so they don't apply to screed camber
      if ( deckType == pgsTypes::sdtCompositeSIP )
      {
         delta_diaphragm = 0;
      }

      dSIDL = delta_trafficbarrier + delta_sidewalk + delta_overlay + delta_user + delta_diaphragm;
   }

   return dSIDL;
}
Float64 CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig) const
{
   return GetDCamberForGirderScheduleEx(poi, time, pConfig, true);
}

Float64 CAnalysisAgentImp::GetDCamberForGirderScheduleUnfactored(const pgsPointOfInterest& poi, Int16 time, const GDRCONFIG* pConfig) const
{
   return GetDCamberForGirderScheduleEx(poi, time, pConfig, false);
}

Float64 CAnalysisAgentImp::GetDCamberForGirderScheduleEx(const pgsPointOfInterest& poi,Int16 time,const GDRCONFIG* pConfig, bool applyFactors) const
{
   if (pConfig == nullptr)
   {
      GET_IFACE(ILossParameters, pLossParams);
      if (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP)
      {
         pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

         GET_IFACE(IBridge, pBridge);
         GET_IFACE(IIntervals, pIntervals);

         IntervalIndexType intervalIdx;
         if (pBridge->GetDeckType() == pgsTypes::sdtNone)
         {
            intervalIdx = pIntervals->GetIntervalCount() - 1;
         }
         else
         {
            IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
            ATLASSERT(castDeckIntervalIdx != INVALID_INDEX);
            intervalIdx = castDeckIntervalIdx - 1;
         }

         Float64 Dmin, Dmax;
         GetDeflection(intervalIdx, pgsTypes::ServiceI, poi, bat, true, false, true, true, &Dmin, &Dmax);
         ATLASSERT(IsEqual(Dmin, Dmax)); // no live load so these should be the same

         return Dmin;
      }
      else
      {
         const CSegmentKey& segmentKey = poi.GetSegmentKey();

         CamberModelData model = GetPrestressDeflectionModel(segmentKey, m_PrestressDeflectionModels);
         CamberModelData initTempModel = GetPrestressDeflectionModel(segmentKey, m_InitialTempPrestressDeflectionModels);
         CamberModelData releaseTempModel = GetPrestressDeflectionModel(segmentKey, m_ReleaseTempPrestressDeflectionModels);

         Float64 Dy, Rz;
         GetDCamberForGirderScheduleEx2(poi, nullptr, model, initTempModel, releaseTempModel, time, applyFactors, &Dy, &Rz);
         return Dy;
      }
   }
   else
   {
      ValidateCamberModels(pConfig);

      Float64 Dy, Rz;
      GetDCamberForGirderScheduleEx2(poi, pConfig, m_CacheConfig_PrestressDeflectionModel, m_CacheConfig_InitialTempPrestressDeflectionModels, m_CacheConfig_ReleaseTempPrestressDeflectionModels, time, applyFactors, &Dy, &Rz);
      return Dy;
   }
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const
{
   if (pConfig == nullptr)
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CamberModelData model = GetPrestressDeflectionModel(segmentKey, m_PrestressDeflectionModels);
      CamberModelData model1 = GetPrestressDeflectionModel(segmentKey, m_InitialTempPrestressDeflectionModels);
      CamberModelData model2 = GetPrestressDeflectionModel(segmentKey, m_ReleaseTempPrestressDeflectionModels);
      GetCreepDeflection(poi, nullptr, model, model1, model2, creepPeriod, constructionRate, datum, pDy, pRz);
   }
   else
   {
      ValidateCamberModels(pConfig);
      GetCreepDeflection(poi, pConfig, m_CacheConfig_PrestressDeflectionModel, m_CacheConfig_InitialTempPrestressDeflectionModels, m_CacheConfig_ReleaseTempPrestressDeflectionModels, creepPeriod, constructionRate, datum, pDy, pRz);
   }
}

void CAnalysisAgentImp::GetScreedCamberEx(const pgsPointOfInterest& poi, const GDRCONFIG* pConfig, bool applyFactors, Float64* pDy, Float64* pRz) const
{
   if (pConfig == nullptr)
   {
      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

      GET_IFACE(ILossParameters, pLossParams);
      if (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP)
      {
         // time step method includes all effects including creep and shrinkage
         // Screed camber is the camber from deck placement until the bridge is open to traffic
         // Get both cumulative deflections and return the difference
         GET_IFACE(IIntervals, pIntervals);
         IntervalIndexType intervalIdx;

         GET_IFACE(IBridge, pBridge);
         if (pBridge->GetDeckType() == pgsTypes::sdtNone)
         {
            intervalIdx = pIntervals->GetIntervalCount() - 1;
         }
         else
         {
            IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
            intervalIdx = castDeckIntervalIdx - 1;
         }

         IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
         Float64 Dmin, Dmax;
         GetDeflection(intervalIdx, pgsTypes::ServiceI, poi, bat, true/*include prestress*/, false/*no liveload*/, true /*include elevation adjustment*/, true /*include precamber*/, &Dmin, &Dmax);
         ATLASSERT(IsEqual(Dmin, Dmax)); // no live load so these should be the same

         Float64 DRmin, DRmax;
         GetRotation(intervalIdx, pgsTypes::ServiceI, poi, bat, true/*include prestress*/, false/*no liveload*/, true /*include elevation adjustment*/, true /*include precamber*/, &DRmin, &DRmax);
         ATLASSERT(IsEqual(DRmin, DRmax)); // no live load so these should be the same

         Float64 Fmin, Fmax;
         GetDeflection(liveLoadIntervalIdx, pgsTypes::ServiceI, poi, bat, true/*include prestress*/, false/*no liveload*/, true /*include elevation adjustment*/, true /*include precamber*/, &Fmin, &Fmax);
         ATLASSERT(IsEqual(Fmin, Fmax)); // no live load so these should be the same

         Float64 FRmin, FRmax;
         GetRotation(liveLoadIntervalIdx, pgsTypes::ServiceI, poi, bat, true/*include prestress*/, false/*no liveload*/, true /*include elevation adjustment*/, true /*include precamber*/, &FRmin, &FRmax);
         ATLASSERT(IsEqual(FRmin, FRmax)); // no live load so these should be the same

         *pDy = Fmin - Dmin;
         *pRz = FRmin - DRmin;
      }
      else
      {
         // For regular analysis, we assume that creep and shrinkage related deflections are done
         // when the deck becomes composite. Here we just add up the deflections
         GET_IFACE(IBridge, pBridge);
         pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

         const CSegmentKey& segmentKey(poi.GetSegmentKey());

         GET_IFACE(IIntervals, pIntervals);
         IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
         IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
         IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

         // NOTE: No need to validate camber models
         Float64 Dslab = 0;
         Float64 Dslab_adj = 0;
         Float64 Dpanel = 0;
         Float64 DslabPad = 0;
         Float64 Dslab_pad_adj = 0;
         Float64 Dtrafficbarrier = 0;
         Float64 Dsidewalk = 0;
         Float64 Doverlay = 0;
         Float64 Duser1 = 0;
         Float64 Duser2 = 0;

         Float64 Rslab = 0;
         Float64 Rslab_adj = 0;
         Float64 Rpanel = 0;
         Float64 RslabPad = 0;
         Float64 Rslab_pad_adj = 0;
         Float64 Rtrafficbarrier = 0;
         Float64 Rsidewalk = 0;
         Float64 Roverlay = 0;
         Float64 Ruser1 = 0;
         Float64 Ruser2 = 0;

         GetDeckDeflection(poi, pConfig, &Dslab, &Rslab, &DslabPad, &RslabPad); // compensates for change in EI from original deflection and the value in the config
         GetDesignSlabDeflectionAdjustment(poi, pConfig, &Dslab_adj, &Rslab_adj); // compensates for change in slab load due to change in slab overhangs associated with "A" dimension
         GetDesignSlabPadDeflectionAdjustment(poi, pConfig, &Dslab_pad_adj, &Rslab_pad_adj); // compensates for change in slab haunch load due to change in "A" dimension

         Dslab -= Dslab_adj;
         DslabPad -= Dslab_pad_adj;

         Rslab -= Rslab_adj;
         RslabPad -= Rslab_pad_adj;

         if (deckType == pgsTypes::sdtCompositeSIP)
         {
            GetDeckPanelDeflection(poi, pConfig, &Dpanel, &Rpanel);
         }

         Dtrafficbarrier = GetDeflection(railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false);
         Rtrafficbarrier = GetRotation(railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false);
         Dsidewalk = GetDeflection(railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental, false);
         Rsidewalk = GetRotation(railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental, false);

         // Only get deflections for user defined loads that occur during deck placement and later
         GET_IFACE(IPointOfInterest, pPoi);
         CSpanKey spanKey;
         Float64 Xspan;
         pPoi->ConvertPoiToSpanPoint(poi, &spanKey, &Xspan);
         std::vector<IntervalIndexType> vUserLoadIntervals(pIntervals->GetUserDefinedLoadIntervals(spanKey));
         vUserLoadIntervals.erase(std::remove_if(vUserLoadIntervals.begin(), vUserLoadIntervals.end(), [&noncompositeUserLoadIntervalIdx](const auto& intervalIdx) {return intervalIdx < noncompositeUserLoadIntervalIdx;}), vUserLoadIntervals.end());
         std::vector<IntervalIndexType>::iterator userLoadIter(vUserLoadIntervals.begin());
         std::vector<IntervalIndexType>::iterator userLoadIterEnd(vUserLoadIntervals.end());
         Uint32 ucnt = 0;
         for (; userLoadIter != userLoadIterEnd; userLoadIter++)
         {
            IntervalIndexType intervalIdx = *userLoadIter;
            Float64 D, R;

            GetUserLoadDeflection(intervalIdx, poi, pConfig, &D, &R);

            // For PGSuper there are two user load stages and they are factored differently. User1 occurs when slab is cast. For splice,
            // the user load stages are always factored by 1.0, so this does not matter (now)
            if (intervalIdx == noncompositeUserLoadIntervalIdx)
            {
               Duser1 += D;
               Ruser1 += R;
            }
            else
            {
               Duser2 += D;
               Ruser2 += R;
            }

            ucnt++;
         }

         // This is an old convention in PGSuper: If there is no deck, user1 is included in the D camber
         if (deckType == pgsTypes::sdtNone)
         {
            Duser1 = 0.0;
            Ruser1 = 0.0;
         }

         if (!pBridge->IsFutureOverlay() && overlayIntervalIdx != INVALID_INDEX)
         {
            Doverlay = GetDeflection(overlayIntervalIdx, pgsTypes::pftOverlay, poi, bat, rtIncremental, false);
            Roverlay = GetRotation(overlayIntervalIdx, pgsTypes::pftOverlay, poi, bat, rtIncremental, false);
         }

         // apply camber multipliers
         CamberMultipliers cm = GetCamberMultipliersEx(poi.GetSegmentKey(), applyFactors);

         *pDy = cm.SlabUser1Factor*(Dslab + Duser1) + cm.SlabPadLoadFactor*DslabPad + cm.DeckPanelFactor*Dpanel
            + cm.BarrierSwOverlayUser2Factor*(Dtrafficbarrier + Dsidewalk + Doverlay + Duser2);

         *pRz = cm.SlabUser1Factor*(Rslab + Ruser1) + cm.SlabPadLoadFactor*RslabPad + cm.DeckPanelFactor*Rpanel
            + cm.BarrierSwOverlayUser2Factor*(Rtrafficbarrier + Rsidewalk + Roverlay + Ruser2);
      }

      // Switch the sign. Negative deflection creates positive screed camber
      (*pDy) *= -1;
      (*pRz) *= -1;
   }
   else
   {
      // this version is only for PGSuper design mode
#if defined _DEBUG
      GET_IFACE(ILossParameters, pLossParams);
      ATLASSERT(pLossParams->GetLossMethod() != pgsTypes::TIME_STEP);
#endif

      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

      GET_IFACE(IBridge, pBridge);
      pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

      const CSegmentKey& segmentKey(poi.GetSegmentKey());

      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType noncompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
      IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();
      IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

      // NOTE: No need to validate camber models
      Float64 Dslab = 0;
      Float64 Dpanel = 0;
      Float64 DslabPad = 0;
      Float64 Dslab_adj = 0;
      Float64 Dslab_pad_adj = 0;
      Float64 Dtrafficbarrier = 0;
      Float64 Dsidewalk = 0;
      Float64 Doverlay = 0;
      Float64 Duser1 = 0;
      Float64 Duser2 = 0;

      Float64 Rslab = 0;
      Float64 Rpanel = 0;
      Float64 RslabPad = 0;
      Float64 Rslab_adj = 0;
      Float64 Rslab_pad_adj = 0;
      Float64 Rtrafficbarrier = 0;
      Float64 Rsidewalk = 0;
      Float64 Roverlay = 0;
      Float64 Ruser1 = 0;
      Float64 Ruser2 = 0;

      GetDeckDeflection(poi, pConfig, &Dslab, &Rslab, &DslabPad, &RslabPad); // compensates for change in EI from original deflection and the value in the config
      GetDesignSlabDeflectionAdjustment(poi, pConfig, &Dslab_adj, &Rslab_adj); // compensates for change in slab load due to change in slab overhangs associated with "A" dimension
      GetDesignSlabPadDeflectionAdjustment(poi, pConfig, &Dslab_pad_adj, &Rslab_pad_adj); // compensates for change in slab haunch load due to change in "A" dimension

      Dslab -= Dslab_adj;
      DslabPad -= Dslab_pad_adj;

      Rslab -= Rslab_adj;
      RslabPad -= Rslab_pad_adj;

      if (deckType == pgsTypes::sdtCompositeSIP)
      {
         GetDeckPanelDeflection(poi, pConfig, &Dpanel, &Rpanel);
      }

      // deflections are computed based on current parameters for the bridge.
      // E in the config object may be different than E used to compute the deflection.
      // The deflection adjustment factor accounts for the differences in E.
      Float64 k2 = GetDeflectionAdjustmentFactor(poi, pConfig, railingSystemIntervalIdx);

      Dtrafficbarrier = k2*GetDeflection(railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false);
      Rtrafficbarrier = k2*GetRotation(railingSystemIntervalIdx, pgsTypes::pftTrafficBarrier, poi, bat, rtIncremental, false);

      Dsidewalk = k2*GetDeflection(railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental, false);
      Rsidewalk = k2*GetRotation(railingSystemIntervalIdx, pgsTypes::pftSidewalk, poi, bat, rtIncremental, false);

      // Only get deflections for user defined loads that occur during deck placement and later
      GET_IFACE(IPointOfInterest, pPoi);
      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi, &spanKey, &Xspan);
      std::vector<IntervalIndexType> vUserLoadIntervals(pIntervals->GetUserDefinedLoadIntervals(spanKey));
      vUserLoadIntervals.erase(std::remove_if(vUserLoadIntervals.begin(), vUserLoadIntervals.end(), [&noncompositeUserLoadIntervalIdx](const auto& intervalIdx) {return intervalIdx < noncompositeUserLoadIntervalIdx;}), vUserLoadIntervals.end());
      Uint32 ucnt = 0;
      for ( const auto& intervalIdx : vUserLoadIntervals )
      {
         Float64 D, R;

         k2 = GetDeflectionAdjustmentFactor(poi, pConfig, intervalIdx);
         GetUserLoadDeflection(intervalIdx, poi, pConfig, &D, &R);

         // For PGSuper there are two user load stages and they are factored differently. User1 occurs when slab is cast. For splice,
         // the user load stages are always factored by 1.0, so this does not matter (now)
         if (intervalIdx == noncompositeUserLoadIntervalIdx)
         {
            Duser1 += k2*D;
            Ruser1 += k2*R;
         }
         else
         {
            Duser2 += k2*D;
            Ruser2 += k2*R;
         }
      }

      // This is an old convention in PGSuper: If there is no deck, user1 is included in the D camber
      if (deckType == pgsTypes::sdtNone)
      {
         Duser1 = 0.0;
         Ruser1 = 0.0;
      }

      if (!pBridge->IsFutureOverlay() && overlayIntervalIdx != INVALID_INDEX)
      {
         k2 = GetDeflectionAdjustmentFactor(poi, pConfig, overlayIntervalIdx);
         Doverlay = k2*GetDeflection(overlayIntervalIdx, pgsTypes::pftOverlay, poi, bat, rtIncremental, false);
         Roverlay = k2*GetRotation(overlayIntervalIdx, pgsTypes::pftOverlay, poi, bat, rtIncremental, false);
      }

      // apply camber multipliers
      CamberMultipliers cm = GetCamberMultipliersEx(poi.GetSegmentKey(), applyFactors);

      *pDy = cm.SlabUser1Factor*(Dslab + Duser1) + cm.SlabPadLoadFactor*DslabPad + cm.DeckPanelFactor*Dpanel
         + cm.BarrierSwOverlayUser2Factor*(Dtrafficbarrier + Dsidewalk + Doverlay + Duser2);

      *pRz = cm.SlabUser1Factor*(Rslab + Ruser1) + cm.SlabPadLoadFactor*RslabPad + cm.DeckPanelFactor*Rpanel
         + cm.BarrierSwOverlayUser2Factor*(Rtrafficbarrier + Rsidewalk + Roverlay + Ruser2);

      // Switch the sign. Negative deflection creates positive screed camber
      (*pDy) *= -1;
      (*pRz) *= -1;
   }
}

void CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pSlabDy,Float64* pSlabRz,Float64* pSlabPadDy,Float64* pSlabPadRz) const
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   if (castDeckIntervalIdx == INVALID_INDEX)
   {
      // there is no deck
#if defined _DEBUG
      GET_IFACE(IBridge, pBridge);
      pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
      ATLASSERT(deckType == pgsTypes::sdtNone);
#endif
      *pSlabDy = 0;
      *pSlabRz = 0;
      *pSlabPadDy = 0;
      *pSlabPadRz = 0;
      return;
   }

   Float64 dy_slab = GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtIncremental, false);
   Float64 rz_slab = GetRotation(castDeckIntervalIdx, pgsTypes::pftSlab, poi, bat, rtIncremental, false);

   Float64 dy_slab_pad = GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, bat, rtIncremental, false);
   Float64 rz_slab_pad = GetRotation(castDeckIntervalIdx, pgsTypes::pftSlabPad, poi, bat, rtIncremental, false);

   *pSlabDy = dy_slab;
   *pSlabRz = rz_slab;
   *pSlabPadDy = dy_slab_pad;
   *pSlabPadRz = rz_slab_pad;

   if (pConfig)
   {
      Float64 k = GetDeflectionAdjustmentFactor(poi, pConfig, castDeckIntervalIdx);
      (*pSlabDy) *= k;
      (*pSlabRz) *= k;
      (*pSlabPadDy) *= k;
      (*pSlabPadRz) *= k;
   }
}

void CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // NOTE: it is assumed that deck panels are placed at the same time the
   // cast-in-place topping is installed.
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   *pDy = GetDeflection(castDeckIntervalIdx, pgsTypes::pftSlabPanel, poi, bat, rtIncremental, false);
   *pRz = GetRotation(castDeckIntervalIdx, pgsTypes::pftSlabPanel, poi, bat, rtIncremental, false);

   if (pConfig)
   {
      Float64 k = GetDeflectionAdjustmentFactor(poi, pConfig, castDeckIntervalIdx);

      (*pDy) *= k;
      (*pRz) *= k;
   }
}

void CAnalysisAgentImp::GetShearKeyDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType shearKeyIntervalIdx = pIntervals->GetCastShearKeyInterval();

   if (HasShearKeyLoad(poi.GetSegmentKey()))
   {
      *pDy = GetDeflection(shearKeyIntervalIdx, pgsTypes::pftShearKey, poi, bat, rtIncremental, false);
      *pRz = GetRotation(shearKeyIntervalIdx, pgsTypes::pftShearKey, poi, bat, rtIncremental, false);

      if (pConfig)
      {
         Float64 k = GetDeflectionAdjustmentFactor(poi, pConfig, shearKeyIntervalIdx);

         (*pDy) *= k;
         (*pRz) *= k;
      }
   }
   else
   {
      *pDy = 0.0;
      *pRz = 0.0;
   }
}

void CAnalysisAgentImp::GetConstructionLoadDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType constructionLoadIntervalIdx = pIntervals->GetConstructionLoadInterval();

   *pDy = GetDeflection(constructionLoadIntervalIdx, pgsTypes::pftConstruction, poi, bat, rtIncremental, false);
   *pRz = GetRotation(constructionLoadIntervalIdx, pgsTypes::pftConstruction, poi, bat, rtIncremental, false);

   if (pConfig)
   {
      Float64 k = GetDeflectionAdjustmentFactor(poi, pConfig, constructionLoadIntervalIdx);

      (*pDy) *= k;
      (*pRz) *= k;
   }
}

void CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const
{
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();

   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   *pDy = GetDeflection(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi, bat, rtIncremental, false);
   *pRz = GetRotation(castDiaphragmIntervalIdx, pgsTypes::pftDiaphragm, poi, bat, rtIncremental, false);

   if (pConfig)
   {
      Float64 k = GetDeflectionAdjustmentFactor(poi, pConfig, castDiaphragmIntervalIdx);
      (*pDy) *= k;
      (*pRz) *= k;
   }
}

void CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   Float64 Ddc = GetDeflection(intervalIdx, pgsTypes::pftUserDC, poi, bat, rtIncremental, false);
   Float64 Ddw = GetDeflection(intervalIdx, pgsTypes::pftUserDW, poi, bat, rtIncremental, false);

   Float64 Rdc = GetRotation(intervalIdx, pgsTypes::pftUserDC, poi, bat, rtIncremental, false);
   Float64 Rdw = GetRotation(intervalIdx, pgsTypes::pftUserDW, poi, bat, rtIncremental, false);

   *pDy = Ddc + Ddw;
   *pRz = Rdc + Rdw;

   if (pConfig)
   {
      Float64 k = GetDeflectionAdjustmentFactor(poi, pConfig, intervalIdx);

      (*pDy) *= k;
      (*pRz) *= k;
   }
}

void CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const
{
   // NOTE: No need to validate camber models
   Float64 Dslab           = 0;
   Float64 Dslab_adj       = 0;
   Float64 DslabPad        = 0;
   Float64 Dslab_pad_adj   = 0;
   Float64 Dtrafficbarrier = 0;
   Float64 Dsidewalk       = 0;
   Float64 Doverlay        = 0;

   Float64 Rslab           = 0;
   Float64 Rslab_adj       = 0;
   Float64 RslabPad        = 0;
   Float64 Rslab_pad_adj   = 0;
   Float64 Rtrafficbarrier = 0;
   Float64 Rsidewalk       = 0;
   Float64 Roverlay        = 0;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();

   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   GetDeckDeflection(poi, pConfig, &Dslab, &Rslab, &DslabPad, &RslabPad); // compensates for change in EI from original deflection and the value in the config
   GetDesignSlabDeflectionAdjustment(poi, pConfig, &Dslab_adj, &Rslab_adj); // compensates for change in slab load due to change in slab overhangs associated with "A" dimension
   GetDesignSlabPadDeflectionAdjustment(poi, pConfig, &Dslab_pad_adj, &Rslab_pad_adj); // compensates for change in slab haunch load due to change in "A" dimension

   Dslab -= Dslab_adj;
   DslabPad -= Dslab_pad_adj;

   Rslab -= Rslab_adj;
   RslabPad -= Rslab_pad_adj;

   Float64 k2 = GetDeflectionAdjustmentFactor(poi,pConfig,railingSystemIntervalIdx);
   Dtrafficbarrier = k2*GetDeflection(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,poi,bat, rtIncremental, false);
   Rtrafficbarrier = k2*GetRotation(  railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,poi,bat, rtIncremental, false);

   Dsidewalk = k2*GetDeflection(railingSystemIntervalIdx,pgsTypes::pftSidewalk,poi,bat, rtIncremental, false);
   Rsidewalk = k2*GetRotation(  railingSystemIntervalIdx,pgsTypes::pftSidewalk,poi,bat, rtIncremental, false);

   if ( overlayIntervalIdx != INVALID_INDEX )
   {
      Float64 k2 = GetDeflectionAdjustmentFactor(poi,pConfig,overlayIntervalIdx);
      Doverlay = k2*GetDeflection(overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental, false);
      Roverlay = k2*GetRotation(  overlayIntervalIdx,pgsTypes::pftOverlay,poi,bat, rtIncremental, false);
   }

   // Switch the sign. Negative deflection creates positive screed camber
   *pDy = Dslab + DslabPad + Dtrafficbarrier + Dsidewalk + Doverlay;
   *pRz = Rslab + RslabPad + Rtrafficbarrier + Rsidewalk + Roverlay;
}

Float64 CAnalysisAgentImp::GetLowerBoundCamberVariabilityFactor() const
{
   GET_IFACE(ILibrary,pLibrary);
   GET_IFACE(ISpecification,pSpec);

   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 fac = pSpecEntry->GetCamberVariability();

   fac = 1.0-fac;
   return fac;
}

CamberMultipliers CAnalysisAgentImp::GetCamberMultipliers(const CSegmentKey& segmentKey) const
{
   return GetCamberMultipliersEx(segmentKey, true);
}

CamberMultipliers CAnalysisAgentImp::GetCamberMultipliersEx(const CSegmentKey& segmentKey, bool applyFactors) const
{
   if (applyFactors)
   {
      GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup      = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirderLibraryEntry(segmentKey.girderIndex);

   return pGdrEntry->GetCamberMultipliers();
   }
   else
   {
      // factors all 1.0
      return CamberMultipliers();
   }
}

Float64 CAnalysisAgentImp::GetPrecamber(const CSegmentKey& segmentKey) const
{
   ASSERT_SEGMENT_KEY(segmentKey);
   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);
   return pSegment->Precamber;
}

Float64 CAnalysisAgentImp::GetPrecamber(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum) const
{
   Float64 D, R;
   GetPrecamber(poi, datum, &D, &R);
   return D;
}

void CAnalysisAgentImp::GetPrecamber(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, Float64* pDprecamber, Float64* pRprecamber) const
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IBridge, pBridge);
   Float64 Ls = pBridge->GetSegmentLength(segmentKey);

   Float64 Dprecamber, Rprecamber;
   GetRawPrecamber(poi, Ls, &Dprecamber, &Rprecamber);

   GET_IFACE(IPointOfInterest, pPoi);
   PoiList vPoi;
   vPoi.reserve(2);
   switch (datum)
   {
   case pgsTypes::pddRelease:
      pPoi->GetPointsOfInterest(segmentKey, POI_RELEASED_SEGMENT | POI_0L | POI_10L ,&vPoi);
      break;

   case pgsTypes::pddLifting:
      pPoi->GetPointsOfInterest(segmentKey, POI_LIFT_SEGMENT | POI_0L | POI_10L, &vPoi);
      break;

   case pgsTypes::pddStorage:
      pPoi->GetPointsOfInterest(segmentKey, POI_STORAGE_SEGMENT | POI_0L | POI_10L, &vPoi);
      break;

   case pgsTypes::pddHauling:
      pPoi->GetPointsOfInterest(segmentKey, POI_HAUL_SEGMENT | POI_0L | POI_10L, &vPoi);
      break;

   case pgsTypes::pddErected:
      pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT | POI_0L | POI_10L, &vPoi);
      break;

   default:
      ATLASSERT(false); // should never get here
   }
   ATLASSERT(vPoi.size() == 2);
   Float64 D1, R1;
   GetRawPrecamber(vPoi.front(), Ls, &D1, &R1);
   
   Float64 D2, R2;
   GetRawPrecamber(vPoi.back(), Ls, &D2, &R2);

   Float64 adjustment = LinInterp(poi.GetDistFromStart(), D1, D2, Ls);

   Dprecamber -= adjustment;

   *pDprecamber = Dprecamber;
   *pRprecamber = Rprecamber;
}

/////////////////////////////////////////////////////////////////////////////
// IPretensionStresses
//
Float64 CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx) const
{
   return m_pGirderModelManager->GetStress(intervalIdx,poi,stressLocation,bIncludeLiveLoad,limitState,vehicleIdx);
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation topLoc,pgsTypes::StressLocation botLoc,bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx,Float64* pfTop,Float64* pfBot) const
{
   return m_pGirderModelManager->GetStress(intervalIdx,poi,topLoc,botLoc,bIncludeLiveLoad,limitState,vehicleIdx,pfTop,pfBot);
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx, const PoiList& vPoi, pgsTypes::StressLocation stressLocation, bool bIncludeLiveLoad, pgsTypes::LimitState limitState,VehicleIndexType vehicleIdx, std::vector<Float64>* pStresses) const
{
   pStresses->clear();
   pStresses->reserve(vPoi.size());
   for ( const auto& poi : vPoi)
   {
      Float64 stress = GetStress(intervalIdx, poi, stressLocation, bIncludeLiveLoad, limitState, vehicleIdx);
      pStresses->push_back(stress);
   }
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx, const PoiList& vPoi, pgsTypes::StressLocation topLoc, pgsTypes::StressLocation botLoc, bool bIncludeLiveLoad, pgsTypes::LimitState limitState, VehicleIndexType vehicleIdx, std::vector<Float64>* pvfTop, std::vector<Float64>* pvfBot) const
{
   pvfTop->clear();
   pvfTop->reserve(vPoi.size());

   pvfBot->clear();
   pvfBot->reserve(vPoi.size());

   for (const auto& poi : vPoi)
   {
      Float64 fTop, fBot;
      GetStress(intervalIdx, poi, topLoc, botLoc, bIncludeLiveLoad, limitState, vehicleIdx, &fTop, &fBot);
      pvfTop->push_back(fTop);
      pvfBot->push_back(fBot);
   }
}

Float64 CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,Float64 P,Float64 ex,Float64 ey) const
{
   return m_pGirderModelManager->GetStress(intervalIdx,poi,stressLocation,P,ex,ey);
}

Float64 CAnalysisAgentImp::GetStressPerStrand(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation stressLocation) const
{
   GET_IFACE(IPretensionForce,pPsForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   GET_IFACE(ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();
   // If gross properties analysis, we want the prestress force at the end of the interval. It will include
   // elastic effects. If transformed properties analysis, we want the force at the start of the interval.
   pgsTypes::IntervalTimeType timeType (spMode == pgsTypes::spmGross ? pgsTypes::End : pgsTypes::Start);

   Float64 P = pPsForce->GetPrestressForcePerStrand(poi,strandType,intervalIdx,timeType);
   Float64 nSEffective;
   Float64 ex, ey;
   pStrandGeom->GetEccentricity(intervalIdx, poi, strandType, &nSEffective, &ex, &ey);

   return GetStress(intervalIdx,poi,stressLocation,P,ex,ey);
}

Float64 CAnalysisAgentImp::GetDesignStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,const GDRCONFIG& config,bool bIncludeLiveLoad, pgsTypes::LimitState limitState) const
{
   Float64 f;
   GetDesignStress(intervalIdx, poi, stressLocation, stressLocation, config, bIncludeLiveLoad, limitState, &f, &f);
   return f;
}

void CAnalysisAgentImp::GetDesignStress(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, pgsTypes::StressLocation topLoc, pgsTypes::StressLocation botLoc, const GDRCONFIG& config, bool bIncludeLiveLoad, pgsTypes::LimitState limitState, Float64* pfTop, Float64* pfBot) const
{
   // Computes design-time stresses due to prestressing
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IPretensionForce, pPsForce);
   GET_IFACE(IStrandGeometry, pStrandGeom);

   GET_IFACE(ISectionProperties, pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();
   // If gross properties analysis, we want the prestress force at the end of the interval. It will include
   // elastic effects. If transformed properties analysis, we want the force at the start of the interval
   // becase the stress analysis will intrinsically include elastic effects.
   pgsTypes::IntervalTimeType timeType(spMode == pgsTypes::spmGross ? pgsTypes::End : pgsTypes::Start);

   Float64 P;
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   if (intervalIdx < liveLoadIntervalIdx)
   {
      P = pPsForce->GetPrestressForce(poi, pgsTypes::Permanent, intervalIdx, timeType, &config);
   }
   else
   {
      if (bIncludeLiveLoad)
      {
         P = pPsForce->GetPrestressForceWithLiveLoad(poi, pgsTypes::Permanent, limitState, INVALID_INDEX/*controlling live load*/, &config);
      }
      else
      {
         P = pPsForce->GetPrestressForce(poi, pgsTypes::Permanent, intervalIdx, timeType, &config);
      }
   }

   // NOTE: since we are doing design, the main bridge model may not have temporary strand removal
   // intervals. Use the deck casting interval as the break point for "before temporary strands are removed"
   // and "after temporary strands are removed"
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   bool bIncludeTemporaryStrands = intervalIdx < tsRemovalIntervalIdx ? true : false;
   if (bIncludeTemporaryStrands)
   {
      P += pPsForce->GetPrestressForce(poi, pgsTypes::Temporary, intervalIdx, timeType, &config);
   }

   Float64 nSEffective;
   pgsTypes::SectionPropertyType spType = (spMode == pgsTypes::spmGross ? pgsTypes::sptGrossNoncomposite : pgsTypes::sptTransformedNoncomposite);
   Float64 ex, ey;
   pStrandGeom->GetEccentricity(spType, intervalIdx, poi, bIncludeTemporaryStrands, &config, &nSEffective, &ex, &ey);

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   *pfTop = GetStress(releaseIntervalIdx, poi, topLoc, P, ex, ey);
   *pfBot = GetStress(releaseIntervalIdx, poi, botLoc, P, ex, ey);
}

/////////////////////////////////////////////////////////////////////////////
// IBridgeDescriptionEventSink
//
HRESULT CAnalysisAgentImp::OnBridgeChanged(CBridgeChangedHint* pHint)
{
   LOG("OnBridgeChanged Event Received");
   Invalidate();
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnGirderFamilyChanged()
{
   LOG("OnGirderFamilyChanged Event Received");
   Invalidate();
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnGirderChanged(const CGirderKey& girderKey,Uint32 lHint)
{
   LOG("OnGirderChanged Event Received");

   Invalidate();

   return S_OK;
}

HRESULT CAnalysisAgentImp::OnLiveLoadChanged()
{
   LOG("OnLiveLoadChanged Event Received");
   Invalidate();
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnLiveLoadNameChanged(LPCTSTR strOldName,LPCTSTR strNewName)
{
   LOG("OnLiveLoadNameChanged Event Received");
   m_pGirderModelManager->ChangeLiveLoadName(strOldName,strNewName);
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnConstructionLoadChanged()
{
   LOG("OnConstructionLoadChanged Event Received");
   Invalidate();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ISpecificationEventSink
//
HRESULT CAnalysisAgentImp::OnSpecificationChanged()
{
   Invalidate();
   LOG("OnSpecificationChanged Event Received");
   return S_OK;
}

HRESULT CAnalysisAgentImp::OnAnalysisTypeChanged()
{
   //Invalidate();
   // I don't think we have to do anything when the analysis type changes
   // The casting yard models wont change
   // The simple span models, if built, wont change
   // The continuous span models, if built, wont change
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// IRatingSpecificationEventSink
//
HRESULT CAnalysisAgentImp::OnRatingSpecificationChanged()
{
   Invalidate();
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ILoadModifiersEventSink
//
HRESULT CAnalysisAgentImp::OnLoadModifiersChanged()
{
   Invalidate();
   LOG("OnLoadModifiersChanged Event Received");
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// ILossParametersEventSink
//
HRESULT CAnalysisAgentImp::OnLossParametersChanged()
{
   Invalidate();
   LOG("OnLossParametersChanged Event Received");
   return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// Helper functions
#if defined _DEBUG
IntervalIndexType GetInterval(const CSegmentKey& segmentKey, pgsTypes::PrestressDeflectionDatum datum)
{
   IntervalIndexType intervalIdx = INVALID_INDEX;
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IIntervals, pIntervals);
   switch (datum)
   {
   case pgsTypes::pddRelease:
      intervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      break;

   case pgsTypes::pddLifting:
      intervalIdx = pIntervals->GetLiftSegmentInterval(segmentKey);
      break;

   case pgsTypes::pddStorage:
      intervalIdx = pIntervals->GetStorageInterval(segmentKey);
      break;

   case pgsTypes::pddHauling:
      intervalIdx = pIntervals->GetHaulSegmentInterval(segmentKey);
      break;

   case pgsTypes::pddErected:
      intervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      break;

   default:
      ATLASSERT(false);
   }
   return intervalIdx;
}
#endif

void CAnalysisAgentImp::GetPermanentPrestressDeflection(const pgsPointOfInterest& poi, pgsTypes::PrestressDeflectionDatum datum, const GDRCONFIG* pConfig, Float64* pDx, Float64* pDy, Float64* pRz) const
{
   if (pConfig == nullptr)
   {
      CamberModelData camber_model_data = GetPrestressDeflectionModel(poi.GetSegmentKey(), m_PrestressDeflectionModels);

      Float64 DXharped, Dharped, Rharped;
      GetPrestressDeflectionFromModel(poi, camber_model_data, pgsTypes::Harped, datum, &DXharped,&Dharped, &Rharped);

      Float64 DXstraight, Dstraight, Rstraight;
      GetPrestressDeflectionFromModel(poi, camber_model_data, pgsTypes::Straight, datum, &DXstraight,&Dstraight, &Rstraight);

      *pDx = DXstraight + DXharped;
      *pDy = Dstraight + Dharped;
      *pRz = Rstraight + Rharped;

#if defined _DEBUG
      // above deflections don't included temporary strands
      IntervalIndexType intervalIdx = GetInterval(poi.GetSegmentKey(), datum);

      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType tsInstallationIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(poi.GetSegmentKey());
      IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(poi.GetSegmentKey());

      Float64 DXtsi(0), Dtsi(0), Rtsi(0);
      if (tsInstallationIntervalIdx <= intervalIdx)
      {
         CamberModelData temp_strand_model_data = GetPrestressDeflectionModel(poi.GetSegmentKey(), m_InitialTempPrestressDeflectionModels);
         GetPrestressDeflectionFromModel(poi, temp_strand_model_data, pgsTypes::Temporary, datum, &DXtsi, &Dtsi, &Rtsi);
      }

      Float64 DXtsr(0), Dtsr(0), Rtsr(0);
      if (tsRemovalIntervalIdx <= intervalIdx)
      {
         CamberModelData temp_strand_model_data = GetPrestressDeflectionModel(poi.GetSegmentKey(), m_ReleaseTempPrestressDeflectionModels);
         GetPrestressDeflectionFromModel(poi, temp_strand_model_data, pgsTypes::Temporary, datum, &DXtsr, &Dtsi, &Rtsr);
      }

      // these deflections include temporary strands
      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);
      Float64 dx = GetXDeflection(intervalIdx, pgsTypes::pftPretension, poi, bat, rtCumulative);
      Float64 dy = GetDeflection(intervalIdx, pgsTypes::pftPretension, poi, bat, rtCumulative, false);

      Float64 _dx_ = *pDx + DXtsi + DXtsr;
      Float64 _dy_ = *pDy + Dtsi + Dtsr;
      ATLASSERT(IsEqual(_dx_, dx));
      ATLASSERT(IsEqual(_dy_, dy));
#endif
   }
   else
   {
      ValidateCamberModels(pConfig);

      Float64 DXharped, Dharped, Rharped;
      GetPrestressDeflectionFromModel(poi, m_CacheConfig_PrestressDeflectionModel, pgsTypes::Harped, datum, &DXharped, &Dharped, &Rharped);

      Float64 DXstraight, Dstraight, Rstraight;
      GetPrestressDeflectionFromModel(poi, m_CacheConfig_PrestressDeflectionModel, pgsTypes::Straight, datum, &DXstraight, &Dstraight, &Rstraight);

      *pDx = DXstraight + DXharped;
      *pDy = Dstraight + Dharped;
      *pRz = Rstraight + Rharped;
   }
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,pgsTypes::PrestressDeflectionDatum datum,const GDRCONFIG* pConfig,Float64* pDx,Float64* pDy,Float64* pRz) const
{
   if (pConfig == nullptr)
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CamberModelData model_data = GetPrestressDeflectionModel(segmentKey, m_InitialTempPrestressDeflectionModels);

      GetInitialTempPrestressDeflection(poi, model_data, datum, pDx, pDy, pRz);
   }
   else
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CamberModelData model1, model2;
      BuildTempCamberModel(segmentKey, pConfig, &model1, &model2);

      GetInitialTempPrestressDeflection(poi, model1, datum, pDx, pDy, pRz);
   }
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDx,Float64* pDy,Float64* pRz) const
{
   if (pConfig == nullptr)
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CamberModelData model = GetPrestressDeflectionModel(segmentKey, m_ReleaseTempPrestressDeflectionModels);

      GetReleaseTempPrestressDeflection(poi, model, pDx, pDy, pRz);
   }
   else
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CamberModelData model1, model2;
      BuildTempCamberModel(segmentKey, pConfig, &model1, &model2);

      GetReleaseTempPrestressDeflection(poi, model2, pDx, pDy, pRz);
   }
}

void CAnalysisAgentImp::GetPrestressDeflectionFromModel(const pgsPointOfInterest& poi,CamberModelData& modelData,pgsTypes::StrandType strandType,pgsTypes::PrestressDeflectionDatum datum,Float64* pDx,Float64* pDy,Float64* pRz) const
{
   ATLASSERT(strandType != pgsTypes::Permanent); // must be straight, harped, or temporary

   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      *pDx = 0;
      *pDy = 0;
      *pRz = 0;
      return;
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // We need to compute the deflection relative to the bearings even though the prestress
   // deflection occurs over the length of the entire girder.  To accomplish this, simply
   // deduct the deflection at the bearing (when the girder is supported on its ends) from
   // the deflection at the specified location.
   CComQIPtr<IFem2dModelResults> results(modelData.Model);
   Float64 Dx, Dy, Rz;

   PoiIDPairType femPoiID = modelData.PoiMap.GetModelPoi(poi);
   if ( femPoiID.first == INVALID_ID )
   {
      pgsPointOfInterest thePOI;
      if ( poi.GetID() == INVALID_ID )
      {
         GET_IFACE(IPointOfInterest,pPOI);
         thePOI = pPOI->GetPointOfInterest(segmentKey,poi.GetDistFromStart());
      }
      else
      {
         thePOI = poi;
      }

      femPoiID = AddPointOfInterest(modelData,thePOI);
      ATLASSERT( 0 <= femPoiID.first );
   }

   LoadCaseIDType lcidX, lcidY;
   GetPrestressLoadCaseIDs(strandType, &lcidX, &lcidY);

   // get deflection due to prestressing associated with moments about the x-axis
   HRESULT hr = results->ComputePOIDeflections(lcidX,femPoiID.first,lotGlobal,&Dx,&Dy,&Rz);
   ATLASSERT( SUCCEEDED(hr) );
   Float64 delta_y1 = Dy;

   *pRz = Rz;

   // get deflection due to prestressing associated with moments about the y-axis
   // remember that we have a plane frame model so we've applied the loads in the same direction as those causing y-deflections
   hr = results->ComputePOIDeflections(lcidY, femPoiID.first, lotGlobal, &Dx, &Dy, &Rz);
   ATLASSERT(SUCCEEDED(hr));
   Float64 delta_x1 = Dy; // remember that this is using the vertical deflection stiffness. we have to adjust for the lateral stiffness

   // get the section properties used to build the model
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   GET_IFACE(ISectionProperties, pSectProp);
   PoiList vMyPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vMyPoi);
   ATLASSERT(vMyPoi.size() == 1);
   const pgsPointOfInterest& spPoi = vMyPoi.front();
   ATLASSERT(spPoi.IsMidSpan(POI_RELEASED_SEGMENT));

   Float64 Ixx = pSectProp->GetIxx(releaseIntervalIdx, spPoi);
   Float64 Iyy = pSectProp->GetIyy(releaseIntervalIdx, spPoi);
   Float64 Ixy = pSectProp->GetIxy(releaseIntervalIdx, spPoi);

   // delta_x was computed using the vertical deflection stiffness
   // to adjust for this, multiply by (IxxIyy-Ixy^2)/Iyy and then
   // divide by Ixx/(IxxIyy-Ixy^2)... this results in a multiplication by
   // Ixx/Iyy. 
   delta_x1 *= (Ixx / Iyy);

   // compute the lateral deflection due to moments about the x-axis
   Float64 delta_x2 = delta_y1*(-Ixy / Iyy);

   // compute the vertical deflection due to moments about the y-axis
   Float64 delta_y2 = delta_x1*(-Ixy / Ixx); 

   // total deflection
   Float64 delta_x = delta_x1 + delta_x2;
   Float64 delta_y = delta_y1 + delta_y2;

   // we have the deflection relative to the release support locations
   // we want the deflection relative to a different datum
   PoiAttributeType poiDatum;
   switch(datum)
   {
   case pgsTypes::pddRelease:
      poiDatum = POI_RELEASED_SEGMENT;
      break;

   case pgsTypes::pddLifting:
      poiDatum = POI_LIFT_SEGMENT;
      break;

   case pgsTypes::pddStorage:
      poiDatum = POI_STORAGE_SEGMENT;
      break;

   case pgsTypes::pddHauling:
      poiDatum = POI_HAUL_SEGMENT;
      break;

   case pgsTypes::pddErected:
      poiDatum = POI_ERECTED_SEGMENT;
      break;

   default:
      ATLASSERT(false); // should never get here
      poiDatum = POI_RELEASED_SEGMENT;
   }

   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, poiDatum | POI_0L | POI_10L, &vPoi);
   ATLASSERT(vPoi.size() == 2);

   const pgsPointOfInterest& poiAtStart( vPoi.front() );
   ATLASSERT(poiAtStart.IsTenthPoint(poiDatum) == 1);
   ATLASSERT( 0 <= poiAtStart.GetID() );
   
   femPoiID = modelData.PoiMap.GetModelPoi(poiAtStart);
   results->ComputePOIDeflections(lcidX, femPoiID.first, lotGlobal, &Dx, &Dy, &Rz);
   Float64 start_delta_brg_y1 = Dy;

   results->ComputePOIDeflections(lcidY, femPoiID.first, lotGlobal, &Dx, &Dy, &Rz);
   Float64 start_delta_brg_x1 = Dy; // based on vertical stiffness
   start_delta_brg_x1 *= (Ixx / Iyy); // adjust for lateral stiffness

   Float64 start_delta_brg_x2 = start_delta_brg_y1*(-Ixy / Iyy); // lateral deflection due to Mx
   Float64 start_delta_brg_y2 = start_delta_brg_x1*(-Ixy / Ixx); // vertical deflection due to My

   // total deflection
   Float64 start_delta_brg_x = start_delta_brg_x1 + start_delta_brg_x2;
   Float64 start_delta_brg_y = start_delta_brg_y1 + start_delta_brg_y2;

   pgsPointOfInterest poiAtEnd( vPoi.back() );
   ATLASSERT(poiAtEnd.IsTenthPoint(poiDatum) == 11);
   ATLASSERT( 0 <= poiAtEnd.GetID() );
   femPoiID = modelData.PoiMap.GetModelPoi(poiAtEnd);

   results->ComputePOIDeflections(lcidX, femPoiID.first, lotGlobal, &Dx, &Dy, &Rz);
   Float64 end_delta_brg_y1 = Dy;

   results->ComputePOIDeflections(lcidY, femPoiID.first, lotGlobal, &Dx, &Dy, &Rz);
   Float64 end_delta_brg_x1 = Dy; // based on vertical stiffness
   end_delta_brg_x1 *= (Ixx / Iyy); // adjust for lateral stiffness

   Float64 end_delta_brg_x2 = end_delta_brg_y1*(-Ixy / Iyy); // lateral deflection due to Mx
   Float64 end_delta_brg_y2 = end_delta_brg_x1*(-Ixy / Ixx); // vertical deflection due to My

   // total deflection
   Float64 end_delta_brg_x = end_delta_brg_x1 + end_delta_brg_x2;
   Float64 end_delta_brg_y = end_delta_brg_y1 + end_delta_brg_y2;

   Float64 z = poi.GetDistFromStart() - poiAtStart.GetDistFromStart();
   Float64 delta_z = poiAtEnd.GetDistFromStart() - poiAtStart.GetDistFromStart();

   Float64 delta_brg_x = LinInterp(z, start_delta_brg_x, end_delta_brg_x, delta_z);
   delta_x -= delta_brg_x;

   Float64 delta_brg_y = LinInterp(z, start_delta_brg_y, end_delta_brg_y, delta_z);
   delta_y -= delta_brg_y;

   *pDx = delta_x;
   *pDy = delta_y;
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,pgsTypes::PrestressDeflectionDatum datum,Float64* pDx,Float64* pDy,Float64* pRz) const
{
   GetPrestressDeflectionFromModel(poi,modelData,pgsTypes::Temporary,datum,pDx,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,Float64* pDx,Float64* pDy,Float64* pRz) const
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      *pDx = 0;
      *pDy = 0;
      *pRz = 0;
      return;
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   LoadCaseIDType lcidX, lcidY;
   GetPrestressLoadCaseIDs(pgsTypes::Temporary, &lcidX, &lcidY);

   // Get deflection at poi
   CComQIPtr<IFem2dModelResults> results(modelData.Model);
   Float64 Dx, Dy, Rz;
   PoiIDPairType femPoiID = modelData.PoiMap.GetModelPoi(poi);
   if (femPoiID.first == INVALID_ID)
   {
      PoiIDPairType result = pgsGirderModelFactory::AddPointOfInterest(modelData.Model, poi);
      modelData.PoiMap.AddMap(poi, result);
      femPoiID = modelData.PoiMap.GetModelPoi(poi);
      ATLASSERT(femPoiID.first != INVALID_ID);
   }
   results->ComputePOIDeflections(lcidX,femPoiID.first,lotGlobal,&Dx,&Dy,&Rz);
   Float64 delta_poi_y = Dy;

   *pRz = Rz;

   results->ComputePOIDeflections(lcidY, femPoiID.first, lotGlobal, &Dx, &Dy, &Rz);
   Float64 delta_poi_x = Dy;

   // get the section properties used to build the model
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   GET_IFACE(ISectionProperties, pSectProp);
   PoiList vMyPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_5L | POI_RELEASED_SEGMENT, &vMyPoi);
   ATLASSERT(vMyPoi.size() == 1);
   const pgsPointOfInterest& spPoi = vMyPoi.front();
   ATLASSERT(spPoi.IsMidSpan(POI_RELEASED_SEGMENT));

   Float64 Ixx = pSectProp->GetIxx(erectionIntervalIdx, spPoi);
   Float64 Iyy = pSectProp->GetIyy(erectionIntervalIdx, spPoi);
   Float64 Ixy = pSectProp->GetIxy(erectionIntervalIdx, spPoi);

   // delta_poi_x is based on section properties for vertical deflection... adjust
   delta_poi_x *= (Ixx / Iyy);

   // compute the lateral deflection due to moments about the x-axis
   Float64 dx = delta_poi_y*(-Ixy / Iyy);

   // compute the vertical deflection due to moments about the y-axis
   Float64 dy = delta_poi_x*(-Ixy / Ixx);

   delta_poi_x += dx;
   delta_poi_y += dy;

   PoiList vPoi;
   pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vPoi);
   ATLASSERT(vPoi.size() == 2);
   const pgsPointOfInterest& poiStart(vPoi.front());
   const pgsPointOfInterest& poiEnd(vPoi.back());
   ATLASSERT(poiStart.IsTenthPoint(POI_ERECTED_SEGMENT) == 1);
   ATLASSERT(poiEnd.IsTenthPoint(POI_ERECTED_SEGMENT) == 11);

   // Get deflection at start bearing
   femPoiID = modelData.PoiMap.GetModelPoi(poiStart);
   if (femPoiID.first == INVALID_ID)
   {
      PoiIDPairType result = pgsGirderModelFactory::AddPointOfInterest(modelData.Model, poiStart);
      modelData.PoiMap.AddMap(poiStart, result);
      femPoiID = modelData.PoiMap.GetModelPoi(poiStart);
      ATLASSERT(femPoiID.first != INVALID_ID);
   }
   results->ComputePOIDeflections(lcidX, femPoiID.first, lotGlobal, &Dx, &Dy, &Rz);
   Float64 start_delta_y_brg = Dy;

   results->ComputePOIDeflections(lcidY, femPoiID.first, lotGlobal, &Dx, &Dy, &Rz);
   Float64 start_delta_x_brg = Dy;

   start_delta_x_brg *= (Ixx / Iyy);

   dx = start_delta_y_brg*(-Ixy / Iyy);
   dy = start_delta_x_brg*(-Ixy / Iyy);

   start_delta_x_brg += dx;
   start_delta_y_brg += dy;

   // Get deflection at end bearing
   femPoiID = modelData.PoiMap.GetModelPoi(poiEnd);
   if (femPoiID.first == INVALID_ID)
   {
      PoiIDPairType result = pgsGirderModelFactory::AddPointOfInterest(modelData.Model, poiEnd);
      modelData.PoiMap.AddMap(poiEnd, result);
      femPoiID = modelData.PoiMap.GetModelPoi(poiEnd);
      ATLASSERT(femPoiID.first != INVALID_ID);
   }
   results->ComputePOIDeflections(lcidX,femPoiID.first,lotGlobal,&Dx,&Dy,&Rz);
   Float64 end_delta_y_brg = Dy;

   results->ComputePOIDeflections(lcidY, femPoiID.first, lotGlobal, &Dx, &Dy, &Rz);
   Float64 end_delta_x_brg = Dy;

   end_delta_x_brg *= (Ixx / Iyy);

   dx = end_delta_y_brg*(-Ixy / Iyy);
   dy = end_delta_x_brg*(-Ixy / Iyy);

   end_delta_x_brg += dx;
   end_delta_y_brg += dy;

   Float64 z = poi.GetDistFromStart() - poiStart.GetDistFromStart();
   Float64 delta_z = poiEnd.GetDistFromStart() - poiStart.GetDistFromStart();

   Float64 delta_x_brg = LinInterp(z, start_delta_x_brg, end_delta_x_brg, delta_z);
   Float64 delta_y_brg = LinInterp(z, start_delta_y_brg, end_delta_y_brg, delta_z);

   Float64 delta_x = delta_poi_x - delta_x_brg;
   Float64 delta_y = delta_poi_y - delta_y_brg;

   *pDy = delta_y;
   *pDx = delta_x;
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const
{
   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   GET_IFACE(IStrandGeometry, pStrandGeom);
   bool bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary, pConfig) && pStrandGeom->GetTemporaryStrandUsage(segmentKey, pConfig) != pgsTypes::ttsPTBeforeShipping);

   Float64 Dcreep = 0;
   switch( deckType )
   {
      case pgsTypes::sdtCompositeCIP:
      case pgsTypes::sdtCompositeOverlay:
      case pgsTypes::sdtNonstructuralOverlay:
         (bTempStrands ? GetCreepDeflection_CIP_TempStrands(poi,pConfig,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz)
                       : GetCreepDeflection_CIP(poi,pConfig,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz));
         break;

      case pgsTypes::sdtCompositeSIP:
         (bTempStrands ? GetCreepDeflection_SIP_TempStrands(poi,pConfig,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz) 
                       : GetCreepDeflection_SIP(poi,pConfig,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz));
         break;

      case pgsTypes::sdtNone:
         (bTempStrands ? GetCreepDeflection_NoDeck_TempStrands(poi,pConfig,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz) 
                       : GetCreepDeflection_NoDeck(poi,pConfig,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz));
         break;

      default:
         ATLASSERT(false); // should never get here... is there a new deck type?
   }
}

void CAnalysisAgentImp::GetCreepDeflection_CIP_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const
{
   ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastIntermediateDiaphragmsInterval();


   Float64 DXharped, Dharped, Rharped;
   GetPrestressDeflectionFromModel( poi, initModelData,     pgsTypes::Harped,    pgsTypes::pddStorage, &DXharped, &Dharped, &Rharped);

   Float64 DXstraight, Dstraight, Rstraight;
   GetPrestressDeflectionFromModel( poi, initModelData,     pgsTypes::Straight,    pgsTypes::pddStorage, &DXstraight, &Dstraight, &Rstraight);

   Float64 DXtpsi, Dtpsi,Rtpsi;
   GetPrestressDeflectionFromModel( poi, initTempModelData, pgsTypes::Temporary, pgsTypes::pddStorage, &DXtpsi, &Dtpsi, &Rtpsi);

   Float64 Dps = Dharped + Dstraight + Dtpsi;
   Float64 Rps = Rharped + Rstraight + Rtpsi;

   Float64 Ct1;
   Float64 Ct2;
   Float64 Ct3;
   Float64 DgStorage, RgStorage;
   Float64 DgErected, RgErected;
   Float64 DgInc,     RgInc;

   GetGirderDeflectionForCamber(poi, pConfig, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc);
   Ct1 = GetCreepCoefficient(segmentKey, cpReleaseToDiaphragm, constructionRate, pConfig);
   Ct2 = GetCreepCoefficient(segmentKey, cpDiaphragmToDeck, constructionRate, pConfig);
   Ct3 = GetCreepCoefficient(segmentKey, cpReleaseToDeck, constructionRate, pConfig);

   // To account for the fact that deflections are measured from different datums during storage
   // and after erection, we have to compute offsets that account for the translated coordinate systems.
   // These values adjust deformations that are measured relative to the storage supports so that
   // they are relative to the final bearing locations.
   Float64 DcreepSupport = 0; // adjusts deformation due to creep
   Float64 RcreepSupport = 0;
   Float64 DgirderSupport = 0; // adjusts deformation due to girder self weight
   Float64 RgirderSupport = 0;
   Float64 DpsSupport = 0; // adjusts deformation due to prestress
   Float64 RpsSupport = 0;
   if ( datum != pgsTypes::pddStorage )
   {
      // get POI at final bearing locations.... 
      // we want to deduct the deformation relative to the storage supports at these locations from the storage deformations
      // to make the deformation relative to the final bearings
      GET_IFACE(IPointOfInterest,pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 2);
      const pgsPointOfInterest& poiLeft(vPoi.front());
      const pgsPointOfInterest& poiRight(vPoi.back());
      ATLASSERT(poiLeft.IsTenthPoint(POI_ERECTED_SEGMENT) == 1);
      ATLASSERT(poiRight.IsTenthPoint(POI_ERECTED_SEGMENT) == 11);
      ATLASSERT(poiLeft.GetID() != INVALID_ID && poiRight.GetID() != INVALID_ID);

      // get creep deformations during storage at location of final bearings
      Float64 DyCreepLeft,RzCreepLeft;
      Float64 DyCreepRight,RzCreepRight;
      GetCreepDeflection_CIP_TempStrands(poiLeft, pConfig,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepLeft,  &RzCreepLeft);
      GetCreepDeflection_CIP_TempStrands(poiRight,pConfig,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepRight, &RzCreepRight);
      // compute adjustment for the current poi
      DcreepSupport = ::LinInterp(poi.GetDistFromStart(),DyCreepLeft,DyCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RcreepSupport = ::LinInterp(poi.GetDistFromStart(),RzCreepLeft,RzCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());

      // get girder deflections during storage at the location of the final bearings
      Float64 D1, D2, D1E, D2E, R1, R2, R1E, R2E;
      Float64 DI1, DI2, RI1, RI2;
      GetGirderDeflectionForCamber(poiLeft, pConfig, &D1, &R1, &D1E, &R1E, &DI1, &RI1);
      GetGirderDeflectionForCamber(poiRight, pConfig, &D2, &R2, &D2E, &R2E, &DI2, &RI2);

      // compute adjustment for the current poi
      DgirderSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RgirderSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   
      // get prestress deflections during storage at the location of the final bearings
      if ( pConfig == nullptr )
      {
         GET_IFACE(IIntervals, pIntervals);
         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

         D1 = GetDeflection(storageIntervalIdx, pgsTypes::pftPretension, poiLeft, bat, rtCumulative, false);
         R1 = GetRotation(storageIntervalIdx, pgsTypes::pftPretension, poiLeft, bat, rtCumulative, false);

         D2 = GetDeflection(storageIntervalIdx, pgsTypes::pftPretension, poiRight, bat, rtCumulative, false);
         R2 = GetRotation(storageIntervalIdx, pgsTypes::pftPretension, poiRight, bat, rtCumulative, false);
      }
      else
      {
         Float64 DXharped, Dharped, Rharped;
         Float64 DXstraight, Dstraight, Rstraight;
         Float64 DXti, Dti, Rti;
         GetPrestressDeflectionFromModel(poiLeft, initModelData, pgsTypes::Harped, pgsTypes::pddStorage, &DXharped, &Dharped, &Rharped);
         GetPrestressDeflectionFromModel(poiLeft, initModelData, pgsTypes::Straight, pgsTypes::pddStorage, &DXstraight, &Dstraight, &Rstraight);
         GetPrestressDeflectionFromModel(poiLeft, initTempModelData, pgsTypes::Temporary, pgsTypes::pddStorage, &DXti, &Dti, &Rti);
         D1 = Dharped + Dstraight + Dti;
         R1 = Rharped + Rstraight + Rti;

         GetPrestressDeflectionFromModel(poiRight, initModelData, pgsTypes::Harped, pgsTypes::pddStorage, &DXharped, &Dharped, &Rharped);
         GetPrestressDeflectionFromModel(poiRight, initModelData, pgsTypes::Straight, pgsTypes::pddStorage, &DXstraight, &Dstraight, &Rstraight);
         GetPrestressDeflectionFromModel(poiRight, initTempModelData, pgsTypes::Temporary, pgsTypes::pddStorage, &DXti, &Dti, &Rti);
         D2 = Dharped + Dstraight + Dti;
         R2 = Rharped + Rstraight + Rti;
      }
      // compute adjustment for the current poi
      DpsSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RpsSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      *pDy = Ct1*(DgStorage + Dps);
      *pRz = Ct1*(RgStorage + Rps);

      if ( datum == pgsTypes::pddErected )
      {
         // creep deflection after erection, relative to final bearings
         *pDy -= DcreepSupport;
         *pRz -= RcreepSupport;
      }

      return;
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      ATLASSERT(datum == pgsTypes::pddErected);

      // creep 2 - Immediately after diarphagm/temporary strands to deck
      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize); // want the largest downward deflection
      Float64 Ddiaphragm = GetDeflection(castDiaphragmIntervalIdx,pgsTypes::pftDiaphragm,poi,bat, rtIncremental, false);
      Float64 Rdiaphragm = GetRotation(  castDiaphragmIntervalIdx,pgsTypes::pftDiaphragm,poi,bat, rtIncremental, false);

      Float64 DXtpsr, Dtpsr,Rtpsr;
      GetPrestressDeflectionFromModel( poi, releaseTempModelData, pgsTypes::Temporary, pgsTypes::pddErected, &DXtpsr, &Dtpsr, &Rtpsr);

      *pDy = (Ct3-Ct1)*(DgStorage + Dps) - (Ct3-Ct1)*(DgirderSupport + DpsSupport) + Ct2*(DgInc + Ddiaphragm + Dtpsr);
      *pRz = (Ct3-Ct1)*(RgStorage + Rps) - (Ct3-Ct1)*(DgirderSupport + DpsSupport) + Ct2*(RgInc + Rdiaphragm + Rtpsr);
   }
   else /*if ( creepPeriod == cpReleaseToDeck )*/
   {
      ATLASSERT(datum == pgsTypes::pddErected);
      Float64 D1, D2, R1, R2;
      GetCreepDeflection_CIP_TempStrands(poi,pConfig,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate,datum,&D1,&R1);
      GetCreepDeflection_CIP_TempStrands(poi,pConfig,initModelData,initTempModelData,releaseTempModelData, cpDiaphragmToDeck,    constructionRate,datum,&D2,&R2);

      *pDy = D1 + D2;
      *pRz = R1 + R2;
   }
}

void CAnalysisAgentImp::GetCreepDeflection_CIP(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const
{
   ATLASSERT( datum != pgsTypes::pddRelease ); // no creep at release
   ATLASSERT( creepPeriod == cpReleaseToDeck || creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   Float64 Dps, Rps; // measured relative to storage supports
   if ( pConfig == nullptr )
   {
      GET_IFACE(IIntervals, pIntervals);
      IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);
      Dps = GetDeflection(storageIntervalIdx, pgsTypes::pftPretension, poi, bat, rtCumulative, false);
      Rps = GetRotation(storageIntervalIdx, pgsTypes::pftPretension, poi, bat, rtCumulative, false);
   }
   else
   {
      Float64 DXharped, Dharped, Rharped;
      GetPrestressDeflectionFromModel(poi, initModelData, pgsTypes::Harped, pgsTypes::pddStorage, &DXharped, &Dharped, &Rharped);

      Float64 DXstraight, Dstraight, Rstraight;
      GetPrestressDeflectionFromModel(poi, initModelData, pgsTypes::Straight, pgsTypes::pddStorage, &DXstraight, &Dstraight, &Rstraight);

      Dps = Dharped + Dstraight;
      Rps = Rharped + Rstraight;
   }

   Float64 Ct1;
   Float64 Ct2;
   Float64 Ct3;
   Float64 DgStorage, RgStorage;
   Float64 DgErected, RgErected;
   Float64 DgInc,     RgInc;

   GetGirderDeflectionForCamber(poi, pConfig, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc);
   Ct1 = GetCreepCoefficient(segmentKey, cpReleaseToDiaphragm, constructionRate, pConfig);
   Ct2 = GetCreepCoefficient(segmentKey, cpDiaphragmToDeck, constructionRate, pConfig);
   Ct3 = GetCreepCoefficient(segmentKey, cpReleaseToDeck, constructionRate, pConfig);

   // To account for the fact that deflections are measured from different datums during storage
   // and after erection, we have to compute offsets that account for the translated coordinate systems.
   // These values adjust deformations that are measured relative to the storage supports so that
   // they are relative to the final bearing locations.
   Float64 DcreepSupport = 0; // adjusts deformation due to creep
   Float64 RcreepSupport = 0;
   Float64 DgirderSupport = 0; // adjusts deformation due to girder self weight
   Float64 RgirderSupport = 0;
   Float64 DpsSupport = 0; // adjusts deformation due to prestress
   Float64 RpsSupport = 0;
   if ( datum != pgsTypes::pddStorage )
   {
      // get POI at final bearing locations.... 
      // we want to deduct the deformation relative to the storage supports at these locations from the storage deformations
      // to make the deformation relative to the final bearings
      GET_IFACE(IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 2);
      const pgsPointOfInterest& poiLeft(vPoi.front());
      const pgsPointOfInterest& poiRight(vPoi.back());
      ATLASSERT(poiLeft.IsTenthPoint(POI_ERECTED_SEGMENT) == 1);
      ATLASSERT(poiRight.IsTenthPoint(POI_ERECTED_SEGMENT) == 11);
      ATLASSERT(poiLeft.GetID() != INVALID_ID && poiRight.GetID() != INVALID_ID);

      // get creep deformations during storage at location of final bearings
      Float64 DyCreepLeft,RzCreepLeft;
      Float64 DyCreepRight,RzCreepRight;
      GetCreepDeflection_CIP(poiLeft, pConfig,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepLeft,  &RzCreepLeft);
      GetCreepDeflection_CIP(poiRight,pConfig,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepRight, &RzCreepRight);
      // compute adjustment for the current poi
      DcreepSupport = ::LinInterp(poi.GetDistFromStart(),DyCreepLeft,DyCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RcreepSupport = ::LinInterp(poi.GetDistFromStart(),RzCreepLeft,RzCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());

      // get girder deflections during storage at the location of the final bearings
      Float64 D1, D2, D1E, D2E, R1, R2, R1E, R2E;
      Float64 DI1, DI2, RI1, RI2;
      GetGirderDeflectionForCamber(poiLeft, pConfig, &D1, &R1, &D1E, &R1E, &DI1, &RI1);
      GetGirderDeflectionForCamber(poiRight, pConfig, &D2, &R2, &D2E, &R2E, &DI2, &RI2);

      // compute adjustment for the current poi
      DgirderSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RgirderSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   
      // get prestress deflections during storage at the location of the final bearings
      if ( pConfig == nullptr )
      {
         GET_IFACE(IIntervals, pIntervals);
         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

         D1 = GetDeflection(storageIntervalIdx, pgsTypes::pftPretension, poiLeft, bat, rtCumulative, false);
         R1 = GetRotation(storageIntervalIdx, pgsTypes::pftPretension, poiLeft, bat, rtCumulative, false);

         D2 = GetDeflection(storageIntervalIdx, pgsTypes::pftPretension, poiRight, bat, rtCumulative, false);
         R2 = GetRotation(storageIntervalIdx, pgsTypes::pftPretension, poiRight, bat, rtCumulative, false);
      }
      else
      {
         Float64 DXharped, Dharped, Rharped;
         Float64 DXstraight, Dstraight, Rstraight;
         GetPrestressDeflectionFromModel(poiLeft, initModelData, pgsTypes::Harped, datum, &DXharped, &Dharped, &Rharped);
         GetPrestressDeflectionFromModel(poiLeft, initModelData, pgsTypes::Straight, datum, &DXstraight, &Dstraight, &Rstraight);
         D1 = Dharped + Dstraight;
         R1 = Rharped + Rstraight;

         GetPrestressDeflectionFromModel(poiRight, initModelData, pgsTypes::Harped, datum, &DXharped, &Dharped, &Rharped);
         GetPrestressDeflectionFromModel(poiRight, initModelData, pgsTypes::Straight, datum, &DXstraight, &Dstraight, &Rstraight);
         D2 = Dharped + Dstraight;
         R2 = Rharped + Rstraight;
      }
      // compute adjustment for the current poi
      DpsSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RpsSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // creep deflection during storage, relative to storage supports
      *pDy = Ct1*(DgStorage + Dps);
      *pRz = Ct1*(RgStorage + Rps);

      if ( datum == pgsTypes::pddErected )
      {
         // creep deflection after erection, relative to final bearings
         *pDy -= DcreepSupport;
         *pRz -= RcreepSupport;
      }
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      ATLASSERT(datum == pgsTypes::pddErected);
      // (creep measured from storage supports at this poi) - (creep at final bearing locations measured from storage supports) + (creep due to incremental girder self-weight deflection)
      // |---------------------------------------------------------------------------------------------------------------------|
      //            |
      //            +- This is equal to the creep relative to the final bearing locations
      *pDy = (Ct3 - Ct1)*(DgStorage + Dps) - (Ct3 - Ct1)*(DgirderSupport + DpsSupport) + Ct2*DgInc;
      *pRz = (Ct3 - Ct1)*(RgStorage + Rps) - (Ct3 - Ct1)*(RgirderSupport + RpsSupport) + Ct2*RgInc;
   }
   else /*if ( creepPeriod == cpReleaseToDeck )*/
   {
      ATLASSERT(datum == pgsTypes::pddErected);
      Float64 D1, D2, R1, R2;
      GetCreepDeflection_CIP(poi,pConfig,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate,datum,&D1,&R1);
      GetCreepDeflection_CIP(poi,pConfig,initModelData,initTempModelData,releaseTempModelData, cpDiaphragmToDeck,    constructionRate,datum,&D2,&R2);

      *pDy = D1 + D2;
      *pRz = R1 + R2;
   }
}

void CAnalysisAgentImp::GetCreepDeflection_SIP_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const
{
   // Creep periods and loading are the same as for CIP decks
   // an improvement could be to add a third creep stage for creep after deck panel placement to deck casting
   GetCreepDeflection_CIP_TempStrands(poi,pConfig,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz);
}

void CAnalysisAgentImp::GetCreepDeflection_SIP(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const
{
   // Creep periods and loading are the same as for CIP decks
   // an improvement could be to add a third creep stage for creep after deck panel placement to deck casting
   GetCreepDeflection_CIP(poi,pConfig,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,datum,pDy,pRz);
}

void CAnalysisAgentImp::GetCreepDeflection_NoDeck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const
{
   //ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck || creepPeriod == cpDeckToFinal);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 DXharped, Dharped, Rharped;
   GetPrestressDeflectionFromModel( poi, initModelData,     pgsTypes::Harped,    pgsTypes::pddStorage, &DXharped, &Dharped, &Rharped);

   Float64 DXstraight, Dstraight, Rstraight;
   GetPrestressDeflectionFromModel( poi, initModelData,     pgsTypes::Straight,    pgsTypes::pddStorage, &DXstraight, &Dstraight, &Rstraight);

   Float64 DXtpsi, Dtpsi, Rtpsi;
   GetPrestressDeflectionFromModel( poi, initTempModelData, pgsTypes::Temporary, pgsTypes::pddStorage, &DXtpsi, &Dtpsi, &Rtpsi);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 DXtpsr, Dtpsr, Rtpsr;
   GetPrestressDeflectionFromModel( poi, releaseTempModelData, pgsTypes::Temporary, pgsTypes::pddErected, &DXtpsr, &Dtpsr, &Rtpsr);

   Float64 DgStorage, RgStorage;
   Float64 DgErected, RgErected;
   Float64 DgInc, RgInc;
   Float64 Ddiaphragm, Rdiaphragm;
   Float64 Dshearkey, Rshearkey;
   Float64 Dconstr, Rconstr;
   Float64 Duser1, Ruser1;
   Float64 Duser2, Ruser2;
   Float64 Dbarrier, Rbarrier;

   GET_IFACE(IIntervals,pIntervals);
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   std::vector<IntervalIndexType> vUserLoadIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey);
   ATLASSERT(vUserLoadIntervals.size() <= 2);
#endif

   IntervalIndexType nonCompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();

#if defined _DEBUG
   std::vector<IntervalIndexType>::iterator iter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator end(vUserLoadIntervals.end());
   for ( ; iter != end; iter++ )
   {
      ATLASSERT(*iter == nonCompositeUserLoadIntervalIdx || *iter == compositeUserLoadIntervalIdx);
   }
#endif

   GetGirderDeflectionForCamber(poi, pConfig, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc);
   GetDiaphragmDeflection(poi, pConfig, &Ddiaphragm, &Rdiaphragm);
   GetShearKeyDeflection(poi, pConfig, &Dshearkey, &Rshearkey);
   GetConstructionLoadDeflection(poi, pConfig, &Dconstr, &Rconstr);
   GetUserLoadDeflection(nonCompositeUserLoadIntervalIdx, poi, pConfig, &Duser1, &Ruser1);
   GetUserLoadDeflection(compositeUserLoadIntervalIdx, poi, pConfig, &Duser2, &Ruser2);
   GetSlabBarrierOverlayDeflection(poi, pConfig, &Dbarrier, &Rbarrier);

   // To account for the fact that deflections are measured from different datums during storage
   // and after erection, we have to compute offsets that account for the translated coordinate systems.
   // These values adjust deformations that are measured relative to the storage supports so that
   // they are relative to the final bearing locations.
   Float64 DcreepSupport = 0; // adjusts deformation due to creep
   Float64 RcreepSupport = 0;
   Float64 DgirderSupport = 0; // adjusts deformation due to girder self weight
   Float64 RgirderSupport = 0;
   Float64 DpsSupport = 0; // adjusts deformation due to prestress
   Float64 RpsSupport = 0;
   if ( datum != pgsTypes::pddStorage )
   {
      // get POI at final bearing locations.... 
      // we want to deduct the deformation relative to the storage supports at these locations from the storage deformations
      // to make the deformation relative to the final bearings
      GET_IFACE(IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 2);
      const pgsPointOfInterest& poiLeft(vPoi.front());
      const pgsPointOfInterest& poiRight(vPoi.back());
      ATLASSERT(poiLeft.IsTenthPoint(POI_ERECTED_SEGMENT) == 1);
      ATLASSERT(poiRight.IsTenthPoint(POI_ERECTED_SEGMENT) == 11);
      ATLASSERT(poiLeft.GetID() != INVALID_ID && poiRight.GetID() != INVALID_ID);

      // get creep deformations during storage at location of final bearings
      Float64 DyCreepLeft,RzCreepLeft;
      Float64 DyCreepRight,RzCreepRight;
      GetCreepDeflection_NoDeck_TempStrands(poiLeft, pConfig,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepLeft,  &RzCreepLeft);
      GetCreepDeflection_NoDeck_TempStrands(poiRight,pConfig,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepRight, &RzCreepRight);
      // compute adjustment for the current poi
      DcreepSupport = ::LinInterp(poi.GetDistFromStart(),DyCreepLeft,DyCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RcreepSupport = ::LinInterp(poi.GetDistFromStart(),RzCreepLeft,RzCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());

      // get girder deflections during storage at the location of the final bearings
      Float64 D1, D2, D1E, D2E, R1, R2, R1E, R2E;
      Float64 DI1, DI2, RI1, RI2;
      GetGirderDeflectionForCamber(poiLeft, pConfig, &D1, &R1, &D1E, &R1E, &DI1, &RI1);
      GetGirderDeflectionForCamber(poiRight, pConfig, &D2, &R2, &D2E, &R2E, &DI2, &RI2);

      // compute adjustment for the current poi
      DgirderSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RgirderSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   
      // get prestress deflections during storage at the location of the final bearings
      if ( pConfig == nullptr )
      {
         GET_IFACE(IIntervals, pIntervals);
         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

         D1 = GetDeflection(storageIntervalIdx, pgsTypes::pftPretension, poiLeft, bat, rtCumulative, false);
         R1 = GetRotation(storageIntervalIdx, pgsTypes::pftPretension, poiLeft, bat, rtCumulative, false);

         D2 = GetDeflection(storageIntervalIdx, pgsTypes::pftPretension, poiRight, bat, rtCumulative, false);
         R2 = GetRotation(storageIntervalIdx, pgsTypes::pftPretension, poiRight, bat, rtCumulative, false);
      }
      else
      {
         Float64 DXharped, Dharped, Rharped;
         Float64 DXstraight, Dstraight, Rstraight;
         GetPrestressDeflectionFromModel(poiLeft, initModelData, pgsTypes::Harped,   datum, &DXharped,   &Dharped,   &Rharped);
         GetPrestressDeflectionFromModel(poiLeft, initModelData, pgsTypes::Straight, datum, &DXstraight, &Dstraight, &Rstraight);
         D1 = Dharped + Dstraight;
         R1 = Rharped + Rstraight;

         GetPrestressDeflectionFromModel(poiRight, initModelData, pgsTypes::Harped,   datum, &DXharped,   &Dharped,   &Rharped);
         GetPrestressDeflectionFromModel(poiRight, initModelData, pgsTypes::Straight, datum, &DXstraight, &Dstraight, &Rstraight);
         D2 = Dharped + Dstraight;
         R2 = Rharped + Rstraight;
      }
      // compute adjustment for the current poi
      DpsSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RpsSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // Creep1
      Float64 Ct1 = GetCreepCoefficient(segmentKey, cpReleaseToDiaphragm, constructionRate, pConfig);

      *pDy = Ct1*(DgStorage + Dps + Dtpsi);
      *pRz = Ct1*(RgStorage + Rps + Rtpsi);

      if ( datum == pgsTypes::pddErected )
      {
         // creep deflection after erection, relative to final bearings
         *pDy -= DcreepSupport;
         *pRz -= RcreepSupport;
      }
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      // Creep2
      Float64 Ct1, Ct2, Ct3;

      Ct1 = GetCreepCoefficient(segmentKey, cpReleaseToDiaphragm, constructionRate, pConfig);
      Ct2 = GetCreepCoefficient(segmentKey, cpDiaphragmToDeck, constructionRate, pConfig);
      Ct3 = GetCreepCoefficient(segmentKey, cpReleaseToDeck, constructionRate, pConfig);

      ATLASSERT(datum == pgsTypes::pddErected);
      *pDy = (Ct3 - Ct1)*(DgStorage + Dps) - (Ct3 - Ct1)*(DgirderSupport + DpsSupport) + Ct2*(DgInc + Ddiaphragm + Duser1 + Dtpsr + Dconstr + Dshearkey);
      *pRz = (Ct3 - Ct1)*(RgStorage + Rps) - (Ct3 - Ct1)*(RgirderSupport + RpsSupport) + Ct2*(RgInc + Rdiaphragm + Ruser1 + Rtpsr + Rconstr + Rshearkey);
   }
   else
   {
      //ATLASSERT(creepPeriod == cpDeckToFinal);
      // Creep3
      Float64 Ct1, Ct2, Ct3, Ct4, Ct5;

      Ct1 = GetCreepCoefficient(segmentKey, cpReleaseToDeck, constructionRate, pConfig);
      Ct2 = GetCreepCoefficient(segmentKey, cpReleaseToFinal, constructionRate, pConfig);
      Ct3 = GetCreepCoefficient(segmentKey, cpDiaphragmToDeck, constructionRate, pConfig);
      Ct4 = GetCreepCoefficient(segmentKey, cpDiaphragmToFinal, constructionRate, pConfig);
      Ct5 = GetCreepCoefficient(segmentKey, cpDeckToFinal, constructionRate, pConfig);

      ATLASSERT(datum == pgsTypes::pddErected);
      *pDy = (Ct2 - Ct1)*(DgStorage + Dps) - (Ct2 - Ct1)*(DgirderSupport + DpsSupport) + (Ct4 - Ct3)*(DgInc + Ddiaphragm + Dtpsr + Duser1 + Dconstr + Dshearkey) + Ct5*(Dbarrier + Duser2);
      *pRz = (Ct2 - Ct1)*(RgStorage + Rps) - (Ct2 - Ct1)*(RgirderSupport + RpsSupport) + (Ct4 - Ct3)*(RgInc + Rdiaphragm + Rtpsr + Ruser1 + Rconstr + Rshearkey) + Ct5*(Rbarrier + Ruser2);
   }
}

void CAnalysisAgentImp::GetCreepDeflection_NoDeck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,pgsTypes::PrestressDeflectionDatum datum,Float64* pDy,Float64* pRz ) const
{
   //ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck || creepPeriod == cpDeckToFinal);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 DXharped, Dharped, Rharped;
   GetPrestressDeflectionFromModel( poi, initModelData,     pgsTypes::Harped,    pgsTypes::pddStorage, &DXharped, &Dharped, &Rharped);

   Float64 DXstraight, Dstraight, Rstraight;
   GetPrestressDeflectionFromModel( poi, initModelData,     pgsTypes::Straight,    pgsTypes::pddStorage, &DXstraight, &Dstraight, &Rstraight);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 DgStorage, RgStorage;
   Float64 DgErected, RgErected;
   Float64 DgInc, RgInc;
   Float64 Ddiaphragm, Rdiaphragm;
   Float64 Dshearkey, Rshearkey;
   Float64 Dconstr, Rconstr;
   Float64 Duser1, Ruser1;
   Float64 Duser2, Ruser2;
   Float64 Dbarrier, Rbarrier;


   GET_IFACE(IIntervals,pIntervals);
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   std::vector<IntervalIndexType> vUserLoadIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey);
   ATLASSERT(vUserLoadIntervals.size() <= 2);
#endif

   IntervalIndexType userLoad1IntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();
   IntervalIndexType userLoad2IntervalIdx = pIntervals->GetCompositeUserLoadInterval();

#if defined _DEBUG
   std::vector<IntervalIndexType>::iterator iter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator end(vUserLoadIntervals.end());
   for ( ; iter != end; iter++ )
   {
      ATLASSERT(*iter == userLoad1IntervalIdx || *iter == userLoad2IntervalIdx);
   }
#endif

   // this eliminates all the duplicate methods
   GetGirderDeflectionForCamber(poi, pConfig, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc);
   GetDiaphragmDeflection(poi, pConfig, &Ddiaphragm, &Rdiaphragm);
   GetShearKeyDeflection(poi, pConfig, &Dshearkey, &Rshearkey);
   GetConstructionLoadDeflection(poi, pConfig, &Dconstr, &Rconstr);
   GetUserLoadDeflection(userLoad1IntervalIdx, poi, pConfig, &Duser1, &Ruser1);
   GetUserLoadDeflection(userLoad2IntervalIdx, poi, pConfig, &Duser2, &Ruser2);
   GetSlabBarrierOverlayDeflection(poi, pConfig, &Dbarrier, &Rbarrier);

   // To account for the fact that deflections are measured from different datums during storage
   // and after erection, we have to compute offsets that account for the translated coordinate systems.
   // These values adjust deformations that are measured relative to the storage supports so that
   // they are relative to the final bearing locations.
   Float64 DcreepSupport = 0; // adjusts deformation due to creep
   Float64 RcreepSupport = 0;
   Float64 DgirderSupport = 0; // adjusts deformation due to girder self weight
   Float64 RgirderSupport = 0;
   Float64 DpsSupport = 0; // adjusts deformation due to prestress
   Float64 RpsSupport = 0;
   if ( datum != pgsTypes::pddStorage )
   {
      // get POI at final bearing locations.... 
      // we want to deduct the deformation relative to the storage supports at these locations from the storage deformations
      // to make the deformation relative to the final bearings
      GET_IFACE(IPointOfInterest, pPoi);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 2);
      const pgsPointOfInterest& poiLeft(vPoi.front());
      const pgsPointOfInterest& poiRight(vPoi.back());
      ATLASSERT(poiLeft.IsTenthPoint(POI_ERECTED_SEGMENT) == 1);
      ATLASSERT(poiRight.IsTenthPoint(POI_ERECTED_SEGMENT) == 11);
      ATLASSERT(poiLeft.GetID() != INVALID_ID && poiRight.GetID() != INVALID_ID);

      // get creep deformations during storage at location of final bearings
      Float64 DyCreepLeft,RzCreepLeft;
      Float64 DyCreepRight,RzCreepRight;
      GetCreepDeflection_NoDeck(poiLeft, pConfig,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepLeft,  &RzCreepLeft);
      GetCreepDeflection_NoDeck(poiRight,pConfig,initModelData,initTempModelData,releaseTempModelData, cpReleaseToDiaphragm, constructionRate, pgsTypes::pddStorage, &DyCreepRight, &RzCreepRight);
      // compute adjustment for the current poi
      DcreepSupport = ::LinInterp(poi.GetDistFromStart(),DyCreepLeft,DyCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RcreepSupport = ::LinInterp(poi.GetDistFromStart(),RzCreepLeft,RzCreepRight,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());

      // get girder deflections during storage at the location of the final bearings
      Float64 D1, D2, D1E, D2E, R1, R2, R1E, R2E;
      Float64 DI1, DI2, RI1, RI2;
      GetGirderDeflectionForCamber(poiLeft, pConfig, &D1, &R1, &D1E, &R1E, &DI1, &RI1);
      GetGirderDeflectionForCamber(poiRight, pConfig, &D2, &R2, &D2E, &R2E, &DI2, &RI2);

      DgirderSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RgirderSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   
      // get prestress deflections during storage at the location of the final bearings
      if ( pConfig == nullptr )
      {
         GET_IFACE(IIntervals, pIntervals);
         IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
         pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

         D1 = GetDeflection(storageIntervalIdx, pgsTypes::pftPretension, poiLeft, bat, rtCumulative, false);
         R1 = GetRotation(storageIntervalIdx, pgsTypes::pftPretension, poiLeft, bat, rtCumulative, false);

         D2 = GetDeflection(storageIntervalIdx, pgsTypes::pftPretension, poiRight, bat, rtCumulative, false);
         R2 = GetRotation(storageIntervalIdx, pgsTypes::pftPretension, poiRight, bat, rtCumulative, false);
      }
      else
      {
         Float64 DXharped, Dharped, Rharped;
         Float64 DXstraight, Dstraight, Rstraight;
         GetPrestressDeflectionFromModel(poiLeft, initModelData, pgsTypes::Harped,   datum, &DXharped,   &Dharped,   &Rharped);
         GetPrestressDeflectionFromModel(poiLeft, initModelData, pgsTypes::Straight, datum, &DXstraight, &Dstraight, &Rstraight);
         D1 = Dharped + Dstraight;
         R1 = Rharped + Rstraight;

         GetPrestressDeflectionFromModel(poiRight, initModelData, pgsTypes::Harped,   datum, &DXharped,   &Dharped,   &Rharped);
         GetPrestressDeflectionFromModel(poiRight, initModelData, pgsTypes::Straight, datum, &DXstraight, &Dstraight, &Rstraight);
         D2 = Dharped + Dstraight;
         R2 = Rharped + Rstraight;
      }
      // compute adjustment for the current poi
      DpsSupport = ::LinInterp(poi.GetDistFromStart(),D1,D2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
      RpsSupport = ::LinInterp(poi.GetDistFromStart(),R1,R2,poiRight.GetDistFromStart() - poiLeft.GetDistFromStart());
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // Creep1
      Float64 Ct1 = GetCreepCoefficient(segmentKey, cpReleaseToDiaphragm, constructionRate, pConfig);

      *pDy = Ct1*(DgStorage + Dps);
      *pRz = Ct1*(RgStorage + Rps);

      if ( datum == pgsTypes::pddErected )
      {
         // creep deflection after erection, relative to final bearings
         *pDy -= DcreepSupport;
         *pRz -= RcreepSupport;
      }
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      // Creep2
      Float64 Ct1, Ct2, Ct3;

      Ct1 = GetCreepCoefficient(segmentKey, cpReleaseToDiaphragm, constructionRate, pConfig);
      Ct2 = GetCreepCoefficient(segmentKey, cpDiaphragmToDeck, constructionRate, pConfig);
      Ct3 = GetCreepCoefficient(segmentKey, cpReleaseToDeck, constructionRate, pConfig);

      ATLASSERT(datum == pgsTypes::pddErected);
      *pDy = (Ct3 - Ct1)*(DgStorage + Dps) - (Ct3 - Ct1)*(DgirderSupport + DpsSupport) + Ct2*(DgInc + Ddiaphragm + Duser1 + Dconstr + Dshearkey);
      *pRz = (Ct3 - Ct1)*(RgStorage + Rps) - (Ct3 - Ct1)*(RgirderSupport + RpsSupport) + Ct2*(RgInc + Rdiaphragm + Ruser1 + Rconstr + Rshearkey);
   }
   else
   {
      // Creep3
      Float64 Ct1, Ct2, Ct3, Ct4, Ct5;

      Ct1 = GetCreepCoefficient(segmentKey, cpReleaseToDeck, constructionRate, pConfig);
      Ct2 = GetCreepCoefficient(segmentKey, cpReleaseToFinal, constructionRate, pConfig);
      Ct3 = GetCreepCoefficient(segmentKey, cpDiaphragmToDeck, constructionRate, pConfig);
      Ct4 = GetCreepCoefficient(segmentKey, cpDiaphragmToFinal, constructionRate, pConfig);
      Ct5 = GetCreepCoefficient(segmentKey, cpDeckToFinal, constructionRate, pConfig);

      *pDy = (Ct2 - Ct1)*(DgStorage + Dps) - (Ct2 - Ct1)*(DgirderSupport + DpsSupport) + (Ct4 - Ct3)*(DgInc + Ddiaphragm + Duser1 + Dconstr + Dshearkey) + Ct5*(Dbarrier + Duser2);
      *pRz = (Ct2 - Ct1)*(RgStorage + Rps) - (Ct2 - Ct1)*(RgirderSupport + RpsSupport) + (Ct4 - Ct3)*(RgInc + Rdiaphragm + Ruser1 + Rconstr + Rshearkey) + Ct5*(Rbarrier + Ruser2);
   }
}

void CAnalysisAgentImp::GetExcessCamberEx2(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,bool applyFactors,Float64* pDy,Float64* pRz) const
{
   Float64 Dy, Dz;
   GetDCamberForGirderScheduleEx2( poi, pConfig, initModelData, initTempModelData, releaseTempModelData, time, applyFactors, &Dy, &Dz );

   Float64 Cy, Cz;
   GetScreedCamberEx(poi,pConfig,applyFactors,&Cy,&Cz);

   *pDy = Dy - Cy;  // excess camber = D - C
   *pRz = Dz - Cz;

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   if ( deckType == pgsTypes::sdtNone )
   {
      // apply camber multipliers
      CamberMultipliers cm = GetCamberMultipliersEx(poi.GetSegmentKey(), applyFactors);

      Float64 Dcreep3, Rcreep3;
      GetCreepDeflection(poi,ICamber::cpDeckToFinal,time,pgsTypes::pddErected,pConfig,&Dcreep3,&Rcreep3 );
      *pDy += cm.CreepFactor * Dcreep3;
      *pRz += cm.CreepFactor * Rcreep3;
   }
}

void CAnalysisAgentImp::GetDCamberForGirderScheduleEx2(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,bool applyFactors,Float64* pDy,Float64* pRz) const
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IStrandGeometry, pStrandGeom);
   bool bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey, pgsTypes::Temporary, pConfig) && pStrandGeom->GetTemporaryStrandUsage(segmentKey, pConfig) != pgsTypes::ttsPTBeforeShipping);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   Float64 D, R;

   switch( deckType )
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeOverlay:
   case pgsTypes::sdtCompositeSIP:
   case pgsTypes::sdtNonstructuralOverlay:
      (bTempStrands ? GetD_Deck_TempStrands(poi,pConfig, initModelData,initTempModelData,releaseTempModelData,time,applyFactors,&D,&R) :
                      GetD_Deck(poi, pConfig, initModelData,initTempModelData,releaseTempModelData,time,applyFactors,&D,&R));
      break;

   case pgsTypes::sdtNone:
      (bTempStrands ? GetD_NoDeck_TempStrands(poi, pConfig, initModelData,initTempModelData,releaseTempModelData,time,applyFactors,&D,&R) :
                      GetD_NoDeck(poi, pConfig, initModelData,initTempModelData,releaseTempModelData,time,applyFactors,&D,&R));
      break;

   default:
      ATLASSERT(false);
   }

   *pDy = D;
   *pRz = R;
}

void CAnalysisAgentImp::GetD_Deck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const
{
   Float64 Dps, Dtpsi, Dtpsr, DgStorage, DgErected, DgInc, Dcreep1, Ddiaphragm, Dcreep2, Dshearkey, Dconstr;
   Float64 Rps, Rtpsi, Rtpsr, RgStorage, RgErected, RgInc, Rcreep1, Rdiaphragm, Rcreep2, Rshearkey, Rconstr;

   Float64 Dx, DXtpsi, DXtpsr;
   GetPermanentPrestressDeflection(poi, pgsTypes::pddErected, pConfig, &Dx,&Dps, &Rps);
   GetInitialTempPrestressDeflection(poi, pgsTypes::pddErected, pConfig, &DXtpsi, &Dtpsi, &Rtpsi);
   GetReleaseTempPrestressDeflection(poi, pConfig, &DXtpsr, &Dtpsr, &Rtpsr);
   GetGirderDeflectionForCamber(poi, pConfig, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc);
   if (pConfig)
   {
      GetCreepDeflection(poi, pConfig, initModelData, initTempModelData, releaseTempModelData, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected, &Dcreep1, &Rcreep1);
   }
   else
   {
      GetCreepDeflection(poi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected, pConfig, &Dcreep1, &Rcreep1);
   }

   GetDiaphragmDeflection(poi, pConfig, &Ddiaphragm, &Rdiaphragm);
   GetShearKeyDeflection(poi, pConfig, &Dshearkey, &Rshearkey);
   GetConstructionLoadDeflection(poi, pConfig, &Dconstr, &Rconstr);
   if (pConfig)
   {
      GetCreepDeflection(poi, pConfig, initModelData, initTempModelData, releaseTempModelData, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected, &Dcreep2, &Rcreep2);
   }
   else
   {
      GetCreepDeflection(poi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected, pConfig, &Dcreep2, &Rcreep2);
   }

   // apply camber multipliers
   CamberMultipliers cm = GetCamberMultipliersEx(poi.GetSegmentKey(), applyFactors);

   Float64 D1 = cm.ErectionFactor*(DgErected + Dps + Dtpsi);
   Float64 D2 = D1 + cm.CreepFactor*Dcreep1;
   Float64 D3 = D2 + cm.DiaphragmFactor*(Ddiaphragm + Dshearkey + Dconstr) + cm.ErectionFactor*Dtpsr;
   Float64 D4 = D3 + cm.CreepFactor*Dcreep2;
   *pDy = D4;

   Float64 R1 = cm.ErectionFactor*(RgErected + Rps + Rtpsi);
   Float64 R2 = R1 + cm.CreepFactor*Rcreep1;
   Float64 R3 = R2 + cm.DiaphragmFactor*(Rdiaphragm + Rshearkey + Rconstr) +  cm.ErectionFactor*Rtpsr;
   Float64 R4 = R3 + cm.CreepFactor*Rcreep2;

   *pRz = R4;
}

void CAnalysisAgentImp::GetD_Deck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const
{
   Float64 Dps, DgStorage, DgErected, DgInc, Dcreep, Ddiaphragm, Dshearkey, Dconstr;
   Float64 Rps, RgStorage, RgErected, RgInc, Rcreep, Rdiaphragm, Rshearkey, Rconstr;

   Float64 Dx;
   GetPermanentPrestressDeflection(poi, pgsTypes::pddErected, pConfig, &Dx, &Dps, &Rps);
   GetGirderDeflectionForCamber(poi, pConfig, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc);
   GetDiaphragmDeflection(poi, pConfig, &Ddiaphragm, &Rdiaphragm);
   GetShearKeyDeflection(poi, pConfig, &Dshearkey, &Rshearkey);
   GetConstructionLoadDeflection(poi, pConfig, &Dconstr, &Rconstr);
   if (pConfig)
   {
      GetCreepDeflection(poi, pConfig, initModelData, initTempModelData, releaseTempModelData, ICamber::cpReleaseToDeck, constructionRate, pgsTypes::pddErected, &Dcreep, &Rcreep);
   }
   else
   {
      GetCreepDeflection(poi, ICamber::cpReleaseToDeck, constructionRate, pgsTypes::pddErected, pConfig, &Dcreep, &Rcreep);
   }

   // apply camber multipliers
   CamberMultipliers cm = GetCamberMultipliersEx(poi.GetSegmentKey(), applyFactors);

   *pDy = cm.ErectionFactor*(DgErected + Dps) + cm.CreepFactor*Dcreep + cm.DiaphragmFactor*(Ddiaphragm + Dshearkey + Dconstr) ;
   *pRz = cm.ErectionFactor*(RgErected + Rps) + cm.CreepFactor*Rcreep + cm.DiaphragmFactor*(Rdiaphragm + Rshearkey + Rconstr);
}

void CAnalysisAgentImp::GetD_NoDeck_TempStrands(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const
{
   // Interpert "D" as deflection before application of superimposed dead loads
   Float64 Dps, Dtpsi, Dtpsr, DgStorage, DgErected, DgInc, Dcreep1, Ddiaphragm, Dshearkey, Dcreep2, Dconstr, Duser1;
   Float64 Rps, Rtpsi, Rtpsr, RgStorage, RgErected, RgInc, Rcreep1, Rdiaphragm, Rshearkey, Rcreep2, Rconstr, Ruser1;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   std::vector<IntervalIndexType> vUserLoadIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey);
   ATLASSERT(vUserLoadIntervals.size() <= 2);
#endif

   IntervalIndexType nonCompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();

#if defined _DEBUG
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();
   std::vector<IntervalIndexType>::iterator iter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator end(vUserLoadIntervals.end());
   for ( ; iter != end; iter++ )
   {
      ATLASSERT(*iter == nonCompositeUserLoadIntervalIdx || *iter == compositeUserLoadIntervalIdx);
   }
#endif

   Float64 Dx;
   GetPermanentPrestressDeflection(poi, pgsTypes::pddErected, pConfig, &Dx, &Dps, &Rps);
   GetInitialTempPrestressDeflection(poi, pgsTypes::pddErected, pConfig, &Dx, &Dtpsi, &Rtpsi);
   GetReleaseTempPrestressDeflection(poi, pConfig, &Dx, &Dtpsr, &Rtpsr);
   GetGirderDeflectionForCamber(poi, pConfig, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc);
   GetCreepDeflection(poi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected, pConfig, &Dcreep1, &Rcreep1);
   GetDiaphragmDeflection(poi, pConfig, &Ddiaphragm, &Rdiaphragm);
   GetShearKeyDeflection(poi, pConfig, &Dshearkey, &Rshearkey);
   GetConstructionLoadDeflection(poi, pConfig, &Dconstr, &Rconstr);
   GetUserLoadDeflection(nonCompositeUserLoadIntervalIdx, poi, pConfig, &Duser1, &Ruser1);
   GetCreepDeflection(poi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected, pConfig, &Dcreep2, &Rcreep2);

   // apply camber multipliers
   CamberMultipliers cm = GetCamberMultipliersEx(poi.GetSegmentKey(), applyFactors);

   Float64 D1 = cm.ErectionFactor*(DgErected + Dps + Dtpsi);
   Float64 D2 = D1 + cm.CreepFactor*Dcreep1;
   Float64 D3 = D2 + cm.DiaphragmFactor*(Ddiaphragm + Dshearkey + Dconstr) + cm.ErectionFactor*Dtpsr + cm.SlabUser1Factor*Duser1;
   Float64 D4 = D3 + cm.CreepFactor*Dcreep2;
   *pDy = D4;

   Float64 R1 = cm.ErectionFactor*(RgErected + Rps + Rtpsi);
   Float64 R2 = R1 + cm.CreepFactor*Rcreep1;
   Float64 R3 = R2 + cm.DiaphragmFactor*(Rdiaphragm + Rshearkey + Rconstr) + cm.ErectionFactor*Rtpsr + cm.SlabUser1Factor*Ruser1;
   Float64 R4 = R3 + cm.CreepFactor*Rcreep2;
   *pRz = R4;
}

void CAnalysisAgentImp::GetD_NoDeck(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,bool applyFactors,Float64* pDy,Float64* pRz) const
{
   // Interpert "D" as deflection before application of superimposed dead loads
   Float64 Dps, DgStorage, DgErected, DgInc, Dcreep1, Ddiaphragm, Dshearkey, Dcreep2, Dconstr, Duser1;
   Float64 Rps, RgStorage, RgErected, RgInc, Rcreep1, Rdiaphragm, Rshearkey, Rcreep2, Rconstr, Ruser1;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);

   std::vector<IntervalIndexType> vUserLoadIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey);
   ATLASSERT(vUserLoadIntervals.size() <= 2);
#endif

   IntervalIndexType nonCompositeUserLoadIntervalIdx = pIntervals->GetNoncompositeUserLoadInterval();

#if defined _DEBUG
   IntervalIndexType compositeUserLoadIntervalIdx = pIntervals->GetCompositeUserLoadInterval();
   std::vector<IntervalIndexType>::iterator iter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator end(vUserLoadIntervals.end());
   for ( ; iter != end; iter++ )
   {
      ATLASSERT(*iter == nonCompositeUserLoadIntervalIdx || *iter == compositeUserLoadIntervalIdx);
   }
#endif

   Float64 Dx;
   GetPermanentPrestressDeflection(poi, pgsTypes::pddErected, pConfig, &Dx, &Dps, &Rps);
   GetGirderDeflectionForCamber(poi, pConfig, &DgStorage, &RgStorage, &DgErected, &RgErected, &DgInc, &RgInc);
   GetCreepDeflection(poi, ICamber::cpReleaseToDiaphragm, constructionRate, pgsTypes::pddErected, pConfig, &Dcreep1, &Rcreep1);
   GetDiaphragmDeflection(poi, pConfig, &Ddiaphragm, &Rdiaphragm);
   GetShearKeyDeflection(poi, pConfig, &Dshearkey, &Rshearkey);
   GetConstructionLoadDeflection(poi, pConfig, &Dconstr, &Rconstr);
   GetUserLoadDeflection(nonCompositeUserLoadIntervalIdx, poi, pConfig, &Duser1, &Ruser1);
   GetCreepDeflection(poi, ICamber::cpDiaphragmToDeck, constructionRate, pgsTypes::pddErected, pConfig, &Dcreep2, &Rcreep2);

   // apply camber multipliers
   CamberMultipliers cm = GetCamberMultipliersEx(poi.GetSegmentKey(), applyFactors);

   Float64 D1 = cm.ErectionFactor*(DgErected + Dps);
   Float64 D2 = D1 + cm.CreepFactor*Dcreep1;
   Float64 D3 = D2 + cm.DiaphragmFactor*(Ddiaphragm + Dshearkey + Dconstr) + cm.SlabUser1Factor*Duser1;
   Float64 D4 = D3 + cm.CreepFactor*Dcreep2;
   *pDy = D4;

   Float64 R1 = cm.ErectionFactor*(RgErected + Rps);
   Float64 R2 = R1 + cm.CreepFactor*Rcreep1;
   Float64 R3 = R2 + cm.DiaphragmFactor*(Rdiaphragm + Rshearkey + Rconstr) + cm.SlabUser1Factor*Duser1;
   Float64 R4 = R3 + cm.CreepFactor*Rcreep2;
   *pRz = R4;
}

void CAnalysisAgentImp::GetDesignSlabDeflectionAdjustment(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const
{
   GetDesignDeflectionAdjustment(g_lcidDesignSlab, poi, pConfig, pDy, pRz);
}

void CAnalysisAgentImp::GetDesignSlabPadDeflectionAdjustment(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,Float64* pDy,Float64* pRz) const
{
   GetDesignDeflectionAdjustment(g_lcidDesignSlabPad, poi, pConfig, pDy, pRz);
}


Float64 CAnalysisAgentImp::GetConcreteStrengthAtTimeOfLoading(const CSegmentKey& segmentKey, LoadingEvent le) const
{
   GET_IFACE(IMaterials, pMaterial);
   GET_IFACE(IIntervals, pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   Float64 Fc;

   switch (le)
   {
   case ICamber::leRelease:
      Fc = pMaterial->GetSegmentFc(segmentKey, releaseIntervalIdx);
      break;

   case ICamber::leDiaphragm:
   case ICamber::leDeck:
      Fc = pMaterial->GetSegmentFc(segmentKey, castDeckIntervalIdx);
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return Fc;
}

Float64 CAnalysisAgentImp::GetConcreteStrengthAtTimeOfLoading(const GDRCONFIG& config, LoadingEvent le) const
{
   Float64 Fc;

   switch (le)
   {
   case ICamber::leRelease:
   case ICamber::leDiaphragm:
   case ICamber::leDeck:
      Fc = config.Fci;
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return Fc;
}

ICamber::LoadingEvent CAnalysisAgentImp::GetLoadingEvent(CreepPeriod creepPeriod) const
{
   LoadingEvent le;
   switch( creepPeriod )
   {
   case cpReleaseToDiaphragm:
   case cpReleaseToDeck:
   case cpReleaseToFinal:
      le = leRelease;
      break;

   case cpDiaphragmToDeck:
   case cpDiaphragmToFinal:
      le = leDiaphragm;
      break;

   case cpDeckToFinal:
      le = leDeck;
      break;

   default:
      ATLASSERT(false);
   }

   return le;
}

/////////////////////////////////////////////////////////////////////////
// IContraflexurePoints
void CAnalysisAgentImp::GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints) const
{
   m_pGirderModelManager->GetContraflexurePoints(spanKey,cfPoints,nPoints);
}

/////////////////////////////////////////////////////////////////////////
// IContinuity
bool CAnalysisAgentImp::IsContinuityFullyEffective(const CGirderKey& girderKey) const
{
   // The continuity of a girder line is fully effective if the continuity stress level
   // at all continuity piers is compressive.
   //
   // Continuity is fully effective at piers where a precast segment spans
   // across the pier.
   //
   // Check for continuity at the start and end of each girder group (that is where the continuity diaphragms are located)
   // If the continuity is not effective at any continuity diaphragm, the contininuity is not effective for the full girder line

   bool bContinuous = false;

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);

      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);

      for (int i = 0; i < 2; i++ )
      {
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)i;
         PierIndexType pierIdx = pGroup->GetPierIndex(endType);

         bool bContinuousLeft, bContinuousRight;
         pBridge->IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);

         bool bIntegralLeft, bIntegralRight;
         pBridge->IsIntegralAtPier(pierIdx,&bIntegralLeft,&bIntegralRight);

         if ( (bContinuousLeft && bContinuousRight) || (bIntegralLeft && bIntegralRight)  )
         {
            Float64 fb = GetContinuityStressLevel(pierIdx,thisGirderKey);
            bContinuous = (0 <= fb ? false : true);
         }
         else
         {
            bContinuous = false;
         }

         if ( bContinuous )
         {
            return bContinuous;
         }
      }
   }

   return bContinuous;
}

Float64 CAnalysisAgentImp::GetContinuityStressLevel(PierIndexType pierIdx,const CGirderKey& girderKey) const
{
   ATLASSERT(girderKey.girderIndex != INVALID_INDEX);

   // for evaluation of LRFD 5.12.3.3.5 (pre2017: 5.14.1.4.5) - Degree of continuity

   // If we are in simple span analysis mode, there is no continuity
   // no matter what the boundary conditions are
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   if ( analysisType == pgsTypes::Simple )
   {
      return 0.0;
   }

   // check the boundary conditions
   GET_IFACE(IBridge,pBridge);
   bool bIntegralLeft,bIntegralRight;
   pBridge->IsIntegralAtPier(pierIdx,&bIntegralLeft,&bIntegralRight);
   bool bContinuousLeft,bContinuousRight;
   pBridge->IsContinuousAtPier(pierIdx,&bContinuousLeft,&bContinuousRight);

   if ( !bIntegralLeft && !bIntegralRight && !bContinuousLeft && !bContinuousRight )
   {
      return 0.0;
   }

   // how does this work for segments that span over a pier? I don't
   // think that 5.12.3.3.5 (pre2017: 5.14.1.4.5) applies to spliced girder bridges unless we are looking
   // at a pier that is between groups
   ATLASSERT(pBridge->IsBoundaryPier(pierIdx));

   GroupIndexType backGroupIdx, aheadGroupIdx;
   pBridge->GetGirderGroupIndex(pierIdx,&backGroupIdx,&aheadGroupIdx);

#if defined _DEBUG
   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPierData2* pPier = pIBridgeDesc->GetPier(pierIdx);
   if ( pPier->HasCantilever() )
   {
      if ( pPier->GetIndex() == 0 )
      {
         ATLASSERT(backGroupIdx == INVALID_INDEX);
         ATLASSERT(aheadGroupIdx == 0);
      }
      else
      {
         ATLASSERT(pPier->GetIndex() == pIBridgeDesc->GetPierCount()-1);
         ATLASSERT(backGroupIdx == pIBridgeDesc->GetGirderGroupCount()-1);
         ATLASSERT(aheadGroupIdx == INVALID_INDEX);
      }
   }
   else
   {
      if ( pPier->GetIndex() == 0 )
      {
         ATLASSERT(backGroupIdx == INVALID_INDEX);
         ATLASSERT(aheadGroupIdx == 0);
      }
      else if ( pPier->GetIndex() == pIBridgeDesc->GetPierCount()-1 )
      {
         ATLASSERT(backGroupIdx == pIBridgeDesc->GetGirderGroupCount()-1);
         ATLASSERT(aheadGroupIdx == INVALID_INDEX);
      }
      else
      {
         ATLASSERT(backGroupIdx == aheadGroupIdx-1);
      }
   }
#endif

   GirderIndexType gdrIdx = girderKey.girderIndex;

   // computes the stress at the bottom of the girder on each side of the pier
   // returns the greater of the two values
   GET_IFACE(IPointOfInterest,pPoi);

   // deal with girder index when there are different number of girders in each group
   GirderIndexType prev_group_gdr_idx = gdrIdx;
   GirderIndexType next_group_gdr_idx = gdrIdx;

   if ( backGroupIdx != INVALID_INDEX )
   {
      prev_group_gdr_idx = Min(gdrIdx,pBridge->GetGirderCount(backGroupIdx)-1);
   }

   if ( aheadGroupIdx != INVALID_INDEX )
   {
      next_group_gdr_idx = Min(gdrIdx,pBridge->GetGirderCount(aheadGroupIdx)-1);
   }

   CollectionIndexType nPOI = 0;
   pgsPointOfInterest vPOI[2];

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType constructionLoadIntervalIdx = pIntervals->GetConstructionLoadInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   IntervalIndexType continuity_interval[2];

   IntervalIndexType leftContinuityIntervalIndex, rightContinuityIntervalIndex;
   pIntervals->GetContinuityInterval(pierIdx,&leftContinuityIntervalIndex,&rightContinuityIntervalIndex);


   // get poi at cl bearing at end of prev group
   if ( backGroupIdx != INVALID_INDEX )
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(backGroupIdx,prev_group_gdr_idx);
      CSegmentKey thisSegmentKey(backGroupIdx,prev_group_gdr_idx,nSegments-1);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(thisSegmentKey, POI_10L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      vPOI[nPOI] = vPoi.front();
      continuity_interval[nPOI] = leftContinuityIntervalIndex;
      nPOI++;
   }

   // get poi at cl bearing at start of next group
   if ( aheadGroupIdx != INVALID_INDEX )
   {
      CSegmentKey thisSegmentKey(aheadGroupIdx,next_group_gdr_idx,0);
      PoiList vPoi;
      pPoi->GetPointsOfInterest(thisSegmentKey, POI_0L | POI_ERECTED_SEGMENT, &vPoi);
      ATLASSERT(vPoi.size() == 1);
      vPOI[nPOI] = vPoi.front();
      continuity_interval[nPOI] = rightContinuityIntervalIndex;
      nPOI++;
   }

   Float64 f[2] = {0,0};
   for ( CollectionIndexType i = 0; i < nPOI; i++ )
   {
      pgsPointOfInterest& poi = vPOI[i];
      ATLASSERT( 0 <= poi.GetID() );

      pgsTypes::BridgeAnalysisType bat = pgsTypes::ContinuousSpan;

      Float64 fbConstruction, fbSlab, fbSlabPad, fbTrafficBarrier, fbSidewalk, fbOverlay, fbUserDC, fbUserDW, fbUserLLIM, fbLLIM;

      Float64 fTop,fBottom;

      if ( continuity_interval[i] == castDeckIntervalIdx && castDeckIntervalIdx != INVALID_INDEX)
      {
         GetStress(castDeckIntervalIdx,pgsTypes::pftSlab,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbSlab = fBottom;

         GetStress(castDeckIntervalIdx,pgsTypes::pftSlabPad,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbSlabPad = fBottom;

      }
      else
      {
         fbSlab         = 0;
         fbSlabPad      = 0;
      }

      if (constructionLoadIntervalIdx == INVALID_INDEX)
      {
         fbConstruction = 0;
      }
      else
      {
         GetStress(constructionLoadIntervalIdx, pgsTypes::pftConstruction, poi, bat, rtIncremental, pgsTypes::TopGirder, pgsTypes::BottomGirder, &fTop, &fBottom);
         fbConstruction = fBottom;
      }

      GetStress(railingSystemIntervalIdx,pgsTypes::pftTrafficBarrier,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbTrafficBarrier = fBottom;

      GetStress(railingSystemIntervalIdx,pgsTypes::pftSidewalk,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbSidewalk = fBottom;

      if ( overlayIntervalIdx == INVALID_INDEX )
      {
         fbOverlay = 0;
      }
      else
      {
         GetStress(overlayIntervalIdx, pgsTypes::pftOverlay, poi, bat, rtIncremental, pgsTypes::TopGirder, pgsTypes::BottomGirder, &fTop, &fBottom);
         fbOverlay = fBottom;
      }

      fbUserDC = 0;
      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
      std::vector<IntervalIndexType> vUserDCIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey,pgsTypes::pftUserDC);
      for( const auto& intervalIdx : vUserDCIntervals)
      {
         GetStress(intervalIdx,pgsTypes::pftUserDC,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbUserDC += fBottom;
      }

      fbUserDW = 0;
      std::vector<IntervalIndexType> vUserDWIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey,pgsTypes::pftUserDW);
      for(const auto& intervalIdx : vUserDWIntervals)
      {
         GetStress(intervalIdx,pgsTypes::pftUserDW,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbUserDW += fBottom;
      }

      GetStress(liveLoadIntervalIdx,pgsTypes::pftUserLLIM,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbUserLLIM = fBottom;

      Float64 fTopMin,fTopMax,fBotMin,fBotMax;
      GetCombinedLiveLoadStress(liveLoadIntervalIdx,pgsTypes::lltDesign,poi,bat,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTopMin,&fTopMax,&fBotMin,&fBotMax);
      fbLLIM = fBotMin; // greatest compression

      fBottom = fbConstruction + fbSlab + fbSlabPad + fbTrafficBarrier + fbSidewalk + fbOverlay + fbUserDC + fbUserDW + 0.5*(fbUserLLIM + fbLLIM);

      f[i] = fBottom;
   }

   return (nPOI == 1 ? f[0] : Max(f[0],f[1]));
}

/////////////////////////////////////////////////
// IPrecompressedTensileZone
void CAnalysisAgentImp::IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation,bool* pbTopPTZ,bool* pbBotPTZ) const
{
   IsInPrecompressedTensileZone(poi,limitState,topStressLocation,botStressLocation,nullptr,pbTopPTZ,pbBotPTZ);
}

void CAnalysisAgentImp::IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation topStressLocation,pgsTypes::StressLocation botStressLocation,const GDRCONFIG* pConfig,bool* pbTopPTZ,bool* pbBotPTZ) const
{
   bool bTopGirder = IsGirderStressLocation(topStressLocation);
   bool bBotGirder = IsGirderStressLocation(botStressLocation);
   if ( bTopGirder && bBotGirder )
   {
      IsGirderInPrecompressedTensileZone(poi,limitState,pConfig,pbTopPTZ,pbBotPTZ);
   }
   else if ( !bTopGirder && !bBotGirder )
   {
      IsDeckInPrecompressedTensileZone(poi,limitState,pbTopPTZ,pbBotPTZ);
   }
   else
   {
      bool bDummy;
      if ( bTopGirder )
      {
         IsGirderInPrecompressedTensileZone(poi,limitState,pConfig,pbTopPTZ,&bDummy);
      }
      else
      {
         IsDeckInPrecompressedTensileZone(poi,limitState,pbTopPTZ,&bDummy);
      }

      if ( bBotGirder )
      {
         IsGirderInPrecompressedTensileZone(poi,limitState,pConfig,&bDummy,pbBotPTZ);
      }
      else
      {
         IsDeckInPrecompressedTensileZone(poi,limitState,&bDummy,pbBotPTZ);
      }
   }
}

bool CAnalysisAgentImp::IsDeckPrecompressed(const CGirderKey& girderKey) const
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   if ( compositeDeckIntervalIdx == INVALID_INDEX )
   {
      return false; // this happens when there is not a deck
   }

   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
   {
      IntervalIndexType stressTendonIntervalIdx = pIntervals->GetStressTendonInterval(girderKey,ductIdx);
      if ( compositeDeckIntervalIdx <= stressTendonIntervalIdx )
      {
         // this tendon is stressed after the deck is composite so the deck is considered precompressed
         return true;
      }
   }

   // didn't find a tendon that is stressed after the deck is composite... deck not precompressed
   return false;
}

/////////////////////////////////////////////////
// IReactions
void CAnalysisAgentImp::GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const
{
   std::vector<CSegmentKey> segmentKeys;
   segmentKeys.push_back(segmentKey);
   std::vector<Float64> Rleft, Rright;
   GetSegmentReactions(segmentKeys,intervalIdx,pfType,bat,resultsType,&Rleft,&Rright);

   ATLASSERT(Rleft.size()  == 1);
   ATLASSERT(Rright.size() == 1);

   *pRleft  = Rleft.front();
   *pRright = Rright.front();
}

void CAnalysisAgentImp::GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const
{
   pRleft->reserve(segmentKeys.size());
   pRright->reserve(segmentKeys.size());

   GET_IFACE(IIntervals,pIntervals);
   std::vector<CSegmentKey>::const_iterator segKeyIter(segmentKeys.begin());
   std::vector<CSegmentKey>::const_iterator segKeyIterEnd(segmentKeys.end());
   for ( ; segKeyIter != segKeyIterEnd; segKeyIter++ )
   {
      const CSegmentKey& segmentKey = *segKeyIter;

      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      Float64 Rleft(0), Rright(0);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetReaction(segmentKey,intervalIdx,pfType,resultsType,&Rleft,&Rright);
      }
      pRleft->push_back(Rleft);
      pRright->push_back(Rright);
   }
}

void CAnalysisAgentImp::GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright) const
{
   std::vector<CSegmentKey> segmentKeys;
   segmentKeys.push_back(segmentKey);
   std::vector<Float64> Rleft, Rright;
   GetSegmentReactions(segmentKeys,intervalIdx,comboType,bat,resultsType,&Rleft,&Rright);

   ATLASSERT(Rleft.size()  == 1);
   ATLASSERT(Rright.size() == 1);

   *pRleft  = Rleft.front();
   *pRright = Rright.front();
}

void CAnalysisAgentImp::GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright) const
{
   pRleft->reserve(segmentKeys.size());
   pRright->reserve(segmentKeys.size());

   GET_IFACE(IIntervals,pIntervals);
   std::vector<CSegmentKey>::const_iterator segKeyIter(segmentKeys.begin());
   std::vector<CSegmentKey>::const_iterator segKeyIterEnd(segmentKeys.end());
   for ( ; segKeyIter != segKeyIterEnd; segKeyIter++ )
   {
      const CSegmentKey& segmentKey = *segKeyIter;

      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      Float64 Rleft(0), Rright(0);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetReaction(segmentKey,intervalIdx,comboType,resultsType,&Rleft,&Rright);
      }
      pRleft->push_back(Rleft);
      pRright->push_back(Rright);
   }
}

void CAnalysisAgentImp::GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,Float64* pRleftMin,Float64* pRleftMax,Float64* pRrightMin,Float64* pRrightMax) const
{
   std::vector<CSegmentKey> segmentKeys;
   segmentKeys.push_back(segmentKey);
   std::vector<Float64> RleftMin,RleftMax,RrightMin,RrightMax;
   GetSegmentReactions(segmentKeys,intervalIdx,limitState,bat,&RleftMin,&RleftMax,&RrightMin,&RrightMax);
   *pRleftMin  = RleftMin.front();
   *pRleftMax  = RleftMax.front();
   *pRrightMin = RrightMin.front();
   *pRrightMax = RrightMax.front();
}

void CAnalysisAgentImp::GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRleftMin,std::vector<Float64>* pRleftMax,std::vector<Float64>* pRrightMin,std::vector<Float64>* pRrightMax) const
{
   pRleftMin->reserve(segmentKeys.size());
   pRleftMax->reserve(segmentKeys.size());
   pRrightMin->reserve(segmentKeys.size());
   pRrightMax->reserve(segmentKeys.size());

   GET_IFACE(IIntervals,pIntervals);
   std::vector<CSegmentKey>::const_iterator segKeyIter(segmentKeys.begin());
   std::vector<CSegmentKey>::const_iterator segKeyIterEnd(segmentKeys.end());
   for ( ; segKeyIter != segKeyIterEnd; segKeyIter++ )
   {
      const CSegmentKey& segmentKey = *segKeyIter;

      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      Float64 RleftMin(0), RleftMax(0), RrightMin(0), RrightMax(0);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetReaction(segmentKey,intervalIdx,limitState,&RleftMin,&RleftMax,&RrightMin,&RrightMax);
      }

      pRleftMin->push_back(RleftMin);
      pRleftMax->push_back(RleftMax);
      pRrightMin->push_back(RrightMin);
      pRrightMax->push_back(RrightMax);
   }
}

REACTION CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   vSupports.emplace_back(supportIdx, supportType);
   std::vector<REACTION> reactions(GetReaction(girderKey,vSupports,intervalIdx,pfType,bat,resultsType));

   ATLASSERT(reactions.size() == 1);

   return reactions.front();
}

std::vector<REACTION> CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   if ( pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      std::vector<REACTION> results;
      GET_IFACE(ILossParameters,pLossParameters);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         GET_IFACE_NOCHECK(ILosses,pLosses);
         ComputeTimeDependentEffects(girderKey,intervalIdx);
         if ( resultsType == rtCumulative )
         {
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
            results.resize(vSupports.size());
            for ( IntervalIndexType iIdx = erectionIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               GET_IFACE(IIntervals,pIntervals);
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,pfType - pgsTypes::pftCreep);
                  std::vector<REACTION> vReactions = GetReaction(girderKey,vSupports,iIdx,strLoadingName,bat,rtIncremental);
                  std::transform(results.cbegin(),results.cend(),vReactions.cbegin(),results.begin(),[](const auto& a, const auto& b) {return a + b;});
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,pfType - pgsTypes::pftCreep);
               results = GetReaction(girderKey,vSupports,intervalIdx,strLoadingName,bat,rtIncremental);
            }
            else
            {
               results.resize(vSupports.size());
            }
         }
      }
      else
      {
         results.resize(vSupports.size());
      }
      return results;
   }
   else
   {
      if (pfType == pgsTypes::pftPretension)
      {
         // pretensioning doesn't cause reactions
         std::vector<REACTION> results;
         results.resize(vSupports.size());
         return results;
      }
      else
      {
         return m_pGirderModelManager->GetReaction(girderKey, vSupports, intervalIdx, pfType, bat, resultsType);
      }
   }
}

REACTION CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   vSupports.emplace_back(supportIdx, supportType);
   std::vector<REACTION> reactions(GetReaction(girderKey,vSupports,intervalIdx,comboType,bat,resultsType));

   ATLASSERT(reactions.size() == 1);

   return reactions.front();
}

std::vector<REACTION> CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      std::vector<REACTION> results;
      GET_IFACE(ILossParameters,pLossParameters);
      if ( pLossParameters->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         ComputeTimeDependentEffects(girderKey,intervalIdx);
         if ( resultsType == rtCumulative )
         {
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
            results.resize(vSupports.size());
            for ( IntervalIndexType iIdx = erectionIntervalIdx; iIdx <= intervalIdx; iIdx++ )
            {
               if ( 0 < pIntervals->GetDuration(iIdx) )
               {
                  GET_IFACE(ILosses,pLosses);
                  CString strLoadingName = pLosses->GetRestrainingLoadName(iIdx,comboType - lcCR);
                  std::vector<REACTION> vReactions = GetReaction(girderKey,vSupports,intervalIdx,strLoadingName,bat,rtIncremental);
                  std::transform(results.cbegin(),results.cend(),vReactions.cbegin(),results.begin(),[](const auto& a, const auto& b) {return a + b;});
               }
            }
         }
         else
         {
            GET_IFACE(IIntervals,pIntervals);
            if ( 0 < pIntervals->GetDuration(intervalIdx) )
            {
               GET_IFACE(ILosses,pLosses);
               CString strLoadingName = pLosses->GetRestrainingLoadName(intervalIdx,comboType - lcCR);
               results = GetReaction(girderKey,vSupports,intervalIdx,strLoadingName,bat,rtIncremental);
            }
            else
            {
               results.resize(vSupports.size());
            }
         }
      }
      else
      {
         results.resize(vSupports.size());
      }
      return results;
   }

   if ( comboType == lcPS )
   {
      // secondary effects were requested... the LBAM doesn't have secondary effects... get the product load
      // effects that feed into lcPS
      std::vector<REACTION> reactions(vSupports.size());
      std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      std::vector<pgsTypes::ProductForceType>::iterator pfIter(pfTypes.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfIterEnd(pfTypes.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         pgsTypes::ProductForceType pfType = *pfIter;
         std::vector<REACTION> reaction = GetReaction(girderKey,vSupports,intervalIdx,pfType,bat,resultsType);
         std::transform(reactions.cbegin(),reactions.cend(),reaction.cbegin(),reactions.begin(),[](const auto& a, const auto& b) {return a + b;});
      }
      return reactions;
   }
   else
   {
      return m_pGirderModelManager->GetReaction(girderKey,vSupports,intervalIdx,comboType,bat,resultsType);
   }
}

void CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,REACTION* pRmin,REACTION* pRmax) const
{
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   vSupports.emplace_back(supportIdx, supportType);
   std::vector<REACTION> Rmin, Rmax;
   GetReaction(girderKey,vSupports,intervalIdx,limitState,bat,bIncludeImpact,&Rmin,&Rmax);

   ATLASSERT(Rmin.size() == 1);
   ATLASSERT(Rmax.size() == 1);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
}

void CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax) const
{
   m_pGirderModelManager->GetReaction(girderKey,vSupports,intervalIdx,limitState,bat,bIncludeImpact,pRmin,pRmax);
}

void CAnalysisAgentImp::GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,REACTION* pRmin,REACTION* pRmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig) const
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<REACTION> Rmin;
   std::vector<REACTION> Rmax;
   std::vector<AxleConfiguration> MinAxleConfig;
   std::vector<AxleConfiguration> MaxAxleConfig;

   GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIdx,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,&Rmin,&Rmax,pMinAxleConfig ? &MinAxleConfig : nullptr,pMaxAxleConfig ? &MaxAxleConfig : nullptr);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
   if ( pMinAxleConfig )
   {
      *pMinAxleConfig = MinAxleConfig.front();
   }
   
   if ( pMaxAxleConfig )
   {
      *pMaxAxleConfig = MaxAxleConfig.front();
   }
}

void CAnalysisAgentImp::GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIdx,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig) const
{
   m_pGirderModelManager->GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIdx,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,REACTION* pRmin,REACTION* pRmax,VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx) const
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<REACTION> Rmin;
   std::vector<REACTION> Rmax;
   std::vector<VehicleIndexType> vMinVehIdx;
   std::vector<VehicleIndexType> vMaxVehIdx;

   GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,fetPrimary,&Rmin,&Rmax,&vMinVehIdx,&vMaxVehIdx);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
   if ( pMinVehIdx )
   {
      *pMinVehIdx = vMinVehIdx.front();
   }
   
   if ( pMaxVehIdx )
   {
      *pMaxVehIdx = vMaxVehIdx.front();
   }
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<VehicleIndexType>* pMinVehIdx,std::vector<VehicleIndexType>* pMaxVehIdx) const
{
   m_pGirderModelManager->GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,fetPrimary,pRmin,pRmax,pMinVehIdx,pMaxVehIdx);
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,REACTION* pRmin,REACTION* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx) const
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<REACTION> Rmin;
   std::vector<REACTION> Rmax;
   std::vector<Float64> Tmin;
   std::vector<Float64> Tmax;
   std::vector<VehicleIndexType> vMinVehIdx;
   std::vector<VehicleIndexType> vMaxVehIdx;

   GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,fetPrimary,fetDeflection,&Rmin,&Rmax,&Tmin,&Tmax,&vMinVehIdx,&vMaxVehIdx);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();

   *pTmin = Tmin.front();
   *pTmax = Tmax.front();

   if ( pMinVehIdx )
   {
      *pMinVehIdx = vMinVehIdx.front();
   }
   
   if ( pMaxVehIdx )
   {
      *pMaxVehIdx = vMaxVehIdx.front();
   }
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::ForceEffectType fetPrimary,pgsTypes::ForceEffectType fetDeflection,std::vector<REACTION>* pRmin,std::vector<REACTION>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinVehIdx,std::vector<VehicleIndexType>* pMaxVehIdx) const
{
   m_pGirderModelManager->GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,fetPrimary,fetDeflection,pRmin,pRmax,pTmin,pTmax,pMinVehIdx,pMaxVehIdx);
}

void CAnalysisAgentImp::GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax) const
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<Float64> Rmin;
   std::vector<Float64> Rmax;

   GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,&Rmin,&Rmax);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax) const
{
   m_pGirderModelManager->GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,pRmin,pRmax);
}

/////////////////////////////////////////////////
// IBearingDesign
std::vector<PierIndexType> CAnalysisAgentImp::GetBearingReactionPiers(IntervalIndexType intervalIdx,const CGirderKey& girderKey) const
{
   std::vector<PierIndexType> vPiers;

   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE_NOCHECK(IIntervals,pIntervals); // not always used, but don't want to get it in a loop
   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   PierIndexType nPiers = pBridge->GetPierCount();
   PierIndexType startPierIdx = (girderKey.groupIndex == INVALID_INDEX ? 0 : pBridge->GetGirderGroupStartPier(girderKey.groupIndex));
   PierIndexType endPierIdx   = (girderKey.groupIndex == INVALID_INDEX ? nPiers-1 : pBridge->GetGirderGroupEndPier(girderKey.groupIndex));
   for ( PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++ )
   {
      if ( pBridge->IsBoundaryPier(pierIdx) )
      {
         pgsTypes::BoundaryConditionType bcType = pBridge->GetBoundaryConditionType(pierIdx);
         if ( analysisType == pgsTypes::Simple ||
              pBridge->HasCantilever(pierIdx) || 
              bcType == pgsTypes::bctHinge || bcType == pgsTypes::bctRoller ||
              (bcType == pgsTypes::bctIntegralAfterDeckHingeAhead && pierIdx == startPierIdx) ||
              (bcType == pgsTypes::bctIntegralBeforeDeckHingeAhead && pierIdx == startPierIdx) ||
              (bcType == pgsTypes::bctIntegralAfterDeckHingeBack && pierIdx == endPierIdx) ||
              (bcType == pgsTypes::bctIntegralBeforeDeckHingeBack && pierIdx == endPierIdx) )
         {
            vPiers.push_back(pierIdx);
         }
         else
         {
            // we have a boundary pier without final bearing reactions...
            // if the interval in questions is before continuity is made, then
            // we can assume there is some type of bearing
            IntervalIndexType leftIntervalIdx, rightIntervalIdx;
            pIntervals->GetContinuityInterval(pierIdx,&leftIntervalIdx,&rightIntervalIdx);
            IntervalIndexType continuityIntervalIdx = (pierIdx == startPierIdx ? rightIntervalIdx : leftIntervalIdx);
            if ( intervalIdx < continuityIntervalIdx )
            {
               vPiers.push_back(pierIdx);
            }
         }
      }
      else
      {
         ATLASSERT( pBridge->IsInteriorPier(pierIdx) ); // if not boundary, must be interior
         ATLASSERT( analysisType == pgsTypes::Continuous); // we only have InteriorPiers in spliced girder analysis and we only use continuous analysis mode
         vPiers.push_back(pierIdx); // there is always a reaction at this pier
      }
   }
   return vPiers;
}

Float64 CAnalysisAgentImp::GetBearingProductReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   if ( pfType == pgsTypes::pftPretension || pfType == pgsTypes::pftPostTensioning )
   {
      // Pretension and primary post-tension are internal self equilibriating forces
      // they don't cause external reactions
      return 0.0;
   }
   else
   {
      return m_pGirderModelManager->GetBearingProductReaction(intervalIdx,location,pfType,bat,resultsType);
   }
}

void CAnalysisAgentImp::GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,
                                VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx) const
{
   m_pGirderModelManager->GetBearingLiveLoadReaction(intervalIdx,location,llType,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pTmin,pTmax,pMinVehIdx,pMaxVehIdx);
}

void CAnalysisAgentImp::GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,
                                VehicleIndexType* pMinVehIdx,VehicleIndexType* pMaxVehIdx) const
{
   m_pGirderModelManager->GetBearingLiveLoadRotation(intervalIdx,location,llType,bat,bIncludeImpact,bIncludeLLDF,pTmin,pTmax,pRmin,pRmax,pMinVehIdx,pMaxVehIdx);
}

Float64 CAnalysisAgentImp::GetBearingCombinedReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType) const
{
   Float64 R = 0;
   if ( comboType == lcPS )
   {
      std::vector<pgsTypes::ProductForceType> vpfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      for( const auto& pfType : vpfTypes)
      {
         Float64 r = GetBearingProductReaction(intervalIdx,location,pfType,bat,resultsType);
         R += r;
      }
   }
   else
   {
      R = m_pGirderModelManager->GetBearingCombinedReaction(intervalIdx,location,comboType,bat,resultsType);
   }
   return R;
}

void CAnalysisAgentImp::GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LiveLoadType llType,
                                                           pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                                           Float64* pRmin,Float64* pRmax) const
{
   m_pGirderModelManager->GetBearingCombinedLiveLoadReaction(intervalIdx,location,llType,bat,bIncludeImpact,pRmin,pRmax);
}

void CAnalysisAgentImp::GetBearingLimitStateReaction(IntervalIndexType intervalIdx,const ReactionLocation& location,pgsTypes::LimitState limitState,
                                                     pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                                     Float64* pRmin,Float64* pRmax) const
{
   m_pGirderModelManager->GetBearingLimitStateReaction(intervalIdx,location,limitState,bat,bIncludeImpact,pRmin,pRmax);
}

Float64 CAnalysisAgentImp::GetDeflectionAdjustmentFactor(const pgsPointOfInterest& poi,const GDRCONFIG* pConfig,IntervalIndexType intervalIdx) const
{
   if (pConfig == nullptr)
   {
      return 1.0; // no adjustment necessary
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   Float64 fc = (intervalIdx < erectionIntervalIdx ? pConfig->Fci : pConfig->Fc);

   GET_IFACE(ISectionProperties,pSectProp);

   Float64 Ix = pSectProp->GetIxx(intervalIdx, poi);
   Float64 Ix_adjusted = pSectProp->GetIxx(intervalIdx, poi, fc);

   Float64 Iy = pSectProp->GetIyy(intervalIdx, poi);
   Float64 Iy_adjusted = pSectProp->GetIyy(intervalIdx, poi, fc);

   Float64 Ixy = pSectProp->GetIxy(intervalIdx, poi);
   Float64 Ixy_adjusted = pSectProp->GetIxy(intervalIdx, poi, fc);

   GET_IFACE(IMaterials,pMaterials);
   Float64 Ec = pMaterials->GetSegmentEc(poi.GetSegmentKey(),intervalIdx);
   Float64 Ec_adjusted = (pConfig->bUserEc ? pConfig->Ec : pMaterials->GetEconc(fc,pMaterials->GetSegmentStrengthDensity(poi.GetSegmentKey()),
                                                                               pMaterials->GetSegmentEccK1(poi.GetSegmentKey()),
                                                                               pMaterials->GetSegmentEccK2(poi.GetSegmentKey())));
   Float64 EI = IsZero(Iy) ? 0 : Ec*(Ix*Iy-Ixy*Ixy)/Iy;
   Float64 EI_adjusted = IsZero(Iy_adjusted) ? 0 : Ec_adjusted * (Ix_adjusted*Iy_adjusted - Ixy_adjusted*Ixy_adjusted)/Iy_adjusted;

   Float64 k = (IsZero(EI_adjusted) ? 0 : EI/EI_adjusted);

   return k;
}

void CAnalysisAgentImp::ComputeTimeDependentEffects(const CGirderKey& girderKey,IntervalIndexType intervalIdx) const
{
   // Getting the timestep loss results, causes the creep, shrinkage, relaxation, and prestress forces
   // to be added to the LBAM model...
   GET_IFACE(ILossParameters,pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      GET_IFACE(ILosses,pLosses);
      GET_IFACE(IPointOfInterest,pPoi);

      pgsPointOfInterest poi( pPoi->GetPointOfInterest(CSegmentKey(girderKey,0),0.0) );
      ATLASSERT(poi.GetID() != INVALID_ID);
      pLosses->GetLossDetails(poi,intervalIdx);
   }
}

void CAnalysisAgentImp::IsDeckInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,bool* pbTopPTZ,bool* pbBotPTZ) const
{
   GET_IFACE(IBridge,pBridge);
   if ( IsNonstructuralDeck(pBridge->GetDeckType()) )
   {
      // if there is no deck, the deck can't be in the PTZ
      *pbTopPTZ = false;
      *pbBotPTZ = false;
      return;
   }

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // Get the stress when the bridge is in service (that is when live load is applied)
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType serviceLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // Tensile stresses are greatest at the top of the deck using the minimum model in
   // Envelope mode. In all other modes, Min/Max are the same
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   Float64 fMin[2], fMax[2];
   GetStress(serviceLoadIntervalIdx,limitState,poi,bat,false/*without prestress*/,pgsTypes::TopDeck,   &fMin[TOP],&fMax[TOP]);
   GetStress(serviceLoadIntervalIdx,limitState,poi,bat,false/*without prestress*/,pgsTypes::BottomDeck,&fMin[BOT],&fMax[BOT]);

   // The section is in tension, does the prestress cause compression?
   Float64 fPreTension[2];
   GetStress(serviceLoadIntervalIdx,poi,pgsTypes::TopDeck,pgsTypes::BottomDeck,false/*don't include live load*/,limitState,INVALID_INDEX/*controlling live load*/,&fPreTension[TOP],&fPreTension[BOT]);

   Float64 fPostTension[2];
   GetStress(serviceLoadIntervalIdx,pgsTypes::pftPostTensioning,poi,bat,rtCumulative,pgsTypes::TopDeck,pgsTypes::BottomDeck,&fPostTension[TOP],&fPostTension[BOT]);

   Float64 fPS[2];
   fPS[TOP] = fPreTension[TOP] + fPostTension[TOP];
   fPS[BOT] = fPreTension[BOT] + fPostTension[BOT];

   *pbTopPTZ = 0 < fMax[TOP] && fPS[TOP] < 0 ? true : false;
   *pbBotPTZ = 0 < fMax[BOT] && fPS[BOT] < 0 ? true : false;
}

void CAnalysisAgentImp::IsGirderInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,const GDRCONFIG* pConfig,bool* pbTopPTZ,bool* pbBotPTZ) const
{
   // The specified location is in a precompressed tensile zone if the following requirements are true
   // 1) The location is in tension in the Service III limit state for the final interval
   // 2) Prestressing causes compression at the location

   // First deal with the special cases

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // Special case... this is a regular prestressed girder or a simple span spliced girder and the girder does not have cantilevered ends
   // This case isn't that special, however, we know that the bottom of the girder is in the PTZ and the top is not. There
   // is no need to do all the analytical work to figure this out.
   GET_IFACE(IBridge,pBridge);
   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
   bool bDummy;
   bool bContinuousStart, bContinuousEnd;
   bool bIntegralStart, bIntegralEnd;
   pBridge->IsContinuousAtPier(startPierIdx,&bDummy,&bContinuousStart);
   pBridge->IsIntegralAtPier(startPierIdx,&bDummy,&bIntegralStart);
   pBridge->IsContinuousAtPier(endPierIdx,&bContinuousEnd,&bDummy);
   pBridge->IsIntegralAtPier(endPierIdx,&bIntegralEnd,&bDummy);
   bool bNegMoment = (bContinuousStart || bIntegralStart || bContinuousEnd || bIntegralEnd);
   if ( startPierIdx == endPierIdx-1 && // the group is only one span long
        pBridge->GetSegmentCount(segmentKey) == 1 && // there one segment in the group
        !pBridge->HasCantilever(startPierIdx) && // start is not cantilever
        !pBridge->HasCantilever(endPierIdx) && // end is not cantilever
        !bNegMoment) // no contiuous or integral boundary conditions to cause negative moments
   {
      // we know the answer
      *pbTopPTZ = false;
      *pbBotPTZ = true;
      return;
   }


   // Special case... At the start/end of the first/last segment the stress due to 
   // externally applied loads is zero (moment is zero) for roller/hinge
   // boundary condition and the stress due to the prestressing is also zero (strands not developed). 
   // Consider the bottom of the girder to be in a precompressed tensile zone
   if ( segmentKey.segmentIndex == 0 ) // start of first segment (end of last segment is below)
   {
      bool bModelStartCantilever,bModelEndCantilever;
      pBridge->ModelCantilevers(segmentKey,&bModelStartCantilever,&bModelEndCantilever);

      if ( poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 1 || // start of segment at release
          (poi.IsTenthPoint(POI_ERECTED_SEGMENT)  == 1 && !bModelStartCantilever) ) // CL Brg at start of erected segment
      {
         PierIndexType pierIdx = pBridge->GetGirderGroupStartPier(segmentKey.groupIndex);
         pgsTypes::BoundaryConditionType boundaryConditionType = pBridge->GetBoundaryConditionType(pierIdx);
         if ( boundaryConditionType == pgsTypes::bctHinge || boundaryConditionType == pgsTypes::bctRoller )
         {
            *pbTopPTZ = false;
            *pbBotPTZ = true;
            return;
         }
      }
   }

   // end of last segment
   SegmentIndexType nSegments = pBridge->GetSegmentCount(segmentKey);
   if ( segmentKey.segmentIndex == nSegments-1 )
   {
      bool bModelStartCantilever,bModelEndCantilever;
      pBridge->ModelCantilevers(segmentKey,&bModelStartCantilever,&bModelEndCantilever);

      if ( poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 11 ||
          (poi.IsTenthPoint(POI_ERECTED_SEGMENT)  == 11 && !bModelEndCantilever) )
      {
         PierIndexType pierIdx = pBridge->GetGirderGroupEndPier(segmentKey.groupIndex);
         pgsTypes::BoundaryConditionType boundaryConditionType = pBridge->GetBoundaryConditionType(pierIdx);
         if ( boundaryConditionType == pgsTypes::bctHinge || boundaryConditionType == pgsTypes::bctRoller )
         {
            *pbTopPTZ = false;
            *pbBotPTZ = true;
            return;
         }
      }
   }

   // Special case... ends of any segment for stressing at release
   // Even though there is prestress, at the end faces of the girder there isn't
   // any prestress force because it hasn't been transfered to the girder yet. The
   // prestress force transfers over the transfer length.
   if ( poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 1 || poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 11 )
   {
      *pbTopPTZ = false;
      *pbBotPTZ = true;
      return;
   }

   // Special case... if there aren't any strands the notion of a precompressed tensile zone
   // gets really goofy (there is no precompression). Technically the segment is a reinforced concrete
   // beam. However, this is a precast-prestressed concrete program so we need to have something reasonable
   // for the precompressed tensile zone. If there aren't any strands (or the Pjack is zero), then
   // the bottom of the girder is in the PTZ and the top is not. This is where the PTZ is usually located
   // when there are strands
   Float64 Pjack;
   if ( pConfig )
   {
      Pjack = pConfig->PrestressConfig.Pjack[pgsTypes::Straight] + 
              pConfig->PrestressConfig.Pjack[pgsTypes::Harped]   + 
              pConfig->PrestressConfig.Pjack[pgsTypes::Temporary];
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      Pjack = pStrandGeom->GetPjack(segmentKey,true/*include temp strands*/);
   }

   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(segmentKey);
   if ( IsZero(Pjack) && nDucts == 0 )
   {
      *pbTopPTZ = false;
      *pbBotPTZ = true;
      return;
   }

   // Special case... if the POI is located "near" interior supports with continuous boundary
   // condition tension develops in the top of the girder and prestressing may cause compression
   // in this location (most likely from harped strands). From LRFD C5.12.3.3.6 (pre2017: C5.14.1.4.6),
   // this location is not in a precompressed tensile zone. Assume that "near" means that the POI is 
   // somewhere between mid-span and the closest pier.
   SpanIndexType startSpanIdx, endSpanIdx;
   pBridge->GetSpansForSegment(segmentKey,&startSpanIdx,&endSpanIdx);

   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
   startPierIdx = (PierIndexType)spanKey.spanIndex;
   endPierIdx = startPierIdx+1;

   // some segments are not supported by piers at all. segments can be supported by
   // just temporary supports. an example would be the center segment in a three-segment
   // single span bridge. This special case doesn't apply to that segment.
   bool bStartAtPier = true;  // start end of segment bears on a pier
   bool bEndAtPier   = true;  // end end of segment bears on a pier

   // one of the piers must be a boundary pier or C5.12.3.3.6 (pre2017: C5.14.1.4.6) doesn't apply
   // the segment must start and end in the same span (can't straddle a pier)
   if ( startSpanIdx == endSpanIdx && (pBridge->IsBoundaryPier(startPierIdx) || pBridge->IsBoundaryPier(endPierIdx)) )
   {
      Float64 Xstart, Xend; // dist from end of segment to start/end pier
      if ( pBridge->IsBoundaryPier(startPierIdx) )
      {
         // GetPierLocation returns false if the segment does not bear on the pier
         bStartAtPier = pBridge->GetPierLocation(startPierIdx,segmentKey,&Xstart);
      }
      else
      {
         // not a boundary pier so use a really big number so
         // this end doesn't control
         Xstart = DBL_MAX;
      }

      if ( pBridge->IsBoundaryPier(endPierIdx) )
      {
         // GetPierLocation returns false if the segment does not bear on the pier
         bEndAtPier = pBridge->GetPierLocation(endPierIdx,segmentKey,&Xend);
      }
      else
      {
         // not a boundary pier so use a really big number so
         // this end doesn't control
         Xend = DBL_MAX;
      }

      if ( bStartAtPier && bEndAtPier ) // both ends must bear on a pier
      {
         // distance from POI to pier at start/end of the span
         // If they are equal distances, then the POI is at mid-span
         // and we can't make a direct determination if C5.12.3.3.6 (pre2017: C5.14.1.4.6) applies.
         // If they are both equal, continue with the procedure below
         Float64 offsetStart = fabs(Xstart-poi.GetDistFromStart());
         Float64 offsetEnd   = fabs(Xend-poi.GetDistFromStart());
         if ( !IsEqual(offsetStart,offsetEnd) )
         {
            // poi is closer to one pier then the other.
            // get the boundary conditions of the nearest pier
            pgsTypes::BoundaryConditionType boundaryConditionType;
            if ( offsetStart < offsetEnd )
            {
               // nearest pier is at the start of the span
               boundaryConditionType = pBridge->GetBoundaryConditionType(startPierIdx);
            }
            else
            {
               // nearest pier is at the end of the span
               boundaryConditionType = pBridge->GetBoundaryConditionType(endPierIdx);
            }

            // if hinge or roller boundary condition, C5.12.3.3.6 (pre2017: C5.14.1.4.6) doesn't apply.
            if ( boundaryConditionType != pgsTypes::bctRoller && boundaryConditionType != pgsTypes::bctHinge )
            {
               // connection type is some sort of continuity/integral boundary condition
               // The top of the girder is not in the PTZ.
               *pbTopPTZ = false;
               *pbBotPTZ = true;
               return;
            }
         }
      }
   }

   // Now deal with the regular case

   // Get the stress when the bridge is in service (that is when live load is applied)
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType serviceLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // Tensile stresses are greatest at the top of the girder using the minimum model in
   // Envelope mode. In all other modes, Min/Max are the same
   pgsTypes::BridgeAnalysisType batTop = GetBridgeAnalysisType(pgsTypes::Minimize);
   pgsTypes::BridgeAnalysisType batBot = GetBridgeAnalysisType(pgsTypes::Maximize);

   // Get stresses due to service loads
   Float64 fMin[2], fMax[2];
   GetStress(serviceLoadIntervalIdx,limitState,poi,batTop,false/*without prestress*/,pgsTypes::TopGirder,   &fMin[TOP],&fMax[TOP]);
   GetStress(serviceLoadIntervalIdx,limitState,poi,batBot,false/*without prestress*/,pgsTypes::BottomGirder,&fMin[BOT],&fMax[BOT]);
   //if ( fMax <= 0 )
   //{
   //   return false; // the location is not in tension so is not in the "tension zone"
   //}

   // The section is in tension, does the prestress cause compression in the service load interval?
   Float64 fPreTension[2], fPostTension[2];
   if ( pConfig )
   {
      fPreTension[TOP] = GetDesignStress(serviceLoadIntervalIdx,poi,pgsTypes::TopGirder,   *pConfig,false/*don't include live load*/,limitState);
      fPreTension[BOT] = GetDesignStress(serviceLoadIntervalIdx,poi,pgsTypes::BottomGirder,*pConfig,false/*don't include live load*/,limitState);
      fPostTension[TOP] = 0; // no post-tensioning for precast girder design
      fPostTension[BOT] = 0; // no post-tensioning for precast girder design
   }
   else
   {
      GetStress(serviceLoadIntervalIdx,poi,pgsTypes::TopGirder,pgsTypes::BottomGirder,false/*don't include live load*/,limitState,INVALID_INDEX,&fPreTension[TOP],&fPreTension[BOT]);

      GetStress(serviceLoadIntervalIdx,pgsTypes::pftPostTensioning,poi,batTop,rtCumulative,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fPostTension[TOP],&fPostTension[BOT]);
   }
   
   Float64 fPS[2];
   fPS[TOP] = fPreTension[TOP] + fPostTension[TOP];
   fPS[BOT] = fPreTension[BOT] + fPostTension[BOT];

   *pbTopPTZ = 0 < fMax[TOP] && fPS[TOP] < 0 ? true : false;
   *pbBotPTZ = 0 < fMax[BOT] && fPS[BOT] < 0 ? true : false;
}

void CAnalysisAgentImp::GetTimeStepStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pfTop->clear();
   pfBot->clear();

   GET_IFACE(ILosses,pLosses);

   pgsTypes::FaceType topFace = (IsTopStressLocation(topLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);
   pgsTypes::FaceType botFace = (IsTopStressLocation(botLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);

   for ( const pgsPointOfInterest& poi : vPoi)
   {
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_CONCRETE* pTopConcreteElement = (IsGirderStressLocation(topLocation) ? &tsDetails.Girder : &tsDetails.Deck);
      const TIME_STEP_CONCRETE* pBotConcreteElement = (IsGirderStressLocation(botLocation) ? &tsDetails.Girder : &tsDetails.Deck);

      Float64 fTop = pTopConcreteElement->f[topFace][pfType][resultsType];
      Float64 fBot = pBotConcreteElement->f[botFace][pfType][resultsType];

      pfTop->push_back(fTop);
      pfBot->push_back(fBot);
   }
}

void CAnalysisAgentImp::GetElasticStress(IntervalIndexType intervalIdx,pgsTypes::ProductForceType pfType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   USES_CONVERSION;

   pfTop->clear();
   pfBot->clear();

   if ( pfType == pgsTypes::pftPostTensioning || pfType == pgsTypes::pftCreep || pfType == pgsTypes::pftShrinkage || pfType == pgsTypes::pftRelaxation )
   {
      // no direct post-tension stress in the elastic analysis
      pfTop->resize(vPoi.size(),0.0);
      pfBot->resize(vPoi.size(),0.0);
      return;
   }

   if ( pfType == pgsTypes::pftPretension )
   {
      GetStress(intervalIdx, vPoi, pgsTypes::TopGirder, pgsTypes::BottomGirder,false /*don't include live load effects*/, pgsTypes::ServiceI,INVALID_INDEX,pfTop,pfBot);
      return;
   }

   try
   {
      IntervalIndexType erectionIntervalIdx = GetErectionInterval(vPoi);
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetStress(intervalIdx,pfType,vPoi,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> fTopPrev, fBotPrev;
         std::vector<Float64> fTopThis, fBotThis;
         GetStress(intervalIdx-1,pfType,vPoi,bat,rtCumulative,topLocation,botLocation,&fTopPrev,&fBotPrev);
         GetStress(intervalIdx,  pfType,vPoi,bat,rtCumulative,topLocation,botLocation,&fTopThis,&fBotThis);

         std::transform(fTopThis.cbegin(),fTopThis.cend(),fTopPrev.cbegin(),std::back_inserter(*pfTop),[](const auto& a, const auto& b) {return a - b;});
         std::transform(fBotThis.cbegin(),fBotThis.cend(),fBotPrev.cbegin(),std::back_inserter(*pfBot),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         m_pGirderModelManager->GetStress(intervalIdx,pfType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
      }

      GET_IFACE_NOCHECK(IBridge, pBridge);
      if (pfType == pgsTypes::pftGirder && pBridge->HasTiltedGirders() && erectionIntervalIdx <= intervalIdx)
      {
         std::vector<Float64> ftlat, fblat;
         GetTiltedGirderLateralStresses(vPoi, bat, topLocation, botLocation, &ftlat, &fblat);
         std::transform(pfTop->cbegin(), pfTop->cend(), ftlat.cbegin(), pfTop->begin(), [](const auto& f, const auto& fl) { return f + fl; });
         std::transform(pfBot->cbegin(), pfBot->cend(), fblat.cbegin(), pfBot->begin(), [](const auto& f, const auto& fl) { return f + fl; });
      }
   }
   catch(...)
   {
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetTimeStepStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pfTop->clear();
   pfBot->clear();

   GET_IFACE(ILosses,pLosses);

   std::vector<pgsTypes::ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);

  pgsTypes::FaceType topFace = (IsTopStressLocation(topLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);
  pgsTypes::FaceType botFace = (IsTopStressLocation(botLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);

  for (const pgsPointOfInterest& poi : vPoi)
  {
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_CONCRETE* pTopConcreteElement = (IsGirderStressLocation(topLocation) ? &tsDetails.Girder : &tsDetails.Deck);
      const TIME_STEP_CONCRETE* pBotConcreteElement = (IsGirderStressLocation(botLocation) ? &tsDetails.Girder : &tsDetails.Deck);

      Float64 fTop(0), fBot(0);
      std::vector<pgsTypes::ProductForceType>::iterator pfTypeIter(pfTypes.begin());
      std::vector<pgsTypes::ProductForceType>::iterator pfTypeIterEnd(pfTypes.end());
      for ( ; pfTypeIter != pfTypeIterEnd; pfTypeIter++ )
      {
         pgsTypes::ProductForceType pfType = *pfTypeIter;
         fTop += pTopConcreteElement->f[topFace][pfType][resultsType];
         fBot += pBotConcreteElement->f[botFace][pfType][resultsType];
      }

      pfTop->push_back(fTop);
      pfBot->push_back(fBot);
   }
}

void CAnalysisAgentImp::GetElasticStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot) const
{
   USES_CONVERSION;

   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      ComputeTimeDependentEffects(vPoi.front().get().GetSegmentKey(),intervalIdx);
   }

   pfTop->clear();
   pfBot->clear();

   if ( comboType == lcPS )
   {
      // no secondary effects for elastic analysis
      pfTop->resize(vPoi.size(),0.0);
      pfBot->resize(vPoi.size(),0.0);
      return;
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().get().GetSegmentKey());
      if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetStress(intervalIdx,comboType,vPoi,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
      else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
      {
         // the incremental result at the time of erection is being requested. this is when
         // we switch between segment models and girder models. the incremental results
         // is the cumulative result this interval minus the cumulative result in the previous interval
         std::vector<Float64> fTopPrev, fBotPrev;
         std::vector<Float64> fTopThis, fBotThis;
         GetStress(intervalIdx-1,comboType,vPoi,bat,rtCumulative,topLocation,botLocation,&fTopPrev,&fBotPrev);
         GetStress(intervalIdx,  comboType,vPoi,bat,rtCumulative,topLocation,botLocation,&fTopThis,&fBotThis);

         std::transform(fTopThis.cbegin(),fTopThis.cend(),fTopPrev.cbegin(),std::back_inserter(*pfTop),[](const auto& a, const auto& b) {return a - b;});
         std::transform(fBotThis.cbegin(),fBotThis.cend(),fBotPrev.cbegin(),std::back_inserter(*pfBot),[](const auto& a, const auto& b) {return a - b;});
      }
      else
      {
         m_pGirderModelManager->GetStress(intervalIdx,comboType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
      }

      GET_IFACE_NOCHECK(IBridge, pBridge);
      if (comboType == lcDC && pBridge->HasTiltedGirders() && erectionIntervalIdx <= intervalIdx)
      {
         std::vector<Float64> ftlat, fblat;
         GetTiltedGirderLateralStresses(vPoi, bat, topLocation, botLocation, &ftlat, &fblat);
         std::transform(pfTop->cbegin(), pfTop->cend(), ftlat.cbegin(), pfTop->begin(), [](const auto& f, const auto& fl) { return f + fl; });
         std::transform(pfBot->cbegin(), pfBot->cend(), fblat.cbegin(), pfBot->begin(), [](const auto& f, const auto& fl) { return f + fl; });
      }
   }
   catch(...)
   {
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetTimeStepStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation stressLocation,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pMin->clear();
   pMax->clear();

   Float64 gDCMin, gDCMax;
   Float64 gDWMin, gDWMax;
   Float64 gCRMin, gCRMax;
   Float64 gSHMin, gSHMax;
   Float64 gREMin, gREMax;
   Float64 gPSMin, gPSMax;
   Float64 gLLMin, gLLMax;

   if (IsDesignLimitState(limitState))
   {
      GET_IFACE(ILoadFactors, pILoadFactors);
      const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
      pLoadFactors->GetDC(limitState, &gDCMin, &gDCMax);
      pLoadFactors->GetDW(limitState, &gDWMin, &gDWMax);
      pLoadFactors->GetCR(limitState, &gCRMin, &gCRMax);
      pLoadFactors->GetSH(limitState, &gSHMin, &gSHMax);
      pLoadFactors->GetRE(limitState, &gREMin, &gREMax);
      pLoadFactors->GetPS(limitState, &gPSMin, &gPSMax);
      pLoadFactors->GetLLIM(limitState, &gLLMin, &gLLMax);
   }
   else
   {
      GET_IFACE(IRatingSpecification, pRatingSpec);
      gDCMin = pRatingSpec->GetDeadLoadFactor(limitState);
      gDCMax = gDCMin;

      gDWMin = pRatingSpec->GetWearingSurfaceFactor(limitState);
      gDWMax = gDWMin;

      gCRMin = pRatingSpec->GetCreepFactor(limitState);
      gCRMax = gCRMin;

      gSHMin = pRatingSpec->GetShrinkageFactor(limitState);
      gSHMax = gSHMin;

      gREMin = pRatingSpec->GetRelaxationFactor(limitState);
      gREMax = gREMin;

      gPSMin = pRatingSpec->GetSecondaryEffectsFactor(limitState);
      gPSMax = gPSMin;

      gLLMin = pRatingSpec->GetLiveLoadFactor(limitState,true);
      gLLMax = gLLMin;
   }


   // Use half prestress if Service IA or Fatigue I (See LRFD 5.5.3.1)
   Float64 k = (limitState == pgsTypes::ServiceIA || limitState == pgsTypes::FatigueI) ? 0.5 : 1.0;

   // Get load combination stresses
   pgsTypes::StressLocation topLocation = IsGirderStressLocation(stressLocation) ? pgsTypes::TopGirder    : pgsTypes::TopDeck;
   pgsTypes::StressLocation botLocation = IsGirderStressLocation(stressLocation) ? pgsTypes::BottomGirder : pgsTypes::BottomDeck;
   std::vector<Float64> fDCtop, fDWtop, fCRtop, fSHtop, fREtop, fPStop;
   std::vector<Float64> fDCbot, fDWbot, fCRbot, fSHbot, fREbot, fPSbot;
   GetTimeStepStress(intervalIdx,lcDC,vPoi,bat,rtCumulative,topLocation,botLocation,&fDCtop,&fDCbot);
   GetTimeStepStress(intervalIdx,lcDW,vPoi,bat,rtCumulative,topLocation,botLocation,&fDWtop,&fDWbot);
   GetTimeStepStress(intervalIdx,lcCR,vPoi,bat,rtCumulative,topLocation,botLocation,&fCRtop,&fCRbot);
   GetTimeStepStress(intervalIdx,lcSH,vPoi,bat,rtCumulative,topLocation,botLocation,&fSHtop,&fSHbot);
   GetTimeStepStress(intervalIdx,lcRE,vPoi,bat,rtCumulative,topLocation,botLocation,&fREtop,&fREbot);
   GetTimeStepStress(intervalIdx,lcPS,vPoi,bat,rtCumulative,topLocation,botLocation,&fPStop,&fPSbot);
   
   // we only want top or bottom
   std::vector<Float64>* pfDC = (IsTopStressLocation(stressLocation) ? &fDCtop : &fDCbot);
   std::vector<Float64>* pfDW = (IsTopStressLocation(stressLocation) ? &fDWtop : &fDWbot);
   std::vector<Float64>* pfCR = (IsTopStressLocation(stressLocation) ? &fCRtop : &fCRbot);
   std::vector<Float64>* pfSH = (IsTopStressLocation(stressLocation) ? &fSHtop : &fSHbot);
   std::vector<Float64>* pfRE = (IsTopStressLocation(stressLocation) ? &fREtop : &fREbot);
   std::vector<Float64>* pfPS = (IsTopStressLocation(stressLocation) ? &fPStop : &fPSbot);

   // add in prestress and live load
   GET_IFACE(ILosses,pLosses);

   std::vector<Float64>::iterator dcIter = pfDC->begin();
   std::vector<Float64>::iterator dwIter = pfDW->begin();
   std::vector<Float64>::iterator crIter = pfCR->begin();
   std::vector<Float64>::iterator shIter = pfSH->begin();
   std::vector<Float64>::iterator reIter = pfRE->begin();
   std::vector<Float64>::iterator psIter = pfPS->begin();

   auto poiIter(vPoi.begin());
   auto poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++, dcIter++, dwIter++, crIter++, shIter++, reIter++, psIter++ )
   {
      Float64 fPR(0), fPT(0), fLLMin(0), fLLMax(0);
      const pgsPointOfInterest& poi(*poiIter);
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_CONCRETE* pConcreteElement = (IsGirderStressLocation(stressLocation) ? &tsDetails.Girder : &tsDetails.Deck);
      pgsTypes::FaceType face = IsTopStressLocation(stressLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace;

      if ( bIncludePrestress )
      {
         fPR = pConcreteElement->f[face][pgsTypes::pftPretension    ][rtCumulative];
         fPT = pConcreteElement->f[face][pgsTypes::pftPostTensioning][rtCumulative];
      }

      fLLMin = pConcreteElement->fLLMin[face];
      fLLMax = pConcreteElement->fLLMax[face];

      if ( fLLMax < fLLMin )
      {
         std::swap(fLLMin,fLLMax);
      }

      Float64 fMin = gDCMin*(*dcIter) + gDWMin*(*dwIter) + gCRMin*(*crIter) + gSHMin*(*shIter) + gREMin*(*reIter) + gPSMin*(*psIter) + gLLMin*fLLMin + k*(fPR + fPT);
      Float64 fMax = gDCMax*(*dcIter) + gDWMax*(*dwIter) + gCRMax*(*crIter) + gSHMax*(*shIter) + gREMax*(*reIter) + gPSMax*(*psIter) + gLLMax*fLLMax + k*(fPR + fPT);

      pMin->push_back(fMin);
      pMax->push_back(fMax);
   }
}

void CAnalysisAgentImp::GetElasticStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const PoiList& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation stressLocation,std::vector<Float64>* pMin,std::vector<Float64>* pMax) const
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().get().GetSegmentKey());

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      if ( intervalIdx < releaseIntervalIdx )
      {
         pMin->resize(vPoi.size(),0.0);
         pMax->resize(vPoi.size(),0.0);
      }
      else if ( intervalIdx < erectionIntervalIdx )
      {
         m_pSegmentModelManager->GetStress(intervalIdx,limitState,vPoi,stressLocation,bIncludePrestress,pMin,pMax);
      }
      else
      {
         m_pGirderModelManager->GetStress(intervalIdx,limitState,vPoi,bat,stressLocation,bIncludePrestress,pMin,pMax);
      }

      GET_IFACE(IBridge, pBridge);
      if (pBridge->HasTiltedGirders() && erectionIntervalIdx <= intervalIdx)
      {
         Float64 DCmax,DCmin;
         if (IsDesignLimitState(limitState))
         {
            GET_IFACE(ILoadFactors, pLF);
            const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();
            DCmax = pLoadFactors->GetDCMax(limitState);
            DCmin = pLoadFactors->GetDCMin(limitState);
         }
         else
         {
            GET_IFACE(IRatingSpecification, pRatingSpec);
            DCmin = pRatingSpec->GetDeadLoadFactor(limitState);
            DCmax = DCmin;
         }

         std::vector<Float64> ftlat, fblat;
         GetTiltedGirderLateralStresses(vPoi, bat, stressLocation, stressLocation, &ftlat, &fblat);
         std::transform(pMin->cbegin(), pMin->cend(), ftlat.cbegin(), pMin->begin(), [&DCmin, &DCmax](const auto& f, const auto& fl) { return f + (::BinarySign(f) == ::BinarySign(fl) ? DCmax : DCmin)*fl; });
         std::transform(pMax->cbegin(), pMax->cend(), fblat.cbegin(), pMax->begin(), [&DCmin, &DCmax](const auto& f, const auto& fl) { return f + (::BinarySign(f) == ::BinarySign(fl) ? DCmax : DCmin)*fl; });
      }
   }
   catch(...)
   {
      const_cast<CAnalysisAgentImp*>(this)->Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetTiltedGirderLateralStresses(const PoiList& vPoi, pgsTypes::BridgeAnalysisType bat, pgsTypes::StressLocation topLocation, pgsTypes::StressLocation botLocation, std::vector<Float64>* pfTop, std::vector<Float64>* pfBot) const
{
   // Girders are installed tilted (not plumb), so at the erection interval and thereafter there is a lateral
   // dead load moment component. This component causes stresses that must be included in the total stress

#if defined _DEBUG
   GET_IFACE(IBridge, pBridge);
   ATLASSERT(pBridge->HasTiltedGirders());
#endif

   pfTop->clear();
   pfBot->clear();
   pfTop->resize(vPoi.size(), 0.0);
   pfBot->resize(vPoi.size(), 0.0);

   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(ISectionProperties, pSectProps);
   GET_IFACE(IGirder, pGirder);
   GET_IFACE(IPointOfInterest, pPoi);
   IntervalIndexType thisSegmentErectionIntervalIdx;
   CSegmentKey lastSegmentKey;
   Float64 girder_orientation = 0;
   const auto& begin(vPoi.cbegin());
   auto& iter(vPoi.cbegin());
   const auto& end(vPoi.cend());
   for (; iter != end; iter++)
   {
      const pgsPointOfInterest& poi(*iter);

      if (pPoi->IsOffSegment(poi))
      {
         // poi is not on the girder (probably in an intermediate pier)
         ATLASSERT(pPoi->IsInBoundaryPierDiaphragm(poi)); // if the poi isn't in a boundary pier diaphragm, where is it? Just curious

         // There is no tilt effect
         continue;
      }

      const CSegmentKey& segmentKey = poi.GetSegmentKey();
      if (segmentKey != lastSegmentKey)
      {
         // the segment changed so get the orientation of this segment
         girder_orientation = pGirder->GetOrientation(segmentKey);
         thisSegmentErectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
      }
      lastSegmentKey = segmentKey;

      Float64 Mg = GetMoment(thisSegmentErectionIntervalIdx, pgsTypes::pftGirder, poi, bat, rtCumulative); // get the total moment at erection...
      Float64 Mx = fabs(girder_orientation)*Mg;

      Float64 Iyy = pSectProps->GetIyy(thisSegmentErectionIntervalIdx, poi);

      Float64 delta_ft, delta_fb; // change in stress due to girder orientation (assume small angles)
      if (IsTopStressLocation(topLocation))
      {
         Float64 X = pGirder->GetTopWidth(poi) / 2;
         delta_ft = -1 * Mx*X / Iyy; // -1 for compression top
      }
      else
      {
         Float64 X = pGirder->GetBottomWidth(poi) / 2;
         delta_ft = Mx*X / Iyy; // positive for tension bottom
      }

      if (IsTopStressLocation(botLocation))
      {
         Float64 X = pGirder->GetTopWidth(poi) / 2;
         delta_fb = -1 * Mx*X / Iyy; // -1 for compression top
      }
      else
      {
         Float64 X = pGirder->GetBottomWidth(poi) / 2;
         delta_fb = Mx*X / Iyy; // positive for tension bottom
      }

      IndexType idx = std::distance(begin, iter);
      (*pfTop)[idx] += delta_ft;
      (*pfBot)[idx] += delta_fb;
   }
}

void CAnalysisAgentImp::InitializeAnalysis(const PoiList& vPoi) const
{
   GET_IFACE(ILossParameters,pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      GET_IFACE(IPointOfInterest,pPoi);
      std::vector<CGirderKey> vGirderKeys;
      pPoi->GetGirderKeys(vPoi, &vGirderKeys);
      GirderIndexType firstGirderLineIdx = INVALID_INDEX;
      GirderIndexType lastGirderLineIdx = 0;
      for( const auto& girderKey : vGirderKeys)
      {
         ASSERT_GIRDER_KEY(girderKey);
         firstGirderLineIdx = Min(firstGirderLineIdx,girderKey.girderIndex);
         lastGirderLineIdx  = Max(lastGirderLineIdx, girderKey.girderIndex);
      }

      for ( GirderIndexType girderLineIdx = firstGirderLineIdx; girderLineIdx <= lastGirderLineIdx; girderLineIdx++ )
      {
         std::set<GirderIndexType>::iterator found = m_ExternalLoadState.find(girderLineIdx);
         if ( found == m_ExternalLoadState.end() )
         {
            GET_IFACE(ILosses,pLosses);
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
            for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ ) 
            {
               VERIFY(const_cast<CAnalysisAgentImp*>(this)->CreateLoading(girderLineIdx,pLosses->GetRestrainingLoadName(intervalIdx,TIMESTEP_CR)));
               VERIFY(const_cast<CAnalysisAgentImp*>(this)->CreateLoading(girderLineIdx,pLosses->GetRestrainingLoadName(intervalIdx,TIMESTEP_SH)));
               VERIFY(const_cast<CAnalysisAgentImp*>(this)->CreateLoading(girderLineIdx,pLosses->GetRestrainingLoadName(intervalIdx,TIMESTEP_RE)));
            }
            m_ExternalLoadState.insert(girderLineIdx);
         }
      } // next girder line
   } // end if time step
}

bool RemoveStrongbacksSupports(const CTemporarySupportData* pTS)
{
   return (pTS->GetSupportType() == pgsTypes::StrongBack ? true : false);
}

std::vector<std::pair<pgsPointOfInterest,IntervalIndexType>> CAnalysisAgentImp::GetSegmentPreviousSupportLocations(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx) const
{
   ASSERT_SEGMENT_KEY(segmentKey);

   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<std::pair<pgsPointOfInterest,IntervalIndexType>> vPoi;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      IntervalIndexType releaseIntervalIdx  = pIntervals->GetPrestressReleaseInterval(segmentKey);
      IntervalIndexType liftingIntervalIdx  = pIntervals->GetLiftSegmentInterval(segmentKey);
      IntervalIndexType storageIntervalIdx  = pIntervals->GetStorageInterval(segmentKey);
      IntervalIndexType haulingIntervalIdx  = pIntervals->GetHaulSegmentInterval(segmentKey);

      PoiAttributeType poiReference;
      ATLASSERT(intervalIdx != releaseIntervalIdx);
      if ( releaseIntervalIdx <= intervalIdx && intervalIdx < liftingIntervalIdx )
      {
         poiReference = POI_RELEASED_SEGMENT;
      }
      else if ( liftingIntervalIdx <= intervalIdx && intervalIdx < storageIntervalIdx)
      {
         poiReference = POI_LIFT_SEGMENT;
      }
      else if ( storageIntervalIdx <= intervalIdx && intervalIdx < haulingIntervalIdx )
      {
         poiReference = POI_STORAGE_SEGMENT;
      }
      else
      {
         ATLASSERT(haulingIntervalIdx <= intervalIdx && intervalIdx < erectionIntervalIdx);
         poiReference = POI_HAUL_SEGMENT;
      }

      PoiList vSupportPoi;
      pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_10L | poiReference, &vSupportPoi);
      vPoi.clear();
      for( const auto& poi : vSupportPoi)
      {
         vPoi.emplace_back(poi,intervalIdx-1);
      }
      return vPoi;
   }

   GET_IFACE(IBridgeDescription,pIBridgeDesc);
   const CPrecastSegmentData* pSegment = pIBridgeDesc->GetPrecastSegmentData(segmentKey);


   std::vector<const CPierData2*> vPiers = pSegment->GetPiers();
   IndexType nPiers = vPiers.size();
   if ( 0 < nPiers )
   {
      // segment is supported by at least one pier... piers are the highest priority "hard" supports
      if ( 2 <= nPiers )
      {
         // segment is supported by two or more piers... use the two piers closest to the ends of the segment
         if ( vPiers.front()->IsBoundaryPier() )
         {
            // if supported by a boundary pier, we want the CL Brg poi
            PoiList vSupportPoi;
            pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi);
            vPoi.emplace_back(vSupportPoi.front(),intervalIdx-1);
         }
         else
         {
            // this is an interior pier, we want the CL Pier poi
            vPoi.emplace_back(pPoi->GetPierPointOfInterest(segmentKey,vPiers.front()->GetIndex()),intervalIdx-1);
         }

         if ( vPiers.back()->IsBoundaryPier() )
         {
            // if supported by a boundary pier, we want the CL Brg poi
            PoiList vSupportPoi;
            pPoi->GetPointsOfInterest(segmentKey, POI_10L | POI_ERECTED_SEGMENT, &vSupportPoi);
            vPoi.emplace_back(vSupportPoi.front(),intervalIdx-1);
         }
         else
         {
            vPoi.emplace_back(pPoi->GetPierPointOfInterest(segmentKey,vPiers.back()->GetIndex()),intervalIdx-1);
         }
      }
      else
      {
         ATLASSERT(nPiers == 1);
         std::vector<const CTemporarySupportData*> vTS = pSegment->GetTemporarySupports();
         ATLASSERT(0 < vTS.size()); // must be at least one other support besides the pier
         vTS.erase(std::remove_if(vTS.begin(),vTS.end(),RemoveStrongbacksSupports),vTS.end());
         bool bStrongbacksOnly = false;
         if ( vTS.size() == 0 )
         {
            // the only temporary supports are strongbacks... the only way this is valid is if
            // the segment is the first or last segment
            ATLASSERT(pSegment->GetPrevSegment() == nullptr || pSegment->GetNextSegment() == nullptr);
            vTS = pSegment->GetTemporarySupports();
            bStrongbacksOnly = true;
            ATLASSERT(vTS.size() == 1); // there should only be one temporary support
         }
         ATLASSERT(0 < vTS.size()); // must be at least one non-strongback support or else the model is geometrically unstable
         pgsPointOfInterest poiPier = pPoi->GetPierPointOfInterest(segmentKey,vPiers.front()->GetIndex());
         pgsPointOfInterest poiTS   = pPoi->GetTemporarySupportPointOfInterest(segmentKey,vTS.front()->GetIndex());

         if ( vPiers.front()->GetStation() <= vTS.front()->GetStation() )
         {
            if ( vPiers.front()->IsBoundaryPier() )
            {
               // this is a boundary pier and it is to the left of the temporary support, therefore it must be at the
               // start of the segment... we want the start CL Bearing poi
               PoiList vSupportPoi;
               pPoi->GetPointsOfInterest(segmentKey, POI_0L | POI_ERECTED_SEGMENT, &vSupportPoi);
               vPoi.emplace_back(vSupportPoi.front(),intervalIdx-1);
            }
            else
            {
               // this is an interior pier so we want the CL Pier poi
               vPoi.emplace_back(pPoi->GetPierPointOfInterest(segmentKey,vPiers.front()->GetIndex()),intervalIdx-1);
            }

            pgsPointOfInterest poiTS = pPoi->GetTemporarySupportPointOfInterest(segmentKey,vTS.front()->GetIndex());
            if ( poiTS.HasAttribute(POI_CLOSURE) )
            {
               // temp support is at a closure joint.. this means the segment does not span across this TS. Since
               // this TS is to the right of the pier, we want the CL Bearing at the end of the segment
               ATLASSERT(poiTS.GetSegmentKey() == segmentKey);
               CSegmentKey thisSegmentKey(segmentKey);
               PoiAttributeType attrib = POI_10L | POI_ERECTED_SEGMENT;
               IntervalIndexType iIdx = intervalIdx-1;
               if ( bStrongbacksOnly && intervalIdx == erectionIntervalIdx )
               {
                  thisSegmentKey.segmentIndex++;
                  attrib = POI_START_FACE;
                  iIdx = intervalIdx;
               }
               PoiList vSupportPoi;
               pPoi->GetPointsOfInterest(thisSegmentKey, attrib, &vSupportPoi);
               vPoi.emplace_back(vSupportPoi.front(),iIdx);
            }
            else
            {
               vPoi.emplace_back(poiTS,intervalIdx-1);
            }
         }
         else
         {
            pgsPointOfInterest poiTS = pPoi->GetTemporarySupportPointOfInterest(segmentKey,vTS.front()->GetIndex());
            if ( poiTS.HasAttribute(POI_CLOSURE) )
            {
               // temp support is at a closure joint.. this means the segment does not span across this TS. Since
               // this TS is to the left of the pier, we want the CL Bearing at the start of the segment
               CSegmentKey thisSegmentKey(segmentKey);
               PoiAttributeType attrib = POI_0L | POI_ERECTED_SEGMENT;
               IntervalIndexType iIdx = intervalIdx-1;
               if ( bStrongbacksOnly && intervalIdx == erectionIntervalIdx )
               {
                  thisSegmentKey.segmentIndex--;
                  attrib = POI_END_FACE;
                  iIdx = intervalIdx;
               }
               PoiList vSupportPoi;
               pPoi->GetPointsOfInterest(thisSegmentKey, attrib, &vSupportPoi);
               vPoi.emplace_back(vSupportPoi.front(),iIdx);
            }
            else
            {
               vPoi.emplace_back(poiTS,intervalIdx-1);
            }

            if ( vPiers.front()->IsBoundaryPier() )
            {
               // this is a boundary pier and it is to the right of the temporary support, therefore it must be at the
               // end of the segment... we want the start CL Bearing poi
               PoiList vSupportPoi;
               pPoi->GetPointsOfInterest(segmentKey, POI_ERECTED_SEGMENT | POI_10L, &vSupportPoi);
               vPoi.emplace_back(vSupportPoi.front(), intervalIdx - 1);
            }
            else
            {
               // this is an interior pier so we want the CL Pier poi
               vPoi.emplace_back(pPoi->GetPierPointOfInterest(segmentKey,vPiers.front()->GetIndex()),intervalIdx-1);
            }
         }
      }
   }
   else
   {
      // segment is supported only by temporary supports
      std::vector<const CTemporarySupportData*> vTS = pSegment->GetTemporarySupports();
      vTS.erase(std::remove_if(vTS.begin(),vTS.end(),RemoveStrongbacksSupports),vTS.end());
      bool bStrongbacksOnly = false;
      if ( vTS.size() == 0 )
      {
         // segment is supported only by strongbacks
         vTS = pSegment->GetTemporarySupports();
         bStrongbacksOnly = true;
         ATLASSERT(vTS.size() == 2);
      }

      pgsPointOfInterest poiLeftTS  = pPoi->GetTemporarySupportPointOfInterest(segmentKey,vTS.front()->GetIndex());
      pgsPointOfInterest poiRightTS = pPoi->GetTemporarySupportPointOfInterest(segmentKey,vTS.back()->GetIndex());
      ATLASSERT(poiLeftTS != poiRightTS);

      if ( poiLeftTS.HasAttribute(POI_CLOSURE) )
      {
         // we want the POI at the start CL Bearing
         CSegmentKey thisSegmentKey(segmentKey);
         PoiAttributeType attrib = POI_0L | POI_ERECTED_SEGMENT;
         IntervalIndexType iIdx = intervalIdx-1;
         if ( bStrongbacksOnly && intervalIdx == erectionIntervalIdx )
         {
            thisSegmentKey.segmentIndex--;
            attrib = POI_END_FACE;
            iIdx = intervalIdx;
         }

         PoiList vSupportPoi;
         pPoi->GetPointsOfInterest(thisSegmentKey, attrib, &vSupportPoi);
         vPoi.emplace_back(vSupportPoi.front(),iIdx);
      }
      else
      {
         vPoi.emplace_back(poiLeftTS,intervalIdx-1);
      }

      if ( poiRightTS.HasAttribute(POI_CLOSURE) )
      {
         // we want the POI at the end CL Bearing
         CSegmentKey thisSegmentKey(segmentKey);
         PoiAttributeType attrib = POI_10L | POI_ERECTED_SEGMENT;
         IntervalIndexType iIdx = intervalIdx-1;
         if ( bStrongbacksOnly && intervalIdx == erectionIntervalIdx )
         {
            thisSegmentKey.segmentIndex++;
            attrib = POI_START_FACE;
            iIdx = intervalIdx;
         }
         PoiList vSupportPoi;
         pPoi->GetPointsOfInterest(thisSegmentKey, attrib, &vSupportPoi);
         vPoi.emplace_back(vSupportPoi.front(),iIdx);
      }
      else
      {
         vPoi.emplace_back(poiRightTS,intervalIdx-1);
      }
   }

   ATLASSERT(vPoi.size() == 2);
   return vPoi;
}

void CAnalysisAgentImp::GetRawPrecamber(const pgsPointOfInterest& poi, Float64 Ls,Float64* pD,Float64* pR) const
{
   // gets the precamber using a simple parabola. assuming zero at 0.0 and Ls.
   // Ls is passed in so we don't have to get it in GetPrecamber and in this method
   const CSegmentKey& segmentKey = poi.GetSegmentKey();
   Float64 precamber = GetPrecamber(segmentKey);
   if (IsZero(precamber))
   {
      *pD = 0;
      *pR = 0;
      return;
   }

   Float64 Xpoi = poi.GetDistFromStart();

   *pD = (4 * precamber / Ls)*Xpoi*(1 - Xpoi / Ls);
   *pR = (4 * precamber / Ls)*(1 - 2 * Xpoi / Ls);
}

IntervalIndexType CAnalysisAgentImp::GetErectionInterval(const PoiList& vPoi) const
{
   GET_IFACE(IIntervals, pIntervals);
   GET_IFACE(IPointOfInterest, pPoi);
   std::vector<CSegmentKey> vSegments;
   pPoi->GetSegmentKeys(vPoi, &vSegments);
   IntervalIndexType erectionIntervalIdx;
   if (vSegments.size() == 1)
   {
      erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vSegments.front());
   }
   else
   {
      CGirderKey girderKey(vPoi.front().get().GetSegmentKey());
      erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   }

   return erectionIntervalIdx;
}