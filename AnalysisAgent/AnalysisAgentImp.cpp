///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
#include "AnalysisAgent_i.h"
#include "AnalysisAgentImp.h"
#include "StatusItems.h"
#include "..\PGSuperException.h"

#include <IFace\Intervals.h>

#include <WBFLSTL.h>
#include <MathEx.h>

#include <PgsExt\LoadFactors.h>
#include <PgsExt\DebondUtil.h>
#include <PgsExt\GirderModelFactory.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderMaterial.h>
#include <PgsExt\StrandData.h>

//////////////////////////////////////////////////////////////////////////////////
// NOTES:
//
// The pedestrian live load distribution factor is the pedestrian load.

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

DECLARE_LOGFILE;

// FEM Loading IDs
const LoadCaseIDType g_lcidGirder          = 1;
const LoadCaseIDType g_lcidStraightStrand  = 2;
const LoadCaseIDType g_lcidHarpedStrand    = 3;
const LoadCaseIDType g_lcidTemporaryStrand = 4;

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
   m_pSegmentModelManager = std::auto_ptr<CSegmentModelManager>(new CSegmentModelManager(LOGGER,m_pBroker));

   CGirderModelManager* pOldGirderModels = m_pGirderModelManager.release();
   m_pGirderModelManager = std::auto_ptr<CGirderModelManager>(new CGirderModelManager(LOGGER,m_pBroker,m_StatusGroupID));

#if defined _USE_MULTITHREADING
   m_ThreadManager.CreateThread(CAnalysisAgentImp::DeleteSegmentModelManager,(LPVOID)(pOldSegmentModels));
   m_ThreadManager.CreateThread(CAnalysisAgentImp::DeleteGirderModelManager, (LPVOID)(pOldGirderModels));
#else
   CAnalysisAgentImp::DeleteSegmentModelManager((LPVOID)pOldSegmentModels);
   CAnalysisAgentImp::DeleteGirderModelManager((LPVOID)pOldGirderModels);
#endif

   InvalidateCamberModels();

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
}

void CAnalysisAgentImp::InvalidateCamberModels()
{
   m_PrestressDeflectionModels.clear();
   m_InitialTempPrestressDeflectionModels.clear();
   m_ReleaseTempPrestressDeflectionModels.clear();
}

void CAnalysisAgentImp::ValidateCamberModels(const CSegmentKey& segmentKey)
{
   GDRCONFIG dummy_config;

   CamberModelData camber_model_data;
   BuildCamberModel(segmentKey,false,dummy_config,&camber_model_data);

   m_PrestressDeflectionModels.insert( std::make_pair(segmentKey,camber_model_data) );

   CamberModelData initial_temp_beam;
   CamberModelData release_temp_beam;
   BuildTempCamberModel(segmentKey,false,dummy_config,&initial_temp_beam,&release_temp_beam);
   m_InitialTempPrestressDeflectionModels.insert( std::make_pair(segmentKey,initial_temp_beam) );
   m_ReleaseTempPrestressDeflectionModels.insert( std::make_pair(segmentKey,release_temp_beam) );
}

CAnalysisAgentImp::CamberModelData CAnalysisAgentImp::GetPrestressDeflectionModel(const CSegmentKey& segmentKey,CamberModels& models)
{
   CamberModels::iterator found;
   found = models.find( segmentKey );

   if ( found == models.end() )
   {
      ValidateCamberModels(segmentKey);
      found = models.find( segmentKey );
   }

   // Model should have already been created in ValidateCamberModels
   ATLASSERT( found != models.end() );

   return (*found).second;
}

Float64 g_Ls;
bool RemoveOffSegmentPOI(const pgsPointOfInterest& poi)
{
   return !::InRange(0.0,poi.GetDistFromStart(),g_Ls);
}

void CAnalysisAgentImp::BuildCamberModel(const CSegmentKey& segmentKey,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pModelData)
{
#pragma Reminder("UPDATE: IS THIS CODE NEEDED? THIS DUPLICATES SEGMENT MODEL MANAGER")
   Float64 Ms;  // Concentrated moments at straight strand debond location
   Float64 Msl;   // Concentrated moments at straight strand bond locations (left)
   Float64 Msr;   // Concentrated moments at straight strand bond locations (right)
   Float64 Mhl;   // Concentrated moments at ends of beam for eccentric prestress forces from harped strands (left)
   Float64 Mhr;   // Concentrated moments at ends of beam for eccentric prestress forces from harped strands (right)
   Float64 Nl;   // Vertical loads at left harping point
   Float64 Nr;   // Vertical loads at right harping point
   Float64 Ps;   // Force in straight strands (varies with location due to debonding)
   Float64 Ph;   // Force in harped strands
   Float64 ecc_harped_start; // Eccentricity of harped strands at end of girder
   Float64 ecc_harped_end; // Eccentricity of harped strands at end of girder
   Float64 ecc_harped_hp1;  // Eccentricity of harped strand at harping point (left)
   Float64 ecc_harped_hp2;  // Eccentricity of harped strand at harping point (right)
   Float64 ecc_straight_start;   // Eccentricity of straight strands (left)
   Float64 ecc_straight_end;   // Eccentricity of straight strands (right)
   Float64 ecc_straight_debond;   // Eccentricity of straight strands (location varies)
   Float64 hp1; // Location of left harping point
   Float64 hp2; // Location of right harping point
   Float64 Lg;  // Length of girder

   // These are the interfaces we will be using
   GET_IFACE(IPretensionForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   Float64 E;
   if ( bUseConfig )
   {
      if ( config.bUserEci )
      {
         E = config.Eci;
      }
      else
      {
         E = pMaterial->GetEconc(config.Fci,pMaterial->GetSegmentStrengthDensity(segmentKey),pMaterial->GetSegmentEccK1(segmentKey),pMaterial->GetSegmentEccK2(segmentKey));
      }
   }
   else
   {
      E = pMaterial->GetSegmentEc(segmentKey,releaseIntervalIdx);
   }

   //
   // Create the FEM model (includes girder dead load)
   //
   Lg = pBridge->GetSegmentLength(segmentKey);

   std::vector<pgsPointOfInterest> vPOI( pIPoi->GetPointsOfInterest(segmentKey) );
   g_Ls = Lg;
   vPOI.erase(std::remove_if(vPOI.begin(),vPOI.end(),RemoveOffSegmentPOI),vPOI.end());

   GET_IFACE(ISegmentLiftingPointsOfInterest,pLiftPOI);
   std::vector<pgsPointOfInterest> liftingPOI( pLiftPOI->GetLiftingPointsOfInterest(segmentKey,0) );

   GET_IFACE(ISegmentHaulingPointsOfInterest,pHaulPOI);
   std::vector<pgsPointOfInterest> haulingPOI( pHaulPOI->GetHaulingPointsOfInterest(segmentKey,0) );

   vPOI.insert(vPOI.end(),liftingPOI.begin(),liftingPOI.end());
   vPOI.insert(vPOI.end(),haulingPOI.begin(),haulingPOI.end());
   std::sort(vPOI.begin(),vPOI.end());

   vPOI.erase(std::unique(vPOI.begin(),vPOI.end()), vPOI.end() );

   pgsGirderModelFactory().CreateGirderModel(m_pBroker,releaseIntervalIdx,segmentKey,0.0,Lg,E,g_lcidGirder,false,vPOI,&pModelData->Model,&pModelData->PoiMap);

   //
   // Apply the loads due to prestressing (use prestress force at mid-span)
   //

   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   pModelData->Model->get_Loadings(&loadings);

   loadings->Create(g_lcidHarpedStrand,&loading);

   CComPtr<IFem2dPointLoadCollection> pointLoads;
   loading->get_PointLoads(&pointLoads);
   LoadIDType ptLoadID;
   pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

   vPOI = pIPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_5L,POIFIND_AND);
   ATLASSERT( vPOI.size() == 1 );
   pgsPointOfInterest mid_span_poi( vPOI.front() );

#if defined _DEBUG
   ATLASSERT( mid_span_poi.IsMidSpan(POI_ERECTED_SEGMENT) == true );
#endif

   pgsPointOfInterest poiStart(segmentKey,0.0);
   pgsPointOfInterest poiEnd(segmentKey,Lg);


   // Start with harped strands because they are easiest (assume no debonding)
   hp1 = 0;
   hp2 = 0;
   Nl = 0;
   Nr = 0;
   Mhl = 0;
   Mhr = 0;


   StrandIndexType Nh = (bUseConfig ? config.PrestressConfig.GetStrandCount(pgsTypes::Harped) : pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Harped));
   if ( 0 < Nh )
   {
      // Determine the prestress force
      if ( bUseConfig )
      {
         Ph = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Harped,releaseIntervalIdx,pgsTypes::End,config);
      }
      else
      {
         Ph = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Harped,releaseIntervalIdx,pgsTypes::End);
      }

      // get harping point locations
      vPOI.clear(); // recycle the vector
      vPOI = pIPoi->GetPointsOfInterest(segmentKey,POI_HARPINGPOINT);
      ATLASSERT( 0 <= vPOI.size() && vPOI.size() < 3 );
      pgsPointOfInterest hp1_poi;
      pgsPointOfInterest hp2_poi;
      if ( vPOI.size() == 0 )
      {
         hp1_poi.SetSegmentKey(segmentKey);
         hp1_poi.SetDistFromStart(0.0);
         hp2_poi.SetSegmentKey(segmentKey);
         hp2_poi.SetDistFromStart(0.0);
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }
      else if ( vPOI.size() == 1 )
      { 
         std::vector<pgsPointOfInterest>::const_iterator iter( vPOI.begin() );
         hp1_poi = *iter++;
         hp2_poi = hp1_poi;
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }
      else
      {
         std::vector<pgsPointOfInterest>::const_iterator iter( vPOI.begin() );
         hp1_poi = *iter++;
         hp2_poi = *iter++;
         hp1 = hp1_poi.GetDistFromStart();
         hp2 = hp2_poi.GetDistFromStart();
      }

      // Determine eccentricity of harped strands at end and harp point
      // (assumes eccentricities are the same at each harp point - which they are because
      // of the way the input is defined)
      Float64 nHs_effective;

      if ( bUseConfig )
      {
         ecc_harped_start = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, config, pgsTypes::Harped, &nHs_effective);
         ecc_harped_hp1   = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp1_poi,  config, pgsTypes::Harped, &nHs_effective);
         ecc_harped_hp2   = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp2_poi,  config, pgsTypes::Harped, &nHs_effective);
         ecc_harped_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   config, pgsTypes::Harped, &nHs_effective);
      }
      else
      {
         ecc_harped_start = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Harped, &nHs_effective);
         ecc_harped_hp1   = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp1_poi,  pgsTypes::Harped, &nHs_effective);
         ecc_harped_hp2   = pStrandGeom->GetEccentricity(releaseIntervalIdx, hp2_poi,  pgsTypes::Harped, &nHs_effective);
         ecc_harped_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   pgsTypes::Harped, &nHs_effective);
      }

      // Determine equivalent loads

      // moment
      Mhr = Ph*ecc_harped_start;
      Mhl = Ph*ecc_harped_end;

      // upward force
      Float64 e_prime_start, e_prime_end;
      e_prime_start = ecc_harped_hp1 - ecc_harped_start;
      e_prime_start = IsZero(e_prime_start) ? 0 : e_prime_start;

      e_prime_end = ecc_harped_hp2 - ecc_harped_end;
      e_prime_end = IsZero(e_prime_end) ? 0 : e_prime_end;

      Nl = IsZero(hp1)    ? 0 : Ph*e_prime_start/hp1;
      Nr = IsZero(Lg-hp2) ? 0 : Ph*e_prime_end/(Lg-hp2);
   }

   // add loads to the model
   CComPtr<IFem2dPointLoad> ptLoad;
   MemberIDType mbrID;
   Float64 x;
   pgsGirderModelFactory::FindMember(pModelData->Model,0.00,&mbrID,&x);
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Mhl,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pModelData->Model,Lg,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,-Mhr,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pModelData->Model,hp1,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,Nl,0.00,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pModelData->Model,hp2,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,Nr,0.00,lotGlobal,&ptLoad);

   //
   // Add the effects of the straight strands
   //
   loading.Release();
   pointLoads.Release();

   loadings->Create(g_lcidStraightStrand,&loading);
   loading->get_PointLoads(&pointLoads);
   pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

   // start with the ends of the girder

   // the prestress force varies over the length of the girder.. use the mid-span value as an average
   Float64 nSsEffective;
   if ( bUseConfig )
   {
      ecc_straight_start = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, config, pgsTypes::Straight, &nSsEffective);
      ecc_straight_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   config, pgsTypes::Straight, &nSsEffective);
      Ps = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End,config);
   }
   else
   {
      StrandIndexType Ns = pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Straight);
      ecc_straight_start = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiStart, pgsTypes::Straight, &nSsEffective);
      ecc_straight_end   = pStrandGeom->GetEccentricity(releaseIntervalIdx, poiEnd,   pgsTypes::Straight, &nSsEffective);
      Ps = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End);
   }
   Msl = Ps*ecc_straight_start;
   Msr = Ps*ecc_straight_end;

   pgsGirderModelFactory::FindMember(pModelData->Model,0.00,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Msl,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pModelData->Model,Lg,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,-Msr,lotGlobal,&ptLoad);

   // now do it at the debond sections
   if ( bUseConfig )
   {
      // use tool to extract section data
      CDebondSectionCalculator dbcomp(config.PrestressConfig.Debond[pgsTypes::Straight], Lg);

      // left end first
      Float64 sign = 1;
      SectionIndexType nSections = dbcomp.GetNumLeftSections();
      SectionIndexType sectionIdx = 0;
      for ( sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
      {
         StrandIndexType nDebondedAtSection;
         Float64 location;
         dbcomp.GetLeftSectionInfo(sectionIdx,&location,&nDebondedAtSection);

         // nDebonded is to be interpreted as the number of strands that become bonded at this section
         // (ok, not at this section but lt past this section)
         Float64 nSsEffective;

         Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(mid_span_poi,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End,config);
         ecc_straight_debond = pStrandGeom->GetEccentricity(releaseIntervalIdx, pgsPointOfInterest(segmentKey,location),config, pgsTypes::Straight, &nSsEffective);

         Ms = sign*Ps*ecc_straight_debond;

         pgsGirderModelFactory::FindMember(pModelData->Model,location,&mbrID,&x);
         ptLoad.Release();
         pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Ms,lotGlobal,&ptLoad);
      }

      // right end 
      sign = -1;
      nSections = dbcomp.GetNumRightSections();
      for ( sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
      {
         StrandIndexType nDebondedAtSection;
         Float64 location;
         dbcomp.GetRightSectionInfo(sectionIdx,&location,&nDebondedAtSection);

         Float64 nSsEffective;

         Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(mid_span_poi,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End,config);
         ecc_straight_debond = pStrandGeom->GetEccentricity(releaseIntervalIdx, pgsPointOfInterest(segmentKey,location),config, pgsTypes::Straight, &nSsEffective);

         Ms = sign*Ps*ecc_straight_debond;

         pgsGirderModelFactory::FindMember(pModelData->Model,location,&mbrID,&x);
         ptLoad.Release();
         pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Ms,lotGlobal,&ptLoad);
      }

   }
   else
   {
      for (int iside=0; iside<2; iside++)
      {
         // left end first, right second
         pgsTypes::MemberEndType endType = (pgsTypes::MemberEndType)iside;
         Float64 sign = (iside==0)?  1 : -1;
         StrandIndexType nSections = pStrandGeom->GetNumDebondSections(segmentKey,endType,pgsTypes::Straight);
         for ( Uint16 sectionIdx = 0; sectionIdx < nSections; sectionIdx++ )
         {
            Float64 location = pStrandGeom->GetDebondSection(segmentKey,endType,sectionIdx,pgsTypes::Straight);
            if ( location < 0 || Lg < location )
            {
               continue; // bond occurs after the end of the girder... skip this one
            }

            StrandIndexType nDebondedAtSection = pStrandGeom->GetNumDebondedStrandsAtSection(segmentKey,endType,sectionIdx,pgsTypes::Straight);

            // nDebonded is to be interperted as the number of strands that become bonded at this section
            // (ok, not at this section but lt past this section)
            Float64 nSsEffective;

            Ps = nDebondedAtSection*pPrestressForce->GetPrestressForcePerStrand(mid_span_poi,pgsTypes::Straight,releaseIntervalIdx,pgsTypes::End);
            ecc_straight_debond = pStrandGeom->GetEccentricity(releaseIntervalIdx, pgsPointOfInterest(segmentKey,location), pgsTypes::Straight, &nSsEffective);

            Ms = sign*Ps*ecc_straight_debond;

            pgsGirderModelFactory::FindMember(pModelData->Model,location,&mbrID,&x);
            ptLoad.Release();
            pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Ms,lotGlobal,&ptLoad);
         }
      }
   }
}

void CAnalysisAgentImp::BuildTempCamberModel(const CSegmentKey& segmentKey,bool bUseConfig,const GDRCONFIG& config,CamberModelData* pInitialModelData,CamberModelData* pReleaseModelData)
{
   Float64 Mi;   // Concentrated moments at ends of beams for eccentric prestress forces
   Float64 Mr;
   Float64 Pti;  // Prestress force in temporary strands initially
   Float64 Ptr;  // Prestress force in temporary strands when removed
   Float64 ecc; // Eccentricity of the temporary strands
   std::vector<pgsPointOfInterest> vPOI; // Vector of points of interest

   // These are the interfaces we will be using
   GET_IFACE(IPretensionForce,pPrestressForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(IPointOfInterest,pIPoi);
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType tsInstallIntervalIdx = pIntervals->GetTemporaryStrandInstallationInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tsRemovalIntervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   if ( tsRemovalIntervalIdx == INVALID_INDEX )
   {
      // there aren't any temporary strands so just fake it
      tsRemovalIntervalIdx = erectSegmentIntervalIdx;
   }

   // Build models
   Float64 L;
   Float64 Ec;
   Float64 Eci;
   L = pBridge->GetSegmentLength(segmentKey);

   if ( bUseConfig )
   {
      if ( config.bUserEci )
      {
         Eci = config.Eci;
      }
      else
      {
         Eci = pMaterial->GetEconc(config.Fci,pMaterial->GetSegmentStrengthDensity(segmentKey),pMaterial->GetSegmentEccK1(segmentKey),pMaterial->GetSegmentEccK2(segmentKey));
      }

      if ( config.bUserEc )
      {
         Ec = config.Ec;
      }
      else
      {
         Ec = pMaterial->GetEconc(config.Fc,pMaterial->GetSegmentStrengthDensity(segmentKey),pMaterial->GetSegmentEccK1(segmentKey),pMaterial->GetSegmentEccK2(segmentKey));
      }
   }
   else
   {
      Eci = pMaterial->GetSegmentEc(segmentKey,releaseIntervalIdx);
      Ec  = pMaterial->GetSegmentEc(segmentKey,tsRemovalIntervalIdx);
   }

   vPOI = pIPoi->GetPointsOfInterest(segmentKey);
   vPOI.erase(std::remove_if(vPOI.begin(),vPOI.end(),RemoveOffSegmentPOI),vPOI.end());

   GET_IFACE(ISegmentLiftingPointsOfInterest,pLiftPOI);
   std::vector<pgsPointOfInterest> liftingPOI( pLiftPOI->GetLiftingPointsOfInterest(segmentKey,0) );

   GET_IFACE(ISegmentHaulingPointsOfInterest,pHaulPOI);
   std::vector<pgsPointOfInterest> haulingPOI( pHaulPOI->GetHaulingPointsOfInterest(segmentKey,0) );

   vPOI.insert(vPOI.end(),liftingPOI.begin(),liftingPOI.end());
   vPOI.insert(vPOI.end(),haulingPOI.begin(),haulingPOI.end());
   std::sort(vPOI.begin(),vPOI.end());
   vPOI.erase(std::unique(vPOI.begin(),vPOI.end()), vPOI.end() );

   pgsGirderModelFactory().CreateGirderModel(m_pBroker,releaseIntervalIdx,   segmentKey,0.0,L,Eci,g_lcidGirder,false,vPOI,&pInitialModelData->Model,&pInitialModelData->PoiMap);
   pgsGirderModelFactory().CreateGirderModel(m_pBroker,tsRemovalIntervalIdx, segmentKey,0.0,L,Ec, g_lcidGirder,false,vPOI,&pReleaseModelData->Model,&pReleaseModelData->PoiMap);

   // Determine the prestress forces and eccentricities
   vPOI = pIPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_5L);
   ATLASSERT( vPOI.size() != 0 );
   const pgsPointOfInterest& mid_span_poi = vPOI.front();

   GET_IFACE(ISegmentData,pSegmentData);
   const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

   pgsTypes::TTSUsage tempStrandUsage = (bUseConfig ? config.PrestressConfig.TempStrandUsage : pStrands->GetTemporaryStrandUsage());
   Float64 nTsEffective;

   if ( bUseConfig )
   {
      Pti = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End,config);
      Ptr = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End,config);
      ecc = pStrandGeom->GetEccentricity(tsInstallIntervalIdx, pgsPointOfInterest(segmentKey,0.00),config, pgsTypes::Temporary, &nTsEffective);
   }
   else
   {
      Pti = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Temporary,tsInstallIntervalIdx,pgsTypes::End);
      Ptr = pPrestressForce->GetPrestressForce(mid_span_poi,pgsTypes::Temporary,erectSegmentIntervalIdx,pgsTypes::End);
      ecc = pStrandGeom->GetEccentricity(tsInstallIntervalIdx, pgsPointOfInterest(segmentKey,0.00), pgsTypes::Temporary, &nTsEffective);
   }


   // Determine equivalent loads
   Mi = Pti*ecc;
   Mr = Ptr*ecc;


   // Apply loads to initial model
   CComPtr<IFem2dLoadingCollection> loadings;
   CComPtr<IFem2dLoading> loading;
   pInitialModelData->Model->get_Loadings(&loadings);

   loadings->Create(g_lcidTemporaryStrand,&loading);

   CComPtr<IFem2dPointLoadCollection> pointLoads;
   loading->get_PointLoads(&pointLoads);
   LoadIDType ptLoadID;
   pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

   CComPtr<IFem2dPointLoad> ptLoad;
   MemberIDType mbrID;
   Float64 x;
   pgsGirderModelFactory::FindMember(pInitialModelData->Model,0.00,&mbrID,&x);
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,Mi,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pInitialModelData->Model,L,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,-Mi,lotGlobal,&ptLoad);

   // apply loads to the release model
   ptLoadID = 0;
   loadings.Release();
   loading.Release();
   pReleaseModelData->Model->get_Loadings(&loadings);

   loadings->Create(g_lcidTemporaryStrand,&loading);

   pointLoads.Release();
   loading->get_PointLoads(&pointLoads);
   pointLoads->get_Count((CollectionIndexType*)&ptLoadID);

   pgsGirderModelFactory::FindMember(pReleaseModelData->Model,0.00,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00,-Mr,lotGlobal,&ptLoad);

   pgsGirderModelFactory::FindMember(pReleaseModelData->Model,L,&mbrID,&x);
   ptLoad.Release();
   pointLoads->Create(ptLoadID++,mbrID,x,0.00,0.00, Mr,lotGlobal,&ptLoad);
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
   pBrokerInit->RegInterface( IID_IPosttensionStresses,      this );
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

   m_pSegmentModelManager = std::auto_ptr<CSegmentModelManager>(new CSegmentModelManager(LOGGER,m_pBroker));
   m_pGirderModelManager  = std::auto_ptr<CGirderModelManager> (new CGirderModelManager (LOGGER,m_pBroker,m_StatusGroupID));

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

