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

// BulbTeeFactory.cpp : Implementation of CBulbTeeFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "BulbTeeFactory.h"
#include "BulbTeeDistFactorEngineer.h"
#include "PsBeamLossEngineer.h"
#include "TimeStepLossEngineer.h"
#include "StrandMoverImpl.h"
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

#include <IFace\StatusCenter.h>
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CBulbTeeFactory
HRESULT CBulbTeeFactory::FinalConstruct()
{
   m_bHaveOldTopFlangeThickening = false;
   m_OldTopFlangeThickening = 0; // this is used to hold the old D8 value

   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.emplace_back(_T("C1"));
   m_DimNames.emplace_back(_T("D1"));
   m_DimNames.emplace_back(_T("D2"));
   m_DimNames.emplace_back(_T("D3"));
   m_DimNames.emplace_back(_T("D4"));
   m_DimNames.emplace_back(_T("D5"));
   m_DimNames.emplace_back(_T("D6"));
   m_DimNames.emplace_back(_T("D7"));
   m_DimNames.emplace_back(_T("T1"));
   m_DimNames.emplace_back(_T("T2"));
   m_DimNames.emplace_back(_T("W1"));
   m_DimNames.emplace_back(_T("W2"));
   m_DimNames.emplace_back(_T("W3"));
   m_DimNames.emplace_back(_T("W4"));
   m_DimNames.emplace_back(_T("Wmax"));
   m_DimNames.emplace_back(_T("Wmin"));

   // Default beam is a W74G                                              
   m_DefaultDims.emplace_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // C1
   m_DefaultDims.emplace_back(::ConvertToSysUnits(2.875,unitMeasure::Inch)); // D1
   m_DefaultDims.emplace_back(::ConvertToSysUnits(2.625,unitMeasure::Inch)); // D2
   m_DefaultDims.emplace_back(::ConvertToSysUnits(2.000,unitMeasure::Inch)); // D3
   m_DefaultDims.emplace_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // D4
   m_DefaultDims.emplace_back(::ConvertToSysUnits(3.000,unitMeasure::Inch)); // D5
   m_DefaultDims.emplace_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // D6
   m_DefaultDims.emplace_back(::ConvertToSysUnits(57.00,unitMeasure::Inch)); // D7
   m_DefaultDims.emplace_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // T1
   m_DefaultDims.emplace_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // T2
   m_DefaultDims.emplace_back(::ConvertToSysUnits(16.50,unitMeasure::Inch)); // W1
   m_DefaultDims.emplace_back(::ConvertToSysUnits(2.000,unitMeasure::Inch)); // W2
   m_DefaultDims.emplace_back(::ConvertToSysUnits(9.500,unitMeasure::Inch)); // W3
   m_DefaultDims.emplace_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // W4
   m_DefaultDims.emplace_back(::ConvertToSysUnits(6.000, unitMeasure::Feet)); // Wmax
   m_DefaultDims.emplace_back(::ConvertToSysUnits(4.000, unitMeasure::Feet)); // Wmin

   // SI Units
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D3
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D4
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D5
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D6
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // D7
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // T1
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // T2
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W4
   m_DimUnits[0].emplace_back(&unitMeasure::Meter);      // Wmax
   m_DimUnits[0].emplace_back(&unitMeasure::Meter);      // Wmin

   // US Units
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D3
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D4
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D5
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D6
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // D7
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // T1
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // T2
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W4
   m_DimUnits[1].emplace_back(&unitMeasure::Feet); // Wmax
   m_DimUnits[1].emplace_back(&unitMeasure::Feet); // Wmin

   return S_OK;
}

