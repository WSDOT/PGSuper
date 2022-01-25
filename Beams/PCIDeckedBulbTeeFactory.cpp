///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

// PCIDeckedBulbTeeFactory.cpp : Implementation of CPCIDeckedBulbTeeFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "PCIDeckedBulbTeeFactory.h"
#include <IFace\DistFactorEngineer.h>
#include <IFace\PsLossEngineer.h>
#include <GeomModel\PrecastBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\Alignment.h>
#include <IFace\AgeAdjustedMaterial.h>

#include <Beams\Helper.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderLabel.h>

#include <IFace\StatusCenter.h>
#include <PgsExt\StatusItem.h>

#include <Beams\Interfaces.h>
#include <Plugins\CLSID.h>
#include <Plugins\ConfigureStrandMover.h>
#include <WBFLGenericBridgeTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CPCIDeckedBulbTeeFactory
HRESULT CPCIDeckedBulbTeeFactory::FinalConstruct()
{
   StatusGroupIDType m_StatusGroupID = INVALID_ID;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   // It's possible for the library editor to call this code. In that case there is no broker
   if (pBroker)
   {
      GET_IFACE2(pBroker, IEAFStatusCenter, pStatusCenter);
      m_scidInformationalWarning     = pStatusCenter->RegisterCallback(new pgsInformationalStatusCallback(eafTypes::statusWarning)); 
   }
   else
   {
      m_scidInformationalWarning = 0;
   }

   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.emplace_back(_T("W1")); // W1 in shape
   m_DimNames.emplace_back(_T("W2")); // W6 in shape
   m_DimNames.emplace_back(_T("W3")); // W3 in shape
   m_DimNames.emplace_back(_T("W4"));
   m_DimNames.emplace_back(_T("W5"));
   m_DimNames.emplace_back(_T("D1"));
   m_DimNames.emplace_back(_T("D2"));
   m_DimNames.emplace_back(_T("D3"));
   m_DimNames.emplace_back(_T("D4"));
   m_DimNames.emplace_back(_T("D5"));
   m_DimNames.emplace_back(_T("D6"));
   m_DimNames.emplace_back(_T("D7"));
   m_DimNames.emplace_back(_T("D8"));
   m_DimNames.emplace_back(_T("D9"));
   m_DimNames.emplace_back(_T("H"));
   m_DimNames.emplace_back(_T("T"));
   m_DimNames.emplace_back(_T("R1"));
   m_DimNames.emplace_back(_T("R2"));
   m_DimNames.emplace_back(_T("R3"));
   m_DimNames.emplace_back(_T("C1"));
   m_DimNames.emplace_back(_T("Wmax")); // W1 + 2(W2) for the shape dimensions
   m_DimNames.emplace_back(_T("Wmin"));

   // Default beam
   m_DefaultDims.emplace_back(::ConvertToSysUnits(26.0, unitMeasure::Inch)); // W1
   m_DefaultDims.emplace_back(::ConvertToSysUnits(38.375, unitMeasure::Inch)); // W2
   m_DefaultDims.emplace_back(::ConvertToSysUnits(7.5, unitMeasure::Inch)); // W3
   m_DefaultDims.emplace_back(::ConvertToSysUnits(1.0, unitMeasure::Inch)); // W4
   m_DefaultDims.emplace_back(::ConvertToSysUnits(5.5, unitMeasure::Inch)); // W5
   m_DefaultDims.emplace_back(::ConvertToSysUnits(8.0,unitMeasure::Inch)); // D1
   m_DefaultDims.emplace_back(::ConvertToSysUnits(1.0,unitMeasure::Inch)); // D2
   m_DefaultDims.emplace_back(::ConvertToSysUnits(0.5,unitMeasure::Inch)); // D3
   m_DefaultDims.emplace_back(::ConvertToSysUnits(0.5,unitMeasure::Inch)); // D4
   m_DefaultDims.emplace_back(::ConvertToSysUnits(1.0,unitMeasure::Inch)); // D5
   m_DefaultDims.emplace_back(::ConvertToSysUnits(9.000,unitMeasure::Inch)); // D6
   m_DefaultDims.emplace_back(::ConvertToSysUnits(3.000, unitMeasure::Inch)); // D7
   m_DefaultDims.emplace_back(::ConvertToSysUnits(10.000, unitMeasure::Inch)); // D8
   m_DefaultDims.emplace_back(::ConvertToSysUnits(3.75, unitMeasure::Inch)); // D9
   m_DefaultDims.emplace_back(::ConvertToSysUnits(54.000, unitMeasure::Inch)); // H
   m_DefaultDims.emplace_back(::ConvertToSysUnits(4.000, unitMeasure::Inch)); // T
   m_DefaultDims.emplace_back(::ConvertToSysUnits(9.000,unitMeasure::Inch)); // R1
   m_DefaultDims.emplace_back(::ConvertToSysUnits(12.0,unitMeasure::Inch)); // R2
   m_DefaultDims.emplace_back(::ConvertToSysUnits(2.0, unitMeasure::Inch)); // R3
   m_DefaultDims.emplace_back(::ConvertToSysUnits(1.000, unitMeasure::Inch)); // C1
   m_DefaultDims.emplace_back(::ConvertToSysUnits(10.333333333333, unitMeasure::Feet)); // Wmax
   m_DefaultDims.emplace_back(::ConvertToSysUnits(2.333333333333, unitMeasure::Feet)); // Wmin

   // SI Units
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W4
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W6
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D3
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D4
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D5
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D6
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D7
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D8
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D9
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // H
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // T
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // R1
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // R2
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // R3
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].emplace_back(&unitMeasure::Meter);      // Wmax
   m_DimUnits[0].emplace_back(&unitMeasure::Meter);      // Wmin

   // US Units
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W4
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W5
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D3
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D4
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D5
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D6
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D7
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D8
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D9
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // H
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // T
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // R1
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // R2
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // R3
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].emplace_back(&unitMeasure::Feet); // Wmax
   m_DimUnits[1].emplace_back(&unitMeasure::Feet); // Wmin

   return S_OK;
}