void CAnalysisAgentImp::GetSidewalkLoadFraction(const CSegmentKey& segmentKey,Float64* pSidewalkLoad,Float64* pFraLeft,Float64* pFraRight)
{
   m_pGirderModelManager->GetSidewalkLoadFraction(segmentKey,pSidewalkLoad,pFraLeft,pFraRight);
}

void CAnalysisAgentImp::GetTrafficBarrierLoadFraction(const CSegmentKey& segmentKey, Float64* pBarrierLoad,Float64* pFraExtLeft, Float64* pFraIntLeft,Float64* pFraExtRight,Float64* pFraIntRight)
{
   m_pGirderModelManager->GetTrafficBarrierLoadFraction(segmentKey,pBarrierLoad,pFraExtLeft,pFraIntLeft,pFraExtRight,pFraIntRight);
}


PoiIDType CAnalysisAgentImp::AddPointOfInterest(CamberModelData& models,const pgsPointOfInterest& poi)
{
   PoiIDType femPoiID = pgsGirderModelFactory::AddPointOfInterest(models.Model,poi);
   models.PoiMap.AddMap( poi, femPoiID );
   return femPoiID;
}

/////////////////////////////////////////////////////////////////////////////
// IProductForces
//
pgsTypes::BridgeAnalysisType CAnalysisAgentImp::GetBridgeAnalysisType(pgsTypes::AnalysisType analysisType,pgsTypes::OptimizationType optimization)
{
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple     ? pgsTypes::SimpleSpan : 
                                       analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : 
                                       optimization == pgsTypes::Maximize ? pgsTypes::MaxSimpleContinuousEnvelope : pgsTypes::MinSimpleContinuousEnvelope);
   return bat;
}

pgsTypes::BridgeAnalysisType CAnalysisAgentImp::GetBridgeAnalysisType(pgsTypes::OptimizationType optimization)
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   return GetBridgeAnalysisType(analysisType,optimization);
}


Float64 CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetAxial(intervalIdx,pfType,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

sysSectionValue CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<sysSectionValue> results = GetShear(intervalIdx,pfType,vPoi,bat,resultsType);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetMoment(intervalIdx,pfType,vPoi,bat,resultsType);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetDeflection(intervalIdx,pfType,vPoi,bat,resultsType,bIncludeElevationAdjustment);
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results = GetRotation(intervalIdx,pfType,vPoi,bat,resultsType,bIncludeSlopeAdjustment);
   ATLASSERT(results.size() == 1);
   return results[0];
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop,fBot;
   GetStress(intervalIdx,pfType,vPoi,bat,resultsType,topLocation,botLocation,&fTop,&fBot);

   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);

   *pfTop = fTop[0];
   *pfBot = fBot[0];
}

LPCTSTR CAnalysisAgentImp::GetProductLoadName(ProductForceType pfType)
{
   // these are the names that are displayed to the user in the UI and reports
   // this must be in the same order as the ProductForceType enum
   static LPCTSTR strNames[] = 
   {
      _T("Girder"),
      _T("Construction"),
      _T("Slab"),
      _T("Slab Pad"),
      _T("Slab Panel"),
      _T("Diaphragm"),
      _T("Overlay"),
      _T("Sidewalk"),
      _T("Railing System"),
      _T("User DC"),
      _T("User DW"),
      _T("User LLIM"),
      _T("Shear Key"),
      _T("Pretensioning"),
      _T("Primary Post-tensioning"),
      _T("Secondary Effects"),
      _T("Creep"),
      _T("Shrinkage"),
      _T("Relaxation"),
      _T("Equiv Post-tensioning"),
      _T("Total Post-tensioning"),
      _T("Overlay (rating)")
   };

   // the direct lookup in the array is faster, however if the enum changes (number of values or order of values)
   // it isn't easily detectable... the switch/case below is slower but it can detect errors that result
   // from changing the enum
#if defined _DEBUG
   std::_tstring strName;
   switch(pfType)
   {
   case pftGirder:
      strName = _T("Girder");
      break;

   case pftConstruction:
      strName = _T("Construction");
      break;

   case pftSlab:
      strName = _T("Slab");
      break;

   case pftSlabPad:
      strName = _T("Slab Pad");
      break;

   case pftSlabPanel:
      strName = _T("Slab Panel");
      break;

   case pftDiaphragm:
      strName = _T("Diaphragm");
      break;

   case pftOverlay:
      strName = _T("Overlay");
      break;

   case pftSidewalk:
      strName = _T("Sidewalk");
      break;

   case pftTrafficBarrier:
      strName = _T("Railing System");
      break;

   case pftUserDC:
      strName = _T("User DC");
      break;

   case pftUserDW:
      strName = _T("User DW");
      break;

   case pftUserLLIM:
      strName = _T("User LLIM");
      break;

   case pftShearKey:
      strName = _T("Shear Key");
      break;

   case pftPretension:
      strName = _T("Pretensioning");
      break;

   case pftPrimaryPostTensioning:
      strName = _T("Primary Post-tensioning");
      break;

   case pftSecondaryEffects:
      strName = _T("Secondary Effects");
      break;

   case pftCreep:
      strName = _T("Creep");
      break;

   case pftShrinkage:
      strName = _T("Shrinkage");
      break;

   case pftRelaxation:
      strName = _T("Relaxation");
      break;

   case pftEquivPostTensioning:
      strName = _T("Equiv Post-tensioning");
      break;

   case pftTotalPostTensioning:
      strName = _T("Total Post-tensioning");
      break;

   case pftOverlayRating:
      strName = _T("Overlay (rating)");
      break;

   default:
      ATLASSERT(false); // is there a new type?
      strName = _T("");
      break;
   }

   ATLASSERT(strName == std::_tstring(strNames[pfType]));
#endif

   return strNames[pfType];
}

LPCTSTR CAnalysisAgentImp::GetLoadCombinationName(LoadingCombinationType loadCombo)
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

LPCTSTR CAnalysisAgentImp::GetLimitStateName(pgsTypes::LimitState limitState)
{
   // these are the names that are displayed to the user in the UI and reports
   // this must be in the same order as the LimitState enum
   static LPCTSTR strNames[] =
   {
      _T("Service I"),
      _T("Service IA"),
      _T("Service III"),
      _T("Strength I"),
      _T("Strength II"),
      _T("Fatigue I"),
      _T("Strength I (Inventory)"),
      _T("Strength I (Operating)"),
      _T("Service III (Inventory)"),
      _T("Service III (Operating)"),
      _T("Strength I (Legal - Routine)"),
      _T("Strength I (Legal - Special)"),
      _T("Service III (Legal - Routine)"),
      _T("Service III (Legal - Special)"),
      _T("Strength II (Routine Permit Rating)"),
      _T("Service I (Routine Permit Rating)"),
      _T("Strength II (Special Permit Rating)"),
      _T("Service I (Special Permit Rating)")
   };

   // the direct lookup in the array is faster, however if the enum changes (number of values or order of values)
   // it isn't easily detectable... the switch/case below is slower but it can detect errors that result
   // from changing the enum
#if defined _DEBUG
   std::_tstring strName;
   switch(limitState)
   {
      case pgsTypes::ServiceI:
         strName = _T("Service I");
         break;

      case pgsTypes::ServiceIA:
         strName = _T("Service IA");
         break;

      case pgsTypes::ServiceIII:
         strName = _T("Service III");
         break;

      case pgsTypes::StrengthI:
         strName = _T("Strength I");
         break;

      case pgsTypes::StrengthII:
         strName = _T("Strength II");
         break;

      case pgsTypes::FatigueI:
         strName = _T("Fatigue I");
         break;

      case pgsTypes::StrengthI_Inventory:
         strName = _T("Strength I (Inventory)");
         break;

      case pgsTypes::StrengthI_Operating:
         strName = _T("Strength I (Operating)");
         break;

      case pgsTypes::ServiceIII_Inventory:
         strName = _T("Service III (Inventory)");
         break;

      case pgsTypes::ServiceIII_Operating:
         strName = _T("Service III (Operating)");
         break;

      case pgsTypes::StrengthI_LegalRoutine:
         strName = _T("Strength I (Legal - Routine)");
         break;

      case pgsTypes::StrengthI_LegalSpecial:
         strName = _T("Strength I (Legal - Special)");
         break;

      case pgsTypes::ServiceIII_LegalRoutine:
         strName = _T("Service III (Legal - Routine)");
         break;

      case pgsTypes::ServiceIII_LegalSpecial:
         strName = _T("Service III (Legal - Special)");
         break;

      case pgsTypes::StrengthII_PermitRoutine:
         strName = _T("Strength II (Routine Permit Rating)");
         break;

      case pgsTypes::ServiceI_PermitRoutine:
         strName = _T("Service I (Routine Permit Rating)");
         break;

      case pgsTypes::StrengthII_PermitSpecial:
         strName = _T("Strength II (Special Permit Rating)");
         break;

      case pgsTypes::ServiceI_PermitSpecial:
         strName = _T("Service I (Special Permit Rating)");
         break;

      default:
         ATLASSERT(false); // SHOULD NEVER GET HERE
   }

   ATLASSERT(strName == std::_tstring(strNames[limitState]));
#endif

   return strNames[limitState];
}

void CAnalysisAgentImp::GetGirderSelfWeightLoad(const CSegmentKey& segmentKey,std::vector<GirderLoad>* pDistLoad,std::vector<DiaphragmLoad>* pPointLoad)
{
   m_pGirderModelManager->GetGirderSelfWeightLoad(segmentKey,pDistLoad,pPointLoad);
}

Float64 CAnalysisAgentImp::GetTrafficBarrierLoad(const CSegmentKey& segmentKey)
{
   return m_pGirderModelManager->GetTrafficBarrierLoad(segmentKey);
}

Float64 CAnalysisAgentImp::GetSidewalkLoad(const CSegmentKey& segmentKey)
{
   return m_pGirderModelManager->GetSidewalkLoad(segmentKey);
}

void CAnalysisAgentImp::GetOverlayLoad(const CSegmentKey& segmentKey,std::vector<OverlayLoad>* pOverlayLoads)
{
   m_pGirderModelManager->GetOverlayLoad(segmentKey,pOverlayLoads);
}

void CAnalysisAgentImp::GetConstructionLoad(const CSegmentKey& segmentKey,std::vector<ConstructionLoad>* pConstructionLoads)
{
   m_pGirderModelManager->GetConstructionLoad(segmentKey,pConstructionLoads);
}

void CAnalysisAgentImp::GetMainSpanSlabLoad(const CSegmentKey& segmentKey, std::vector<SlabLoad>* pSlabLoads)
{
   m_pGirderModelManager->GetMainSpanSlabLoad(segmentKey,pSlabLoads);
}

bool CAnalysisAgentImp::HasShearKeyLoad(const CGirderKey& girderKey)
{
   return m_pGirderModelManager->HasShearKeyLoad(girderKey);
}

void CAnalysisAgentImp::GetShearKeyLoad(const CSegmentKey& segmentKey,std::vector<ShearKeyLoad>* pLoads)
{
   m_pGirderModelManager->GetShearKeyLoad(segmentKey,pLoads);
}

bool CAnalysisAgentImp::HasPedestrianLoad()
{
   return m_pGirderModelManager->HasPedestrianLoad();
}

bool CAnalysisAgentImp::HasSidewalkLoad(const CGirderKey& girderKey)
{
   return m_pGirderModelManager->HasSidewalkLoad(girderKey);
}

bool CAnalysisAgentImp::HasPedestrianLoad(const CGirderKey& girderKey)
{
   return m_pGirderModelManager->HasPedestrianLoad(girderKey);
}

Float64 CAnalysisAgentImp::GetPedestrianLoad(const CSegmentKey& segmentKey)
{
   return m_pGirderModelManager->GetPedestrianLoad(segmentKey);
}

Float64 CAnalysisAgentImp::GetPedestrianLoadPerSidewalk(pgsTypes::TrafficBarrierOrientation orientation)
{
   return m_pGirderModelManager->GetPedestrianLoadPerSidewalk(orientation);
}

void CAnalysisAgentImp::GetCantileverSlabLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)
{
   m_pGirderModelManager->GetCantileverSlabLoad(segmentKey,pP1,pM1,pP2,pM2);
}

void CAnalysisAgentImp::GetCantileverSlabPadLoad(const CSegmentKey& segmentKey, Float64* pP1, Float64* pM1, Float64* pP2, Float64* pM2)
{
   m_pGirderModelManager->GetCantileverSlabPadLoad(segmentKey,pP1,pM1,pP2,pM2);
}

void CAnalysisAgentImp::GetPrecastDiaphragmLoads(const CSegmentKey& segmentKey, std::vector<DiaphragmLoad>* pLoads)
{
   m_pGirderModelManager->GetPrecastDiaphragmLoads(segmentKey,pLoads);
}

void CAnalysisAgentImp::GetIntermediateDiaphragmLoads(const CSpanKey& spanKey, std::vector<DiaphragmLoad>* pLoads)
{
   m_pGirderModelManager->GetIntermediateDiaphragmLoads(spanKey,pLoads);
}

void CAnalysisAgentImp::GetPierDiaphragmLoads( PierIndexType pierIdx, GirderIndexType gdrIdx, Float64* pPback, Float64 *pMback, Float64* pPahead, Float64* pMahead)
{
   m_pGirderModelManager->GetPierDiaphragmLoads(pierIdx,gdrIdx,pPback,pMback,pPahead,pMahead);
}

void CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
#pragma Reminder("UPDATE: eliminate this method")
   // Instead use GetDeflection(storageIntervalIdx,pftGirder,poi,bat,rtCumulative,false);
   // and GetRotation(storageIntervalIdx,pftGirder,poi,bat,rtCumulative,false);

   // NOTE: This function assumes that deflection due to girder self-weight is the only loading
   // that goes into the camber calculations
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   // we want the largest downward deflection. With (+) being up and (-) being down
   // we want the minimum (most negative) deflection
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);
   Float64 delta    = GetDeflection(storageIntervalIdx,pftGirder,poi,bat,rtCumulative,false);
   Float64 rotation = GetRotation(storageIntervalIdx,pftGirder,poi,bat,rtCumulative,false);

   *pDy = delta;
   *pRz = rotation;
}

Float64 CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetGirderDeflectionForCamber(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetGirderDeflectionForCamber(poi,config,&dy,&rz);
   return dy;
}

void CAnalysisAgentImp::GetGirderDeflectionForCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   // this is the deflection during storage based on the concrete properties input by the user
   Float64 delta, rotation;
   GetGirderDeflectionForCamber(poi,&delta,&rotation);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // we need to adjust the deflections for the concrete properties in config
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType storageIntervalIdx = pIntervals->GetStorageInterval(segmentKey);

   GET_IFACE(IMaterials,pMaterial);

   // get E used to compute delta and rotation
   Float64 Eoriginal = pMaterial->GetSegmentEc(segmentKey,storageIntervalIdx);

   // get E for the concrete properties in the config object
   Float64 Econfig;
   if ( config.bUserEci )
   {
      Econfig = config.Eci;
   }
   else
   {
      Econfig = pMaterial->GetEconc(config.Fci,pMaterial->GetSegmentStrengthDensity(segmentKey),
                                               pMaterial->GetSegmentEccK1(segmentKey),
                                               pMaterial->GetSegmentEccK2(segmentKey));
   }

   // adjust the deflections
   // deltaOriginal = K/Ix*Eoriginal
   // deltaConfig   = K/Ix*Econfig = (K/Ix*Eoriginal)(Eoriginal/Econfig) = deltaOriginal*(Eoriginal/Econfig)
   delta    *= (Eoriginal/Econfig);
   rotation *= (Eoriginal/Econfig);

   *pDy = delta;
   *pRz = rotation;
}

void CAnalysisAgentImp::GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck)
{
   m_pGirderModelManager->GetLiveLoadMoment(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,VehicleIndexType* pMminTruck,VehicleIndexType* pMmaxTruck)
{
   return m_pGirderModelManager->GetLiveLoadShear(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadDeflection(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   return m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,pierIdx,girderKey,pierFace,bat,bIncludeImpact,bIncludeLLDF,pTmin,pTmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::PierFaceType pierFace,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pTmin,Float64* pTmax,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,pierIdx,girderKey,pierFace,bat,bIncludeImpact,bIncludeLLDF,pTmin,pTmax,pRmin,pRmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,VehicleIndexType* pTopMinConfig,VehicleIndexType* pTopMaxConfig,VehicleIndexType* pBotMinConfig,VehicleIndexType* pBotMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadStress(intervalIdx,llType,poi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pTopMinConfig,pTopMaxConfig,pBotMinConfig,pBotMaxConfig);
}

std::vector<std::_tstring> CAnalysisAgentImp::GetVehicleNames(pgsTypes::LiveLoadType llType,const CGirderKey& girderKey)
{
   return m_pGirderModelManager->GetVehicleNames(llType,girderKey);
}

void CAnalysisAgentImp::GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pMmin,Float64* pMmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadMoment(intervalIdx,llType,vehicleIndex,poi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,sysSectionValue* pVmin,sysSectionValue* pVmax,AxleConfiguration* pMinLeftAxleConfig,AxleConfiguration* pMinRightAxleConfig,AxleConfiguration* pMaxLeftAxleConfig,AxleConfiguration* pMaxRightAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadShear(intervalIdx,llType,vehicleIndex,poi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMinLeftAxleConfig,pMinRightAxleConfig,pMaxLeftAxleConfig,pMaxRightAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pDmin,Float64* pDmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadDeflection(intervalIdx,llType,vehicleIndex,poi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadRotation(intervalIdx,llType,vehicleIndex,poi,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax,AxleConfiguration* pMinAxleConfigTop,AxleConfiguration* pMaxAxleConfigTop,AxleConfiguration* pMinAxleConfigBot,AxleConfiguration* pMaxAxleConfigBot)
{
   m_pGirderModelManager->GetVehicularLiveLoadStress(intervalIdx,llType,vehicleIndex,poi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pMinAxleConfigTop,pMaxAxleConfigTop,pMinAxleConfigBot,pMaxAxleConfigBot);
}

void CAnalysisAgentImp::GetDeflLiveLoadDeflection(DeflectionLiveLoadType type, const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax)
{
   m_pGirderModelManager->GetDeflLiveLoadDeflection(type,poi,bat,pDmin,pDmax);
}

Float64 CAnalysisAgentImp::GetDesignSlabMomentAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   // returns the difference in moment between the slab moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rkPPPartUniformLoad beam = GetDesignSlabModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 x = poi.GetDistFromStart() - start_size;
   sysSectionValue M = beam.ComputeMoment(x);

   ATLASSERT( IsEqual(M.Left(),M.Right()) );
   return M.Left();
}

Float64 CAnalysisAgentImp::GetDesignSlabDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDesignSlabDeflectionAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi,&dy,&rz);
   return dy;
}

void CAnalysisAgentImp::GetDesignSlabStressAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pfTop,Float64* pfBot)
{
   GET_IFACE(ISectionProperties,pSectProp);
   // returns the difference in top and bottom girder stress between the stresses caused by the current slab
   // and the input value.
   Float64 M = GetDesignSlabMomentAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckInterval = pIntervals->GetCastDeckInterval();

   Float64 Sbg = pSectProp->GetS(castDeckInterval,poi,pgsTypes::BottomGirder,fcgdr);
   Float64 Stg = pSectProp->GetS(castDeckInterval,poi,pgsTypes::TopGirder,   fcgdr);

   *pfTop = M/Stg;
   *pfBot = M/Sbg;
}

rkPPPartUniformLoad CAnalysisAgentImp::GetDesignSlabModel(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(ISectionProperties,pSectProp);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   Float64 Ig = pSectProp->GetIx( castDeckIntervalIdx, poi, fcgdr );

   Float64 E = pMaterial->GetEconc(fcgdr,pMaterial->GetSegmentStrengthDensity(segmentKey), 
                                         pMaterial->GetSegmentEccK1(segmentKey), 
                                         pMaterial->GetSegmentEccK2(segmentKey) );

   Float64 L = pBridge->GetSegmentSpanLength(segmentKey);

   Float64 w;
   bool bIsInteriorGirder = pBridge->IsInteriorGirder(segmentKey);
   if ( bIsInteriorGirder )
   {
      // change in main span loading only occurs for exterior girders
      w = 0;
   }
   else
   {
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();
      if ( pDeck->DeckType == pgsTypes::sdtNone || pDeck->OverhangTaper == pgsTypes::dotNone )
      {
         // there isn't a deck, or the overhang doesn't taper, so there isn't
         // a change in loading
         w = 0;
      }
      else
      {
         ATLASSERT( pDeck->OverhangTaper == pgsTypes::dotTopTopFlange || pDeck->OverhangTaper == pgsTypes::dotBottomTopFlange );
         // There is a triangular shape piece of deck concrete that isn't accounted for
         // in the main slab loading based on the input value because the "A" dimension has changed.
         // The slab overhang tapers to either the top or bottom of the top flange and the
         // distance to the top/bottom of the top flange depends on "A"

         // Compute the change in loading due to the change in "A" dimension

         PierIndexType startPierIdx, endPierIdx;
         pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
         ATLASSERT(endPierIdx == startPierIdx+1);

         // current value of "A" dimension
         Float64 AoStart = pBridge->GetSlabOffset(segmentKey.groupIndex,startPierIdx,segmentKey.girderIndex);
         Float64 AoEnd   = pBridge->GetSlabOffset(segmentKey.groupIndex,endPierIdx,  segmentKey.girderIndex);

         // change in overhang depth at the flange tip at the start/end of the girder
         // due to the design "A" dimension
         Float64 delta_overhang_depth_at_flange_tip_start = startSlabOffset - AoStart;
         Float64 delta_overhang_depth_at_flange_tip_end   = endSlabOffset   - AoEnd;

         // change in flange overhang depth at this poi
         // assuming a linear variation between start and end of span
         Float64 delta_overhang_depth_at_flange_tip = ::LinInterp(poi.GetDistFromStart(),delta_overhang_depth_at_flange_tip_start,delta_overhang_depth_at_flange_tip_end,L);

         // Determine the slab overhang at this poi
         Float64 station,offset;
         pBridge->GetStationAndOffset(poi,&station,&offset);
         Float64 dist_from_start_of_bridge = pBridge->GetDistanceFromStartOfBridge(station);

         // slab overhang from CL of girder (normal to alignment)
         Float64 slab_overhang = (segmentKey.girderIndex == 0 ? pBridge->GetLeftSlabOverhang(dist_from_start_of_bridge) : pBridge->GetRightSlabOverhang(dist_from_start_of_bridge));

         if (slab_overhang < 0.0)
         {
            // negative overhang - girder probably has no slab over it
            slab_overhang = 0.0;
         }
         else
         {
            GET_IFACE(IGirder,pGdr);
            Float64 top_width = pGdr->GetTopWidth(poi);

            // slab overhang from edge of girder (normal to alignment)
            slab_overhang -= top_width/2;
         }

         // cross sectional area of the missing dead load
         Float64 delta_slab_overhang_area = slab_overhang*delta_overhang_depth_at_flange_tip/2;

         Float64 density = pMaterial->GetDeckWeightDensity(castDeckIntervalIdx) ;

         w = delta_slab_overhang_area*density*unitSysUnitsMgr::GetGravitationalAcceleration();
      }
   }

#pragma Reminder("UPDATE: This is incorrect if girders are made continuous before slab casting")
#pragma Reminder("UPDATE: Don't have a roark beam for trapezoidal loads")
   rkPPPartUniformLoad beam(0,L,-w,L,E,Ig);
   return beam;
}

Float64 CAnalysisAgentImp::GetDesignSlabPadMomentAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   // returns the difference in moment between the slab pad moment for the current value of slab offset
   // and the input value. Adjustment is positive if the input slab offset is greater than the current value
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rkPPPartUniformLoad beam = GetDesignSlabPadModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 x = poi.GetDistFromStart() - start_size;
   sysSectionValue M = beam.ComputeMoment(x);

   ATLASSERT( IsEqual(M.Left(),M.Right()) );
   return M.Left();
}

Float64 CAnalysisAgentImp::GetDesignSlabPadDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDesignSlabPadDeflectionAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi,&dy,&rz);
   return dy;
}