void CBulbTeeFactory::CreateGirderSection(IBroker* pBroker,StatusItemIDType statusID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   ATLASSERT(overallHeight < 0); // not a variable depth section
   ATLASSERT(bottomFlangeHeight < 0); // not a variable bottom flange section

   CComPtr<IBulbTeeSection> gdrSection;
   gdrSection.CoCreateInstance(CLSID_BulbTeeSection);
   CComPtr<IBulbTee2> beam;
   gdrSection->get_Beam(&beam);

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7;
   Float64 w1,w2,w3,w4,wmin,wmax;
   Float64 t1,t2;
   GetDimensions(dimensions,c1,d1,d2,d3,d4,d5,d6,d7,w1,w2,w3,w4,wmin,wmax,t1,t2);
   beam->put_W1(w1);
   beam->put_W2(w2);
   beam->put_W3(w3);
   beam->put_W4(w4);

   if ( pBroker == nullptr )
   {
      beam->put_W5(wmax / 2);
      beam->put_W6(wmax / 2);
   }
   else
   {
      // Assumes uniform girder spacing

      // use raw input here because requesting it from the bridge will cause an infinite loop.
      // bridge agent calls this during validation
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      Float64 topWidth = pBridgeDesc->GetGirderTopWidth(); // we don't have a girder key so best we can do is get the top level value (which may not be valid)
         
      beam->put_W5(topWidth / 2);
      beam->put_W6(topWidth / 2);
   }

   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_D3(d3);
   beam->put_D4(d4);
   beam->put_D5(d5);
   beam->put_D6(d6);
   beam->put_D7(d7);
   beam->put_T1(t1);
   beam->put_T2(t2);
   beam->put_C1(c1);
   beam->put_C2(0);
   beam->put_n1(0);
   beam->put_n2(0);

   PositionBeamShape(beam);

   gdrSection.QueryInterface(ppSection);
}

void CBulbTeeFactory::CreateSegment(IBroker* pBroker,StatusItemIDType statusID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
{
   CComPtr<ISuperstructureMemberSegment> segment;
   segment.CoCreateInstance(CLSID_ThickenedFlangeBulbTeeSegment);

   ATLASSERT(segment != nullptr);

   segment.CopyTo(ppSegment);
}

void CBulbTeeFactory::CreateSegmentShape(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   // Create basic beam shape from dimensions in girder library
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   Float64 c1;
   Float64 d1, d2, d3, d4, d5, d6, d7;
   Float64 w1, w2, w3, w4, wmin, wmax;
   Float64 t1, t2;
   GetDimensions(dimensions, c1, d1, d2, d3, d4, d5, d6, d7, w1, w2, w3, w4, wmin, wmax, t1, t2);

   CComPtr<IBulbTee2> beam;
   beam.CoCreateInstance(CLSID_BulbTee2);

   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_D3(d3);
   beam->put_D4(d4);
   beam->put_D5(d5);
   beam->put_D6(d6);
   beam->put_D7(d7);
   beam->put_W1(w1);
   beam->put_W2(w2);
   beam->put_W3(w3);
   beam->put_W4(w4);
   beam->put_W5(wmax / 2); // dummy value... will get fixed in ConfigureBeamShape
   beam->put_W6(wmax / 2); // dummy value... will get fixed in ConfigureBeamShape
   beam->put_T1(t1);
   beam->put_T2(t2);
   beam->put_C1(c1);
   beam->put_C2(0);
   beam->put_n1(0);
   beam->put_n2(0);

   // now configure the beam based on the girder modifiers and applicable roadway effects
   ConfigureBeamShape(pBroker, pSegment, beam);

   // lastly, since we are creating a shape at a specific location, we need to apply the top flange thickening if any
   Float64 tft = GetFlangeThickening(pBroker, pSegment, Xs);
   beam->put_D1(d1 + tft);

   PositionBeamShape(beam);

   beam.QueryInterface(ppShape);
}

Float64 CBulbTeeFactory::GetSegmentHeight(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   Float64 H = GetBeamHeight(dimensions, pgsTypes::metStart); // this is the basic beam height

   // Compute the increase in height due to cross slope in the top flange
   // compute measuring from each flange tip to get max value
   Float64 c, n1, n2, left, right;
   GetTopFlangeParameters(pBroker, pSegment, &c, &n1, &n2, &left, &right);
   Float64 t_slope = Max(c*n1, (left + right - c)*n2);

   // Longitudinal top flanging thickening
   Float64 tft = GetFlangeThickening(pBroker, pSegment, Xs);

   H += tft + t_slope;

   return H;
}

void CBulbTeeFactory::ConfigureSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   bool bPrismatic = IsPrismatic(pSegment);

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

      CComPtr<IAgeAdjustedMaterial> aaJointMaterial;
      BuildAgeAdjustedJointMaterialModel(pBroker, pSegment, pSSMbrSegment, &aaJointMaterial);
      aaJointMaterial.QueryInterface(&jointMaterial);
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
   CComQIPtr<IBulbTeeSection> bulbTeeSection(gdrSection);

   CComPtr<IBulbTee2> beam;
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

   //Float64 tj;
   //beam->get_D1(&tj);
   //lj->put_CrossSection(jstConstantDepth);
   //lj->put_JointThickness(tj);
}

void CBulbTeeFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);

   if ( !IsPrismatic(segmentKey) )
   {
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,1*gdrLength/10, POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,2*gdrLength/10, POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,3*gdrLength/10, POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,4*gdrLength/10, POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,5*gdrLength/10, POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,6*gdrLength/10, POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,7*gdrLength/10, POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,8*gdrLength/10, POI_SECTCHANGE_TRANSITION ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,9*gdrLength/10, POI_SECTCHANGE_TRANSITION ) );
   }
}

void CBulbTeeFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusItemIDType statusID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng) const
{
   CComObject<CBulbTeeDistFactorEngineer>* pEngineer;
   CComObject<CBulbTeeDistFactorEngineer>::CreateInstance(&pEngineer);

   pEngineer->Init();
   pEngineer->SetBroker(pBroker,statusID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CBulbTeeFactory::CreatePsLossEngineer(IBroker* pBroker,StatusItemIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng) const
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
   {
      CComObject<CTimeStepLossEngineer>* pEngineer;
      CComObject<CTimeStepLossEngineer>::CreateInstance(&pEngineer);
      pEngineer->SetBroker(pBroker,statusGroupID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
   else
   {
       CComObject<CPsBeamLossEngineer>* pEngineer;
       CComObject<CPsBeamLossEngineer>::CreateInstance(&pEngineer);
       pEngineer->Init(IBeam);
       pEngineer->SetBroker(pBroker,statusGroupID);
       (*ppEng) = pEngineer;
       (*ppEng)->AddRef();
   }
}

void CBulbTeeFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;


   // set the shape for harped strand bounds - only in the thinest part of the web
   CComPtr<IRectangle> harp_rect;
   hr = harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7;
   Float64 w1,w2,w3,w4,wmin,wmax;
   Float64 t1,t2;
   GetDimensions(dimensions,c1,d1,d2,d3,d4,d5,d6,d7,w1,w2,w3,w4,wmin,wmax,t1,t2);

   Float64 width = Min(t1,t2);
   Float64 depth = (Hg < 0 ? d1 + d2 + d3 + d4 + d5 + d6 + d7 : Hg);

   harp_rect->put_Width(width);
   harp_rect->put_Height(depth);

   CComPtr<IPoint2d> hook;
   hook.CoCreateInstance(CLSID_Point2d);
   hook->Move(0, -depth/2.0);

   harp_rect->putref_HookPoint(hook);

   CComPtr<IShape> shape;
   harp_rect->get_Shape(&shape);

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));

   // set vertical offset bounds and increments
   Float64 hptb  = hpTopFace     == IBeamFactory::BeamBottom ? hpTopLimit     - depth : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == IBeamFactory::BeamBottom ? hpBottomLimit  - depth : -hpBottomLimit;
   Float64 endtb = endTopFace    == IBeamFactory::BeamBottom ? endTopLimit    - depth : -endTopLimit;
   Float64 endbb = endBottomFace == IBeamFactory::BeamBottom ? endBottomLimit - depth : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(0, depth, endtb, endbb, hptb, hpbb, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}

const std::vector<std::_tstring>& CBulbTeeFactory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& CBulbTeeFactory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const unitLength*>& CBulbTeeFactory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CBulbTeeFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const
{
   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7;
   Float64 w1,w2,w3,w4,wmin,wmax;
   Float64 t1,t2;
   GetDimensions(dimensions,c1,d1,d2,d3,d4,d5,d6,d7,w1,w2,w3,w4,wmin,wmax,t1,t2);

// 0  D1  
// 1  D2
// 2  D3
// 3  D4
// 4  D5
// 5  D6
// 6  D7
// 7  T1
// 8  T2
// 9  W1
// 10 W2
// 11 W3
// 12 W4
// 13 Wmax
// 14 Wmin
   if ( c1 < 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
      std::_tostringstream os;
      os << _T("C1 must be zero or greater ") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
      std::_tostringstream os;
      os << _T("D1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d2 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D2 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d3 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D3 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d4 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][3];
      std::_tostringstream os;
      os << _T("D4 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d5 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D5 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d6 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D6 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d7 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][6];
      std::_tostringstream os;
      os << _T("D7 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
      std::_tostringstream os;
      os << _T("W1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( w2 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("W2 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w3 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][11];
      std::_tostringstream os;
      os << _T("W3 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( w4 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("W4 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }
   
   if ( t1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][7];
      std::_tostringstream os;
      os << _T("T1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( t2 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
      std::_tostringstream os;
      os << _T("T2 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( c1 > d4 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
      std::_tostringstream os;
      os << _T("C1 must be less than D4 ") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if (wmax < wmin)
   {
      std::_tostringstream os;
      os << _T("Wmax must be greater than or equal to Wmin") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   Float64 inp_toler = ::ConvertToSysUnits(2.0, unitMeasure::Millimeter);

   Float64 min_topflange = t2+2.0*(w3+w4);
   if ( wmin + inp_toler < min_topflange )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][14];
      Float64 mf_u = ::ConvertFromSysUnits(min_topflange,*pUnit);

      std::_tostringstream os;
      os << _T("Wmin must be greater than or equal to bottom flange width = ")<<mf_u<< pUnit->UnitTag() <<_T(" = T2 + 2.0*(W3+W4)") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   min_topflange = t1+2.0*(w1+w2);
   if ( wmin + inp_toler < min_topflange  )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][14];
      Float64 mf_u = ::ConvertFromSysUnits(min_topflange,*pUnit);

      std::_tostringstream os;
      os << _T("Wmin must be greater than or equal to T1 + 2.0*(W1 + W2) = ")<<mf_u<< pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }


   return true;
}

void CBulbTeeFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) const
{
   // NOTE:
   // Version 2, added D8
   // Version 3, added C1
   // Vesrion 4, removed D8
   pSave->BeginUnit(_T("BulbTeeDimensions"),4.0);
   for( const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CBulbTeeFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad) const
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

   if (14 <= parent_version && !pLoad->BeginUnit(_T("BulbTeeDimensions")))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   Float64 dimVersion = pLoad->GetVersion();

   for(const auto& name : m_DimNames)
   {
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
      {
         if ((parent_version < 14 || (14 <= parent_version && dimVersion < 2)) && name == _T("D8"))
         {
            // error reading "D8"
            // If the file is from a time before D8 was a dimension, just go to the next dimension
            continue;
         }
         else if (14 <= parent_version && (2 == dimVersion || dimVersion == 3) && name == _T("T1"))
         {
            // we attempted to read dimension but it failed, and the file is from a time when "D8" was a valid dimension
            // try reading D8
            // D8 was only valud in dimVersion 3
            if (pLoad->Property(_T("D8"), &value))
            {
               // D8 was read, as expected
               ATLASSERT(m_bHaveOldTopFlangeThickening == false); // we read a value, but it wasn't moved to the owning library entry
               m_bHaveOldTopFlangeThickening = true;
               m_OldTopFlangeThickening = value;

               // now try to read the dimension we were originally reading
               if (!pLoad->Property(name.c_str(), &value))
               {
                  THROW_LOAD(InvalidFileFormat, pLoad);
               }
            }
         }
         else if ((parent_version < 14 || (14 <= parent_version && dimVersion < 3)) && name == _T("C1"))
         {
            // error reading "C1"
            // If the file is from a time before C1 was a dimension, just use a default value of 0
            value = 0;
         }
         else
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }

      }
      dimensions.emplace_back(name,value);
   }

   if (14 <= parent_version && !pLoad->EndUnit())
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   return dimensions;
}

bool CBulbTeeFactory::IsPrismatic(const IBeamFactory::Dimensions& dimensions) const
{
   // d8 was removed
   //Float64 d8 = GetDimension(dimensions,_T("D8"));
   //return IsZero(d8) ? true : false;
   return false;
}

bool CBulbTeeFactory::IsPrismatic(const CSegmentKey& segmentKey) const
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

bool CBulbTeeFactory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

Float64 CBulbTeeFactory::GetInternalSurfaceAreaOfVoids(IBroker* pBroker,const CSegmentKey& segmentKey) const
{
   return 0;
}

std::_tstring CBulbTeeFactory::GetImage() const
{
   return std::_tstring(_T("BulbTee.png"));
}

std::_tstring CBulbTeeFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CBulbTeeFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CBulbTeeFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CBulbTeeFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CBulbTeeFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
{
   return _T("BulbTee_Effective_Flange_Width_Interior_Girder.gif");
}

std::_tstring CBulbTeeFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
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

CLSID CBulbTeeFactory::GetCLSID() const
{
   return CLSID_BulbTeeFactory;
}

std::_tstring CBulbTeeFactory::GetName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CBulbTeeFactory::GetFamilyCLSID() const
{
   return CLSID_DeckBulbTeeBeamFamily;
}

std::_tstring CBulbTeeFactory::GetGirderFamilyName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CBulbTeeFactory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CBulbTeeFactory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CBulbTeeFactory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR CBulbTeeFactory::GetImageResourceName() const
{
   return _T("BULBTEE");
}

HICON  CBulbTeeFactory::GetIcon()  const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_BULBTEE) );
}

void CBulbTeeFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions, Float64& c1,
                                  Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,Float64& d6,Float64& d7,
                                  Float64& w1,Float64& w2,Float64& w3,Float64& w4,Float64& wmin,Float64& wmax,
                                  Float64& t1,Float64& t2) const
{
   c1 = GetDimension(dimensions,_T("C1"));
   d1 = GetDimension(dimensions,_T("D1"));
   d2 = GetDimension(dimensions,_T("D2"));
   d3 = GetDimension(dimensions,_T("D3"));
   d4 = GetDimension(dimensions,_T("D4"));
   d5 = GetDimension(dimensions,_T("D5"));
   d6 = GetDimension(dimensions,_T("D6"));
   d7 = GetDimension(dimensions,_T("D7"));
   w1 = GetDimension(dimensions,_T("W1"));
   w2 = GetDimension(dimensions,_T("W2"));
   w3 = GetDimension(dimensions,_T("W3"));
   w4 = GetDimension(dimensions,_T("W4"));
   wmin = GetDimension(dimensions,_T("Wmin"));
   wmax = GetDimension(dimensions,_T("Wmax"));
   t1 = GetDimension(dimensions,_T("T1"));
   t2 = GetDimension(dimensions,_T("T2"));
}

Float64 CBulbTeeFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name) const
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