void CPCIDeckedBulbTeeFactory::CreateGirderSection(IBroker* pBroker,StatusItemIDType statusID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   m_StatusGroupID = statusID; // catch status group id here so we can use it later

   ATLASSERT(overallHeight < 0); // not a variable depth section
   ATLASSERT(bottomFlangeHeight < 0); // not a variable bottom flange section

   CComPtr<IPCIDeckedBulbTeeSection> gdrSection;
   gdrSection.CoCreateInstance(CLSID_PCIDeckedBulbTeeSection);

   CComPtr<IPCIDeckedIBeam> beam;
   gdrSection->get_Beam(&beam);

   Float64 c1;
   Float64 d1, d2, d3, d4, d5, d6, d7, d8, d9;
   Float64 h, t;
   Float64 w1,w2,w3,w4,w5,wmin,wmax;
   Float64 r1,r2,r3;
   GetDimensions(dimensions,w1,w2,w3,w4,w5,d1,d2,d3,d4,d5,d6,d7,d8,d9,h,t,r1,r2,r3,c1,wmin,wmax);

   beam->put_W1(w1);
   beam->put_W3(w3);
   beam->put_W6(w2); // w2 in UI is the bottom flange width, which is w6 on the shape

   if ( pBroker == nullptr )
   {
      beam->put_W2(wmax / 2 - w1 / 2);
   }
   else
   {
      // Assumes uniform girder spacing

      // use raw input here because requesting it from the bridge will cause an infinite loop.
      // bridge agent calls this during validation
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      Float64 topWidth = pBridgeDesc->GetGirderTopWidth(); // we don't have a girder key so best we can do is get the top level value (which may not be valid)
      beam->put_W2(topWidth / 2 - w1 / 2);
   }

   beam->put_W4(w4);
   beam->put_W5(w5);

   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_D3(d3);
   beam->put_D4(d4);
   beam->put_D5(d5);
   beam->put_D6(d6);
   beam->put_D7(d7);
   beam->put_D8(d8);
   beam->put_D9(d9);

   beam->put_H(h);
   beam->put_T(t);
   beam->put_R1(r1);
   beam->put_R2(r2);
   beam->put_R3(r3);
   beam->put_C1(c1);

   PositionBeamShape(beam);

   gdrSection.QueryInterface(ppSection);
}

void CPCIDeckedBulbTeeFactory::CreateSegment(IBroker* pBroker,StatusItemIDType statusID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
{
   CComPtr<ISuperstructureMemberSegment> segment;
   segment.CoCreateInstance(CLSID_ThickenedFlangeBulbTeeSegment);

   ATLASSERT(segment != nullptr);

   segment.CopyTo(ppSegment);
}

void CPCIDeckedBulbTeeFactory::CreateSegmentShape(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   // Create basic beam shape from dimensions in girder library
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   Float64 c1;
   Float64 d1, d2, d3, d4, d5, d6, d7, d8, d9;
   Float64 h, t;
   Float64 w1, w2, w3, w4, w5, wmin, wmax;
   Float64 r1, r2, r3;
   GetDimensions(dimensions, w1, w2, w3, w4, w5, d1, d2, d3, d4, d5, d6, d7, d8, d9, h, t, r1, r2, r3, c1, wmin, wmax);

   CComPtr<IPCIDeckedIBeam> beam;
   beam.CoCreateInstance(CLSID_PCIDeckedIBeam);

   beam->put_W1(w1);
   beam->put_W2(wmax / 2 - w1 / 2); // dummy value... will get fixed in ConfigureBeamShape
   beam->put_W3(w3);
   beam->put_W6(w2); // w2 in UI is the bottom flange width, which is w6 on the shape
   beam->put_W4(w4);
   beam->put_W5(w5);

   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_D3(d3);
   beam->put_D4(d4);
   beam->put_D5(d5);
   beam->put_D6(d6);
   beam->put_D7(d7);
   beam->put_D8(d8);
   beam->put_D9(d9);

   beam->put_H(h);
   beam->put_T(t);
   beam->put_R1(r1);
   beam->put_R2(r2);
   beam->put_R3(r3);
   beam->put_C1(c1);

   // now configure the beam based on the girder modifiers and applicable roadway effects
   ConfigureBeamShape(pBroker, pSegment, beam);

   //// lastly, since we are creating a shape at a specific location, we need to apply the top flange thickening if any
   //Float64 tft = GetFlangeThickening(pBroker, pSegment, Xs);
   //beam->put_D1(d1 + tft);

   PositionBeamShape(beam);

   beam.QueryInterface(ppShape);
}

Float64 CPCIDeckedBulbTeeFactory::GetSegmentHeight(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   return GetDimension(dimensions, _T("H"));

   //Float64 H = GetBeamHeight(dimensions, pgsTypes::metStart); // this is the basic beam height

   //// Compute the increase in height due to cross slope in the top flange
   //// compute by measuring from each flange tip to get max value
   //Float64 c, n1, n2, left, right;
   //GetTopFlangeParameters(pBroker, pSegment, &c, &n1, &n2, &left, &right);
   //Float64 t_slope = Max(c*n1, (left + right - c)*n2);

   //// Longitudinal top flanging thickening
   //Float64 tft = GetFlangeThickening(pBroker, pSegment, Xs);

   //H += tft + t_slope;

   //return H;
}

void CPCIDeckedBulbTeeFactory::ConfigureSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   bool bPrismatic = IsPrismatic(pSegment);

   bool bHasLongitudinalJoints = pBridgeDesc->HasStructuralLongitudinalJoints();

   // Build up the beam shape
   // Beam materials
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   CComPtr<IMaterial> material;
   CComPtr<IMaterial> jointMaterial;
   if (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP)
   {
      CComPtr<IAgeAdjustedMaterial> aaMaterial;
      BuildAgeAdjustedGirderMaterialModel(pBroker, pSegment, pSSMbrSegment, &aaMaterial);
      aaMaterial.QueryInterface(&material);

      if (bHasLongitudinalJoints)
      {
         CComPtr<IAgeAdjustedMaterial> aaJointMaterial;
         BuildAgeAdjustedJointMaterialModel(pBroker, pSegment, pSSMbrSegment, &aaJointMaterial);
         aaJointMaterial.QueryInterface(&jointMaterial);
      }
   }
   else
   {
      GET_IFACE2(pBroker, IIntervals, pIntervals);
      GET_IFACE2(pBroker, IMaterials, pMaterial);
      material.CoCreateInstance(CLSID_Material);
      jointMaterial.CoCreateInstance(CLSID_Material);

      IntervalIndexType compositeJointIntervalIdx = pIntervals->GetCompositeLongitudinalJointInterval();

      IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
      for (IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++)
      {
         Float64 E = pMaterial->GetSegmentEc(segmentKey, intervalIdx);
         Float64 D = pMaterial->GetSegmentWeightDensity(segmentKey, intervalIdx);
         material->put_E(intervalIdx, E);
         material->put_Density(intervalIdx, D);

         Float64 Ej = pMaterial->GetLongitudinalJointEc(intervalIdx);
         Float64 Dj = pMaterial->GetLongitudinalJointWeightDensity(intervalIdx);
         jointMaterial->put_E(intervalIdx, Ej);
         jointMaterial->put_Density(intervalIdx, Dj);
      }
   }

   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   CComPtr<IGirderSection> gdrSection;
   CreateGirderSection(pBroker, statusID, dimensions, -1, -1, &gdrSection);
   CComQIPtr<IPCIDeckedBulbTeeSection> bulbTeeSection(gdrSection);

   CComPtr<IPCIDeckedIBeam> beam;
   bulbTeeSection->get_Beam(&beam);

   ConfigureBeamShape(pBroker, pSegment, beam);

   CComQIPtr<IThickenedFlangeSegment> thickenedFlangeSegment(pSSMbrSegment);
   if (pSegment->TopFlangeThickeningType == pgsTypes::tftNone)
   {
      thickenedFlangeSegment->put_FlangeThickening(0.0);
   }
   else
   {
      thickenedFlangeSegment->put_FlangeThickeningType(pSegment->TopFlangeThickeningType == pgsTypes::tftEnds ? ttEnds : ttCenter);
      thickenedFlangeSegment->put_FlangeThickening(pSegment->TopFlangeThickening);
   }

   CComQIPtr<IShape> shape(gdrSection);
   ATLASSERT(shape);

   thickenedFlangeSegment->AddShape(shape, material, nullptr);

   CComQIPtr<ILongitudinalJoints> lj(thickenedFlangeSegment);
   lj->putref_JointMaterial(jointMaterial);

   lj->put_CrossSection(jstFromAdjacentBeams);

   // the code below shows what to do for constant depth longitudinal joint
   //Float64 tj;
   //beam->get_D1(&tj);
   //lj->put_CrossSection(jstConstantDepth);
   //lj->put_JointThickness(tj);
}

void CPCIDeckedBulbTeeFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);
   VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);

   if ( !IsPrismatic(segmentKey) )
   {
      VERIFY(pPoiMgr->AddPointOfInterest(pgsPointOfInterest(segmentKey,1*gdrLength/10,POI_SECTCHANGE_TRANSITION)) != INVALID_ID);
      VERIFY(pPoiMgr->AddPointOfInterest(pgsPointOfInterest(segmentKey,2*gdrLength/10,POI_SECTCHANGE_TRANSITION)) != INVALID_ID);
      VERIFY(pPoiMgr->AddPointOfInterest(pgsPointOfInterest(segmentKey,3*gdrLength/10,POI_SECTCHANGE_TRANSITION)) != INVALID_ID);
      VERIFY(pPoiMgr->AddPointOfInterest(pgsPointOfInterest(segmentKey,4*gdrLength/10,POI_SECTCHANGE_TRANSITION)) != INVALID_ID);
      VERIFY(pPoiMgr->AddPointOfInterest(pgsPointOfInterest(segmentKey,5*gdrLength/10,POI_SECTCHANGE_TRANSITION)) != INVALID_ID);
      VERIFY(pPoiMgr->AddPointOfInterest(pgsPointOfInterest(segmentKey,6*gdrLength/10,POI_SECTCHANGE_TRANSITION)) != INVALID_ID);
      VERIFY(pPoiMgr->AddPointOfInterest(pgsPointOfInterest(segmentKey,7*gdrLength/10,POI_SECTCHANGE_TRANSITION)) != INVALID_ID);
      VERIFY(pPoiMgr->AddPointOfInterest(pgsPointOfInterest(segmentKey,8*gdrLength/10,POI_SECTCHANGE_TRANSITION)) != INVALID_ID);
      VERIFY(pPoiMgr->AddPointOfInterest(pgsPointOfInterest(segmentKey,9*gdrLength/10,POI_SECTCHANGE_TRANSITION)) != INVALID_ID);
   }
}

void CPCIDeckedBulbTeeFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusItemIDType statusID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng) const
{
   CComPtr<IBulbTeeDistFactorEngineer> pEngineer;
   HRESULT hr = ::CoCreateInstance(CLSID_BulbTeeDistFactorEngineer, nullptr, CLSCTX_ALL, IID_IBulbTeeDistFactorEngineer, (void**)&pEngineer);

   pEngineer->Init();

   CComQIPtr<IDistFactorEngineer, &IID_IDistFactorEngineer> pDFEng(pEngineer);
   pDFEng->SetBroker(pBroker, statusID);

   pDFEng.CopyTo(ppEng);
}