void CAnalysisAgentImp::GetDesignSlabPadStressAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pfTop,Float64* pfBot)
{
   GET_IFACE(ISectionProperties,pSectProp);
   // returns the difference in top and bottom girder stress between the stresses caused by the current slab pad
   // and the input value.
   Float64 M = GetDesignSlabPadMomentAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckInterval = pIntervals->GetCastDeckInterval();

   Float64 Sbg = pSectProp->GetS(castDeckInterval,poi,pgsTypes::BottomGirder,fcgdr);
   Float64 Stg = pSectProp->GetS(castDeckInterval,poi,pgsTypes::TopGirder,   fcgdr);

   *pfTop = M/Stg;
   *pfBot = M/Sbg;
}

rkPPPartUniformLoad CAnalysisAgentImp::GetDesignSlabPadModel(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi)
{
   // Creates a simple beam analysis object. The loading is the change in slab pad weight due to the difference
   // in the slab pad weight from the user input and the slab pad weight due to the "A" dimension being
   // used in a design trial.
   //
   // The slab offset parameters passed into this method are the trial values.
   //
   // For example, the moment in the girder due to slab pad dead load during design is the moment due to the
   // original slab pad, minus the change in moment due to a smaller or larger slab pad. Instead of getting
   // change in moments, we model a change in loading.
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IGirder,pGdr);
   GET_IFACE(ISectionProperties,pSectProp);

   Float64 AdStart = startSlabOffset;
   Float64 AdEnd   = endSlabOffset;

   PierIndexType startPierIdx, endPierIdx;
   pBridge->GetGirderGroupPiers(segmentKey.groupIndex,&startPierIdx,&endPierIdx);
   ATLASSERT(endPierIdx == startPierIdx+1);

   Float64 AoStart = pBridge->GetSlabOffset(segmentKey.groupIndex,startPierIdx,segmentKey.girderIndex);
   Float64 AoEnd   = pBridge->GetSlabOffset(segmentKey.groupIndex,endPierIdx,  segmentKey.girderIndex);

   Float64 top_flange_width = pGdr->GetTopFlangeWidth( poi );

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   Float64 Ig = pSectProp->GetIx( castDeckIntervalIdx, poi, fcgdr );
   
   Float64 E = pMaterial->GetEconc(fcgdr,pMaterial->GetSegmentStrengthDensity(segmentKey), 
                                         pMaterial->GetSegmentEccK1(segmentKey), 
                                         pMaterial->GetSegmentEccK2(segmentKey) );

   Float64 L = pBridge->GetSegmentSpanLength(segmentKey);

#pragma Reminder("NOTE: This is incorrect if girders are made continuous before slab casting")
   // a simple span model is being created. if the girders are continuous before slab casting
   // the change in load due to the change in "A" dimension should be applied to a continuous model

   // Get change in "A" dimension at this poi
   Float64 density = pMaterial->GetDeckWeightDensity(castDeckIntervalIdx) ;
   Float64 deltaA = ::LinInterp(poi.GetDistFromStart(),AdStart-AoStart,AdEnd-AoEnd,L);
   Float64 w = deltaA * top_flange_width * density * unitSysUnitsMgr::GetGravitationalAcceleration();

#pragma Reminder("NOTE: Should be using a trapezoidal load.")

   rkPPPartUniformLoad beam(0,L,-w,L,E,Ig);
   return beam;
}

void CAnalysisAgentImp::DumpAnalysisModels(GirderIndexType gdrIdx)
{
   m_pSegmentModelManager->DumpAnalysisModels(gdrIdx);
   m_pGirderModelManager->DumpAnalysisModels(gdrIdx);
}

void CAnalysisAgentImp::GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64* pftop,Float64* pfbot)
{
   m_pGirderModelManager->GetDeckShrinkageStresses(poi,pftop,pfbot);
}

void CAnalysisAgentImp::GetDeckShrinkageStresses(const pgsPointOfInterest& poi,Float64 fcGdr,Float64* pftop,Float64* pfbot)
{
   m_pGirderModelManager->GetDeckShrinkageStresses(poi,fcGdr,pftop,pfbot);
}

std::vector<Float64> CAnalysisAgentImp::GetTimeStepPrestressAxial(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   ATLASSERT(pfType == pftPretension || pfType == pftTotalPostTensioning || pfType == pftPrimaryPostTensioning || pfType == pftSecondaryEffects);
   ATLASSERT(bat == pgsTypes::ContinuousSpan);

   std::vector<Float64> P;
   P.reserve(vPoi.size());

   if ( pfType == pftTotalPostTensioning )
   {
      // the deflections due to total post-tensioning is the sum of the primary and secondary effects
      std::vector<Float64> primary(  GetTimeStepPrestressAxial(intervalIdx,pftPrimaryPostTensioning,vPoi,bat,resultsType));
      std::vector<Float64> secondary(GetTimeStepPrestressAxial(intervalIdx,pftSecondaryEffects,     vPoi,bat,resultsType));

      std::transform(primary.begin(),primary.end(),secondary.begin(),std::back_inserter(P),std::plus<Float64>());
      return P;
   }


   GET_IFACE(ILosses,pILosses);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      const LOSSDETAILS* pLossDetails = pILosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails = pLossDetails->TimeStepDetails[intervalIdx];

      Float64 dP = 0;
      if ( resultsType == rtIncremental )
      {
         dP = tsDetails.dMi[pfType]; // incremental
      }
      else
      {
         dP = tsDetails.Mi[pfType]; // cumulative
      }
      P.push_back(dP);
   }

   return P;
}

std::vector<Float64> CAnalysisAgentImp::GetTimeStepPrestressMoment(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   ATLASSERT(pfType == pftPretension || pfType == pftTotalPostTensioning || pfType == pftPrimaryPostTensioning || pfType == pftSecondaryEffects);
   ATLASSERT(bat == pgsTypes::ContinuousSpan);

   std::vector<Float64> M;
   M.reserve(vPoi.size());

   if ( pfType == pftTotalPostTensioning )
   {
      // the deflections due to total post-tensioning is the sum of the primary and secondary effects
      std::vector<Float64> primary(  GetTimeStepPrestressMoment(intervalIdx,pftPrimaryPostTensioning,vPoi,bat,resultsType));
      std::vector<Float64> secondary(GetTimeStepPrestressMoment(intervalIdx,pftSecondaryEffects,     vPoi,bat,resultsType));

      std::transform(primary.begin(),primary.end(),secondary.begin(),std::back_inserter(M),std::plus<Float64>());
      return M;
   }


   GET_IFACE(ILosses,pILosses);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

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

void CAnalysisAgentImp::ApplyElevationAdjustment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,std::vector<Float64>* pDeflection1,std::vector<Float64>* pDeflection2)
{
   GET_IFACE(IBridge,pBridge);
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   std::vector<Float64> vAdjustments;
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Float64 elevAdj = pBridge->GetElevationAdjustment(intervalIdx,poi);
      vAdjustments.push_back(elevAdj);
   }

   if ( pDeflection1 )
   {
      std::transform(pDeflection1->begin(),pDeflection1->end(),vAdjustments.begin(),pDeflection1->begin(),std::plus<Float64>());
   }

   if ( pDeflection2 )
   {
      std::transform(pDeflection2->begin(),pDeflection2->end(),vAdjustments.begin(),pDeflection2->begin(),std::plus<Float64>());
   }
}

void CAnalysisAgentImp::ApplyRotationAdjustment(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,std::vector<Float64>* pDeflection1,std::vector<Float64>* pDeflection2)
{
   GET_IFACE(IBridge,pBridge);
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   std::vector<Float64> vAdjustments;
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;
      Float64 slopeAdj = pBridge->GetRotationAdjustment(intervalIdx,poi);
      vAdjustments.push_back(slopeAdj);
   }

   if ( pDeflection1 )
   {
      std::transform(pDeflection1->begin(),pDeflection1->end(),vAdjustments.begin(),pDeflection1->begin(),std::plus<Float64>());
   }

   if ( pDeflection2 )
   {
      std::transform(pDeflection2->begin(),pDeflection2->end(),vAdjustments.begin(),pDeflection2->begin(),std::plus<Float64>());
   }
}

std::_tstring CAnalysisAgentImp::GetLiveLoadName(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex)
{
   return m_pGirderModelManager->GetLiveLoadName(llType,vehicleIndex);
}

pgsTypes::LiveLoadApplicabilityType CAnalysisAgentImp::GetLiveLoadApplicability(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex)
{
   return m_pGirderModelManager->GetLiveLoadApplicability(llType,vehicleIndex);
}

VehicleIndexType CAnalysisAgentImp::GetVehicleCount(pgsTypes::LiveLoadType llType)
{
   return m_pGirderModelManager->GetVehicleCount(llType);
}

Float64 CAnalysisAgentImp::GetVehicleWeight(pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex)
{
   return m_pGirderModelManager->GetVehicleWeight(llType,vehicleIndex);
}

std::vector<EquivPretensionLoad> CAnalysisAgentImp::GetEquivPretensionLoads(const CSegmentKey& segmentKey,pgsTypes::StrandType strandType)
{
   return m_pSegmentModelManager->GetEquivPretensionLoads(segmentKey,strandType);
}

void CAnalysisAgentImp::GetEquivPostTensionLoads(const CGirderKey& girderKey,DuctIndexType ductIdx,std::vector<EquivPostTensionPointLoad>& ptLoads,std::vector<EquivPostTensionDistributedLoad>& distLoads,std::vector<EquivPostTensionMomentLoad>& momentLoads)
{
   m_pGirderModelManager->GetEquivPostTensionLoads(girderKey,ductIdx,ptLoads,distLoads,momentLoads);
}

/////////////////////////////////////////////////////////////////////////////
// IProductForces2
//
std::vector<Float64> CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   USES_CONVERSION;

   std::vector<Float64> results;

   if ( pfType == pftPretension || pfType == pftTotalPostTensioning || pfType == pftPrimaryPostTensioning || pfType == pftSecondaryEffects )
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
         std::vector<CSegmentKey> vSegmentKeys(pPoi->GetSegmentKeys(vPoi));
         ATLASSERT(vSegmentKeys.size() == 1); // this method assumes all the poi are for the same segment
#endif
         // This is not time-step analysis.
         // The pretension effects are handled in the segment and girder models for
         // elastic analysis... we want to use the code further down in this method.
         if ( pfType == pftPretension )
         {
            // for elastic analysis, force effects due to pretensioning are always those at release
            CSegmentKey segmentKey = vPoi.front().GetSegmentKey();
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            intervalIdx = releaseIntervalIdx;
         }

         // Post-tensioning is not modeled for linear elastic analysis, so the
         // results are zero
         if ( pfType == pftPrimaryPostTensioning || pfType == pftSecondaryEffects )
         {
            results.resize(vPoi.size(),0.0);
            return results;
         }
      }
   }

   if ( pfType == pftCreep || pfType == pftShrinkage || pfType == pftRelaxation )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
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
         std::transform(Athis.begin(),Athis.end(),Aprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetAxial(intervalIdx,pfType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   USES_CONVERSION;

   std::vector<sysSectionValue> results;

   if ( pfType == pftPretension || pfType == pftPrimaryPostTensioning )
   {
      // pre and post-tensioning don't cause external shear forces.
      // There is a vertical component of prestress, Vp, that is used
      // in shear analysis, but that isn't this...
      results.resize(vPoi.size(),sysSectionValue(0,0));
      return results;
   }

   // Secondary effects aren't computed in the LBAM model
   // Secondary = Total - Primary. Since the primary PT only
   // creates moment, not shear, its contribution is zero.
   // This leaves Secondary = Total. Total PT isn't in the
   // LBAM either, but results of Equiv post tensioning is
   // it is the same as total.
   if ( pfType == pftTotalPostTensioning || pfType == pftSecondaryEffects )
   {
      pfType = pftEquivPostTensioning;
   }

   if ( pfType == pftCreep || pfType == pftShrinkage || pfType == pftRelaxation )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
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
         std::transform(Vthis.begin(),Vthis.end(),Vprev.begin(),std::back_inserter(results),std::minus<sysSectionValue>());
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
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   USES_CONVERSION;

   std::vector<Float64> results;

   if ( pfType == pftPretension || pfType == pftTotalPostTensioning || pfType == pftPrimaryPostTensioning || pfType == pftSecondaryEffects )
   {
      GET_IFACE( ILossParameters, pLossParams);
      if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
      {
         return GetTimeStepPrestressMoment(intervalIdx,pfType,vPoi,bat,resultsType);
      }
      else
      {
#if defined _DEBUG
         //GET_IFACE(IPointOfInterest,pPoi);
         //std::vector<CSegmentKey> vSegmentKeys(pPoi->GetSegmentKeys(vPoi));
         //ATLASSERT(vSegmentKeys.size() == 1); // this method assumes all the poi are for the same segment
#endif
         // This is not time-step analysis.
         // The pretension effects are handled in the segment and girder models for
         // elastic analysis... we want to use the code further down in this method.
         if ( pfType == pftPretension )
         {
            // for elastic analysis, force effects due to pretensioning are always those at release
            CSegmentKey segmentKey = vPoi.front().GetSegmentKey();
            GET_IFACE(IIntervals,pIntervals);
            IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
            intervalIdx = releaseIntervalIdx;
         }

         // Post-tensioning is not modeled for linear elastic analysis, so the
         // results are zero
         if ( pfType == pftPrimaryPostTensioning || pfType == pftSecondaryEffects )
         {
            results.resize(vPoi.size(),0.0);
            return results;
         }
      }
   }

   if ( pfType == pftCreep || pfType == pftShrinkage || pfType == pftRelaxation )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
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
         std::transform(Mthis.begin(),Mthis.end(),Mprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetMoment(intervalIdx,pfType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys(pPoi->GetGirderKeys(vPoi));
   ATLASSERT(vGirderKeys.size() == 1); // this method assumes all the poi are for the same girder
#endif

   if ( pfType == pftCreep || pfType == pftShrinkage || pfType == pftRelaxation )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
   }

   std::vector<Float64> deflections;
   deflections.reserve(vPoi.size());

   if ( pfType == pftTotalPostTensioning )
   {
      pfType = pftEquivPostTensioning;
   }

   if ( pfType == pftSecondaryEffects || pfType == pftPrimaryPostTensioning )
   {
      deflections.resize(vPoi.size(),0.0);
   }
   else
   {
      try
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());

         if ( intervalIdx < erectionIntervalIdx )
         {
            // before erection - results are in the segment models
            deflections = m_pSegmentModelManager->GetDeflection(intervalIdx,pfType,vPoi,resultsType);
         }
         else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
         {
            // the incremental result at the time of erection is being requested. this is when
            // we switch between segment models and girder models. the incremental results
            // is the cumulative result this interval minus the cumulative result in the previous interval
            std::vector<Float64> Dprev = GetDeflection(intervalIdx-1,pfType,vPoi,bat,rtCumulative,false);
            std::vector<Float64> Dthis = GetDeflection(intervalIdx,  pfType,vPoi,bat,rtCumulative,false);
            std::transform(Dthis.begin(),Dthis.end(),Dprev.begin(),std::back_inserter(deflections),std::minus<Float64>());
         }
         else
         {
#pragma Reminder("UPDATE: this isn't the right deflection")
            // there is a problem with deflection like pftGirder. The deflection computed from
            // the girder model manager is the deflection of teh girder self weight using Ec. 
            // However the deflection is set based on Eci and then only changes because of
            // changing boundary conditions from release, lifting, storage, transportation, and
            // finally erection.
            //
            // Look at the camber details for a PGSuper analysis. You'll see that for the support location
            // being the same for release, storage, and erection, the girder self-weight
            // deflections are different. The modulus changing over time will not change the deflection,
            // however that is exactly what is happening in this analysis..
            //
            // The same issue occurs for rotations.

            deflections = m_pGirderModelManager->GetDeflection(intervalIdx,pfType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         Invalidate(false);
         throw;
      }
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,&deflections,NULL);
   }

   return deflections;
}

std::vector<Float64> CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys(pPoi->GetGirderKeys(vPoi));
   ATLASSERT(vGirderKeys.size() == 1); // this method assumes all the poi are for the same girder
#endif

   if ( pfType == pftCreep || pfType == pftShrinkage || pfType == pftRelaxation )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
   }

   std::vector<Float64> rotations;
   rotations.reserve(vPoi.size());

   if ( pfType == pftTotalPostTensioning )
   {
      pfType = pftEquivPostTensioning;
   }

   if ( pfType == pftSecondaryEffects || pfType == pftPrimaryPostTensioning )
   {
      rotations.resize(vPoi.size(),0.0);
   }
   else
   {
      try
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
         if ( intervalIdx < erectionIntervalIdx )
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
            std::transform(Rthis.begin(),Rthis.end(),Rprev.begin(),std::back_inserter(rotations),std::minus<Float64>());
         }
         else
         {
            rotations = m_pGirderModelManager->GetRotation(intervalIdx,pfType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         Invalidate(false);
         throw;
      }
   }

   if ( bIncludeSlopeAdjustment )
   {
      ApplyRotationAdjustment(intervalIdx,vPoi,&rotations,NULL);
   }

   return rotations;
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
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

void CAnalysisAgentImp::GetLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<VehicleIndexType>* pMminTruck,std::vector<VehicleIndexType>* pMmaxTruck)
{
   m_pGirderModelManager->GetLiveLoadMoment(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMminTruck,pMmaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<VehicleIndexType>* pMinTruck,std::vector<VehicleIndexType>* pMaxTruck)
{
   m_pGirderModelManager->GetLiveLoadShear(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMinTruck,pMaxTruck);
}

void CAnalysisAgentImp::GetLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadDeflection(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadRotation(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<VehicleIndexType>* pTopMinIndex,std::vector<VehicleIndexType>* pTopMaxIndex,std::vector<VehicleIndexType>* pBotMinIndex,std::vector<VehicleIndexType>* pBotMaxIndex)
{
   m_pGirderModelManager->GetLiveLoadStress(intervalIdx,llType,vPoi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pTopMinIndex,pTopMaxIndex,pBotMinIndex,pBotMaxIndex);
}

void CAnalysisAgentImp::GetVehicularLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadMoment(intervalIdx,llType,vehicleIndex,vPoi,bat,bIncludeImpact,bIncludeLLDF,pMmin,pMmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax,std::vector<AxleConfiguration>* pMinLeftAxleConfig,std::vector<AxleConfiguration>* pMinRightAxleConfig,std::vector<AxleConfiguration>* pMaxLeftAxleConfig,std::vector<AxleConfiguration>* pMaxRightAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadShear(intervalIdx,llType,vehicleIndex,vPoi,bat,bIncludeImpact,bIncludeLLDF,pVmin,pVmax,pMinLeftAxleConfig,pMinRightAxleConfig,pMaxLeftAxleConfig,pMaxRightAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax,std::vector<AxleConfiguration>* pMinAxleConfigTop,std::vector<AxleConfiguration>* pMaxAxleConfigTop,std::vector<AxleConfiguration>* pMinAxleConfigBot,std::vector<AxleConfiguration>* pMaxAxleConfigBot)
{
   m_pGirderModelManager->GetVehicularLiveLoadStress(intervalIdx,llType,vehicleIndex,vPoi,bat,bIncludeImpact,bIncludeLLDF,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax,pMinAxleConfigTop,pMaxAxleConfigTop,pMinAxleConfigBot,pMaxAxleConfigBot);
}

void CAnalysisAgentImp::GetVehicularLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadDeflection(intervalIdx,llType,vehicleIndex,vPoi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetVehicularLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadRotation(intervalIdx,llType,vehicleIndex,vPoi,bat,bIncludeImpact,bIncludeLLDF,pDmin,pDmax,pMinAxleConfig,pMaxAxleConfig);
}

/////////////////////////////////////////////////////////////////////////////
// ICombinedForces
//
sysSectionValue CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> V( GetShear(intervalIdx,comboType,vPoi,bat,resultsType) );

   ATLASSERT(V.size() == 1);

   return V.front();
}

Float64 CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetMoment(intervalIdx,comboType,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetDeflection(intervalIdx,comboType,vPoi,bat,resultsType,bIncludeElevationAdjustment) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

Float64 CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetRotation(intervalIdx,comboType,vPoi,bat,resultsType,bIncludeSlopeAdjustment) );
   ATLASSERT(results.size() == 1);
   return results.front();
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> fTop, fBot;
   GetStress(intervalIdx,comboType,vPoi,bat,resultsType,topLocation,botLocation,&fTop,&fBot);
   
   ATLASSERT(fTop.size() == 1 && fBot.size() == 1);

   *pfTop = fTop.front();
   *pfBot = fBot.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   GetCombinedLiveLoadMoment(intervalIdx,llType,vPoi,bat,&Mmin,&Mmax);

   ATLASSERT(Mmin.size() == 1 && Mmax.size() == 1);
   *pMin = Mmin.front();
   *pMax = Mmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,sysSectionValue* pVmin,sysSectionValue* pVmax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   GetCombinedLiveLoadShear(intervalIdx,llType,vPoi,bat,bIncludeImpact,&Vmin,&Vmax);

   ATLASSERT( Vmin.size() == 1 && Vmax.size() == 1 );
   *pVmin = Vmin.front();
   *pVmax = Vmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pDmin,Float64* pDmax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetCombinedLiveLoadDeflection(intervalIdx,llType,vPoi,bat,&Dmin,&Dmax);

   ATLASSERT( Dmin.size() == 1 && Dmax.size() == 1 );
   *pDmin = Dmin.front();
   *pDmax = Dmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pRmin,Float64* pRmax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Rmin, Rmax;
   GetCombinedLiveLoadRotation(intervalIdx,llType,vPoi,bat,&Rmin,&Rmax);

   ATLASSERT( Rmin.size() == 1 && Rmax.size() == 1 );
   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTopMin,Float64* pfTopMax,Float64* pfBotMin,Float64* pfBotMax)
{
   std::vector<pgsPointOfInterest> vPoi;
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
void CAnalysisAgentImp::ComputeTimeDependentEffects(const CGirderKey& girderKey,IntervalIndexType intervalIdx)
{
   // Getting the timestep loss results, causes the creep, shrinkage, and prestress forces
   // to be added to the LBAM model...
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetLossMethod() == LOSSES_TIME_STEP )
   {
      GET_IFACE(ILosses,pLosses);
      GET_IFACE(IPointOfInterest,pPoi);

      pgsPointOfInterest poi( pPoi->GetPointOfInterest(CSegmentKey(girderKey,0),0.0) );
      ATLASSERT(poi.GetID() != INVALID_ID);
      pLosses->GetLossDetails(poi,intervalIdx);
   }
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   USES_CONVERSION;

   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
   }

   std::vector<sysSectionValue> results;
   if ( comboType == lcPS )
   {
      // secondary effects were requested... the LBAM doesn't have secondary effects... get the product load
      // effects that feed into lcPS
      results.resize(vPoi.size(),0.0); // initilize the results vector with 0.0
      std::vector<ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      std::vector<ProductForceType>::iterator pfIter(pfTypes.begin());
      std::vector<ProductForceType>::iterator pfIterEnd(pfTypes.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         ProductForceType pfType = *pfIter;
         std::vector<sysSectionValue> V = GetShear(intervalIdx,pfType,vPoi,bat,resultsType);

         // add V to results and assign answer to results
         std::transform(V.begin(),V.end(),results.begin(),results.begin(),std::plus<sysSectionValue>());
      }
   }
   else
   {
      try
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
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
            std::transform(Vthis.begin(),Vthis.end(),Vprev.begin(),std::back_inserter(results),std::minus<sysSectionValue>());
         }
         else
         {
            results = m_pGirderModelManager->GetShear(intervalIdx,comboType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         Invalidate(false);
         throw;
      }
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   USES_CONVERSION;

   std::vector<Float64> results;
   if ( comboType == lcPS )
   {
      // secondary effects were requested... the LBAM doesn't have secondary effects... get the product load
      // effects that feed into lcPS
      results.resize(vPoi.size(),0.0); // initilize the results vector with 0.0
      std::vector<ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      std::vector<ProductForceType>::iterator pfIter(pfTypes.begin());
      std::vector<ProductForceType>::iterator pfIterEnd(pfTypes.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         ProductForceType pfType = *pfIter;
         std::vector<Float64> M = GetMoment(intervalIdx,pfType,vPoi,bat,resultsType);

         // add M to results and assign answer to results
         std::transform(M.begin(),M.end(),results.begin(),results.begin(),std::plus<Float64>());
      }

      return results;
   }

   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
   }

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
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
         std::transform(Mthis.begin(),Mthis.end(),Mprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetMoment(intervalIdx,comboType,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
   }

   std::vector<Float64> deflection;
   if ( comboType == lcPS )
   {
      // secondary effects aren't directly computed. get the individual product forces
      // and sum them up here
      deflection.resize(vPoi.size(),0.0);
      std::vector<ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      BOOST_FOREACH(ProductForceType pfType,pfTypes)
      {
         std::vector<Float64> delta = GetDeflection(intervalIdx,pfType,vPoi,bat,resultsType,false);
         std::transform(delta.begin(),delta.end(),deflection.begin(),deflection.begin(),std::plus<Float64>());
      }
   }
   else
   {
      try
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
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
            std::transform(Dthis.begin(),Dthis.end(),Dprev.begin(),std::back_inserter(deflection),std::minus<Float64>());
         }
         else
         {
            deflection = m_pGirderModelManager->GetDeflection(intervalIdx,comboType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         Invalidate(false);
         throw;
      }
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,&deflection,NULL);
   }

   return deflection;
}