pgsTypes::SupportedBeamSpacings CBulbTeeFactory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacentWithTopWidth);
   sbs.push_back(pgsTypes::sbsGeneralAdjacentWithTopWidth);
   return sbs;
}

bool CBulbTeeFactory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(),spacingType);
   return found == sbs.end() ? false : true;
}

bool CBulbTeeFactory::ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
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

pgsTypes::SupportedDeckTypes CBulbTeeFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
{
   pgsTypes::SupportedDeckTypes sdt;

   switch(sbs)
   {
   case pgsTypes::sbsUniformAdjacentWithTopWidth:
   case pgsTypes::sbsGeneralAdjacentWithTopWidth:
      sdt.push_back(pgsTypes::sdtCompositeCIP);
      sdt.push_back(pgsTypes::sdtCompositeOverlay);
      sdt.push_back(pgsTypes::sdtNonstructuralOverlay);
      sdt.push_back(pgsTypes::sdtNone);
      break;

   default:
      ATLASSERT(false);
   }

   return sdt;
}

std::vector<pgsTypes::GirderOrientationType> CBulbTeeFactory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb,pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal };
   return types;
}

bool CBulbTeeFactory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType CBulbTeeFactory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes CBulbTeeFactory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CBulbTeeFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
{
   pgsTypes::SupportedDiaphragmLocationTypes locations;
   switch(type)
   {
   case pgsTypes::dtCastInPlace :
      locations.push_back(pgsTypes::dltExternal);
      break;

   default:
      ATLASSERT(false);
   }

   return locations;
}

void CBulbTeeFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   // this is for joint spacing... effective want unrestrained joint spacing
   // need 0 for welded shear tab and grout key connection and a specific value for UHPC-type connections
   *minSpacing = 0;
   *maxSpacing = MAX_GIRDER_SPACING;
}

std::vector<pgsTypes::TopWidthType> CBulbTeeFactory::GetSupportedTopWidthTypes() const
{
   std::vector<pgsTypes::TopWidthType> types{ pgsTypes::twtSymmetric,pgsTypes::twtCenteredCG,pgsTypes::twtAsymmetric };
   ATLASSERT(CanTopWidthVary() == false); // the equations for CenteredCG have not been derived for variable top width
   return types;
}

void CBulbTeeFactory::GetAllowableTopWidthRange(const IBeamFactory::Dimensions& dimensions, Float64* pWmin, Float64* pWmax) const
{
   *pWmin = GetDimension(dimensions, _T("Wmin"));
   *pWmax = GetDimension(dimensions, _T("Wmax"));
}

WebIndexType CBulbTeeFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions) const
{
   return 1;
}

Float64 CBulbTeeFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 D1 = GetDimension(dimensions,_T("D1"));
   Float64 D2 = GetDimension(dimensions,_T("D2"));
   Float64 D3 = GetDimension(dimensions,_T("D3"));
   Float64 D4 = GetDimension(dimensions,_T("D4"));
   Float64 D5 = GetDimension(dimensions,_T("D5"));
   Float64 D6 = GetDimension(dimensions,_T("D6"));
   Float64 D7 = GetDimension(dimensions,_T("D7"));

   return D1 + D2 + D3 + D4 + D5 + D6 + D7;
}

Float64 CBulbTeeFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("Wmax"));
}

bool CBulbTeeFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void CBulbTeeFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool CBulbTeeFactory::HasLongitudinalJoints() const
{
   return true;
}

bool CBulbTeeFactory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
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

bool CBulbTeeFactory::HasTopFlangeThickening() const
{
   return true;
}

bool CBulbTeeFactory::CanPrecamber() const
{
   return true;
}

GirderIndexType CBulbTeeFactory::GetMinimumBeamCount() const
{
   return 2;
}

//////////////////////////////////////////////
// IBeamFactoryCompatibility
pgsCompatibilityData* CBulbTeeFactory::GetCompatibilityData() const
{
   if (m_bHaveOldTopFlangeThickening)
   {
      pgsCompatibilityData* pData = new pgsCompatibilityData;
      pData->AddValue(_T("D8"), m_OldTopFlangeThickening);
      m_bHaveOldTopFlangeThickening = false; // the old data has been moved to the owning library entry
      return pData;
   }
   else
   {
      // we don't have an old D8 value (probably because this is a newer file)
      return nullptr;
   }
}

void CBulbTeeFactory::UpdateBridgeModel(CBridgeDescription2* pBridgeDesc, const GirderLibraryEntry* pGirderEntry) const
{
   pgsCompatibilityData* pData = pGirderEntry->GetCompatibilityData();
   if (pData == nullptr)
   {
      // no compatiblity data, so we aren't making any updates
      return;
   }

   Float64 D8 = pData->GetValue(_T("D8"));
   if (IsZero(D8))
   {
      // the top flanges where really thickened
      return;
   }

   // The top flange thickening dimension, "D8", has been removed from Deck Bulb Tee girders
   // If the value was set to something other than 0.0, we need to move that data to the girder segment
   ATLASSERT(pBridgeDesc->GetGirderFamilyName() == CString(_T("Deck Bulb Tee")));
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
      {
         CSplicedGirderData* pGirder = pGroup->GetGirder(gdrIdx);
         const GirderLibraryEntry* pYourGirderEntry = pGirder->GetGirderLibraryEntry();

         if (pYourGirderEntry == pGirderEntry)
         {
            SegmentIndexType nSegments = pGirder->GetSegmentCount();
            for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
            {
               CPrecastSegmentData* pSegment = pGirder->GetSegment(segIdx);
               pSegment->TopFlangeThickeningType = pgsTypes::tftEnds;
               pSegment->TopFlangeThickening = D8;
            }
         }
      }
   }
}