void CPCIDeckedBulbTeeFactory::CreatePsLossEngineer(IBroker* pBroker,StatusItemIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng) const
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if (pLossParams->GetLossMethod() == pgsTypes::TIME_STEP)
   {
      CComPtr<IPsLossEngineer> engineer;
      HRESULT hr = ::CoCreateInstance(CLSID_TimeStepLossEngineer, nullptr, CLSCTX_ALL, IID_IPsLossEngineer, (void**)&engineer);
      CComQIPtr<IInitialize, &IID_IInitialize> initEngineer(engineer);
      initEngineer->SetBroker(pBroker, statusGroupID);
      (*ppEng) = engineer;
      (*ppEng)->AddRef();
   }
   else
   {
      CComPtr<IPsBeamLossEngineer> engineer;
      HRESULT hr = ::CoCreateInstance(CLSID_PsBeamLossEngineer, nullptr, CLSCTX_ALL, IID_IPsLossEngineer, (void**)&engineer);
      ATLASSERT(SUCCEEDED(hr));
      engineer->Init(IBeam);

      CComQIPtr<IInitialize, &IID_IInitialize> initEngineer(engineer);
      initEngineer->SetBroker(pBroker, statusGroupID);

      (*ppEng) = engineer;
      (*ppEng)->AddRef();
   }
}

void CPCIDeckedBulbTeeFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
{
   HRESULT hr = S_OK;

   // set the shape for harped strand bounds - only in the thinest part of the web
   CComPtr<IRectangle> harp_rect;
   hr = harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   Float64 c1;
   Float64 d1, d2, d3, d4, d5, d6, d7, d8, d9;
   Float64 h, t;
   Float64 w1, w2, w3, w4, w5, wmin, wmax;
   Float64 r1, r2, r3;
   GetDimensions(dimensions, w1, w2, w3, w4, w5, d1, d2, d3, d4, d5, d6, d7, d8, d9, h, t, r1, r2, r3, c1, wmin, wmax);

   harp_rect->put_Width(t);
   harp_rect->put_Height(h);

   CComPtr<IPoint2d> hook;
   hook.CoCreateInstance(CLSID_Point2d);
   hook->Move(0, -h/2.0);

   harp_rect->putref_HookPoint(hook);

   CComPtr<IShape> shape;
   harp_rect->get_Shape(&shape);

   CComPtr<IStrandMover> sm;
   sm.CoCreateInstance(CLSID_StrandMoverImpl);

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));

   // set vertical offset bounds and increments
   Float64 hptb  = hpTopFace     == IBeamFactory::BeamBottom ? hpTopLimit     - h : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == IBeamFactory::BeamBottom ? hpBottomLimit  - h : -hpBottomLimit;
   Float64 endtb = endTopFace    == IBeamFactory::BeamBottom ? endTopLimit    - h : -endTopLimit;
   Float64 endbb = endBottomFace == IBeamFactory::BeamBottom ? endBottomLimit - h : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(0, h, endtb, endbb, hptb, hpbb, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}

const std::vector<std::_tstring>& CPCIDeckedBulbTeeFactory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& CPCIDeckedBulbTeeFactory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const unitLength*>& CPCIDeckedBulbTeeFactory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CPCIDeckedBulbTeeFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const
{
   Float64 c1;
   Float64 d1, d2, d3, d4, d5, d6, d7, d8, d9;
   Float64 h, t;
   Float64 w1, w2, w3, w4, w5, wmin, wmax;
   Float64 r1, r2, r3;
   GetDimensions(dimensions, w1, w2, w3, w4, w5, d1, d2, d3, d4, d5, d6, d7, d8, d9, h, t, r1, r2, r3, c1, wmin, wmax);

#pragma Reminder("WORKING HERE - PCI UHPC validate section dimensions")
//// 0  D1  
//// 1  D2
//// 2  D3
//// 3  D4
//// 4  D5
//// 5  D6
//// 6  D7
//// 7  T1
//// 8  T2
//// 9  W1
//// 10 W2
//// 11 W3
//// 12 W4
//// 13 Wmax
//// 14 Wmin
//   if ( c1 < 0.0 )
//   {
//      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
//      std::_tostringstream os;
//      os << _T("C1 must be zero or greater ") << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//
//   if ( d1 <= 0.0 )
//   {
//      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
//      std::_tostringstream os;
//      os << _T("D1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//
//   if ( d2 < 0.0 )
//   {
//      std::_tostringstream os;
//      os << _T("D2 must be a positive value") << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//
//   if ( d3 < 0.0 )
//   {
//      std::_tostringstream os;
//      os << _T("D3 must be a positive value") << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//
//   if ( d4 <= 0.0 )
//   {
//      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][3];
//      std::_tostringstream os;
//      os << _T("D4 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//
//   if ( d5 < 0.0 )
//   {
//      std::_tostringstream os;
//      os << _T("D5 must be a positive value") << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//
//   if ( d6 < 0.0 )
//   {
//      std::_tostringstream os;
//      os << _T("D6 must be a positive value") << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//
//   if ( d7 <= 0.0 )
//   {
//      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][6];
//      std::_tostringstream os;
//      os << _T("D7 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//
//   if ( w1 <= 0.0 )
//   {
//      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
//      std::_tostringstream os;
//      os << _T("W1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }   
//
//   if ( w2 < 0.0 )
//   {
//      std::_tostringstream os;
//      os << _T("W2 must be a positive value") << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//
//   if ( w3 <= 0.0 )
//   {
//      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][11];
//      std::_tostringstream os;
//      os << _T("W3 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }   
//
//   if ( w4 < 0.0 )
//   {
//      std::_tostringstream os;
//      os << _T("W4 must be a positive value") << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//   
//   if ( t1 <= 0.0 )
//   {
//      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][7];
//      std::_tostringstream os;
//      os << _T("T1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }   
//   
//   if ( t2 <= 0.0 )
//   {
//      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
//      std::_tostringstream os;
//      os << _T("T2 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }   
//
//   if ( c1 > d4 )
//   {
//      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
//      std::_tostringstream os;
//      os << _T("C1 must be less than D4 ") << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }   
//
//   if (wmax < wmin)
//   {
//      std::_tostringstream os;
//      os << _T("Wmax must be greater than or equal to Wmin") << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//
//   Float64 inp_toler = ::ConvertToSysUnits(2.0, unitMeasure::Millimeter);
//
//   Float64 min_topflange = t2+2.0*(w3+w4);
//   if ( wmin + inp_toler < min_topflange )
//   {
//      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][14];
//      Float64 mf_u = ::ConvertFromSysUnits(min_topflange,*pUnit);
//
//      std::_tostringstream os;
//      os << _T("Wmin must be greater than or equal to bottom flange width = ")<<mf_u<< pUnit->UnitTag() <<_T(" = T2 + 2.0*(W3+W4)") << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }
//
//   min_topflange = t1+2.0*(w1+w2);
//   if ( wmin + inp_toler < min_topflange  )
//   {
//      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][14];
//      Float64 mf_u = ::ConvertFromSysUnits(min_topflange,*pUnit);
//
//      std::_tostringstream os;
//      os << _T("Wmin must be greater than or equal to T1 + 2.0*(W1 + W2) = ")<<mf_u<< pUnit->UnitTag() << std::ends;
//      *strErrMsg = os.str();
//      return false;
//   }


   return true;
}

void CPCIDeckedBulbTeeFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) const
{
   pSave->BeginUnit(_T("PCIDeckedBulbTeeDimensions"),1.0);
   for( const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CPCIDeckedBulbTeeFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad) const
{
   Float64 parent_version;
   if (pLoad->GetParentUnit() == _T("GirderLibraryEntry"))
   {
      parent_version = pLoad->GetParentVersion();
   }
   else
   {
      parent_version = pLoad->GetVersion();
   }
   
   IBeamFactory::Dimensions dimensions;

   if (14 <= parent_version && !pLoad->BeginUnit(_T("PCIDeckedBulbTeeDimensions")))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   Float64 dimVersion = pLoad->GetVersion();

   for(const auto& name : m_DimNames)
   {
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
      dimensions.emplace_back(name,value);
   }

   if (14 <= parent_version && !pLoad->EndUnit())
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   return dimensions;
}

bool CPCIDeckedBulbTeeFactory::IsPrismatic(const IBeamFactory::Dimensions& dimensions) const
{
   return true;
}

bool CPCIDeckedBulbTeeFactory::IsPrismatic(const CSegmentKey& segmentKey) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);
   return IsPrismatic(pSegment);
}

bool CPCIDeckedBulbTeeFactory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

std::_tstring CPCIDeckedBulbTeeFactory::GetImage() const
{
   return std::_tstring(_T("PCIDeckedBulbTee.png"));
}

std::_tstring CPCIDeckedBulbTeeFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("BulbTee_Composite.gif");
      break;

   case pgsTypes::sdtNonstructuralOverlay:
   case pgsTypes::sdtNone:
      strImage =  _T("BulbTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CPCIDeckedBulbTeeFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("+Mn_BulbTee_Composite.gif");
      break;

   case pgsTypes::sdtNonstructuralOverlay:
   case pgsTypes::sdtNone:
      strImage =  _T("+Mn_BulbTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CPCIDeckedBulbTeeFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("-Mn_BulbTee_Composite.gif");
      break;

   case pgsTypes::sdtNonstructuralOverlay:
   case pgsTypes::sdtNone:
      strImage =  _T("-Mn_BulbTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CPCIDeckedBulbTeeFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("Vn_BulbTee_Composite.gif");
      break;

   case pgsTypes::sdtNonstructuralOverlay:
   case pgsTypes::sdtNone:
      strImage =  _T("Vn_BulbTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CPCIDeckedBulbTeeFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
{
   return _T("BulbTee_Effective_Flange_Width_Interior_Girder.gif");
}

std::_tstring CPCIDeckedBulbTeeFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("BulbTee_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("BulbTee_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CPCIDeckedBulbTeeFactory::GetCLSID() const
{
   return CLSID_PCIDeckedBulbTeeFactory;
}

std::_tstring CPCIDeckedBulbTeeFactory::GetName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CPCIDeckedBulbTeeFactory::GetFamilyCLSID() const
{
   return CLSID_DeckBulbTeeBeamFamily;
}

std::_tstring CPCIDeckedBulbTeeFactory::GetGirderFamilyName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CPCIDeckedBulbTeeFactory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CPCIDeckedBulbTeeFactory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CPCIDeckedBulbTeeFactory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR CPCIDeckedBulbTeeFactory::GetImageResourceName() const
{
    return _T("PCIDECKEDBULBTEE");
}

HICON  CPCIDeckedBulbTeeFactory::GetIcon()  const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_BULBTEE) );
}

void CPCIDeckedBulbTeeFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions, Float64& w1,
   Float64& w2, Float64& w3, Float64& w4, Float64& w5, Float64& d1, Float64& d2, Float64& d3, Float64& d4, Float64& d5, Float64& d6, Float64& d7, Float64& d8, Float64& d9,
   Float64& h, Float64& t, Float64& r1, Float64& r2, Float64& r3, Float64& c1, Float64& wmin, Float64& wmax) const
{
    w1 = GetDimension(dimensions, _T("W1"));
    w2 = GetDimension(dimensions, _T("W2"));
    w3 = GetDimension(dimensions, _T("W3"));
    w4 = GetDimension(dimensions, _T("W4"));
    w5 = GetDimension(dimensions, _T("W5"));
    d1 = GetDimension(dimensions, _T("D1"));
    d2 = GetDimension(dimensions, _T("D2"));
    d3 = GetDimension(dimensions, _T("D3"));
    d4 = GetDimension(dimensions, _T("D4"));
    d5 = GetDimension(dimensions, _T("D5"));
    d6 = GetDimension(dimensions, _T("D6"));
    d7 = GetDimension(dimensions, _T("D7"));
    d8 = GetDimension(dimensions, _T("D8"));
    d9 = GetDimension(dimensions, _T("D9"));
    h = GetDimension(dimensions, _T("H"));
    t = GetDimension(dimensions, _T("T"));
    r1 = GetDimension(dimensions, _T("R1"));
    r2 = GetDimension(dimensions, _T("R2"));
    r3 = GetDimension(dimensions, _T("R3"));
    c1 = GetDimension(dimensions, _T("C1"));
    wmin = GetDimension(dimensions, _T("Wmin"));
    wmax = GetDimension(dimensions, _T("Wmax"));
}

Float64 CPCIDeckedBulbTeeFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name) const
{
   for ( const auto& dim : dimensions)
   {
      if (name == dim.first)
      {
         return dim.second;
      }
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedBeamSpacings CPCIDeckedBulbTeeFactory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacentWithTopWidth);
   sbs.push_back(pgsTypes::sbsGeneralAdjacentWithTopWidth);
   return sbs;
}