std::vector<Float64> CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
   }

   std::vector<Float64> rotation;
   if ( comboType == lcPS )
   {
      // secondary effects aren't directly computed. get the individual product forces
      // and sum them up here
      rotation.resize(vPoi.size(),0.0);
      std::vector<ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      BOOST_FOREACH(ProductForceType pfType,pfTypes)
      {
         std::vector<Float64> delta = GetRotation(intervalIdx,pfType,vPoi,bat,resultsType,false);
         std::transform(delta.begin(),delta.end(),rotation.begin(),rotation.begin(),std::plus<Float64>());
      }
   }
   else
   {
      try
      {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(vPoi.front().GetSegmentKey());
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
            std::transform(Dthis.begin(),Dthis.end(),Dprev.begin(),std::back_inserter(rotation),std::minus<Float64>());
         }
         else
         {
            rotation = m_pGirderModelManager->GetRotation(intervalIdx,comboType,vPoi,bat,resultsType);
         }
      }
      catch(...)
      {
         // reset all of our data.
         Invalidate(false);
         throw;
      }
   }

   if ( bIncludeSlopeAdjustment )
   {
      ApplyRotationAdjustment(intervalIdx,vPoi,&rotation,NULL);
   }

   return rotation;
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
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

void CAnalysisAgentImp::GetCombinedLiveLoadMoment(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMmin,std::vector<Float64>* pMmax)
{
   m_pGirderModelManager->GetCombinedLiveLoadMoment(intervalIdx,llType,vPoi,bat,pMmin,pMmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadShear(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<sysSectionValue>* pVmin,std::vector<sysSectionValue>* pVmax)
{
   m_pGirderModelManager->GetCombinedLiveLoadShear(intervalIdx,llType,vPoi,bat,bIncludeImpact,pVmin,pVmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadDeflection(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pDmin,std::vector<Float64>* pDmax)
{
   m_pGirderModelManager->GetCombinedLiveLoadDeflection(intervalIdx,llType,vPoi,bat,pDmin,pDmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax)
{
   m_pGirderModelManager->GetCombinedLiveLoadRotation(intervalIdx,llType,vPoi,bat,pRmin,pRmax);
}

void CAnalysisAgentImp::GetCombinedLiveLoadStress(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTopMin,std::vector<Float64>* pfTopMax,std::vector<Float64>* pfBotMin,std::vector<Float64>* pfBotMax)
{
   m_pGirderModelManager->GetCombinedLiveLoadStress(intervalIdx,llType,vPoi,bat,topLocation,botLocation,pfTopMin,pfTopMax,pfBotMin,pfBotMax);
}

/////////////////////////////////////////////////////////////////////////////
// ILimitStateForces
//
void CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<sysSectionValue> Vmin, Vmax;
   GetShear(intervalIdx,limitState,vPoi,bat,&Vmin,&Vmax);

   ATLASSERT(Vmin.size() == 1);
   ATLASSERT(Vmax.size() == 1);

   *pMin = Vmin.front();
   *pMax = Vmax.front();
}

void CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Mmin, Mmax;
   GetMoment(intervalIdx,limitState,vPoi,bat,&Mmin,&Mmax);

   ATLASSERT(Mmin.size() == 1);
   ATLASSERT(Mmax.size() == 1);

   *pMin = Mmin.front();
   *pMax = Mmax.front();
}

void CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncPrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetDeflection(intervalIdx,limitState,vPoi,bat,bIncPrestress,bIncludeLiveLoad,bIncludeElevationAdjustment,&Dmin,&Dmax);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);

   *pMin = Dmin.front();
   *pMax = Dmax.front();
}

void CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Dmin, Dmax;
   GetRotation(intervalIdx,limitState,vPoi,bat,bIncludePrestress,bIncludeLiveLoad,bIncludeSlopeAdjustment,&Dmin,&Dmax);

   ATLASSERT(Dmin.size() == 1);
   ATLASSERT(Dmax.size() == 1);

   *pMin = Dmin.front();
   *pMax = Dmax.front();
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,Float64* pMin,Float64* pMax)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> Fmin, Fmax;
   GetStress(intervalIdx,limitState,vPoi,bat,bIncludePrestress,loc,&Fmin,&Fmax);

   ATLASSERT(Fmin.size() == 1);
   ATLASSERT(Fmax.size() == 1);

   *pMin = Fmin.front();
   *pMax = Fmax.front();
}

Float64 CAnalysisAgentImp::GetSlabDesignMoment(pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);

   std::vector<Float64> M = GetSlabDesignMoment(limitState,vPoi,bat);

   ATLASSERT(M.size() == vPoi.size());

   return M.front();
}

bool CAnalysisAgentImp::IsStrengthIIApplicable(const CGirderKey& girderKey)
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
void CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<sysSectionValue>* pMin,std::vector<sysSectionValue>* pMax)
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(segmentKey);
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
      Invalidate(false);
      throw;
   }


   // LBAM analysis doesn't include secondary effects... add them here

   // Load factors for secondary effects
   Float64 gPSMin, gPSMax;
   if ( ::IsRatingLimitState(limitState) )
   {
      GET_IFACE(IRatingSpecification,pRatingSpec);
      gPSMin = pRatingSpec->GetSecondaryEffectsFactor(limitState);
      gPSMax = gPSMin;
   }
   else
   {
      GET_IFACE(ILoadFactors,pILoadFactors);
      const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
      gPSMin = pLoadFactors->PSmin[limitState];
      gPSMax = pLoadFactors->PSmax[limitState];
   }

   std::vector<sysSectionValue> Vps = GetShear(intervalIdx,pftSecondaryEffects,vPoi,bat,rtCumulative);
   std::vector<sysSectionValue>::iterator VpsIter(Vps.begin());
   std::vector<sysSectionValue>::iterator VpsIterEnd(Vps.end());
   std::vector<sysSectionValue>::iterator minIter(pMin->begin());
   std::vector<sysSectionValue>::iterator maxIter(pMax->begin());
   for ( ; VpsIter != VpsIterEnd; VpsIter++, minIter++, maxIter++ )
   {
      (*minIter) += gPSMin*(*VpsIter);
      (*maxIter) += gPSMax*(*VpsIter);
   }
}

void CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(segmentKey);
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
      Invalidate(false);
      throw;
   }

   // LBAM analysis doesn't include secondary effects... add them here

   // Load factors for secondary effects
   Float64 gPSMin, gPSMax;
   if ( ::IsRatingLimitState(limitState) )
   {
      GET_IFACE(IRatingSpecification,pRatingSpec);
      gPSMin = pRatingSpec->GetSecondaryEffectsFactor(limitState);
      gPSMax = gPSMin;
   }
   else
   {
      GET_IFACE(ILoadFactors,pILoadFactors);
      const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
      gPSMin = pLoadFactors->PSmin[limitState];
      gPSMax = pLoadFactors->PSmax[limitState];
   }

   std::vector<Float64> Mps = GetMoment(intervalIdx,pftSecondaryEffects,vPoi,bat,rtCumulative);
   std::vector<Float64>::iterator MpsIter(Mps.begin());
   std::vector<Float64>::iterator MpsIterEnd(Mps.end());
   std::vector<Float64>::iterator MminIter(pMin->begin());
   std::vector<Float64>::iterator MmaxIter(pMax->begin());
   for ( ; MpsIter != MpsIterEnd; MpsIter++, MminIter++, MmaxIter++ )
   {
      (*MminIter) += gPSMin*(*MpsIter);
      (*MmaxIter) += gPSMax*(*MpsIter);
   }
}

std::vector<Float64> CAnalysisAgentImp::GetSlabDesignMoment(pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat)
{
   std::vector<Float64> Msd;
   try
   {
      // need to do the time-step analysis because it adds loads to the LBAM
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),INVALID_INDEX);

      Msd = m_pGirderModelManager->GetSlabDesignMoment(limitState,vPoi,bat);
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   // LBAM analysis doesn't include secondary effects... add them here

   // Load factors for secondary effects
   Float64 gPSMin;
   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   gPSMin = pLoadFactors->PSmin[limitState];

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType lastIntervalIdx = pIntervals->GetIntervalCount()-1;

   std::vector<Float64> Mps = GetMoment(lastIntervalIdx,pftSecondaryEffects,vPoi,bat,rtCumulative);
   std::vector<Float64>::iterator MpsIter(Mps.begin());
   std::vector<Float64>::iterator MpsIterEnd(Mps.end());
   std::vector<Float64>::iterator MsdIter(Msd.begin());
   for ( ; MpsIter != MpsIterEnd; MpsIter++, MsdIter++ )
   {
      (*MsdIter) += gPSMin*(*MpsIter);
   }

   return Msd;
}

void CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeElevationAdjustment,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys(pPoi->GetGirderKeys(vPoi));
   ATLASSERT(vGirderKeys.size() == 1); // this method assumes all the poi are for the same girder
#endif

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
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
      Invalidate(false);
      throw;
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,pMin,pMax);
   }
}

void CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,bool bIncludeLiveLoad,bool bIncludeSlopeAdjustment,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
#if defined _DEBUG
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<CGirderKey> vGirderKeys(pPoi->GetGirderKeys(vPoi));
   ATLASSERT(vGirderKeys.size() == 1); // this method assumes all the poi are for the same girder
#endif

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   // need to do the time-step analysis because it adds loads to the LBAM
   ComputeTimeDependentEffects(segmentKey,intervalIdx);

   pMin->clear();
   pMax->clear();

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
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
      Invalidate(false);
      throw;
   }

   if ( bIncludeSlopeAdjustment )
   {
      ApplyRotationAdjustment(intervalIdx,vPoi,pMin,pMax);
   }
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation loc,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
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

void CAnalysisAgentImp::GetReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pMin,Float64* pMax)
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
      Invalidate(false);
      throw;
   }
}