bool CBulbTeeFactory::IsPrismatic(const CPrecastSegmentData* pSegment) const
{
   if (pSegment->TopFlangeThickeningType == pgsTypes::tftNone || IsZero(pSegment->TopFlangeThickening))
   {
      return true;
   }

   return false;
}
void CBulbTeeFactory::ConfigureBeamShape(IBroker* pBroker, const CPrecastSegmentData* pSegment, IBulbTee2* pBeam) const
{
   // pBeam is the basic section... figure out actual top width parameters
   Float64 c2, n1, n2, left, right;
   GetTopFlangeParameters(pBroker, pSegment, &c2, &n1, &n2, &left, &right);
   pBeam->put_C2(c2);
   pBeam->put_n1(n1);
   pBeam->put_n2(n2);
   pBeam->put_W5(left);
   pBeam->put_W6(right);
}

void CBulbTeeFactory::GetTopWidth(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs,Float64* pLeft, Float64* pRight) const
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

void CBulbTeeFactory::GetTopFlangeParameters(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64* pC, Float64* pN1, Float64* pN2,Float64* pLeft,Float64* pRight) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();

   pgsTypes::TopWidthType topWidthType;
   Float64 leftStart, rightStart, leftEnd, rightEnd;
   pGirder->GetTopWidth(&topWidthType, &leftStart, &rightStart, &leftEnd, &rightEnd);

   switch (topWidthType)
   {
   case pgsTypes::twtSymmetric:
   case pgsTypes::twtCenteredCG: // we'll update the values below for this case
      leftStart /= 2;
      rightStart = leftStart;
      break;

   case pgsTypes::twtAsymmetric:
      // we already have left/right values
      break;

   default:
      ATLASSERT(false);
   }

   Float64 n1(0), n2(0), c2(0); // c2 is the distance from the left flange tip to where the top flange slope changes from n1 to n2
   Float64 left(leftStart), right(rightStart);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->GetDeckType();
   if ((deckType == pgsTypes::sdtNone || deckType == pgsTypes::sdtCompositeOverlay || deckType == pgsTypes::sdtNonstructuralOverlay) && pBridgeDesc->GetGirderOrientation() == pgsTypes::Plumb)
   {
      // need to get cross slope shape parameters
      // c2, n1, n2

      const auto& segmentKey(pSegment->GetSegmentKey());

      GET_IFACE2(pBroker, IBridge, pBridge);
      Float64 station, offset;
      pBridge->GetStationAndOffset(segmentKey, 0.0, &station, &offset);

      GET_IFACE2(pBroker, IRoadway, pAlignment);
      Float64 CPO = pAlignment->GetCrownPointOffset(station);

      Float64 alignment_offset = pBridge->GetAlignmentOffset();

      Float64 left_edge_offset = offset - leftStart;
      Float64 right_edge_offset = offset + rightStart;

      if (IsLT(left_edge_offset, CPO) && IsLT(CPO, right_edge_offset))
      {
         // the crown point occurs somewhere in the top flange
         n1 = pAlignment->GetSlope(station, left_edge_offset);
         n2 = pAlignment->GetSlope(station, right_edge_offset);
         c2 = fabs(CPO - left_edge_offset);
      }
      else
      {
         // the entire top flange is sloped at n2
         n1 = 0;
         n2 = pAlignment->GetSlope(station, offset);
         c2 = 0; // no part of the top flange is sloped at n1
      }

      if (topWidthType == pgsTypes::twtCenteredCG)
      {
         // forget about input top width for left and right... compute it so the CG is centered on the CL Web
         
         // the equations below are for contant width girder
         Float64 Wtf = leftStart + rightStart;

         const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
         const auto& dimensions = pGirderEntry->GetDimensions();

         Float64 he; // minimum top flange thickness at girder ends
         Float64 hm; // minimum top flange thickness at middle (L/2)
         he = GetDimension(dimensions, _T("D1"));
         hm = he;
         if (pSegment->TopFlangeThickeningType == pgsTypes::tftEnds)
         {
            he += pSegment->TopFlangeThickening;
         }
         else
         {
            hm += pSegment->TopFlangeThickening;
         }


         Float64 k;
         Float64 A = fabs(c2*n1);
         Float64 B = fabs((Wtf - c2)*n2);
         if ((IsZero(A) && n2 < 0) || (B < A && 0 <= n1 && n2 <= 0) || (n1 <= 0 && n2 <= 0))
         {
            // case 1b and 3
            Float64 n = 0.5*Wtf*Wtf*(he + 2 * hm) + 0.5*(n2 - n1)*c2*c2*c2 - 0.5*n2*Wtf*Wtf*Wtf;
            Float64 d = 2 * Wtf*(he + 2 * hm) + 3 * (n2 - n1)*c2*c2 - 3 * n2*Wtf*Wtf;
            k = (2 * n) / (Wtf*d);
         }
         else if ((A <= B && 0 <= n1 && n2 <= 0) || (0 <= n1 && 0 <= n2))
         {
            // case 1a and 2
            Float64 n = 0.5*Wtf*Wtf*(he + 2 * hm) + 0.5*(n2 - n1)*c2*c2*c2 - 1.5*(n2 - n1)*Wtf*Wtf*c2 + n2*Wtf*Wtf*Wtf;
            Float64 d = 2 * Wtf*(he + 2 * hm) + 3 * (n2 - n1)*c2*c2 - 6 * (n2 - n1)*Wtf*c2 + 3 * n2*Wtf*Wtf;
            k = (2 * n) / (Wtf*d);
         }
         else
         {
            // case 4
            ATLASSERT(n1 <= 0 && 0 <= n2);
            Float64 n = 0.5*Wtf*Wtf*(he + 2 * hm) + 0.5*(n2 - n1)*c2*c2*c2 - 1.5*n2*Wtf*Wtf*c2 + n2*Wtf*Wtf*Wtf;
            Float64 d = 2 * Wtf*(he + 2 * hm) - 6 * n2*Wtf*c2 + 3 * (n2 - n1)*c2*c2 + 3 * n2*Wtf*Wtf;
            k = (2 * n) / (Wtf*d);
         }

         ATLASSERT(0.0 <= k && k <= 1.0);

         left = k*Wtf;
         right = (1 - k)*Wtf;
      }
   }


   *pC = c2;
   *pN1 = n1;
   *pN2 = n2;
   *pLeft = left;
   *pRight = right;
}

Float64 CBulbTeeFactory::GetFlangeThickening(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   Float64 tft = 0;
   if (pSegment->TopFlangeThickeningType != pgsTypes::tftNone)
   {
      // parabolic interpolation of the depth of the top flange thickening
      const CSegmentKey& segmentKey(pSegment->GetSegmentKey());
      
      GET_IFACE2(pBroker, IBridge, pBridge);
      Float64 Ls = pBridge->GetSegmentLength(segmentKey);

      Float64 thickening = pSegment->TopFlangeThickening;

      if (pSegment->TopFlangeThickeningType == pgsTypes::tftEnds )
      {
         tft = 4 * thickening*Xs*Xs / (Ls*Ls) - 4 * thickening*Xs / Ls + thickening;
      }
      else
      {
         tft = -4 * thickening*Xs*Xs / (Ls*Ls) + 4 * thickening*Xs / Ls;
      }
   }
   return tft;
}

void CBulbTeeFactory::PositionBeamShape(IBulbTee2* pBeam) const
{

   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_CLHeight(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}