bool CPCIDeckedBulbTeeFactory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(),spacingType);
   return found == sbs.end() ? false : true;
}

bool CPCIDeckedBulbTeeFactory::ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   ATLASSERT(!IsSupportedBeamSpacing(spacingType));
   if (spacingType == pgsTypes::sbsUniform || spacingType == pgsTypes::sbsUniformAdjacent || spacingType == pgsTypes::sbsConstantAdjacent)
   {
      *pNewSpacingType = pgsTypes::sbsUniformAdjacentWithTopWidth;
   }
   else if (spacingType == pgsTypes::sbsGeneralAdjacent)
   {
      *pNewSpacingType = pgsTypes::sbsGeneralAdjacentWithTopWidth;
   }
   else
   {
      ATLASSERT(false); // how did this happen?
      return false; // non-convertable spacing type
   }

   Float64 Jmin, Jmax;
   GetAllowableSpacingRange(dimensions, pgsTypes::sdtNone, *pNewSpacingType, &Jmin, &Jmax);
   *pNewSpacing = Jmin; // this is the joint width
   *pNewTopWidth = spacing;

   return true;
}

pgsTypes::WorkPointLocations CPCIDeckedBulbTeeFactory::GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::WorkPointLocations wpls;
   wpls.push_back(pgsTypes::wplTopGirder);
//   wpls.push_back(pgsTypes::wplBottomGirder);

   return wpls;
}

bool CPCIDeckedBulbTeeFactory::IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation wpType) const
{
   pgsTypes::WorkPointLocations sbs = GetSupportedWorkPointLocations(spacingType);
   auto found = std::find(sbs.cbegin(), sbs.cend(),wpType);
   return found == sbs.end() ? false : true;
}

pgsTypes::SupportedDeckTypes CPCIDeckedBulbTeeFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
{
   pgsTypes::SupportedDeckTypes sdt;

   switch(sbs)
   {
   case pgsTypes::sbsUniformAdjacentWithTopWidth:
   case pgsTypes::sbsGeneralAdjacentWithTopWidth:
      //sdt.push_back(pgsTypes::sdtCompositeCIP);
      sdt.push_back(pgsTypes::sdtCompositeOverlay);
      sdt.push_back(pgsTypes::sdtNonstructuralOverlay);
      sdt.push_back(pgsTypes::sdtNone);
      break;

   default:
      ATLASSERT(false);
   }

   return sdt;
}

std::vector<pgsTypes::GirderOrientationType> CPCIDeckedBulbTeeFactory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb,pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal, pgsTypes::Balanced };
   return types;
}

bool CPCIDeckedBulbTeeFactory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType CPCIDeckedBulbTeeFactory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes CPCIDeckedBulbTeeFactory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CPCIDeckedBulbTeeFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
{
   pgsTypes::SupportedDiaphragmLocationTypes locations;
   switch(type)
   {
   case pgsTypes::dtPrecast:
   case pgsTypes::dtCastInPlace :
      locations.push_back(pgsTypes::dltExternal);
      break;

   default:
      ATLASSERT(false);
   }

   return locations;
}

void CPCIDeckedBulbTeeFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   // this is for joint spacing... effective want unrestrained joint spacing
   // need 0 for welded shear tab and grout key connection and a specific value for UHPC-type connections
   *minSpacing = 0;
   *maxSpacing = MAX_GIRDER_SPACING;
}

std::vector<pgsTypes::TopWidthType> CPCIDeckedBulbTeeFactory::GetSupportedTopWidthTypes() const
{
   //std::vector<pgsTypes::TopWidthType> types{ pgsTypes::twtSymmetric,pgsTypes::twtCenteredCG,pgsTypes::twtAsymmetric };
   std::vector<pgsTypes::TopWidthType> types{ pgsTypes::twtSymmetric };
   ATLASSERT(CanTopWidthVary() == false); // the equations for CenteredCG have not been derived for variable top width
   return types;
}

void CPCIDeckedBulbTeeFactory::GetAllowableTopWidthRange(pgsTypes::TopWidthType topWidthType, const IBeamFactory::Dimensions& dimensions, Float64* pWleftMin, Float64* pWleftMax, Float64* pWrightMin, Float64* pWrightMax) const
{
   Float64 Wmin = GetDimension(dimensions, _T("Wmin"));
   Float64 Wmax = GetDimension(dimensions, _T("Wmax"));
   if (topWidthType == pgsTypes::twtAsymmetric)
   {
      *pWleftMin = Wmin / 2;
      *pWleftMax = Wmax / 2;
      *pWrightMin = Wmin / 2;
      *pWrightMax = Wmax / 2;
   }
   else
   {
      // no left/right for these cases so left holds the values...
      *pWleftMin = Wmin;
      *pWleftMax = Wmax;
      *pWrightMin = 0;
      *pWrightMax = 0;
   }
}

WebIndexType CPCIDeckedBulbTeeFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions) const
{
   return 1;
}

Float64 CPCIDeckedBulbTeeFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("H"));
}

Float64 CPCIDeckedBulbTeeFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("Wmax"));
}

void CPCIDeckedBulbTeeFactory::GetBeamTopWidth(const IBeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const
{
   Float64 Wmin = GetDimension(dimensions, _T("Wmin"));

   Float64 top = Wmin;
   top /= 2.0;

   *pLeftWidth = top;
   *pRightWidth = top;
}

bool CPCIDeckedBulbTeeFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void CPCIDeckedBulbTeeFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool CPCIDeckedBulbTeeFactory::HasLongitudinalJoints() const
{
   return true;
}

bool CPCIDeckedBulbTeeFactory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   if (deckType == pgsTypes::sdtCompositeCIP)
   {
      return false;
   }
   else
   {

      return connectivity == pgsTypes::atcConnectedAsUnit ? true : false;
   }
}

bool CPCIDeckedBulbTeeFactory::HasTopFlangeThickening() const
{
   return false;
}

bool CPCIDeckedBulbTeeFactory::CanPrecamber() const
{
   return false;
}

GirderIndexType CPCIDeckedBulbTeeFactory::GetMinimumBeamCount() const
{
   return 2;
}


bool CPCIDeckedBulbTeeFactory::IsPrismatic(const CPrecastSegmentData* pSegment) const
{
    return true;
   //if (pSegment->TopFlangeThickeningType == pgsTypes::tftNone || IsZero(pSegment->TopFlangeThickening))
   //{
   //   return true;
   //}

   //return false;
}

void CPCIDeckedBulbTeeFactory::ConfigureBeamShape(IBroker* pBroker, const CPrecastSegmentData* pSegment, IPCIDeckedIBeam* pBeam) const
{
   //// pBeam is the basic section... figure out actual top width parameters
   //Float64 c2, n1, n2, left, right;
   //GetTopFlangeParameters(pBroker, pSegment, &c2, &n1, &n2, &left, &right);
   //pBeam->put_C2(c2);
   //pBeam->put_n1(n1);
   //pBeam->put_n2(n2);
   //pBeam->put_W5(left);
   //pBeam->put_W6(right);
}