///////////////////////////////////////////////////////
// IExternalLoading
bool CAnalysisAgentImp::CreateLoading(GirderIndexType girderLineIdx,LPCTSTR strLoadingName)
{
   if ( !m_pSegmentModelManager->CreateLoading(girderLineIdx,strLoadingName) )
   {
      return false;
   }

   if ( !m_pGirderModelManager->CreateLoading(girderLineIdx,strLoadingName) )
   {
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

bool CAnalysisAgentImp::CreateConcentratedLoad(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi,Float64 Fx,Float64 Fy,Float64 Mz)
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
#pragma Reminder("UPDATE: this should be CreateDistributedLoad") 
   // distributed load is the more general version and it is supported by the FEM model...
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

bool CAnalysisAgentImp::CreateUniformLoad(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 wx,Float64 wy)
{
#pragma Reminder("UPDATE: this should be CreateDistributedLoad") 
   // distributed load is the more general version and it is supported by the FEM model...
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

bool CAnalysisAgentImp::CreateInitialStrainLoad(IntervalIndexType intervalIdx,ProductForceType pfType,const pgsPointOfInterest& poi1,const pgsPointOfInterest& poi2,Float64 e,Float64 r)
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

Float64 CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetAxial(intervalIdx,strLoadingName,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

sysSectionValue CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<sysSectionValue> results( GetShear(intervalIdx,strLoadingName,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetMoment(intervalIdx,strLoadingName,vPoi,bat,resultsType) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetDeflection(intervalIdx,strLoadingName,vPoi,bat,resultsType,bIncludeElevationAdjustment) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> results( GetRotation(intervalIdx,strLoadingName,vPoi,bat,resultsType,bIncludeSlopeAdjustment) );
   ATLASSERT(results.size() == 1);
   return results[0];
}

Float64 CAnalysisAgentImp::GetReaction(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetFirstSegmentErectionInterval(girderKey);
   if ( intervalIdx < erectionIntervalIdx )
   {
      return m_pSegmentModelManager->GetReaction(intervalIdx,strLoadingName,pierIdx,girderKey,resultsType);
   }
   else if ( intervalIdx == erectionIntervalIdx && resultsType == rtIncremental )
   {
      Float64 Rprev = GetReaction(intervalIdx-1,strLoadingName,pierIdx,girderKey,bat,resultsType);
      Float64 Rthis = GetReaction(intervalIdx,  strLoadingName,pierIdx,girderKey,bat,resultsType);
      return Rthis-Rprev;
   }
   else
   {
      return m_pGirderModelManager->GetReaction(intervalIdx,strLoadingName,pierIdx,girderKey,bat,resultsType);
   }
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,Float64* pfTop,Float64* pfBot)
{
   std::vector<pgsPointOfInterest> vPoi;
   vPoi.push_back(poi);
   std::vector<Float64> fTop,fBot;
   GetStress(intervalIdx,strLoadingName,vPoi,bat,resultsType,topLocation,botLocation,&fTop,&fBot);
   ATLASSERT(fTop.size() == 1);
   ATLASSERT(fBot.size() == 1);
   *pfTop = fTop[0];
   *pfBot = fBot[0];
}

std::vector<Float64> CAnalysisAgentImp::GetAxial(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<Float64> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
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
         std::transform(Athis.begin(),Athis.end(),Aprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetAxial(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<sysSectionValue> CAnalysisAgentImp::GetShear(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<sysSectionValue> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
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
         std::transform(Vthis.begin(),Vthis.end(),Vprev.begin(),std::back_inserter(results),std::minus<sysSectionValue>());
      }
      else
      {
         results = m_pGirderModelManager->GetShear(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetMoment(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType)
{
   std::vector<Float64> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
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
         std::transform(Mthis.begin(),Mthis.end(),Mprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetMoment(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetDeflection(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeElevationAdjustment)
{
   std::vector<Float64> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
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
         std::transform(Dthis.begin(),Dthis.end(),Dprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetDeflection(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   if ( bIncludeElevationAdjustment )
   {
      ApplyElevationAdjustment(intervalIdx,vPoi,&results,NULL);
   }

   return results;
}

std::vector<Float64> CAnalysisAgentImp::GetRotation(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,bool bIncludeSlopeAdjustment)
{
   std::vector<Float64> results;
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
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
         std::transform(Rthis.begin(),Rthis.end(),Rprev.begin(),std::back_inserter(results),std::minus<Float64>());
      }
      else
      {
         results = m_pGirderModelManager->GetRotation(intervalIdx,strLoadingName,vPoi,bat,resultsType);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }

   if ( bIncludeSlopeAdjustment )
   {
      ApplyRotationAdjustment(intervalIdx,vPoi,&results,NULL);
   }

   return results;
}

void CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,LPCTSTR strLoadingName,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
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
         std::transform(fTopThis.begin(),fTopThis.end(),fTopPrev.begin(),std::back_inserter(*pfTop),std::minus<Float64>());
         std::transform(fBotThis.begin(),fBotThis.end(),fBotPrev.begin(),std::back_inserter(*pfBot),std::minus<Float64>());
      }
      else
      {
         m_pGirderModelManager->GetStress(intervalIdx,strLoadingName,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

///////////////////////////
void CAnalysisAgentImp::GetDesignStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::StressLocation loc,Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,pgsTypes::BridgeAnalysisType bat,Float64* pMin,Float64* pMax)
{
   // Computes design-time stresses due to external loads
   ATLASSERT(IsGirderStressLocation(loc)); // expecting stress location to be on the girder

   // figure out which stage the girder loading is applied
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx           = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType erectSegmentIntervalIdx      = pIntervals->GetErectSegmentInterval(segmentKey);
   IntervalIndexType tempStrandRemovalintervalIdx = pIntervals->GetTemporaryStrandRemovalInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx          = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx     = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType trafficBarrierIntervalIdx    = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx           = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx          = pIntervals->GetLiveLoadInterval();

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

   Float64 Stc = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::TopGirder);
   Float64 Sbc = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::BottomGirder);

   Float64 Stop = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::TopGirder,   fcgdr);
   Float64 Sbot = pSectProp->GetS(compositeDeckIntervalIdx,poi,pgsTypes::BottomGirder,fcgdr);

   Float64 k_top = Stc/Stop; // scale factor that converts top stress of composite section to a section with this f'c
   Float64 k_bot = Sbc/Sbot; // scale factor that converts bot stress of composite section to a section with this f'c

   Float64 ftop1, ftop2, ftop3Min, ftop3Max;
   Float64 fbot1, fbot2, fbot3Min, fbot3Max;

   ftop1 = ftop2 = ftop3Min = ftop3Max = 0;
   fbot1 = fbot2 = fbot3Min = fbot3Max = 0;

   // Load Factors
   GET_IFACE(ILoadFactors,pLF);
   const CLoadFactors* pLoadFactors = pLF->GetLoadFactors();

   Float64 dc = pLoadFactors->DCmax[limitState];
   Float64 dw = pLoadFactors->DWmax[limitState];
   Float64 ll = pLoadFactors->LLIMmax[limitState];

   Float64 ft,fb;

   // Erect Segment (also covers temporary strand removal)
   if ( erectSegmentIntervalIdx <= intervalIdx )
   {
      GetStress(erectSegmentIntervalIdx,pftGirder,poi,bat,rtCumulative,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 = dc*ft;   
      fbot1 = dc*fb;
   }

   // Casting Deck (non-composite girder carrying deck dead load)
   if ( castDeckIntervalIdx <= intervalIdx )
   {
      GetStress(castDeckIntervalIdx,pftConstruction,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pftSlab,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetDesignSlabStressAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pftSlabPad,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pftSlabPanel,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetDesignSlabPadStressAdjustment(fcgdr,startSlabOffset,endSlabOffset,poi,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pftDiaphragm,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pftShearKey,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pftUserDC,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dc*ft;   
      fbot1 += dc*fb;

      GetStress(castDeckIntervalIdx,pftUserDW,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop1 += dw*ft;   
      fbot1 += dw*fb;
   }

   // Composite Section Carrying Superimposed Dead Loads
   if ( compositeDeckIntervalIdx <= intervalIdx )
   {
      GetStress(trafficBarrierIntervalIdx,pftTrafficBarrier,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop2 = dc*k_top*ft;   
      fbot2 = dc*k_bot*fb;

      GetStress(trafficBarrierIntervalIdx,pftSidewalk,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop2 += dc*k_top*ft;   
      fbot2 += dc*k_bot*fb;

      if ( overlayIntervalIdx != INVALID_INDEX && overlayIntervalIdx <= intervalIdx )
      {
         GetStress(overlayIntervalIdx,pftOverlay,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
         ftop2 += dw*k_top*ft;   
         fbot2 += dw*k_bot*fb;
      }

      GetStress(compositeDeckIntervalIdx,pftUserDC,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop2 += dc*k_top*ft;   
      fbot2 += dc*k_bot*fb;

      GetStress(compositeDeckIntervalIdx,pftUserDW,poi,bat,rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
      ftop2 += dw*k_top*ft;   
      fbot2 += dw*k_bot*fb;

      // slab shrinkage stresses
      Float64 ft_ss, fb_ss;
      GetDeckShrinkageStresses(poi,fcgdr,&ft_ss,&fb_ss);
      ftop2 += ft_ss;
      fbot2 += fb_ss;
   }

   // Open to traffic, carrying live load
   if ( liveLoadIntervalIdx <= intervalIdx )
   {
      ftop3Min = ftop3Max = 0.0;   
      fbot3Min = fbot3Max = 0.0;   

      GET_IFACE(ISegmentData,pSegmentData);
      const CGirderMaterial* pGirderMaterial = pSegmentData->GetSegmentMaterial(segmentKey);

      Float64 fc_lldf = fcgdr;
      if ( pGirderMaterial->Concrete.bUserEc )
      {
         fc_lldf = lrfdConcreteUtil::FcFromEc( pGirderMaterial->Concrete.Ec, pGirderMaterial->Concrete.StrengthDensity );
      }


      Float64 ftMin,ftMax,fbMin,fbMax;
      if ( limitState == pgsTypes::FatigueI )
      {
         GetLiveLoadStress(liveLoadIntervalIdx,pgsTypes::lltFatigue, poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);
      }
      else
      {
         GetLiveLoadStress(liveLoadIntervalIdx,pgsTypes::lltDesign,  poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);
      }

      ftop3Min += ll*k_top*ftMin;   
      fbot3Min += ll*k_bot*fbMin;

      ftop3Max += ll*k_top*ftMax;   
      fbot3Max += ll*k_bot*fbMax;

      GetLiveLoadStress(liveLoadIntervalIdx,pgsTypes::lltPedestrian,  poi,bat,true,true,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ftMin,&ftMax,&fbMin,&fbMax);

      ftop3Min += ll*k_top*ftMin;   
      fbot3Min += ll*k_bot*fbMin;

      ftop3Max += ll*k_top*ftMax;   
      fbot3Max += ll*k_bot*fbMax;

      GetStress(liveLoadIntervalIdx,pftUserLLIM,poi,bat,rtCumulative,pgsTypes::TopGirder,pgsTypes::BottomGirder,&ft,&fb);
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

std::vector<Float64> CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation stressLocation)
{
   std::vector<Float64> stresses;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      Float64 stress = GetStress(intervalIdx,poi,stressLocation);
      stresses.push_back(stress);
   }

   return stresses;
}

//////////////////////////////////////////////////////////////////////
// IPosttensionStresses
//
Float64 CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,DuctIndexType ductIdx)
{
   return m_pGirderModelManager->GetStress(intervalIdx,poi,stressLocation,ductIdx);
}

std::vector<Float64> CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::StressLocation stressLocation,DuctIndexType ductIdx)
{
   std::vector<Float64> stresses;
   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi = *iter;

      Float64 stress = GetStress(intervalIdx,poi,stressLocation,ductIdx);
      stresses.push_back(stress);
   }

   return stresses;
}

//////////////////////////////////////////////////////////////////////
void CAnalysisAgentImp::GetConcurrentShear(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,sysSectionValue* pMin,sysSectionValue* pMax)
{
   try
   {
      m_pGirderModelManager->GetConcurrentShear(intervalIdx,limitState,poi,bat,pMin,pMax);
   }
   catch(...)
   {
      // reset all of our data.
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetViMmax(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::BridgeAnalysisType bat,Float64* pVi,Float64* pMmax)
{
   m_pGirderModelManager->GetViMmax(intervalIdx,limitState,poi,bat,pVi,pMmax);
}

/////////////////////////////////////////////////////////////////////////////
// ICamber
//
Uint32 CAnalysisAgentImp::GetCreepMethod()
{
   GET_IFACE(ILibrary,pLib);
   GET_IFACE(ISpecification,pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry(pSpec->GetSpecification().c_str());

   return pSpecEntry->GetCreepMethod();
}

Float64 CAnalysisAgentImp::GetCreepCoefficient(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details = GetCreepCoefficientDetails(segmentKey,creepPeriod,constructionRate);
   return details.Ct;
}

Float64 CAnalysisAgentImp::GetCreepCoefficient(const CSegmentKey& segmentKey,const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details = GetCreepCoefficientDetails(segmentKey,config,creepPeriod,constructionRate);
   return details.Ct;
}

CREEPCOEFFICIENTDETAILS CAnalysisAgentImp::GetCreepCoefficientDetails(const CSegmentKey& segmentKey,const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate)
{
   CREEPCOEFFICIENTDETAILS details;

   GET_IFACE(IEnvironment,pEnvironment);

   GET_IFACE(ISectionProperties,pSectProp);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   // if fc < 0 use current fc girder
   LoadingEvent le = GetLoadingEvent(creepPeriod);
   Float64 fc = GetConcreteStrengthAtTimeOfLoading(config,le);

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
               cc.SetInitialAge( (constructionRate == CREEP_MINTIME ? pSpecEntry->GetCreepDuration1Min() : pSpecEntry->GetCreepDuration1Max()) - cc.GetAdjustedInitialAge() );
               cc.SetMaturity( pSpecEntry->GetTotalCreepDuration() );
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

   return details;
}


CREEPCOEFFICIENTDETAILS CAnalysisAgentImp::GetCreepCoefficientDetails(const CSegmentKey& segmentKey, CreepPeriod creepPeriod, Int16 constructionRate)
{
   std::map<CSegmentKey,CREEPCOEFFICIENTDETAILS>::iterator found = m_CreepCoefficientDetails[constructionRate][creepPeriod].find(segmentKey);
   if ( found != m_CreepCoefficientDetails[constructionRate][creepPeriod].end() )
   {
      return (*found).second;
   }

   GET_IFACE(IPointOfInterest, IPoi);
   std::vector<pgsPointOfInterest> pois( IPoi->GetPointsOfInterest(segmentKey,POI_ERECTED_SEGMENT | POI_5L,POIFIND_AND) );
   ATLASSERT(pois.size() == 1);
   pgsPointOfInterest poi = pois[0];

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
   CREEPCOEFFICIENTDETAILS ccd = GetCreepCoefficientDetails(segmentKey,config,creepPeriod,constructionRate);
   m_CreepCoefficientDetails[constructionRate][creepPeriod].insert(std::make_pair(segmentKey,ccd));
   return ccd;
}

Float64 CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings)
{
   Float64 dy,rz;
   GetPrestressDeflection(poi,bRelativeToBearings,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings)
{
   Float64 dy,rz;
   GetPrestressDeflection(poi,config,bRelativeToBearings,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings)
{
   Float64 dy,rz;
   GetInitialTempPrestressDeflection(poi,bRelativeToBearings,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings)
{
   Float64 dy,rz;
   GetInitialTempPrestressDeflection(poi,config,bRelativeToBearings,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetReleaseTempPrestressDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetReleaseTempPrestressDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate)
{
   Float64 dy,rz;
   GetCreepDeflection(poi,creepPeriod,constructionRate,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate)
{
   Float64 dy,rz;
   GetCreepDeflection(poi,config,creepPeriod,constructionRate,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDeckDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetDeckDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDeckPanelDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetDeckPanelDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetDiaphragmDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetDiaphragmDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi) 
{
   Float64 dy,rz;
   GetUserLoadDeflection(intervalIdx,poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetUserLoadDeflection(intervalIdx,poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetSlabBarrierOverlayDeflection(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetSlabBarrierOverlayDeflection(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi)
{
   Float64 dy,rz;
   GetScreedCamber(poi,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   Float64 dy,rz;
   GetScreedCamber(poi,config,&dy,&rz);
   return dy;
}

Float64 CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,Int16 time)
{
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
#if defined _DEBUG
      // this is slower, but use it as a check on the more direct method used below
      Float64 D = GetDCamberForGirderSchedule(poi,time);
      Float64 C = GetScreedCamber(poi);
      Float64 excess = D - C;
#endif

      // excess camber is the camber that remains in the girder when the bridge is open
      // to traffic. we simply need to get the deflection when the bridge goes into
      // service.
      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
      Float64 Dmin, Dmax;
      GetDeflection(liveLoadIntervalIdx,pgsTypes::ServiceI,poi,bat,true,false/*exclude live load deflection*/,true,&Dmin,&Dmax);
      ATLASSERT(IsEqual(Dmin,Dmax)); // no live load so these should be the same

      ATLASSERT(IsEqual(Dmax,excess));

      return Dmax;
   }
   else
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CamberModelData model         = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
      CamberModelData initTempModel = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
      CamberModelData relsTempModel = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

      Float64 Dy, Rz;
      GetExcessCamber(poi,model,initTempModel,relsTempModel,time,&Dy,&Rz);
      return Dy;
   }
}

Float64 CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData initModel;
   BuildCamberModel(segmentKey,true,config,&initModel);

   CamberModelData tempModel1,tempModel2;
   BuildTempCamberModel(segmentKey,true,config,&tempModel1,&tempModel2);
   
   Float64 Dy, Rz;
   GetExcessCamber(poi,config,initModel,tempModel1,tempModel2,time,&Dy, &Rz);
   return Dy;
}

Float64 CAnalysisAgentImp::GetExcessCamberRotation(const pgsPointOfInterest& poi,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model         = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
   CamberModelData initTempModel = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   CamberModelData relsTempModel = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

   Float64 dy,rz;
   GetExcessCamber(poi,model,initTempModel,relsTempModel,time,&dy,&rz);
   return rz;
}

Float64 CAnalysisAgentImp::GetExcessCamberRotation(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData initModel;
   BuildCamberModel(segmentKey,true,config,&initModel);

   CamberModelData tempModel1,tempModel2;
   BuildTempCamberModel(segmentKey,true,config,&tempModel1,&tempModel2);
   
   Float64 dy,rz;
   GetExcessCamber(poi,config,initModel,tempModel1,tempModel2,time,&dy,&rz);
   return rz;
}

Float64 CAnalysisAgentImp::GetSidlDeflection(const pgsPointOfInterest& poi)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);
   return GetSidlDeflection(poi,config);
}

Float64 CAnalysisAgentImp::GetSidlDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config)
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : pgsTypes::MinSimpleContinuousEnvelope);

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
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,compositeDeckIntervalIdx);

   delta_diaphragm      = GetDiaphragmDeflection(poi,config);
   delta_trafficbarrier = k2*GetDeflection(compositeDeckIntervalIdx, pftTrafficBarrier, poi, bat, rtIncremental, false);
   delta_sidewalk       = k2*GetDeflection(compositeDeckIntervalIdx, pftSidewalk,       poi, bat, rtIncremental, false);

   for ( IntervalIndexType intervalIdx = compositeDeckIntervalIdx; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 dUser = GetUserLoadDeflection(intervalIdx,poi,config);
      delta_user += dUser;
   }

   if ( !pBridge->IsFutureOverlay() && overlayIntervalIdx != INVALID_INDEX )
   {
      delta_overlay = k2*GetDeflection(overlayIntervalIdx,pftOverlay,poi,bat, rtIncremental, false);
   }

   Float64 dSIDL;
   bool bTempStrands = (0 < config.PrestressConfig.GetStrandCount(pgsTypes::Temporary) && 
                        config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
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

Float64 CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,Int16 time)
{
   GET_IFACE( ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
      Float64 Dmin, Dmax;
      GetDeflection(castDeckIntervalIdx-1,pgsTypes::ServiceI,poi,bat,true,false,true,&Dmin,&Dmax);
      ATLASSERT(IsEqual(Dmin,Dmax)); // no live load so these should be the same

      return Dmax;
   }
   else
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      CamberModelData model            = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
      CamberModelData initTempModel    = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
      CamberModelData releaseTempModel = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

      Float64 Dy, Rz;
      GetDCamberForGirderSchedule(poi,model,initTempModel,releaseTempModel,time,&Dy,&Rz);
      return Dy;
   }
}

Float64 CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,Int16 time)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData initModel;
   BuildCamberModel(segmentKey,true,config,&initModel);

   CamberModelData tempModel1, tempModel2;
   BuildTempCamberModel(segmentKey,true,config,&tempModel1,&tempModel2);

   Float64 Dy, Rz;
   GetDCamberForGirderSchedule(poi,config,initModel,tempModel1, tempModel2, time, &Dy, &Rz);
   return Dy;
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GDRCONFIG dummy_config;

   CamberModelData model  = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);
   CamberModelData model1 = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   CamberModelData model2 = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);
   GetCreepDeflection(poi,false,dummy_config,model,model1,model2, creepPeriod, constructionRate, pDy, pRz);
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi, const GDRCONFIG& config, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model;
   BuildCamberModel(segmentKey,true,config,&model);

   CamberModelData model1,model2;
   BuildTempCamberModel(segmentKey,true,config,&model1,&model2);
   
   GetCreepDeflection(poi,true,config,model,model1,model2, creepPeriod, constructionRate,pDy,pRz);
}

void CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   // Screen camber is equal to and opposite the deflection caused by loads applied to the structure
   // from just before deck casting to the bridge opening to traffic.
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : pgsTypes::MinSimpleContinuousEnvelope);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval(); // this is when the bridge is open for service

   Float64 DdeckCasting, D;
   GetDeflection(castDeckIntervalIdx-1,pgsTypes::ServiceI,poi,bat,true,false,true,&DdeckCasting,&D);

   Float64 RdeckCasting, R;
   GetRotation(castDeckIntervalIdx-1,pgsTypes::ServiceI,poi,bat,true,false,true,&RdeckCasting,&R);

   Float64 Dopen;
   GetDeflection(liveLoadIntervalIdx,pgsTypes::ServiceI,poi,bat,true,false,true,&Dopen,&D);

   Float64 Ropen;
   GetRotation(liveLoadIntervalIdx,pgsTypes::ServiceI,poi,bat,true,false,true,&Ropen,&R);

   GET_IFACE(IBridge,pBridge);
   if ( pBridge->IsFutureOverlay() )
   {
      // overlay is applied well after the bridge is open so we want the deflection that does not include overlay
      Dopen = D;
      Ropen = R;
   }

   D = Dopen - DdeckCasting;
   R = Ropen - RdeckCasting;

   *pDy = -1.0*D;
   *pRz = -1.0*R;
}

void CAnalysisAgentImp::GetScreedCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   // this version is only for PGSuper design mode
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();
   pgsTypes::BridgeAnalysisType bat = (analysisType == pgsTypes::Simple ? pgsTypes::SimpleSpan : analysisType == pgsTypes::Continuous ? pgsTypes::ContinuousSpan : pgsTypes::MinSimpleContinuousEnvelope);

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx      = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();

   // NOTE: No need to validate camber models
   Float64 Dslab            = 0;
   Float64 Dslab_adj        = 0;
   Float64 Dslab_pad_adj    = 0;
   Float64 Dtrafficbarrier  = 0;
   Float64 Dsidewalk        = 0;
   Float64 Doverlay         = 0;
   Float64 Ddiaphragm       = 0;
   Float64 Duser            = 0;

   Float64 Rslab            = 0;
   Float64 Rslab_adj        = 0;
   Float64 Rslab_pad_adj    = 0;
   Float64 Rtrafficbarrier  = 0;
   Float64 Rsidewalk        = 0;
   Float64 Roverlay         = 0;
   Float64 Rdiaphragm       = 0;
   Float64 Ruser            = 0;

   // deflections are computed based on current parameters for the bridge.
   // E in the config object may be different than E used to compute the deflection.
   // The deflection adjustment factor accounts for the differences in E.
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,railingSystemIntervalIdx);
   GetDeckDeflection(poi,config,&Dslab,&Rslab); 
   GetDesignSlabDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_adj,&Rslab_adj);
   GetDesignSlabPadDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_pad_adj,&Rslab_pad_adj);
   GetDiaphragmDeflection(poi,config,&Ddiaphragm,&Rdiaphragm);

   
   Dtrafficbarrier = k2*GetDeflection(railingSystemIntervalIdx, pftTrafficBarrier, poi, bat, rtIncremental, false);
   Rtrafficbarrier = k2*GetRotation(  railingSystemIntervalIdx, pftTrafficBarrier, poi, bat, rtIncremental, false);

   Dsidewalk = k2*GetDeflection(railingSystemIntervalIdx, pftSidewalk, poi, bat, rtIncremental, false);
   Rsidewalk = k2*GetRotation(  railingSystemIntervalIdx, pftSidewalk, poi, bat, rtIncremental, false);

   // Only get deflections for user defined loads that occur during deck placement and later
   GET_IFACE(IPointOfInterest,pPoi);
   CSpanKey spanKey;
   Float64 Xspan;
   pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
   std::vector<IntervalIndexType> vUserLoadIntervals(pIntervals->GetUserDefinedLoadIntervals(spanKey));
   vUserLoadIntervals.erase(std::remove_if(vUserLoadIntervals.begin(),vUserLoadIntervals.end(),std::bind2nd(std::less<IntervalIndexType>(),castDeckIntervalIdx)),vUserLoadIntervals.end());
   std::vector<IntervalIndexType>::iterator userLoadIter(vUserLoadIntervals.begin());
   std::vector<IntervalIndexType>::iterator userLoadIterEnd(vUserLoadIntervals.end());
   for ( ; userLoadIter != userLoadIterEnd; userLoadIter++ )
   {
      IntervalIndexType intervalIdx = *userLoadIter;
      Float64 D,R;

      k2 = GetDeflectionAdjustmentFactor(poi,config,intervalIdx);
      GetUserLoadDeflection(intervalIdx,poi,config,&D,&R);

      Duser += k2*D;
      Ruser += k2*R;
   }

   if ( !pBridge->IsFutureOverlay() && overlayIntervalIdx != INVALID_INDEX )
   {
      k2 = GetDeflectionAdjustmentFactor(poi,config,overlayIntervalIdx);
      Doverlay = k2*GetDeflection(overlayIntervalIdx,pftOverlay,poi,bat, rtIncremental, false);
      Roverlay = k2*GetRotation(  overlayIntervalIdx,pftOverlay,poi,bat, rtIncremental, false);
   }

   bool bTempStrands = (0 < config.PrestressConfig.GetStrandCount(pgsTypes::Temporary) &&  // there are temp strands
                   config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) // and they are not post-tensioned before shipping
                   ? true : false;

   if ( bTempStrands )
   {
      *pDy = Dslab + Dslab_adj + Dslab_pad_adj + Dtrafficbarrier + Dsidewalk + Doverlay + Duser;
      *pRz = Rslab + Rslab_adj + Rslab_pad_adj + Rtrafficbarrier + Rsidewalk + Roverlay + Ruser;
   }
   else
   {
      // for SIP decks, diaphagms are applied before the cast portion of the slab so they don't apply to screed camber
      if ( deckType == pgsTypes::sdtCompositeSIP )
      {
         Ddiaphragm = 0;
         Rdiaphragm = 0;
      }

      *pDy = Dslab + Dslab_adj + Dslab_pad_adj + Dtrafficbarrier + Dsidewalk + Doverlay + Duser + Ddiaphragm;
      *pRz = Rslab + Rslab_adj + Rslab_pad_adj + Rtrafficbarrier + Rsidewalk + Roverlay + Ruser + Rdiaphragm;
   }

   // Switch the sign. Negative deflection creates positive screed camber
   (*pDy) *= -1;
   (*pRz) *= -1;
}

void CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   Float64 dy_slab = GetDeflection(castDeckIntervalIdx,pftSlab,poi,bat, rtIncremental, false);
   Float64 rz_slab = GetRotation(  castDeckIntervalIdx,pftSlab,poi,bat, rtIncremental, false);

   Float64 dy_slab_pad = GetDeflection(castDeckIntervalIdx,pftSlabPad,poi,bat, rtIncremental, false);
   Float64 rz_slab_pad = GetRotation(  castDeckIntervalIdx,pftSlabPad,poi,bat, rtIncremental, false);

   *pDy = dy_slab + dy_slab_pad;
   *pRz = rz_slab + rz_slab_pad;
}

void CAnalysisAgentImp::GetDeckDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   GetDeckDeflection(poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,castDeckIntervalIdx);
   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

#pragma Reminder("UPDATE: need a new interval type for deck panel placement?")
   // assuming panels are placed at same time deck is cast
   // bridge model doesn't support the deck panel stage idea
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   *pDy = GetDeflection(castDeckIntervalIdx,pftSlabPanel,poi,bat, rtIncremental, false);
   *pRz = GetRotation(  castDeckIntervalIdx,pftSlabPanel,poi,bat, rtIncremental, false);
}

void CAnalysisAgentImp::GetDeckPanelDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

#pragma Reminder("UPDATE: need a new interval type for deck panel placement?")
   // assuming panels are placed at same time deck is cast
   // bridge model doesn't support the deck panel stage idea
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   GetDeckPanelDeflection(poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,castDeckIntervalIdx);

   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   *pDy = GetDeflection(castDeckIntervalIdx,pftDiaphragm,poi,bat, rtIncremental, false);
   *pRz = GetRotation(  castDeckIntervalIdx,pftDiaphragm,poi,bat, rtIncremental, false);
}

void CAnalysisAgentImp::GetDiaphragmDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   GetDiaphragmDeflection(poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,castDeckIntervalIdx);
   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz) 
{
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   Float64 Ddc = GetDeflection(intervalIdx,pftUserDC, poi,bat, rtIncremental, false);
   Float64 Ddw = GetDeflection(intervalIdx,pftUserDW, poi,bat, rtIncremental, false);

   Float64 Rdc = GetRotation(intervalIdx,pftUserDC, poi,bat, rtIncremental, false);
   Float64 Rdw = GetRotation(intervalIdx,pftUserDW, poi,bat, rtIncremental, false);

   *pDy = Ddc + Ddw;
   *pRz = Rdc + Rdw;
}

void CAnalysisAgentImp::GetUserLoadDeflection(IntervalIndexType intervalIdx, const pgsPointOfInterest& poi, const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   GetUserLoadDeflection(intervalIdx,poi,pDy,pRz);
   Float64 k = GetDeflectionAdjustmentFactor(poi,config,intervalIdx);

   (*pDy) *= k;
   (*pRz) *= k;
}

void CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IBridge,pBridge);
   GDRCONFIG config = pBridge->GetSegmentConfiguration(segmentKey);

   GetSlabBarrierOverlayDeflection(poi,config,pDy,pRz);
}

void CAnalysisAgentImp::GetSlabBarrierOverlayDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   // NOTE: No need to validate camber models
   Float64 Dslab           = 0;
   Float64 Dslab_adj       = 0;
   Float64 Dslab_pad_adj   = 0;
   Float64 Dtrafficbarrier = 0;
   Float64 Dsidewalk       = 0;
   Float64 Doverlay        = 0;

   Float64 Rslab           = 0;
   Float64 Rslab_adj       = 0;
   Float64 Rslab_pad_adj   = 0;
   Float64 Rtrafficbarrier = 0;
   Float64 Rsidewalk       = 0;
   Float64 Roverlay        = 0;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx = pIntervals->GetOverlayInterval();

   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   GetDeckDeflection(poi,config,&Dslab,&Rslab);
   GetDesignSlabDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_adj,&Rslab_adj);
   GetDesignSlabPadDeflectionAdjustment(config.Fc,config.SlabOffset[pgsTypes::metStart],config.SlabOffset[pgsTypes::metEnd],poi,&Dslab_pad_adj,&Rslab_pad_adj);
   
   Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,railingSystemIntervalIdx);
   Dtrafficbarrier = k2*GetDeflection(railingSystemIntervalIdx,pftTrafficBarrier,poi,bat, rtIncremental, false);
   Rtrafficbarrier = k2*GetRotation(  railingSystemIntervalIdx,pftTrafficBarrier,poi,bat, rtIncremental, false);

   Dsidewalk = k2*GetDeflection(railingSystemIntervalIdx,pftSidewalk,poi,bat, rtIncremental, false);
   Rsidewalk = k2*GetRotation(  railingSystemIntervalIdx,pftSidewalk,poi,bat, rtIncremental, false);

   if ( overlayIntervalIdx != INVALID_INDEX )
   {
      Float64 k2 = GetDeflectionAdjustmentFactor(poi,config,overlayIntervalIdx);
      Doverlay = k2*GetDeflection(overlayIntervalIdx,pftOverlay,poi,bat, rtIncremental, false);
      Roverlay = k2*GetRotation(  overlayIntervalIdx,pftOverlay,poi,bat, rtIncremental, false);
   }

   // Switch the sign. Negative deflection creates positive screed camber
   *pDy = Dslab + Dslab_adj + Dslab_pad_adj + Dtrafficbarrier + Dsidewalk + Doverlay;
   *pRz = Rslab + Rslab_adj + Rslab_pad_adj + Rtrafficbarrier + Rsidewalk + Roverlay;
}


Float64 CAnalysisAgentImp::GetLowerBoundCamberVariabilityFactor()const
{
   GET_IFACE(ILibrary,pLibrary);
   GET_IFACE(ISpecification,pSpec);

   const SpecLibraryEntry* pSpecEntry = pLibrary->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Float64 fac = pSpecEntry->GetCamberVariability();

   fac = 1.0-fac;
   return fac;
}
/////////////////////////////////////////////////////////////////////////////
// IPretensionStresses
//
Float64 CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation)
{
   return m_pGirderModelManager->GetStress(intervalIdx,poi,stressLocation);
}

Float64 CAnalysisAgentImp::GetStress(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,Float64 P,Float64 e)
{
   return m_pGirderModelManager->GetStress(intervalIdx,poi,stressLocation,P,e);
}

Float64 CAnalysisAgentImp::GetStressPerStrand(IntervalIndexType intervalIdx,const pgsPointOfInterest& poi,pgsTypes::StrandType strandType,pgsTypes::StressLocation stressLocation)
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
   Float64 e = pStrandGeom->GetEccentricity(intervalIdx,poi,strandType,&nSEffective);

   return GetStress(intervalIdx,poi,stressLocation,P,e);
}

Float64 CAnalysisAgentImp::GetDesignStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const pgsPointOfInterest& poi,pgsTypes::StressLocation stressLocation,const GDRCONFIG& config)
{
   // Computes design-time stresses due to prestressing
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   GET_IFACE(IPretensionForce,pPsForce);
   GET_IFACE(IStrandGeometry,pStrandGeom);

   GET_IFACE(ISectionProperties,pSectProp);
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();
   // If gross properties analysis, we want the prestress force at the end of the interval. It will include
   // elastic effects. If transformed properties analysis, we want the force at the start of the interval
   // becase the stress analysis will intrinsically include elastic effects.
   pgsTypes::IntervalTimeType timeType (spMode == pgsTypes::spmGross ? pgsTypes::End : pgsTypes::Start);

   Float64 P;
   IntervalIndexType liveLoadIntervalIdx = pIntervals->GetLiveLoadInterval();
   if ( intervalIdx < liveLoadIntervalIdx )
   {
      P = pPsForce->GetPrestressForce(poi,pgsTypes::Permanent,intervalIdx,timeType,config);
   }
   else
   {
      P = pPsForce->GetPrestressForceWithLiveLoad(poi,pgsTypes::Permanent,pgsTypes::ServiceI,config);
   }

   // NOTE: since we are doing design, the main bridge model may not have temporary strand removal
   // intervals. Use the deck casting interval as the break point for "before temporary strands are removed"
   // and "after temporary strands are removed"
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   bool bIncludeTemporaryStrands = intervalIdx < castDeckIntervalIdx ? true : false;
   if ( bIncludeTemporaryStrands ) 
   {
      P += pPsForce->GetPrestressForce(poi,pgsTypes::Temporary,intervalIdx,timeType,config);
   }

   Float64 nSEffective;
   pgsTypes::SectionPropertyType spType = (spMode == pgsTypes::spmGross ? pgsTypes::sptGrossNoncomposite : pgsTypes::sptTransformedNoncomposite);
   Float64 e = pStrandGeom->GetEccentricity(spType,intervalIdx,poi, config, bIncludeTemporaryStrands, &nSEffective);

   return GetStress(intervalIdx,poi,stressLocation,P,e);
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

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData camber_model_data = GetPrestressDeflectionModel(segmentKey,m_PrestressDeflectionModels);

   Float64 Dharped, Rharped;
   GetPrestressDeflection(poi,camber_model_data,g_lcidHarpedStrand,bRelativeToBearings,&Dharped,&Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection(poi,camber_model_data,g_lcidStraightStrand,bRelativeToBearings,&Dstraight,&Rstraight);

   *pDy = Dstraight + Dharped;
   *pRz = Rstraight + Rharped;
}

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model_data;
   BuildCamberModel(segmentKey,true,config,&model_data);

   Float64 Dharped, Rharped;
   GetPrestressDeflection(poi,model_data,g_lcidHarpedStrand,bRelativeToBearings,&Dharped,&Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection(poi,model_data,g_lcidStraightStrand,bRelativeToBearings,&Dstraight,&Rstraight);

   *pDy = Dstraight + Dharped;
   *pRz = Rstraight + Rharped;
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model_data = GetPrestressDeflectionModel(segmentKey,m_InitialTempPrestressDeflectionModels);
   
   GetInitialTempPrestressDeflection(poi,model_data,bRelativeToBearings,pDy,pRz);
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model1, model2;
   BuildTempCamberModel(segmentKey,true,config,&model1,&model2);

   GetInitialTempPrestressDeflection(poi,model1,bRelativeToBearings,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model = GetPrestressDeflectionModel(segmentKey,m_ReleaseTempPrestressDeflectionModels);

   GetReleaseTempPrestressDeflection(poi,model,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,const GDRCONFIG& config,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   CamberModelData model1,model2;
   BuildTempCamberModel(segmentKey,true,config,&model1,&model2);

   GetReleaseTempPrestressDeflection(poi,model2,pDy,pRz);
}

void CAnalysisAgentImp::GetPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,LoadCaseIDType lcid,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
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

   PoiIDType femPoiID = modelData.PoiMap.GetModelPoi(poi);
   if ( femPoiID == INVALID_ID )
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

      ATLASSERT( thePOI.GetID() != INVALID_ID );

      femPoiID = AddPointOfInterest(modelData,thePOI);
      ATLASSERT( 0 <= femPoiID );
   }

   HRESULT hr = results->ComputePOIDeflections(lcid,femPoiID,lotGlobal,&Dx,&Dy,pRz);
   ATLASSERT( SUCCEEDED(hr) );

   Float64 delta = Dy;
   if ( bRelativeToBearings )
   {
      GET_IFACE(IBridge,pBridge);
      GET_IFACE(IPointOfInterest,pPOI);

      Float64 start_end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
      pgsPointOfInterest poiAtStart( pPOI->GetPointOfInterest(segmentKey,start_end_size) );
      ATLASSERT( 0 <= poiAtStart.GetID() );
   
      femPoiID = modelData.PoiMap.GetModelPoi(poiAtStart);
      results->ComputePOIDeflections(lcid,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
      Float64 start_delta_brg = Dy;

      Float64 end_end_size = pBridge->GetSegmentEndEndDistance(segmentKey);
      Float64 Lg = pBridge->GetSegmentLength(segmentKey);
      pgsPointOfInterest poiAtEnd( pPOI->GetPointOfInterest(segmentKey,Lg-end_end_size) );
      ATLASSERT( 0 <= poiAtEnd.GetID() );
      femPoiID = modelData.PoiMap.GetModelPoi(poiAtEnd);
      results->ComputePOIDeflections(lcid,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
      Float64 end_delta_brg = Dy;

      Float64 delta_brg = LinInterp(poi.GetDistFromStart()-start_end_size,
                                    start_delta_brg,end_delta_brg,Lg);

      delta -= delta_brg;
   }

   *pDy = delta;
}

void CAnalysisAgentImp::GetInitialTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,bool bRelativeToBearings,Float64* pDy,Float64* pRz)
{
   GetPrestressDeflection(poi,modelData,g_lcidTemporaryStrand,bRelativeToBearings,pDy,pRz);
}

void CAnalysisAgentImp::GetReleaseTempPrestressDeflection(const pgsPointOfInterest& poi,CamberModelData& modelData,Float64* pDy,Float64* pRz)
{
   GET_IFACE(IPointOfInterest,pPoi);
   if ( pPoi->IsOffSegment(poi) )
   {
      *pDy = 0;
      *pRz = 0;
      return;
   }

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   MemberIDType mbrID;
   Float64 x;
   pgsGirderModelFactory::FindMember(modelData.Model,poi.GetDistFromStart(),&mbrID,&x);

   // Get deflection at poi
   CComQIPtr<IFem2dModelResults> results(modelData.Model);
   Float64 Dx, Dy, Rz;
   PoiIDType femPoiID = modelData.PoiMap.GetModelPoi(poi);
   results->ComputePOIDeflections(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,pRz);
   Float64 delta_poi = Dy;

   // Get deflection at start bearing
   GET_IFACE(IBridge,pBridge);
   Float64 start_end_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   pgsPointOfInterest poi2( pPoi->GetPointOfInterest(segmentKey,start_end_size) );
   femPoiID = modelData.PoiMap.GetModelPoi(poi2);
   results->ComputePOIDeflections(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
   Float64 start_delta_end_size = Dy;

   // Get deflection at end bearing
   Float64 L = pBridge->GetSegmentLength(segmentKey);
   Float64 end_end_size = pBridge->GetSegmentEndEndDistance(segmentKey);
   poi2 = pPoi->GetPointOfInterest(segmentKey,L-end_end_size);
   femPoiID = modelData.PoiMap.GetModelPoi(poi2);
   results->ComputePOIDeflections(g_lcidTemporaryStrand,femPoiID,lotGlobal,&Dx,&Dy,&Rz);
   Float64 end_delta_end_size = Dy;

   Float64 delta_brg = LinInterp(poi.GetDistFromStart()-start_end_size,
                                    start_delta_end_size,end_delta_end_size,L);

   Float64 delta = delta_poi - delta_brg;
   *pDy = delta;
}

void CAnalysisAgentImp::GetCreepDeflection(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   bool bTempStrands = false;
   
   if ( bUseConfig )
   {
      bTempStrands = (0 < config.PrestressConfig.GetStrandCount(pgsTypes::Temporary) && 
                       config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }
   else
   {
      const CSegmentKey& segmentKey = poi.GetSegmentKey();

      GET_IFACE(IStrandGeometry,pStrandGeom);
      GET_IFACE(ISegmentData,pSegmentData);
      const CStrandData* pStrands = pSegmentData->GetStrandData(segmentKey);

      bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary) && 
                      pStrands->GetTemporaryStrandUsage() != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }

   Float64 Dcreep = 0;
   switch( deckType )
   {
      case pgsTypes::sdtCompositeCIP:
      case pgsTypes::sdtCompositeOverlay:
         (bTempStrands ? GetCreepDeflection_CIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz) 
                       : GetCreepDeflection_CIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz));
         break;

      case pgsTypes::sdtCompositeSIP:
         (bTempStrands ? GetCreepDeflection_SIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz) 
                       : GetCreepDeflection_SIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz));
         break;

      case pgsTypes::sdtNone:
         (bTempStrands ? GetCreepDeflection_NoDeck_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz) 
                       : GetCreepDeflection_NoDeck(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz));
         break;
   }
}

void CAnalysisAgentImp::GetCreepDeflection_CIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDiaphragmIntervalIdx = pIntervals->GetCastDeckInterval();


   Float64 Dharped, Rharped;
   GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    true, &Dharped, &Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    true, &Dstraight, &Rstraight);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 Dtpsi,Rtpsi;
   GetPrestressDeflection( poi, initTempModelData, g_lcidTemporaryStrand, true, &Dtpsi, &Rtpsi);

   Float64 Dgirder, Rgirder;
   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&Dgirder,&Rgirder);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&Dgirder,&Rgirder);
   }

   Float64 Ct1 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate)
                             : GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate));


   // creep 1 - Initial to immediately before diaphragm/temporary strands
   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      *pDy = Ct1*(Dgirder + Dps + Dtpsi);
      *pRz = Ct1*(Rgirder + Rps + Rtpsi);
      return;
   }

   // creep 2 - Immediately after diarphagm/temporary strands to deck
   Float64 Ct2 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,constructionRate) :
                               GetCreepCoefficient(segmentKey,cpReleaseToDeck,constructionRate) );

   Float64 Ct3 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,constructionRate) :
                               GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,constructionRate) );

   Float64 Ddiaphragm = GetDeflection(castDiaphragmIntervalIdx,pftDiaphragm,poi,pgsTypes::SimpleSpan, rtIncremental, false);
   Float64 Rdiaphragm = GetRotation(  castDiaphragmIntervalIdx,pftDiaphragm,poi,pgsTypes::SimpleSpan, rtIncremental, false);

   Float64 Dtpsr,Rtpsr;
   GetPrestressDeflection( poi, releaseTempModelData, g_lcidTemporaryStrand, true, &Dtpsr, &Rtpsr);

   *pDy = (Ct2-Ct1)*(Dgirder + Dps + Dtpsi) + Ct3*(Ddiaphragm + Dtpsr);
   *pRz = (Ct2-Ct1)*(Rgirder + Rps + Rtpsi) + Ct3*(Rdiaphragm + Rtpsr);
}

void CAnalysisAgentImp::GetCreepDeflection_CIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDeck );

   const CSegmentKey& segmentKey = poi.GetSegmentKey();


   Float64 Dharped, Rharped;
   GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    true, &Dharped, &Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    true, &Dstraight, &Rstraight);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 Ct1 = -999;
   Float64 Dgirder = -999;
   Float64 Rgirder = -999;

   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&Dgirder,&Rgirder);
      Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,constructionRate);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&Dgirder,&Rgirder);
      Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,constructionRate);
   }


   *pDy = Ct1*(Dgirder + Dps);
   *pRz = Ct1*(Rgirder + Rps);
}

void CAnalysisAgentImp::GetCreepDeflection_SIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   // Creep periods and loading are the same as for CIP decks
   // an improvement could be to add a third creep stage for creep after deck panel placement to deck casting
   GetCreepDeflection_CIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz);
}

void CAnalysisAgentImp::GetCreepDeflection_SIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   // Creep periods and loading are the same as for CIP decks
   // an improvement could be to add a third creep stage for creep after deck panel placement to deck casting
   GetCreepDeflection_CIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,creepPeriod,constructionRate,pDy,pRz);
}

void CAnalysisAgentImp::GetCreepDeflection_NoDeck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck || creepPeriod == cpDeckToFinal);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 Dharped, Rharped;
   GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    true, &Dharped, &Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    true, &Dstraight, &Rstraight);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 Dtpsi, Rtpsi;
   GetPrestressDeflection( poi, initTempModelData, g_lcidTemporaryStrand, true, &Dtpsi, &Rtpsi);

   Float64 Dtpsr, Rtpsr;
   GetPrestressDeflection( poi, releaseTempModelData, g_lcidTemporaryStrand, true, &Dtpsr, &Rtpsr);

   Float64 Dgirder, Rgirder;
   Float64 Ddiaphragm, Rdiaphragm;
   Float64 Duser1, Ruser1;
   Float64 Duser2, Ruser2;
   Float64 Dbarrier, Rbarrier;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
#pragma Reminder("UPDATE: need user loads for all intervals") // right now just useing "bridge site 1 and 2")

   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,config,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,config,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,config,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,config,&Dbarrier,&Rbarrier);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,&Dbarrier,&Rbarrier);
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // Creep1
      Float64 Ct1 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate) 
                                : GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate) );

      *pDy = Ct1*(Dgirder + Dps + Dtpsi);
      *pRz = Ct1*(Rgirder + Rps + Rtpsi);
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      // Creep2
      Float64 Ct1, Ct2, Ct3;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,   constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,   constructionRate);
      }

      *pDy = (Ct2 - Ct1)*(Dgirder + Dps) + Ct3*(Ddiaphragm + Duser1 + Dtpsr);
      *pRz = (Ct2 - Ct1)*(Rgirder + Rps) + Ct3*(Rdiaphragm + Ruser1 + Rtpsr);
   }
   else
   {
      // Creep3
      Float64 Ct1, Ct2, Ct3, Ct4, Ct5;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,config,cpDeckToFinal,        constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,cpDeckToFinal,        constructionRate);
      }

      *pDy = (Ct2 - Ct1)*(Dgirder + Dps) + (Ct4 - Ct3)*(Ddiaphragm + Duser1 + Dtpsr) + Ct5*(Dbarrier + Duser2);
      *pRz = (Ct2 - Ct1)*(Rgirder + Rps) + (Ct4 - Ct3)*(Rdiaphragm + Ruser1 + Rtpsr) + Ct5*(Rbarrier + Ruser2);
   }
}

void CAnalysisAgentImp::GetCreepDeflection_NoDeck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData, CreepPeriod creepPeriod, Int16 constructionRate,Float64* pDy,Float64* pRz )
{
   ATLASSERT( creepPeriod == cpReleaseToDiaphragm || creepPeriod == cpDiaphragmToDeck || creepPeriod == cpDeckToFinal);

   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   Float64 Dharped, Rharped;
   GetPrestressDeflection( poi, initModelData,     g_lcidHarpedStrand,    true, &Dharped, &Rharped);

   Float64 Dstraight, Rstraight;
   GetPrestressDeflection( poi, initModelData,     g_lcidStraightStrand,    true, &Dstraight, &Rstraight);

   Float64 Dps = Dharped + Dstraight;
   Float64 Rps = Rharped + Rstraight;

   Float64 Dgirder, Rgirder;
   Float64 Ddiaphragm, Rdiaphragm;
   Float64 Duser1, Ruser1;
   Float64 Duser2, Ruser2;
   Float64 Dbarrier, Rbarrier;

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
#pragma Reminder("UPDATE: need user loads for all intervals") // right now just useing "bridge site 1 and 2")


   if ( bUseConfig )
   {
      GetGirderDeflectionForCamber(poi,config,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,config,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,config,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,config,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,config,&Dbarrier,&Rbarrier);
   }
   else
   {
      GetGirderDeflectionForCamber(poi,&Dgirder,&Rgirder);
      GetDiaphragmDeflection(poi,&Ddiaphragm,&Rdiaphragm);
      GetUserLoadDeflection(castDeckIntervalIdx,poi,&Duser1,&Ruser1);
      GetUserLoadDeflection(compositeDeckIntervalIdx,poi,&Duser2,&Ruser2);
      GetSlabBarrierOverlayDeflection(poi,&Dbarrier,&Rbarrier);
   }

   if ( creepPeriod == cpReleaseToDiaphragm )
   {
      // Creep1
      Float64 Ct1 = (bUseConfig ? GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate)
                                : GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate) );

      *pDy = Ct1*(Dgirder + Dps);
      *pRz = Ct1*(Rgirder + Rps);
   }
   else if ( creepPeriod == cpDiaphragmToDeck )
   {
      // Creep2
      Float64 Ct1, Ct2, Ct3;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,   constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDiaphragm,constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,   constructionRate);
      }

      *pDy = (Ct2 - Ct1)*(Dgirder + Dps) + Ct3*(Ddiaphragm + Duser1);
      *pRz = (Ct2 - Ct1)*(Rgirder + Rps) + Ct3*(Rdiaphragm + Ruser1);
   }
   else
   {
      // Creep3
      Float64 Ct1, Ct2, Ct3, Ct4, Ct5;

      if ( bUseConfig )
      {
         Ct1 = GetCreepCoefficient(segmentKey,config,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,config,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,config,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,config,cpDeckToFinal,        constructionRate);
      }
      else
      {
         Ct1 = GetCreepCoefficient(segmentKey,cpReleaseToDeck,      constructionRate);
         Ct2 = GetCreepCoefficient(segmentKey,cpReleaseToFinal,     constructionRate);
         Ct3 = GetCreepCoefficient(segmentKey,cpDiaphragmToDeck,    constructionRate);
         Ct4 = GetCreepCoefficient(segmentKey,cpDiaphragmToFinal,   constructionRate);
         Ct5 = GetCreepCoefficient(segmentKey,cpDeckToFinal,        constructionRate);
      }

      *pDy = (Ct2 - Ct1)*(Dgirder + Dps) + (Ct4 - Ct3)*(Ddiaphragm + Duser1) + Ct5*(Dbarrier + Duser2);
      *pRz = (Ct2 - Ct1)*(Rgirder + Rps) + (Ct4 - Ct3)*(Rdiaphragm + Ruser1) + Ct5*(Rbarrier + Ruser2);
   }
}

void CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   Float64 Dy, Dz;
   GetDCamberForGirderSchedule( poi, initModelData, initTempModelData, releaseTempModelData, time, &Dy, &Dz );

   Float64 Cy, Cz;
   GetScreedCamber(poi,&Cy,&Cz);

   *pDy = Dy - Cy;  // excess camber = D - C
   *pRz = Dz - Cz;

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   if ( deckType == pgsTypes::sdtNone )
   {
      Float64 Dcreep3, Rcreep3;
      GetCreepDeflection(poi,ICamber::cpDeckToFinal,time,&Dcreep3,&Rcreep3 );
      *pDy = Dy + Cy + Dcreep3;
      *pRz = Dz + Cz + Rcreep3;
   }
}

void CAnalysisAgentImp::GetExcessCamber(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   Float64 Dy, Dz;
   GetDCamberForGirderSchedule( poi, config, initModelData, initTempModelData, releaseTempModelData, time, &Dy, &Dz );

   Float64 Cy, Cz;
   GetScreedCamber(poi,config,&Cy,&Cz);

   *pDy = Dy - Cy;  // excess camber = D - C
   *pRz = Dz - Cz;

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();
   if ( deckType == pgsTypes::sdtNone )
   {
      Float64 Dcreep3, Rcreep3;
      GetCreepDeflection(poi,config,ICamber::cpDeckToFinal,time,&Dcreep3,&Rcreep3 );
      *pDy += Dcreep3;
      *pRz += Rcreep3;
   }
}

void CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   GDRCONFIG dummy_config;
   GetDCamberForGirderSchedule(poi,false,dummy_config,initModelData,initTempModelData,releaseTempModelData,time,pDy,pRz);
}

void CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   GetDCamberForGirderSchedule(poi,true,config,initModelData,initTempModelData,releaseTempModelData,time,pDy,pRz);
}

void CAnalysisAgentImp::GetDCamberForGirderSchedule(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 time,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   bool bTempStrands = true;
   if ( bUseConfig )
   {
      bTempStrands = (0 < config.PrestressConfig.GetStrandCount(pgsTypes::Temporary) && 
                      config.PrestressConfig.TempStrandUsage != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }
   else
   {
      GET_IFACE(IStrandGeometry,pStrandGeom);
      GET_IFACE(ISegmentData,pSegmentData);
      const CStrandData* pStrand = pSegmentData->GetStrandData(segmentKey);

      bTempStrands = (0 < pStrandGeom->GetStrandCount(segmentKey,pgsTypes::Temporary) && 
                      pStrand->GetTemporaryStrandUsage() != pgsTypes::ttsPTBeforeShipping) ? true : false;
   }

   GET_IFACE(IBridge,pBridge);
   pgsTypes::SupportedDeckType deckType = pBridge->GetDeckType();

   Float64 D, R;

   switch( deckType )
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeOverlay:
      (bTempStrands ? GetD_CIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R) : GetD_CIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R));
      break;

   case pgsTypes::sdtCompositeSIP:
      (bTempStrands ? GetD_SIP_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R) : GetD_SIP(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R));
      break;

   case pgsTypes::sdtNone:
      (bTempStrands ? GetD_NoDeck_TempStrands(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R) : GetD_NoDeck(poi,bUseConfig,config,initModelData,initTempModelData,releaseTempModelData,time,&D,&R));
      break;

   default:
      ATLASSERT(false);
   }

   *pDy = D;
   *pRz = R;
}

void CAnalysisAgentImp::GetD_CIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   Float64 Dps, Dtpsi, Dtpsr, Dgirder, Dcreep1, Ddiaphragm, Dcreep2;
   Float64 Rps, Rtpsi, Rtpsr, Rgirder, Rcreep1, Rdiaphragm, Rcreep2;
   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi, config, true, &Dtpsi, &Rtpsi );
      GetReleaseTempPrestressDeflection( poi, config, &Dtpsr, &Rtpsr );
      GetGirderDeflectionForCamber( poi, config, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, true, config, initModelData, initTempModelData, releaseTempModelData, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1);
      GetDiaphragmDeflection( poi, config, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, true, config, initModelData, initTempModelData, releaseTempModelData, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2);
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi, true, &Dtpsi, &Rtpsi );
      GetReleaseTempPrestressDeflection( poi, &Dtpsr, &Rtpsr );
      GetGirderDeflectionForCamber( poi, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
   }

   Float64 D1 = Dgirder + Dps + Dtpsi;
   Float64 D2 = D1 + Dcreep1;
   Float64 D3 = D2 + Ddiaphragm + Dtpsr;
   Float64 D4 = D3 + Dcreep2;
   *pDy = D4;

   Float64 R1 = Rgirder + Rps + Rtpsi;
   Float64 R2 = R1 + Rcreep1;
   Float64 R3 = R2 + Rdiaphragm + Rtpsr;
   Float64 R4 = R3 + Rcreep2;

   *pRz = R4;
}