void CPCIDeckedBulbTeeFactory::GetTopWidth(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs,Float64* pLeft, Float64* pRight) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();

   pgsTypes::TopWidthType topWidthType;
   Float64 leftStart, rightStart, leftEnd, rightEnd;
   pGirder->GetTopWidth(&topWidthType, &leftStart, &rightStart, &leftEnd, &rightEnd);

   switch (topWidthType)
   {
   case pgsTypes::twtSymmetric:
   case pgsTypes::twtCenteredCG:
      leftStart /= 2;
      rightStart = leftStart;

      leftEnd /= 2;
      rightEnd = leftEnd;
      break;

   case pgsTypes::twtAsymmetric:
      // we already have left/right values
      break;

   default:
      ATLASSERT(false);
   }

   GET_IFACE2(pBroker, IBridge, pBridge);
   const auto& segmentKey(pSegment->GetSegmentKey());
   Float64 Ls = pBridge->GetSegmentLength(segmentKey);

   *pLeft  = ::LinInterp(Xs, leftStart,  leftEnd,  Ls);
   *pRight = ::LinInterp(Xs, rightStart, rightEnd, Ls);
}
//
//void CPCIDeckedBulbTeeFactory::GetTopFlangeParameters(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64* pC, Float64* pN1, Float64* pN2,Float64* pLeft,Float64* pRight) const
//{
//   const CSplicedGirderData* pGirder = pSegment->GetGirder();
//
//   pgsTypes::TopWidthType topWidthType;
//   Float64 leftStart, rightStart, leftEnd, rightEnd;
//   pGirder->GetTopWidth(&topWidthType, &leftStart, &rightStart, &leftEnd, &rightEnd);
//
//   switch (topWidthType)
//   {
//   case pgsTypes::twtSymmetric:
//   case pgsTypes::twtCenteredCG: // we'll update the values below for this case
//      leftStart /= 2;
//      rightStart = leftStart;
//      break;
//
//   case pgsTypes::twtAsymmetric:
//      // we already have left/right values
//      break;
//
//   default:
//      ATLASSERT(false);
//   }
//
//   Float64 n1(0), n2(0), c2(0); // c2 is the distance from the left flange tip to where the top flange slope changes from n1 to n2
//   Float64 left(leftStart), right(rightStart);
//
//   GET_IFACE2(pBroker,IBridgeDescription, pIBridgeDesc);
//   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
//   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->GetDeckType();
//   if ((deckType == pgsTypes::sdtNone || deckType == pgsTypes::sdtCompositeOverlay || deckType == pgsTypes::sdtNonstructuralOverlay) && pBridgeDesc->GetGirderOrientation() == pgsTypes::Plumb)
//   {
//      // need to get cross slope shape parameters
//      // c2, n1, n2
//
//      const auto& segmentKey(pSegment->GetSegmentKey());
//
//      GET_IFACE2(pBroker, IBridge, pBridge);
//      Float64 station, offset;
//      pBridge->GetStationAndOffset(segmentKey, 0.0, &station, &offset);
//
//      Float64 left_edge_offset = offset - leftStart;
//      Float64 right_edge_offset = offset + rightStart;
//
//      GET_IFACE2(pBroker, IRoadway, pAlignment);
//      // Loop over crown points to see if one lies within the flange width
//      IndexType numCPs = pAlignment->GetCrownPointIndexCount(station);
//      IndexType numCPsfound(0);
//      // Assumption here that outer-most ridge points are off of the bridge. Done for performance
//      for (IndexType iCP = 1; iCP < numCPs-1; iCP++)
//      {
//         Float64 CPO = pAlignment->GetAlignmentOffset(iCP, station);
//
//         if (IsLT(left_edge_offset, CPO) && IsLT(CPO, right_edge_offset))
//         {
//            if (numCPsfound == 0)
//            {
//               // Use the first crown point found to define the shape. We can only use one
//               // the crown point occurs somewhere in the top flange
//               n1 = pAlignment->GetSlope(station, left_edge_offset);
//               n2 = pAlignment->GetSlope(station, right_edge_offset);
//               c2 = fabs(CPO - left_edge_offset);
//            }
//
//            numCPsfound++;
//         }
//      }
//
//      if (numCPsfound == 0)
//      {
//         // the entire top flange is sloped at n2
//         n1 = 0;
//         n2 = pAlignment->GetSlope(station, offset);
//         c2 = 0; // no part of the top flange is sloped at n1
//      }
//      else if (numCPsfound > 1)
//      {
//         GET_IFACE2(pBroker,IEAFStatusCenter,pStatusCenter);
//         std::_tstring str(_T("The decked girder at ") + std::_tstring(SEGMENT_LABEL(segmentKey)) + _T("\'s top flange has more than one crown point above it. Only one crown point will be used to model the top of the girder."));
//         pgsInformationalStatusItem* pStatusItem = new pgsInformationalStatusItem(m_StatusGroupID,m_scidInformationalWarning,str.c_str());
//         pStatusCenter->Add(pStatusItem);
//      }
//
//      if (topWidthType == pgsTypes::twtCenteredCG)
//      {
//         // forget about input top width for left and right... compute it so the CG is centered on the CL Web
//         
//         // the equations below are for contant width girder
//         Float64 Wtf = leftStart + rightStart;
//
//         const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
//         const auto& dimensions = pGirderEntry->GetDimensions();
//
//         Float64 he; // minimum top flange thickness at girder ends
//         Float64 hm; // minimum top flange thickness at middle (L/2)
//         he = GetDimension(dimensions, _T("D1"));
//         hm = he;
//         if (pSegment->TopFlangeThickeningType == pgsTypes::tftEnds)
//         {
//            he += pSegment->TopFlangeThickening;
//         }
//         else
//         {
//            hm += pSegment->TopFlangeThickening;
//         }
//
//
//         Float64 k;
//         Float64 A = fabs(c2*n1);
//         Float64 B = fabs((Wtf - c2)*n2);
//         if ((IsZero(A) && n2 < 0) || (B < A && 0 <= n1 && n2 <= 0) || (n1 <= 0 && n2 <= 0))
//         {
//            // case 1b and 3
//            Float64 n = 0.5*Wtf*Wtf*(he + 2 * hm) + 0.5*(n2 - n1)*c2*c2*c2 - 0.5*n2*Wtf*Wtf*Wtf;
//            Float64 d = 2 * Wtf*(he + 2 * hm) + 3 * (n2 - n1)*c2*c2 - 3 * n2*Wtf*Wtf;
//            k = (2 * n) / (Wtf*d);
//         }
//         else if ((A <= B && 0 <= n1 && n2 <= 0) || (0 <= n1 && 0 <= n2))
//         {
//            // case 1a and 2
//            Float64 n = 0.5*Wtf*Wtf*(he + 2 * hm) + 0.5*(n2 - n1)*c2*c2*c2 - 1.5*(n2 - n1)*Wtf*Wtf*c2 + n2*Wtf*Wtf*Wtf;
//            Float64 d = 2 * Wtf*(he + 2 * hm) + 3 * (n2 - n1)*c2*c2 - 6 * (n2 - n1)*Wtf*c2 + 3 * n2*Wtf*Wtf;
//            k = (2 * n) / (Wtf*d);
//         }
//         else
//         {
//            // case 4
//            ATLASSERT(n1 <= 0 && 0 <= n2);
//            Float64 n = 0.5*Wtf*Wtf*(he + 2 * hm) + 0.5*(n2 - n1)*c2*c2*c2 - 1.5*n2*Wtf*Wtf*c2 + n2*Wtf*Wtf*Wtf;
//            Float64 d = 2 * Wtf*(he + 2 * hm) - 6 * n2*Wtf*c2 + 3 * (n2 - n1)*c2*c2 + 3 * n2*Wtf*Wtf;
//            k = (2 * n) / (Wtf*d);
//         }
//
//         ATLASSERT(0.0 <= k && k <= 1.0);
//
//         left = k*Wtf;
//         right = (1 - k)*Wtf;
//      }
//   }
//
//
//   *pC = c2;
//   *pN1 = n1;
//   *pN2 = n2;
//   *pLeft = left;
//   *pRight = right;
//}

//Float64 CPCIDeckedBulbTeeFactory::GetFlangeThickening(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
//{
//   Float64 tft = 0;
//   if (pSegment->TopFlangeThickeningType != pgsTypes::tftNone)
//   {
//      // parabolic interpolation of the depth of the top flange thickening
//      const CSegmentKey& segmentKey(pSegment->GetSegmentKey());
//      
//      GET_IFACE2(pBroker, IBridge, pBridge);
//      Float64 Ls = pBridge->GetSegmentLength(segmentKey);
//
//      Float64 thickening = pSegment->TopFlangeThickening;
//
//      if (pSegment->TopFlangeThickeningType == pgsTypes::tftEnds )
//      {
//         tft = 4 * thickening*Xs*Xs / (Ls*Ls) - 4 * thickening*Xs / Ls + thickening;
//      }
//      else
//      {
//         tft = -4 * thickening*Xs*Xs / (Ls*Ls) + 4 * thickening*Xs / Ls;
//      }
//   }
//   return tft;
//}

void CPCIDeckedBulbTeeFactory::PositionBeamShape(IPCIDeckedIBeam* pBeam) const
{

   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_H(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}