void CAnalysisAgentImp::GetD_CIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   Float64 Dps, Dgirder, Dcreep;
   Float64 Rps, Rgirder, Rcreep;

   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, config, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, true, config, initModelData, initTempModelData, releaseTempModelData, ICamber::cpReleaseToDeck, constructionRate, &Dcreep, &Rcreep);
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDeck, constructionRate, &Dcreep, &Rcreep );
   }

   Float64 D1 = Dgirder + Dps;
   Float64 D2 = D1 + Dcreep;
   *pDy = D2;

   Float64 R1 = Rgirder + Rps;
   Float64 R2 = R1 + Rcreep;
   *pRz = R2;
}

void CAnalysisAgentImp::GetD_SIP_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   Float64 Dps, Dtpsi, Dtpsr, Dgirder, Dcreep1, Ddiaphragm, Dpanel, Dcreep2;
   Float64 Rps, Rtpsi, Rtpsr, Rgirder, Rcreep1, Rdiaphragm, Rpanel, Rcreep2;

   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi,config,true,&Dtpsi,&Rtpsi );
      GetReleaseTempPrestressDeflection( poi,config,&Dtpsr,&Rtpsr );
      GetGirderDeflectionForCamber( poi,config,&Dgirder,&Rgirder );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi,config, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, config, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
      GetDeckPanelDeflection( poi, config, &Dpanel, &Rpanel );
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi,true,&Dtpsi,&Rtpsi );
      GetReleaseTempPrestressDeflection( poi,&Dtpsr,&Rtpsr );
      GetGirderDeflectionForCamber( poi,&Dgirder,&Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate,&Dcreep1,&Rcreep1 );
      GetDiaphragmDeflection( poi,&Ddiaphragm,&Rdiaphragm );
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate,&Dcreep2,&Rcreep2 );
      GetDeckPanelDeflection( poi,&Dpanel,&Rpanel );
   }

   Float64 D1 = Dgirder + Dps + Dtpsi;
   Float64 D2 = D1 + Dcreep1;
   Float64 D3 = D2 + Ddiaphragm + Dtpsr + Dpanel;
   Float64 D4 = D3 + Dcreep2;
   *pDy = D4;

   Float64 R1 = Rgirder + Rps + Rtpsi;
   Float64 R2 = R1 + Rcreep1;
   Float64 R3 = R2 + Rdiaphragm + Rtpsr + Rpanel;
   Float64 R4 = R3 + Rcreep2;
   *pRz = R4;
}

void CAnalysisAgentImp::GetD_SIP(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   Float64 Dps, Dgirder, Dcreep, Ddiaphragm, Dpanel, Ddeck;
   Float64 Rps, Rgirder, Rcreep, Rdiaphragm, Rpanel, Rdeck;
   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, config, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDeck, constructionRate, &Dcreep, &Rcreep );
      GetDiaphragmDeflection( poi, config, &Ddiaphragm, &Rdiaphragm );
      GetDeckDeflection( poi, config, &Ddeck, &Rdeck );
      GetDeckPanelDeflection( poi, config, &Dpanel, &Rpanel );
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDeck, constructionRate, &Dcreep, &Rcreep );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetDeckDeflection( poi, &Ddeck, &Rdeck );
      GetDeckPanelDeflection( poi, &Dpanel, &Rpanel );
   }

   Float64 D1 = Dgirder + Dps;
   Float64 D2 = D1 + Dcreep;
   Float64 D3 = D2 + Ddiaphragm + Dpanel;
   *pDy = D3;

   Float64 R1 = Rgirder + Rps;
   Float64 R2 = R1 + Rcreep;
   Float64 R3 = R2 + Rdiaphragm + Rpanel;
   *pRz = R3;
}

void CAnalysisAgentImp::GetD_NoDeck_TempStrands(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   // Interpert "D" as deflection before application of superimposed dead loads
   Float64 Dps, Dtpsi, Dtpsr, Dgirder, Dcreep1, Ddiaphragm, Dcreep2, Duser1;
   Float64 Rps, Rtpsi, Rtpsr, Rgirder, Rcreep1, Rdiaphragm, Rcreep2, Ruser1;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
#pragma Reminder("UPDATE: need user loads for all intervals") // right now just useing "bridge site 1 and 2")

   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi,config,true,&Dtpsi,&Rtpsi );
      GetReleaseTempPrestressDeflection( poi,config,&Dtpsr,&Rtpsr );
      GetGirderDeflectionForCamber( poi,config,&Dgirder,&Rgirder );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDiaphragm, constructionRate,&Dcreep1,&Rcreep1 );
      GetDiaphragmDeflection( poi,config,&Ddiaphragm,&Rdiaphragm );
      GetCreepDeflection( poi, config, ICamber::cpDiaphragmToDeck, constructionRate,&Dcreep2,&Rcreep2 );
      GetUserLoadDeflection(castDeckIntervalIdx, poi, config,&Duser1,&Ruser1);
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetInitialTempPrestressDeflection( poi,true,&Dtpsi,&Rtpsi );
      GetReleaseTempPrestressDeflection( poi,&Dtpsr,&Rtpsr );
      GetGirderDeflectionForCamber( poi,&Dgirder,&Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate,&Dcreep1,&Rcreep1 );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
      GetUserLoadDeflection(castDeckIntervalIdx, poi, &Duser1, &Ruser1);
   }

   Float64 D1 = Dgirder + Dps + Dtpsi;
   Float64 D2 = D1 + Dcreep1;
   Float64 D3 = D2 + Ddiaphragm + Dtpsr + Duser1;
   Float64 D4 = D3 + Dcreep2;
   *pDy = D4;

   Float64 R1 = Rgirder + Rps + Rtpsi;
   Float64 R2 = R1 + Rcreep1;
   Float64 R3 = R2 + Rdiaphragm + Rtpsr + Ruser1;
   Float64 R4 = R3 + Rcreep2;
   *pRz = R4;
}

void CAnalysisAgentImp::GetD_NoDeck(const pgsPointOfInterest& poi,bool bUseConfig,const GDRCONFIG& config,CamberModelData& initModelData,CamberModelData& initTempModelData,CamberModelData& releaseTempModelData,Int16 constructionRate,Float64* pDy,Float64* pRz)
{
   // Interpert "D" as deflection before application of superimposed dead loads
   Float64 Dps, Dgirder, Dcreep1, Ddiaphragm, Dcreep2, Duser1;
   Float64 Rps, Rgirder, Rcreep1, Rdiaphragm, Rcreep2, Ruser1;

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
#pragma Reminder("UPDATE: need user loads for all intervals") // right now just useing "bridge site 1 and 2")

   if ( bUseConfig )
   {
      GetPrestressDeflection( poi, config, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, config, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, config, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi, config, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, config, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
      GetUserLoadDeflection(castDeckIntervalIdx, poi, config, &Duser1, &Ruser1);
   }
   else
   {
      GetPrestressDeflection( poi, true, &Dps, &Rps );
      GetGirderDeflectionForCamber( poi, &Dgirder, &Rgirder );
      GetCreepDeflection( poi, ICamber::cpReleaseToDiaphragm, constructionRate, &Dcreep1, &Rcreep1 );
      GetDiaphragmDeflection( poi, &Ddiaphragm, &Rdiaphragm );
      GetCreepDeflection( poi, ICamber::cpDiaphragmToDeck, constructionRate, &Dcreep2, &Rcreep2 );
      GetUserLoadDeflection(castDeckIntervalIdx, poi, &Duser1, &Ruser1);
   }

   Float64 D1 = Dgirder + Dps;
   Float64 D2 = D1 + Dcreep1;
   Float64 D3 = D2 + Ddiaphragm + Duser1;
   Float64 D4 = D3 + Dcreep2;
   *pDy = D4;

   Float64 R1 = Rgirder + Rps;
   Float64 R2 = R1 + Rcreep1;
   Float64 R3 = R2 + Rdiaphragm + Ruser1;
   Float64 R4 = R3 + Rcreep2;
   *pRz = R4;
}

void CAnalysisAgentImp::GetDesignSlabDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rkPPPartUniformLoad beam = GetDesignSlabModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 x = poi.GetDistFromStart() - start_size;

   *pDy = beam.ComputeDeflection(x);
   *pRz = beam.ComputeRotation(x);
}

void CAnalysisAgentImp::GetDesignSlabPadDeflectionAdjustment(Float64 fcgdr,Float64 startSlabOffset,Float64 endSlabOffset,const pgsPointOfInterest& poi,Float64* pDy,Float64* pRz)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   rkPPPartUniformLoad beam = GetDesignSlabPadModel(fcgdr,startSlabOffset,endSlabOffset,poi);

   GET_IFACE(IBridge,pBridge);

   Float64 start_size = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 x = poi.GetDistFromStart() - start_size;

   *pDy = beam.ComputeDeflection(x);
   *pRz = beam.ComputeRotation(x);
}

Float64 CAnalysisAgentImp::GetConcreteStrengthAtTimeOfLoading(const CSegmentKey& segmentKey,LoadingEvent le)
{
   GET_IFACE(IMaterials,pMaterial);
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);
   IntervalIndexType castDeckIntervalIdx = pIntervals->GetCastDeckInterval();

   Float64 Fc;

   switch( le )
   {
   case ICamber::leRelease:
      Fc = pMaterial->GetSegmentFc(segmentKey,releaseIntervalIdx);
      break;

   case ICamber::leDiaphragm:
   case ICamber::leDeck:
      Fc = pMaterial->GetSegmentFc(segmentKey,castDeckIntervalIdx);
      break;

   default:
      ATLASSERT(false); // should never get here
   }

   return Fc;
}

Float64 CAnalysisAgentImp::GetConcreteStrengthAtTimeOfLoading(const GDRCONFIG& config,LoadingEvent le)
{
   Float64 Fc;

   switch( le )
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

ICamber::LoadingEvent CAnalysisAgentImp::GetLoadingEvent(CreepPeriod creepPeriod)
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
void CAnalysisAgentImp::GetContraflexurePoints(const CSpanKey& spanKey,Float64* cfPoints,IndexType* nPoints)
{
   m_pGirderModelManager->GetContraflexurePoints(spanKey,cfPoints,nPoints);
}

/////////////////////////////////////////////////////////////////////////
// IContinuity
bool CAnalysisAgentImp::IsContinuityFullyEffective(const CGirderKey& girderKey)
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

Float64 CAnalysisAgentImp::GetContinuityStressLevel(PierIndexType pierIdx,const CGirderKey& girderKey)
{
   ATLASSERT(girderKey.girderIndex != INVALID_INDEX);

   // for evaluation of LRFD 5.14.1.4.5 - Degree of continuity

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
   // think that 5.14.1.4.5 applies to spliced girder bridges unless we are looking
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
      ATLASSERT(backGroupIdx == aheadGroupIdx-1);
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
   IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
   IntervalIndexType railingSystemIntervalIdx = pIntervals->GetInstallRailingSystemInterval();
   IntervalIndexType overlayIntervalIdx       = pIntervals->GetOverlayInterval();
   IntervalIndexType liveLoadIntervalIdx      = pIntervals->GetLiveLoadInterval();

   IntervalIndexType continuity_interval[2];

   EventIndexType leftContinuityEventIndex, rightContinuityEventIndex;
   pBridge->GetContinuityEventIndex(pierIdx,&leftContinuityEventIndex,&rightContinuityEventIndex);


   // get poi at cl bearing at end of prev group
   if ( backGroupIdx != INVALID_INDEX )
   {
      SegmentIndexType nSegments = pBridge->GetSegmentCount(backGroupIdx,prev_group_gdr_idx);
      CSegmentKey thisSegmentKey(backGroupIdx,prev_group_gdr_idx,nSegments-1);
      std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(thisSegmentKey,POI_ERECTED_SEGMENT | POI_10L,POIFIND_AND));
      ATLASSERT(vPoi.size() == 1);
      vPOI[nPOI] = vPoi.front();
      continuity_interval[nPOI] = pIntervals->GetInterval(leftContinuityEventIndex);
      nPOI++;
   }

   // get poi at cl bearing at start of next group
   if ( aheadGroupIdx != INVALID_INDEX )
   {
      CSegmentKey thisSegmentKey(aheadGroupIdx,next_group_gdr_idx,0);
      std::vector<pgsPointOfInterest> vPoi(pPoi->GetPointsOfInterest(thisSegmentKey,POI_ERECTED_SEGMENT | POI_0L,POIFIND_AND));
      ATLASSERT(vPoi.size() == 1);
      vPOI[nPOI] = vPoi.front();
      continuity_interval[nPOI] = pIntervals->GetInterval(rightContinuityEventIndex);
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

      if ( continuity_interval[i] == castDeckIntervalIdx )
      {
         GetStress(castDeckIntervalIdx,pftSlab,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbSlab = fBottom;

         GetStress(castDeckIntervalIdx,pftSlabPad,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbSlabPad = fBottom;

         GetStress(castDeckIntervalIdx,pftConstruction,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbConstruction = fBottom;
      }
      else
      {
         fbSlab         = 0;
         fbSlabPad      = 0;
         fbConstruction = 0;
      }

      GetStress(railingSystemIntervalIdx,pftTrafficBarrier,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbTrafficBarrier = fBottom;

      GetStress(railingSystemIntervalIdx,pftSidewalk,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
      fbSidewalk = fBottom;

      if ( overlayIntervalIdx != INVALID_INDEX )
      {
         GetStress(overlayIntervalIdx,pftOverlay,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbOverlay = fBottom;
      }
      else
      {
         fbOverlay = 0;
      }

      fbUserDC = 0;
      CSpanKey spanKey;
      Float64 Xspan;
      pPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
      std::vector<IntervalIndexType> vUserDCIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey,pftUserDC);
      BOOST_FOREACH(IntervalIndexType intervalIdx,vUserDCIntervals)
      {
         GetStress(intervalIdx,pftUserDC,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbUserDC += fBottom;
      }

      fbUserDW = 0;
      std::vector<IntervalIndexType> vUserDWIntervals = pIntervals->GetUserDefinedLoadIntervals(spanKey,pftUserDW);
      BOOST_FOREACH(IntervalIndexType intervalIdx,vUserDWIntervals)
      {
         GetStress(intervalIdx,pftUserDW,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
         fbUserDW += fBottom;
      }

      GetStress(compositeDeckIntervalIdx,pftUserLLIM,poi,bat, rtIncremental,pgsTypes::TopGirder,pgsTypes::BottomGirder,&fTop,&fBottom);
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
bool CAnalysisAgentImp::IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation stressLocation)
{
   return IsInPrecompressedTensileZone(poi,limitState,stressLocation,NULL);
}

bool CAnalysisAgentImp::IsInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation stressLocation,const GDRCONFIG* pConfig)
{
   if ( IsDeckStressLocation(stressLocation) )
   {
      return IsDeckInPrecompressedTensileZone(poi,limitState,stressLocation);
   }
   else
   {
      return IsGirderInPrecompressedTensileZone(poi,limitState,stressLocation,pConfig);
   }
}

bool CAnalysisAgentImp::IsDeckPrecompressed(const CGirderKey& girderKey)
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
void CAnalysisAgentImp::GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright)
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

void CAnalysisAgentImp::GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,ProductForceType pfType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright)
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

void CAnalysisAgentImp::GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,Float64* pRleft,Float64* pRright)
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

void CAnalysisAgentImp::GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,std::vector<Float64>* pRleft,std::vector<Float64>* pRright)
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

void CAnalysisAgentImp::GetSegmentReactions(const CSegmentKey& segmentKey,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,Float64* pRleftMin,Float64* pRleftMax,Float64* pRrightMin,Float64* pRrightMax)
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

void CAnalysisAgentImp::GetSegmentReactions(const std::vector<CSegmentKey>& segmentKeys,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat,std::vector<Float64>* pRleftMin,std::vector<Float64>* pRleftMax,std::vector<Float64>* pRrightMin,std::vector<Float64>* pRrightMax)
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

Float64 CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   vSupports.push_back(std::make_pair(supportIdx,supportType));
   std::vector<Float64> reactions(GetReaction(girderKey,vSupports,intervalIdx,pfType,bat,resultsType));

   ATLASSERT(reactions.size() == 1);

   return reactions.front();
}

std::vector<Float64> CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,ProductForceType pfType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   return m_pGirderModelManager->GetReaction(girderKey,vSupports,intervalIdx,pfType,bat,resultsType);
}

Float64 CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   vSupports.push_back(std::make_pair(supportIdx,supportType));
   std::vector<Float64> reactions(GetReaction(girderKey,vSupports,intervalIdx,comboType,bat,resultsType));

   ATLASSERT(reactions.size() == 1);

   return reactions.front();
}

std::vector<Float64> CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,LoadingCombinationType comboType,pgsTypes::BridgeAnalysisType bat, ResultsType resultsType)
{
   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      ComputeTimeDependentEffects(girderKey,intervalIdx);
   }

   if ( comboType == lcPS )
   {
      // secondary effects were requested... the LBAM doesn't have secondary effects... get the product load
      // effects that feed into lcPS
      std::vector<Float64> reactions(vSupports.size(),0.0);
      std::vector<ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      std::vector<ProductForceType>::iterator pfIter(pfTypes.begin());
      std::vector<ProductForceType>::iterator pfIterEnd(pfTypes.end());
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         ProductForceType pfType = *pfIter;
         std::vector<Float64> reaction = GetReaction(girderKey,vSupports,intervalIdx,pfType,bat,resultsType);
         std::transform(reactions.begin(),reactions.end(),reaction.begin(),reactions.begin(),std::plus<Float64>());
      }
      return reactions;
   }
   else
   {
      return m_pGirderModelManager->GetReaction(girderKey,vSupports,intervalIdx,comboType,bat,resultsType);
   }
}

void CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,SupportIndexType supportIdx,pgsTypes::SupportType supportType,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>> vSupports;
   vSupports.push_back(std::make_pair(supportIdx,supportType));
   std::vector<Float64> Rmin, Rmax;
   GetReaction(girderKey,vSupports,intervalIdx,limitState,bat,bIncludeImpact,&Rmin,&Rmax);

   ATLASSERT(Rmin.size() == 1);
   ATLASSERT(Rmax.size() == 1);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
}

void CAnalysisAgentImp::GetReaction(const CGirderKey& girderKey,const std::vector<std::pair<SupportIndexType,pgsTypes::SupportType>>& vSupports,IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,pgsTypes::BridgeAnalysisType bat, bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax)
{
   m_pGirderModelManager->GetReaction(girderKey,vSupports,intervalIdx,limitState,bat,bIncludeImpact,pRmin,pRmax);
}

void CAnalysisAgentImp::GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,AxleConfiguration* pMinAxleConfig,AxleConfiguration* pMaxAxleConfig)
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<Float64> Rmin;
   std::vector<Float64> Rmax;
   std::vector<AxleConfiguration> MinAxleConfig;
   std::vector<AxleConfiguration> MaxAxleConfig;

   GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIndex,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,&Rmin,&Rmax,pMinAxleConfig ? &MinAxleConfig : NULL,pMaxAxleConfig ? &MaxAxleConfig : NULL);

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

void CAnalysisAgentImp::GetVehicularLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,VehicleIndexType vehicleIndex,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<AxleConfiguration>* pMinAxleConfig,std::vector<AxleConfiguration>* pMaxAxleConfig)
{
   m_pGirderModelManager->GetVehicularLiveLoadReaction(intervalIdx,llType,vehicleIndex,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinAxleConfig,pMaxAxleConfig);
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<Float64> Rmin;
   std::vector<Float64> Rmax;
   std::vector<VehicleIndexType> MinConfig;
   std::vector<VehicleIndexType> MaxConfig;

   GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,&Rmin,&Rmax,&MinConfig,&MaxConfig);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
   if ( pMinConfig )
   {
      *pMinConfig = MinConfig.front();
   }
   
   if ( pMaxConfig )
   {
      *pMaxConfig = MaxConfig.front();
   }
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,Float64* pRmin,Float64* pRmax,Float64* pTmin,Float64* pTmax,VehicleIndexType* pMinConfig,VehicleIndexType* pMaxConfig)
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<Float64> Rmin;
   std::vector<Float64> Rmax;
   std::vector<Float64> Tmin;
   std::vector<Float64> Tmax;
   std::vector<VehicleIndexType> MinConfig;
   std::vector<VehicleIndexType> MaxConfig;

   GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,&Rmin,&Rmax,&Tmin,&Tmax,&MinConfig,&MaxConfig);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();

   *pTmin = Tmin.front();
   *pTmax = Tmax.front();

   if ( pMinConfig )
   {
      *pMinConfig = MinConfig.front();
   }
   
   if ( pMaxConfig )
   {
      *pMaxConfig = MaxConfig.front();
   }
}

void CAnalysisAgentImp::GetLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax,std::vector<Float64>* pTmin,std::vector<Float64>* pTmax,std::vector<VehicleIndexType>* pMinConfig,std::vector<VehicleIndexType>* pMaxConfig)
{
   m_pGirderModelManager->GetLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,bIncludeLLDF,pRmin,pRmax,pTmin,pTmax,pMinConfig,pMaxConfig);
}

void CAnalysisAgentImp::GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,PierIndexType pierIdx,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,Float64* pRmin,Float64* pRmax)
{
   std::vector<PierIndexType> vPiers;
   vPiers.push_back(pierIdx);

   std::vector<Float64> Rmin;
   std::vector<Float64> Rmax;

   GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,&Rmin,&Rmax);

   *pRmin = Rmin.front();
   *pRmax = Rmax.front();
}

void CAnalysisAgentImp::GetCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const std::vector<PierIndexType>& vPiers,const CGirderKey& girderKey,pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,std::vector<Float64>* pRmin,std::vector<Float64>* pRmax)
{
   m_pGirderModelManager->GetCombinedLiveLoadReaction(intervalIdx,llType,vPiers,girderKey,bat,bIncludeImpact,pRmin,pRmax);
}

/////////////////////////////////////////////////
// IBearingDesign
bool CAnalysisAgentImp::AreBearingReactionsAvailable(IntervalIndexType intervalIdx,const CGirderKey& girderKey, bool* pbReactionsAtStart, bool* pbReactionsAtEnd)
{
   GET_IFACE(ISpecification,pSpec);
   pgsTypes::AnalysisType analysisType = pSpec->GetAnalysisType();

   GET_IFACE(IBridge,pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();
   PierIndexType nPiers = pBridge->GetPierCount();
   if (nSpans == 1 || analysisType == pgsTypes::Simple)
   {
      // Always can get for bearing reactions for single span or simple spans.
      *pbReactionsAtStart = true;
      *pbReactionsAtEnd   = true;
      return true;
   }
   else
   {
      if (girderKey.groupIndex == ALL_GROUPS)
      {
         *pbReactionsAtStart = true;
         *pbReactionsAtEnd   = true;
         return true;
      }
      else
      {
         // Get boundary conditions at both ends of span
         PierIndexType startPierIdx = pBridge->GetGirderGroupStartPier(girderKey.groupIndex);
         PierIndexType endPierIdx   = pBridge->GetGirderGroupEndPier(girderKey.groupIndex);

         bool bdummy;
         bool bContinuousOnLeft, bContinuousOnRight;
         pBridge->IsContinuousAtPier(startPierIdx, &bdummy,            &bContinuousOnLeft);
         pBridge->IsContinuousAtPier(endPierIdx,   &bContinuousOnRight,&bdummy);

         bool bIntegralOnLeft, bIntegralOnRight;
         pBridge->IsIntegralAtPier(startPierIdx,  &bdummy,          &bIntegralOnLeft);
         pBridge->IsIntegralAtPier(endPierIdx,    &bIntegralOnRight,&bdummy);

         // Bearing reactions are available at simple supports
         bool bSimpleAtStart, bSimpleAtEnd;
         bSimpleAtStart = !bContinuousOnLeft  && !bIntegralOnLeft;
         bSimpleAtEnd   = !bContinuousOnRight && !bIntegralOnRight;

         // Cantilever piers are treated as continuous, however they are really bearing piers
         // like simple span piers.
         if ( pBridge->HasCantilever(startPierIdx) )
         {
            bSimpleAtStart = true;
         }

         if ( pBridge->HasCantilever(endPierIdx) )
         {
            bSimpleAtEnd = true;
         }

         // Finally check if interval is before continuity
         if ( !bSimpleAtStart )
         {
            GET_IFACE(IIntervals,pIntervals);
            EventIndexType dummyEventIdx, eventIdx;
            pBridge->GetContinuityEventIndex(startPierIdx,&dummyEventIdx,&eventIdx);
            IntervalIndexType continuityIntervalIdx = pIntervals->GetInterval(eventIdx);

            if ( intervalIdx < continuityIntervalIdx )
            {
               bSimpleAtStart = true;
            }
         }

         if ( !bSimpleAtEnd )
         {
            GET_IFACE(IIntervals,pIntervals);
            EventIndexType dummyEventIdx, eventIdx;
            pBridge->GetContinuityEventIndex(endPierIdx,&eventIdx,&dummyEventIdx);
            IntervalIndexType continuityIntervalIdx = pIntervals->GetInterval(eventIdx);

            if ( intervalIdx < continuityIntervalIdx )
            {
               bSimpleAtEnd = true;
            }
         }

         *pbReactionsAtStart = bSimpleAtStart;
         *pbReactionsAtEnd   = bSimpleAtEnd;

          // Return true if we have simple supports at either end of girder
         if (bSimpleAtStart || bSimpleAtEnd)
         {
            return true;
         }
         else
         {
            return false;
         }
      }
   }
}

void CAnalysisAgentImp::GetBearingProductReaction(IntervalIndexType intervalIdx,ProductForceType pfType,const CGirderKey& girderKey,
                                                  pgsTypes::BridgeAnalysisType bat,ResultsType resultsType, Float64* pLftEnd,Float64* pRgtEnd)
{
   if ( pfType == pftPretension || pfType == pftPrimaryPostTensioning )
   {
      // Pretension and primary post-tension are internal self equilibriating forces
      // they don't cause external reactions
      *pLftEnd = 0;
      *pRgtEnd = 0;
   }
   else if ( pfType == pftSecondaryEffects )
   {
      Float64 RLeftTotal, RRightTotal;
      Float64 RLeftPrimary, RRightPrimary;
      GetBearingProductReaction(intervalIdx,pftEquivPostTensioning,  girderKey,bat,resultsType,&RLeftTotal,&RRightTotal);
      GetBearingProductReaction(intervalIdx,pftPrimaryPostTensioning,girderKey,bat,resultsType,&RLeftPrimary,&RRightPrimary);
      *pLftEnd = RLeftTotal - RLeftPrimary;
      *pRgtEnd = RRightTotal - RRightPrimary;
   }
   else
   {
      m_pGirderModelManager->GetBearingProductReaction(intervalIdx,pfType,girderKey,bat,resultsType,pLftEnd,pRgtEnd);
   }
}

void CAnalysisAgentImp::GetBearingLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,
                                pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                Float64* pLeftRmin,Float64* pLeftRmax,Float64* pLeftTmin,Float64* pLeftTmax,
                                Float64* pRightRmin,Float64* pRightRmax,Float64* pRightTmin,Float64* pRightTmax,
                                VehicleIndexType* pLeftMinVehIdx,VehicleIndexType* pLeftMaxVehIdx,
                                VehicleIndexType* pRightMinVehIdx,VehicleIndexType* pRightMaxVehIdx)
{
   m_pGirderModelManager->GetBearingLiveLoadReaction(intervalIdx,llType,girderKey,bat,bIncludeImpact,bIncludeLLDF,pLeftRmin,pLeftRmax,pLeftTmin,pLeftTmax,pRightRmin,pRightRmax,pRightTmin,pRightTmax,pLeftMinVehIdx,pLeftMaxVehIdx,pRightMinVehIdx,pRightMaxVehIdx);
}

void CAnalysisAgentImp::GetBearingLiveLoadRotation(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,
                                                   pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,bool bIncludeLLDF, 
                                                   Float64* pLeftTmin,Float64* pLeftTmax,Float64* pLeftRmin,Float64* pLeftRmax,
                                                   Float64* pRightTmin,Float64* pRightTmax,Float64* pRightRmin,Float64* pRightRmax,
                                                   VehicleIndexType* pLeftMinVehIdx,VehicleIndexType* pLeftMaxVehIdx,
                                                   VehicleIndexType* pRightMinVehIdx,VehicleIndexType* pRightMaxVehIdx)
{
   m_pGirderModelManager->GetBearingLiveLoadRotation(intervalIdx,llType,girderKey,bat,bIncludeImpact,bIncludeLLDF,pLeftTmin,pLeftTmax,pLeftRmin,pLeftRmax,pRightTmin,pRightTmax,pRightRmin,pRightRmax,pLeftMinVehIdx,pLeftMaxVehIdx,pRightMinVehIdx,pRightMaxVehIdx);
}

void CAnalysisAgentImp::GetBearingCombinedReaction(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const CGirderKey& girderKey,
                                                   pgsTypes::BridgeAnalysisType bat,ResultsType resultsType, Float64* pLftEnd,Float64* pRgtEnd)
{
   if ( comboType == lcPS )
   {
      std::vector<ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);
      std::vector<ProductForceType>::iterator pfIter(pfTypes.begin());
      std::vector<ProductForceType>::iterator pfIterEnd(pfTypes.end());
      *pLftEnd = 0;
      *pRgtEnd = 0;
      for ( ; pfIter != pfIterEnd; pfIter++ )
      {
         ProductForceType pfType = *pfIter;
         Float64 Rleft, Rright;
         GetBearingProductReaction(intervalIdx,pfType,girderKey,bat,resultsType,&Rleft,&Rright);

         *pLftEnd += Rleft;
         *pRgtEnd += Rright;
      }
   }
   else
   {
      m_pGirderModelManager->GetBearingCombinedReaction(intervalIdx,comboType,girderKey,bat,resultsType,pLftEnd,pRgtEnd);
   }
}

void CAnalysisAgentImp::GetBearingCombinedLiveLoadReaction(IntervalIndexType intervalIdx,pgsTypes::LiveLoadType llType,const CGirderKey& girderKey,
                                                           pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                                           Float64* pLeftRmin, Float64* pLeftRmax, Float64* pRightRmin,Float64* pRightRmax)
{
   m_pGirderModelManager->GetBearingCombinedLiveLoadReaction(intervalIdx,llType,girderKey,bat,bIncludeImpact,pLeftRmin,pLeftRmax,pRightRmin,pRightRmax);
}

void CAnalysisAgentImp::GetBearingLimitStateReaction(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const CGirderKey& girderKey,
                                                     pgsTypes::BridgeAnalysisType bat,bool bIncludeImpact,
                                                     Float64* pLeftRmin, Float64* pLeftRmax, Float64* pRightRmin,Float64* pRightRmax)
{
   m_pGirderModelManager->GetBearingLimitStateReaction(intervalIdx,limitState,girderKey,bat,bIncludeImpact,pLeftRmin,pLeftRmax,pRightRmin,pRightRmax);
}

Float64 CAnalysisAgentImp::GetDeflectionAdjustmentFactor(const pgsPointOfInterest& poi,const GDRCONFIG& config,IntervalIndexType intervalIdx)
{
   const CSegmentKey& segmentKey = poi.GetSegmentKey();

   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);

   Float64 fc = (intervalIdx < erectionIntervalIdx ? config.Fci : config.Fc);

   GET_IFACE(ISectionProperties,pSectProp);

   Float64 Ix          = pSectProp->GetIx(intervalIdx,poi);
   Float64 Ix_adjusted = pSectProp->GetIx(intervalIdx,poi,fc);

   GET_IFACE(IMaterials,pMaterials);
   Float64 Ec = pMaterials->GetSegmentEc(poi.GetSegmentKey(),intervalIdx);
   Float64 Ec_adjusted = (config.bUserEc ? config.Ec : pMaterials->GetEconc(fc,pMaterials->GetSegmentStrengthDensity(poi.GetSegmentKey()),
                                                                               pMaterials->GetSegmentEccK1(poi.GetSegmentKey()),
                                                                               pMaterials->GetSegmentEccK2(poi.GetSegmentKey())));

   Float64 EI = Ec*Ix;
   Float64 EI_adjusted = Ec_adjusted * Ix_adjusted;

   Float64 k = (IsZero(EI_adjusted) ? 0 : EI/EI_adjusted);

   return k;
}

bool CAnalysisAgentImp::IsDeckInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation stressLocation)
{
   ATLASSERT(IsDeckStressLocation(stressLocation));

   GET_IFACE(IBridge,pBridge);
   if ( pBridge->GetDeckType() == pgsTypes::sdtNone )
   {
      // if there is no deck, the deck can't be in the PTZ
      return false;
   }

   const CSegmentKey& segmentKey(poi.GetSegmentKey());

   // Get the stress when the bridge is in service (that is when live load is applied)
   GET_IFACE(IIntervals,pIntervals);
   IntervalIndexType serviceLoadIntervalIdx = pIntervals->GetLiveLoadInterval();

   // Tensile stresses are greatest at the top of the deck using the minimum model in
   // Envelope mode. In all other modes, Min/Max are the same
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(pgsTypes::Minimize);

   Float64 fMin, fMax;
   GetStress(serviceLoadIntervalIdx,limitState,poi,bat,false/*without prestress*/,stressLocation,&fMin,&fMax);
   if ( fMax <= 0 )
   {
      return false; // the location is not in tension so is not in the "tension zone"
   }

   // The section is in tension, does the prestress cause compression?
   Float64 fPreTension  = GetStress(serviceLoadIntervalIdx,poi,stressLocation);
   Float64 fPostTension = GetStress(serviceLoadIntervalIdx,poi,stressLocation,ALL_DUCTS);
   Float64 fPS = fPreTension + fPostTension;

   if ( 0 <= fPS )
   {
      return false; // prestressing does not cause compression here so it is not a "precompressed" location
   }

   return true;
}

bool CAnalysisAgentImp::IsGirderInPrecompressedTensileZone(const pgsPointOfInterest& poi,pgsTypes::LimitState limitState,pgsTypes::StressLocation stressLocation,const GDRCONFIG* pConfig)
{
   ATLASSERT(IsGirderStressLocation(stressLocation));

   // The specified location is in a precompressed tensile zone if the following requirements are true
   // 1) The location is in tension in the Service III limit state for the final interval with the
   //    contribution of prestressing
   // 2) Prestressing causes compression at the location

   // First deal with the special cases

   // Special case... At the start/end of the first/last segment the stress due to 
   // externally applied loads is zero (moment is zero) for roller/hinge
   // boundary condition and the stress due to the prestressing is also zero (strands not developed). 
   // Consider the bottom of the girder to be in a precompressed tensile zone
   GET_IFACE(IBridge,pBridge);
   const CSegmentKey& segmentKey(poi.GetSegmentKey());
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
            return (stressLocation == pgsTypes::BottomGirder ? true : false);
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
            return (stressLocation == pgsTypes::BottomGirder ? true : false);
         }
      }
   }

   // Special case... ends of any segment for stressing at release
   // Even though there is prestress, at the end faces of the girder there isn't
   // any prestress force because it hasn't been transfered to the girder yet. The
   // prestress force transfers over the transfer length.
   if ( poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 1 || poi.IsTenthPoint(POI_RELEASED_SEGMENT) == 11 )
   {
      return (stressLocation == pgsTypes::BottomGirder ? true : false);
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

   if ( IsZero(Pjack) )
   {
      return (stressLocation == pgsTypes::BottomGirder ? true : false);
   }

   // Special case... if the POI is located "near" interior supports with continuous boundary
   // condition tension develops in the top of the girder and prestressing may cause compression
   // in this location (most likely from harped strands). From LRFD C5.14.1.4.6, this location is not
   // in a precompressed tensile zone. Assume that "near" means that the POI is somewhere between 
   // mid-span and the closest pier.
   if ( stressLocation == pgsTypes::TopGirder )
   {
      SpanIndexType startSpanIdx, endSpanIdx;
      pBridge->GetSpansForSegment(segmentKey,&startSpanIdx,&endSpanIdx);

      GET_IFACE(IPointOfInterest,pIPoi);
      CSpanKey spanKey;
      Float64 Xspan;
      pIPoi->ConvertPoiToSpanPoint(poi,&spanKey,&Xspan);
      PierIndexType startPierIdx = (PierIndexType)spanKey.spanIndex;
      PierIndexType endPierIdx = startPierIdx+1;

      // some segments are not supported by piers at all. segments can be supported by
      // just temporary supports. an example would be the center segment in a three-segment
      // single span bridge. This special case doesn't apply to that segment.
      bool bStartAtPier = true;  // start end of segment bears on a pier
      bool bEndAtPier   = true;  // end end of segment bears on a pier

      // one of the piers must be a boundary pier or C5.14.1.4.6 doesn't apply
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
            // and we can't make a direct determination if C5.14.1.4.6 applies.
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

               // if hinge or roller boundary condition, C5.14.1.4.6 doesn't apply.
               if ( boundaryConditionType != pgsTypes::bctRoller && boundaryConditionType != pgsTypes::bctHinge )
               {
                  // connection type is some sort of continuity/integral boundary condition
                  // The top of the girder is not in the PTZ.
                  return false;
               }
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
   pgsTypes::BridgeAnalysisType bat = GetBridgeAnalysisType(::IsTopStressLocation(stressLocation) ? pgsTypes::Minimize : pgsTypes::Maximize);

   // Get stresses due to service loads
   Float64 fMin, fMax;
   GetStress(serviceLoadIntervalIdx,limitState,poi,bat,false/*without prestress*/,stressLocation,&fMin,&fMax);
   if ( fMax <= 0 )
   {
      return false; // the location is not in tension so is not in the "tension zone"
   }

   // The section is in tension, does the prestress cause compression in the service load interval?
   Float64 fPreTension, fPostTension;
   if ( pConfig )
   {
      fPreTension = GetDesignStress(serviceLoadIntervalIdx,limitState,poi,stressLocation,*pConfig);
      fPostTension = 0; // no post-tensioning for precast girder design
   }
   else
   {
      fPreTension  = GetStress(serviceLoadIntervalIdx,poi,stressLocation);
      fPostTension = GetStress(serviceLoadIntervalIdx,poi,stressLocation,ALL_DUCTS);
   }
   
   Float64 fPS = fPreTension + fPostTension;
   if ( 0 <= fPS )
   {
      return false; // prestressing does not cause compression here so it is not a "precompressed" location
   }

   return true;
}


void CAnalysisAgentImp::GetTimeStepStress(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pfTop->clear();
   pfBot->clear();

   GET_IFACE(ILosses,pLosses);

   pgsTypes::FaceType topFace = (IsTopStressLocation(topLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);
   pgsTypes::FaceType botFace = (IsTopStressLocation(botLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);

   std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
   for ( ; iter != end; iter++ )
   {
      const pgsPointOfInterest& poi(*iter);
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_CONCRETE* pTopConcreteElement = (IsGirderStressLocation(topLocation) ? &tsDetails.Girder : &tsDetails.Deck);
      const TIME_STEP_CONCRETE* pBotConcreteElement = (IsGirderStressLocation(botLocation) ? &tsDetails.Girder : &tsDetails.Deck);

      Float64 fTop, fBot;
      if ( pfType == pftTotalPostTensioning )
      {
         // total post-tensioning is primary (P*e) + secondary
         fTop = pTopConcreteElement->f[topFace][pftPrimaryPostTensioning][resultsType]
              + pTopConcreteElement->f[topFace][pftSecondaryEffects     ][resultsType];

         fBot = pBotConcreteElement->f[botFace][pftPrimaryPostTensioning][resultsType]
              + pBotConcreteElement->f[botFace][pftSecondaryEffects     ][resultsType];
      }
      else
      {
         fTop = pTopConcreteElement->f[topFace][pfType][resultsType];
         fBot = pBotConcreteElement->f[botFace][pfType][resultsType];
      }

      pfTop->push_back(fTop);
      pfBot->push_back(fBot);
   }
}

void CAnalysisAgentImp::GetElasticStress(IntervalIndexType intervalIdx,ProductForceType pfType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   USES_CONVERSION;

   pfTop->clear();
   pfBot->clear();

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

   try
   {
      GET_IFACE(IIntervals,pIntervals);
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(segmentKey);
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

         std::transform(fTopThis.begin(),fTopThis.end(),fTopPrev.begin(),std::back_inserter(*pfTop),std::minus<Float64>());
         std::transform(fBotThis.begin(),fBotThis.end(),fBotPrev.begin(),std::back_inserter(*pfBot),std::minus<Float64>());
      }
      else
      {
         m_pGirderModelManager->GetStress(intervalIdx,pfType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
   }
   catch(...)
   {
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetTimeStepStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pfTop->clear();
   pfBot->clear();

   GET_IFACE(ILosses,pLosses);

   std::vector<ProductForceType> pfTypes = CProductLoadMap::GetProductForces(m_pBroker,comboType);

  pgsTypes::FaceType topFace = (IsTopStressLocation(topLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);
  pgsTypes::FaceType botFace = (IsTopStressLocation(botLocation) ? pgsTypes::TopFace : pgsTypes::BottomFace);

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
   for ( ; poiIter != poiIterEnd; poiIter++ )
   {
      const pgsPointOfInterest& poi(*poiIter);
      const LOSSDETAILS* pDetails = pLosses->GetLossDetails(poi,intervalIdx);
      const TIME_STEP_DETAILS& tsDetails(pDetails->TimeStepDetails[intervalIdx]);

      const TIME_STEP_CONCRETE* pTopConcreteElement = (IsGirderStressLocation(topLocation) ? &tsDetails.Girder : &tsDetails.Deck);
      const TIME_STEP_CONCRETE* pBotConcreteElement = (IsGirderStressLocation(botLocation) ? &tsDetails.Girder : &tsDetails.Deck);

      Float64 fTop(0), fBot(0);
      std::vector<ProductForceType>::iterator pfTypeIter(pfTypes.begin());
      std::vector<ProductForceType>::iterator pfTypeIterEnd(pfTypes.end());
      for ( ; pfTypeIter != pfTypeIterEnd; pfTypeIter++ )
      {
         ProductForceType pfType = *pfTypeIter;
         fTop += pTopConcreteElement->f[topFace][pfType][resultsType];
         fBot += pBotConcreteElement->f[botFace][pfType][resultsType];
      }

      pfTop->push_back(fTop);
      pfBot->push_back(fBot);
   }
}

void CAnalysisAgentImp::GetElasticStress(IntervalIndexType intervalIdx,LoadingCombinationType comboType,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,ResultsType resultsType,pgsTypes::StressLocation topLocation,pgsTypes::StressLocation botLocation,std::vector<Float64>* pfTop,std::vector<Float64>* pfBot)
{
   USES_CONVERSION;

   //if comboType is  lcCR, lcSH, or lcRE, need to do the time-step analysis because it adds loads to the LBAM
   if ( comboType == lcCR || comboType == lcSH || comboType == lcRE )
   {
      ComputeTimeDependentEffects(vPoi.front().GetSegmentKey(),intervalIdx);
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
      IntervalIndexType erectionIntervalIdx = pIntervals->GetErectSegmentInterval(vPoi.front().GetSegmentKey());
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

         std::transform(fTopThis.begin(),fTopThis.end(),fTopPrev.begin(),std::back_inserter(*pfTop),std::minus<Float64>());
         std::transform(fBotThis.begin(),fBotThis.end(),fBotPrev.begin(),std::back_inserter(*pfBot),std::minus<Float64>());
      }
      else
      {
         m_pGirderModelManager->GetStress(intervalIdx,comboType,vPoi,bat,resultsType,topLocation,botLocation,pfTop,pfBot);
      }
   }
   catch(...)
   {
      Invalidate(false);
      throw;
   }
}

void CAnalysisAgentImp::GetTimeStepStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation stressLocation,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   ATLASSERT(bat == pgsTypes::ContinuousSpan); // continous is the only valid analysis type for time step analysis

   pMin->clear();
   pMax->clear();

   GET_IFACE(ILoadFactors,pILoadFactors);
   const CLoadFactors* pLoadFactors = pILoadFactors->GetLoadFactors();
   Float64 gLLMin = pLoadFactors->LLIMmin[limitState];
   Float64 gLLMax = pLoadFactors->LLIMmax[limitState];
   Float64 gDCMin = pLoadFactors->DCmin[limitState];
   Float64 gDCMax = pLoadFactors->DCmax[limitState];
   Float64 gDWMin = pLoadFactors->DWmin[limitState];
   Float64 gDWMax = pLoadFactors->DWmax[limitState];
   Float64 gCRMax = pLoadFactors->CRmax[limitState];
   Float64 gCRMin = pLoadFactors->CRmin[limitState];
   Float64 gSHMax = pLoadFactors->SHmax[limitState];
   Float64 gSHMin = pLoadFactors->SHmin[limitState];
   Float64 gPSMax = pLoadFactors->PSmax[limitState]; // this is for secondary effects due to PT
   Float64 gPSMin = pLoadFactors->PSmin[limitState]; // this is for secondary effects due to PT

   // LRFD doesn't have a relaxation load case... creep is it's closest cousin
   Float64 gREMax = gCRMax;
   Float64 gREMin = gCRMin;

   // Use half prestress if Service IA or Fatigue I (See LRFD Table 5.9.4.2.1-1)
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

   std::vector<pgsPointOfInterest>::const_iterator poiIter(vPoi.begin());
   std::vector<pgsPointOfInterest>::const_iterator poiIterEnd(vPoi.end());
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
         fPR = pConcreteElement->f[face][pftPretension           ][rtCumulative];
         fPT = pConcreteElement->f[face][pftPrimaryPostTensioning][rtCumulative];
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

void CAnalysisAgentImp::GetElasticStress(IntervalIndexType intervalIdx,pgsTypes::LimitState limitState,const std::vector<pgsPointOfInterest>& vPoi,pgsTypes::BridgeAnalysisType bat,bool bIncludePrestress,pgsTypes::StressLocation stressLocation,std::vector<Float64>* pMin,std::vector<Float64>* pMax)
{
   USES_CONVERSION;

   const CSegmentKey& segmentKey(vPoi.front().GetSegmentKey());

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
   }
   catch(...)
   {
      Invalidate(false);
      throw;
   }
}
