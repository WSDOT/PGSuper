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

// UBeam2Factory.cpp : Implementation of CUBeam2Factory
#include "stdafx.h"
#include "Beams.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "UBeam2Factory.h"
#include <Beams/UBeamDistFactorEngineer.h>
#include <Beams/PsBeamLossEngineer.h>
#include <Beams/TimeStepLossEngineer.h>
#include "StrandMoverImpl.h"
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\AgeAdjustedMaterial.h>

#include <Beams\Helper.h>
#include <PgsExt/PoiMgr.h>

#include <PsgLib\BridgeDescription2.h>
#include <psgLib/SectionPropertiesCriteria.h>
#include <psgLib/SpecificationCriteria.h>
#include <psgLib/GirderLibraryEntry.h>

using namespace PGS::Beams;

INIT_BEAM_FACTORY_SINGLETON(UBeam2Factory)

UBeam2Factory::UBeam2Factory() : BeamFactory()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.emplace_back(_T("C1"));
   m_DimNames.emplace_back(_T("D1"));
   m_DimNames.emplace_back(_T("D2"));
   m_DimNames.emplace_back(_T("D3"));
   m_DimNames.emplace_back(_T("D4"));
   m_DimNames.emplace_back(_T("D5"));
   m_DimNames.emplace_back(_T("D6"));
   m_DimNames.emplace_back(_T("EndBlockLength"));
   m_DimNames.emplace_back(_T("W1"));
   m_DimNames.emplace_back(_T("W2"));
   m_DimNames.emplace_back(_T("W3"));
   m_DimNames.emplace_back(_T("W4"));
   m_DimNames.emplace_back(_T("W5"));
   m_DimNames.emplace_back(_T("W6"));
   m_DimNames.emplace_back(_T("W7"));

   // Default beam is a TXDOT U40
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.750,WBFL::Units::Measure::Inch)); // C1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(40.00,WBFL::Units::Measure::Inch)); // D1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 8.25,WBFL::Units::Measure::Inch)); // D2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 3.00,WBFL::Units::Measure::Inch)); // D3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.875,WBFL::Units::Measure::Inch)); // D4
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(5.875,WBFL::Units::Measure::Inch)); // D5
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(21.625,WBFL::Units::Measure::Inch)); // D6
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 0.0,WBFL::Units::Measure::Inch)); // End Block Length
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(55.00,WBFL::Units::Measure::Inch)); // W1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(89.00,WBFL::Units::Measure::Inch)); // W2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 3.00,WBFL::Units::Measure::Inch)); // W3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.375,WBFL::Units::Measure::Inch)); // W4
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 8.25,WBFL::Units::Measure::Inch)); // W5
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(15.75,WBFL::Units::Measure::Inch)); // W6
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 1.75,WBFL::Units::Measure::Inch)); // W7

   // SI Units
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // C1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D3
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D4
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D5
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D6
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // End Block Length
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W3
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W4
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W5
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W6
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W7

   // US Units
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // C1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D3
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D4
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D5
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D6
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // End Block Length
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W3
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W4
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W5
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W6
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W7
}

void UBeam2Factory::CreateGirderSection(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const BeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   CComPtr<IUGirderSection2> gdrSection;
   gdrSection.CoCreateInstance(CLSID_UGirderSection2);
   CComPtr<IUBeam2> beam;
   gdrSection->get_Beam(&beam);

   DimensionAndPositionBeam(dimensions, beam);

   gdrSection.QueryInterface(ppSection);
}

void UBeam2Factory::CreateSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
{
   CComPtr<IUGirderSection2EndBlockSegment> segment;
   segment.CoCreateInstance(CLSID_UGirderSection2EndBlockSegment);

   // Build up the beam shape
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment    = pGirder->GetSegment(segmentKey.segmentIndex);

   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   segment->put_EndBlockLength(etStart,endBlockLength);
   segment->put_EndBlockLength(etEnd,endBlockLength);

   CComPtr<IGirderSection> gdrSection;
   CreateGirderSection(pBroker,statusGroupID,dimensions,-1,-1,&gdrSection);

   // Beam materials
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   CComPtr<IMaterial> material;
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      CComPtr<IAgeAdjustedMaterial> aaMaterial;
      BuildAgeAdjustedGirderMaterialModel(pBroker,pSegment,segment,&aaMaterial);
      aaMaterial.QueryInterface(&material);
   }
   else
   {
      GET_IFACE2(pBroker,IIntervals,pIntervals);
      GET_IFACE2(pBroker,IMaterials,pMaterial);
      material.CoCreateInstance(CLSID_Material);

      IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
      for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
      {
         Float64 E = pMaterial->GetSegmentEc(segmentKey,intervalIdx);
         Float64 D = pMaterial->GetSegmentWeightDensity(segmentKey,intervalIdx);

         material->put_E(intervalIdx,E);
         material->put_Density(intervalIdx,D);
      }
   }

   CComQIPtr<IShape> shape(gdrSection);
   ATLASSERT(shape);
   segment->AddShape(shape,material,nullptr);

   CComQIPtr<ISuperstructureMemberSegment> ssmbrSegment(segment);
   ssmbrSegment.CopyTo(ppSegment);
}

void UBeam2Factory::CreateSegmentShape(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   CComPtr<IUBeam2> beam;
   beam.CoCreateInstance(CLSID_UBeam2);

   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   DimensionAndPositionBeam(dimensions, beam);

   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 Lg = pBridge->GetSegmentLength(pSegment->GetSegmentKey());
   Float64 endBlockLength = GetDimension(dimensions, _T("EndBlockLength"));
   if (IsInEndBlock(Xs, sectionBias, endBlockLength, Lg))
   {
      beam->put_UseOutlineOnly(VARIANT_TRUE);
   }

   beam.QueryInterface(ppShape);
}

Float64 UBeam2Factory::GetSegmentHeight(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   Float64 D1 = GetDimension(dimensions, _T("D1"));
   return D1;
}

void UBeam2Factory::ConfigureSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void UBeam2Factory::LayoutSectionChangePointsOfInterest(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE );

   VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);
   VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);

   // put section breaks just on either side of the end blocks/void interface
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   Float64 endBlockLength = pGdrEntry->GetDimension(_T("EndBlockLength"));

   if ( !IsZero(endBlockLength) )
   {
      pgsPointOfInterest poiLeftFace1(segmentKey, endBlockLength, POI_SECTCHANGE_LEFTFACE);
      pgsPointOfInterest poiRightFace1(segmentKey, endBlockLength, POI_SECTCHANGE_RIGHTFACE);
      PoiIDType poiID = pPoiMgr->AddPointOfInterest(poiLeftFace1);
      ATLASSERT(poiID != INVALID_ID);
      poiLeftFace1 = pPoiMgr->GetPointOfInterest(poiID);
      poiRightFace1.SetDistFromStart(poiLeftFace1.GetDistFromStart(),true);
      VERIFY(pPoiMgr->AddPointOfInterest(poiRightFace1) != INVALID_ID);

      pgsPointOfInterest poiRightFace2(segmentKey, gdrLength - endBlockLength, POI_SECTCHANGE_RIGHTFACE);
      pgsPointOfInterest poiLeftFace2(segmentKey, gdrLength - endBlockLength, POI_SECTCHANGE_LEFTFACE);
      poiID = pPoiMgr->AddPointOfInterest(poiRightFace2);
      ATLASSERT(poiID != INVALID_ID);
      poiRightFace2 = pPoiMgr->GetPointOfInterest(poiID);
      poiLeftFace2.SetDistFromStart(poiRightFace2.GetDistFromStart(),true);
      VERIFY(pPoiMgr->AddPointOfInterest(poiLeftFace2) != INVALID_ID);
   }
}

std::unique_ptr<DistFactorEngineer> UBeam2Factory::CreateDistFactorEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect) const
{
   return std::make_unique<UBeamDistFactorEngineer>(pBroker, statusGroupID);
}

std::unique_ptr<PsLossEngineerBase> UBeam2Factory::CreatePsLossEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      return std::make_unique<TimeStepLossEngineer>(pBroker,statusGroupID);
   }
   else
   {
      return std::make_unique<PsBeamLossEngineer>(PsBeamLossEngineer::BeamType::UBeam,pBroker,statusGroupID);
   }
}

void UBeam2Factory::CreateStrandMover(const BeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  BeamFactory::BeamFace endTopFace, Float64 endTopLimit, BeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  BeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, BeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
{
   // build our shape so we can get higher-level info
   CComPtr<IUBeam2> beam;
   beam.CoCreateInstance(CLSID_UBeam2);

   DimensionAndPositionBeam(dimensions, beam);

   // our goal is to build a parallelogram using the thin web dimension from top to bottom
   Float64 t;
   beam->get_T(&t);
   Float64 slope;
   beam->get_Slope(1, &slope);
   Float64 depth;
   beam->get_Height(&depth);
   Float64 w1;
   beam->get_W1(&w1);

   Float64 arc_slope = 1.0/slope;

   Float64 t_x_project = t*sqrt(slope*slope+1)/slope;

   CComPtr<IPolyShape> rgt_harp_poly;
   rgt_harp_poly.CoCreateInstance(CLSID_PolyShape);

   // travel counter clockwise around right web;
   Float64 x1 = w1/2.0;
   Float64 y1 = -depth;

   Float64 x2 = x1 + depth * arc_slope;
   Float64 y2 = 0;

   Float64 x3 = x2 - t_x_project;
   Float64 y3 = y2;

   Float64 x4 = x1 - t_x_project;
   Float64 y4 = y1;

   rgt_harp_poly->AddPoint(x1,y1);
   rgt_harp_poly->AddPoint(x2,y2);
   rgt_harp_poly->AddPoint(x3,y3);
   rgt_harp_poly->AddPoint(x4,y4);
   rgt_harp_poly->AddPoint(x1,y1);

   // left side is same with negative x's
   CComPtr<IPolyShape> lft_harp_poly;
   lft_harp_poly.CoCreateInstance(CLSID_PolyShape);

   lft_harp_poly->AddPoint(-x1,y1);
   lft_harp_poly->AddPoint(-x2,y2);
   lft_harp_poly->AddPoint(-x3,y3);
   lft_harp_poly->AddPoint(-x4,y4);
   lft_harp_poly->AddPoint(-x1,y1);

   CComPtr<IShape> lft_shape, rgt_shape;
   lft_harp_poly->get_Shape(&lft_shape);
   rgt_harp_poly->get_Shape(&rgt_shape);

   // now make our strand mover and fill it up
   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);
   CComPtr<IStrandMover> sm = pStrandMover;

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   HRESULT hr = configurer->AddRegion(lft_shape, -arc_slope);
   ATLASSERT (SUCCEEDED(hr));
   hr = configurer->AddRegion(rgt_shape, arc_slope);
   ATLASSERT (SUCCEEDED(hr));

   // set vertical offset bounds and increments
   Float64 hptb  = hpTopFace     == BeamFactory::BeamFace::Bottom ? hpTopLimit     - depth : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == BeamFactory::BeamFace::Bottom ? hpBottomLimit  - depth : -hpBottomLimit;
   Float64 endtb = endTopFace    == BeamFactory::BeamFace::Bottom ? endTopLimit    - depth : -endTopLimit;
   Float64 endbb = endBottomFace == BeamFactory::BeamFace::Bottom ? endBottomLimit - depth : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(0, depth, endtb, endbb, hptb, hpbb, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}

const std::vector<std::_tstring>& UBeam2Factory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& UBeam2Factory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const WBFL::Units::Length*>& UBeam2Factory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool UBeam2Factory::ValidateDimensions(const Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const
{
   Float64 w1, w2, w3, w4, w5, w6, w7;
   Float64 d1, d2, d3, d4, d5, d6;
   Float64 c1, EndBlockLength;
   GetDimensions(dimensions,d1, d2, d3, d4, d5, d6, w1, w2, w3, w4, w5, w6, w7, c1, EndBlockLength);

// D1  0
// D2  1
// D3  2
// D4  3
// D5  4
// D6  5
// W1  6
// W2  7
// W3  8
// W4  9
// W5  10
// W6  11
// W7  12

   if ( d1 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][1];
      std::_tostringstream os;
      os << _T("D1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d2 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][2];
      std::_tostringstream os;
      os << _T("D2 must be greater than 0.0") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d3 < 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][3];
      std::_tostringstream os;
      os << _T("D3 must be greater than or equal to 0.0") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d1 < d2+d3 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][1];
      std::_tostringstream os;
      os << _T("D1 must be greater than or equal to D2 + D3 (") << WBFL::Units::ConvertFromSysUnits(d2+d3,*pUnit) << _T(" ") << pUnit->UnitTag() << _T(")") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][7];
      std::_tostringstream os;
      os << _T("W1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( w2 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("W2 must be greater than 0.0") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w3 < 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
      std::_tostringstream os;
      os << _T("W3 must be greater than or equal to 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( c1 < 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
      std::_tostringstream os;
      os << _T("C1 must be greater than or equal to 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( c1 > w5 )
   {
      std::_tostringstream os;
      os << _T("C1 must be less than or equal to W5") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( c1 > d5 )
   {
      std::_tostringstream os;
      os << _T("C1 must be less than or equal to D5") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( w1 > w2-2*(w4+w5) )
   {
      std::_tostringstream os;
      os << _T("W1 must be less than w2 - 2 * (w4+w5)") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   


   // build our shape so we can get higher-level info
   CComPtr<IUBeam2> beam;
   beam.CoCreateInstance(CLSID_UBeam2);

   DimensionAndPositionBeam(dimensions, beam);

   // our goal is to build a parallelogram using the thin web dimension from top to bottom
   Float64 t;
   beam->get_T(&t);
   if ( t<=0.0 )
   {
      std::_tostringstream os;
      os << _T("Dimensions are such that the web thickness is zero.") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   
   return true;
}

void UBeam2Factory::SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const BeamFactory::Dimensions& dimensions) const
{
   // Version 2.0 - Added EndBlockLength
   pSave->BeginUnit(_T("UBeam2Dimensions"),2.0);
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

BeamFactory::Dimensions UBeam2Factory::LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const
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

   BeamFactory::Dimensions dimensions;

   Float64 dimVersion = 1.0;
   if ( 14 <= parent_version )
   {
      if (pLoad->BeginUnit(_T("UBeam2Dimensions")))
      {
         dimVersion = pLoad->GetVersion();
      }
      else
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   for(const auto& name : m_DimNames)
   {
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
      {
         // failed to read dimension value...
         
         if ( dimVersion < 2)
         {
            if( parent_version < 3.0 && name == _T("C1") )
            {
               value = 0.0; // set the default value
            }
            else if (name == _T("EndBlockLength"))
            {
               value = 0.0;
            }
            else
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
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

bool UBeam2Factory::IsPrismatic(const BeamFactory::Dimensions& dimensions) const
{
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   return IsZero(endBlockLength) ? true : false;
}

bool UBeam2Factory::IsPrismatic(const CSegmentKey& segmentKey) const
{
   auto pBroker = EAFGetBroker();

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   return IsPrismatic(dimensions);
}

bool UBeam2Factory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

std::_tstring UBeam2Factory::GetImage() const
{
   return std::_tstring(_T("UBeam2.jpg"));
}

std::_tstring UBeam2Factory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      strImage =  _T("UBeam_Slab_CIP.gif");
      break;

   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("UBeam_Slab_SIP.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring UBeam2Factory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("+Mn_UBeam_Composite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring UBeam2Factory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("-Mn_UBeam_Composite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring UBeam2Factory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("Vn_UBeam.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring UBeam2Factory::GetInteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
   {
      return _T("UBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("UBeam_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring UBeam2Factory::GetExteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
   {
      return _T("UBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("UBeam_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID UBeam2Factory::GetCLSID() const
{
   return CLSID_UBeam2Factory;
}

CLSID UBeam2Factory::GetFamilyCLSID() const
{
   return CLSID_UBeamFamily;
}

std::_tstring UBeam2Factory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring UBeam2Factory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE UBeam2Factory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR UBeam2Factory::GetImageResourceName() const
{
   return _T("UBEAM2");
}

HICON  UBeam2Factory::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TEXASU) );
}

void UBeam2Factory::GetDimensions(const BeamFactory::Dimensions& dimensions,
                                  Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,Float64& d6,
                                  Float64& w1,Float64& w2,Float64& w3,Float64& w4,Float64& w5,Float64& w6,Float64& w7,
                                  Float64& c1,Float64& EndBlockLength) const
{
   d1 = GetDimension(dimensions,_T("D1"));
   d2 = GetDimension(dimensions,_T("D2"));
   d3 = GetDimension(dimensions,_T("D3"));
   d4 = GetDimension(dimensions,_T("D4"));
   d5 = GetDimension(dimensions,_T("D5"));
   d6 = GetDimension(dimensions,_T("D6"));
   w1 = GetDimension(dimensions,_T("W1"));
   w2 = GetDimension(dimensions,_T("W2"));
   w3 = GetDimension(dimensions,_T("W3"));
   w4 = GetDimension(dimensions,_T("W4"));
   w5 = GetDimension(dimensions,_T("W5"));
   w6 = GetDimension(dimensions,_T("W6"));
   w7 = GetDimension(dimensions,_T("W7"));
   c1 = GetDimension(dimensions,_T("C1"));
   EndBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
}

Float64 UBeam2Factory::GetDimension(const BeamFactory::Dimensions& dimensions,const std::_tstring& name) const
{
   for (auto const& dim : dimensions)
   {
      if (name == dim.first)
      {
         return dim.second;
      }
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedDeckTypes UBeam2Factory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
{
   pgsTypes::SupportedDeckTypes sdt;
   switch(sbs)
   {
   case pgsTypes::sbsUniform:
   case pgsTypes::sbsGeneral:
      sdt.push_back(pgsTypes::sdtCompositeCIP);
      sdt.push_back(pgsTypes::sdtCompositeSIP);
      break;

   default:
      ATLASSERT(false);
   }
   return sdt;
}

pgsTypes::SupportedBeamSpacings UBeam2Factory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

bool UBeam2Factory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool UBeam2Factory::ConvertBeamSpacing(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   return false;
}

pgsTypes::WorkPointLocations UBeam2Factory::GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::WorkPointLocations wpls;
   wpls.push_back(pgsTypes::wplTopGirder);
   wpls.push_back(pgsTypes::wplBottomGirder);

   return wpls;
}

bool UBeam2Factory::IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation wpType) const
{
   pgsTypes::WorkPointLocations sbs = GetSupportedWorkPointLocations(spacingType);
   auto found = std::find(sbs.cbegin(), sbs.cend(), wpType);
   return found == sbs.end() ? false : true;
}


std::vector<pgsTypes::GirderOrientationType> UBeam2Factory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb, pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal,pgsTypes::Balanced };
   return types;
}

bool UBeam2Factory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType UBeam2Factory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes UBeam2Factory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes UBeam2Factory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
{
   pgsTypes::SupportedDiaphragmLocationTypes locations;
   switch(type)
   {
   case pgsTypes::dtPrecast :
      locations.push_back(pgsTypes::dltInternal);
      break;

   case pgsTypes::dtCastInPlace :
      locations.push_back(pgsTypes::dltInternal);
      locations.push_back(pgsTypes::dltExternal);
      break;

   default:
      ATLASSERT(false);
   }

   return locations;
}

void UBeam2Factory::GetAllowableSpacingRange(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));

   Float64 gw = Max(W1, W2);


   if ( sdt == pgsTypes::sdtCompositeCIP || sdt == pgsTypes::sdtCompositeSIP )
   {
      if ( sbs == pgsTypes::sbsUniform || sbs == pgsTypes::sbsGeneral )
      {
         *minSpacing = gw;
         *maxSpacing = MAX_GIRDER_SPACING;
      }
      else
      {
         ATLASSERT(false); // shouldn't get here
      }
   }
   else
   {
      ATLASSERT(false); // shouldn't get here
   }
}

WebIndexType UBeam2Factory::GetWebCount(const BeamFactory::Dimensions& dimensions) const
{
   return 2;
}

Float64 UBeam2Factory::GetBeamHeight(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("D1"));
}

Float64 UBeam2Factory::GetBeamWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("W2"));
}

void UBeam2Factory::GetBeamTopWidth(const BeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const
{
   Float64 W2 = GetDimension(dimensions,_T("W2"));

   Float64 top = W2;
   top /= 2.0;

   *pLeftWidth = top;
   *pRightWidth = top;
}

bool UBeam2Factory::IsShearKey(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void UBeam2Factory::GetShearKeyAreas(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool UBeam2Factory::HasLongitudinalJoints() const
{
   return false;
}

bool UBeam2Factory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool UBeam2Factory::HasTopFlangeThickening() const
{
   return false;
}

bool UBeam2Factory::CanPrecamber() const
{
   return false;
}

GirderIndexType UBeam2Factory::GetMinimumBeamCount() const
{
   return 1;
}

void UBeam2Factory::DimensionAndPositionBeam(const BeamFactory::Dimensions& dimensions, IUBeam2* pBeam) const
{
   Float64 w1, w2, w3, w4, w5, w6, w7;
   Float64 d1, d2, d3, d4, d5, d6;
   Float64 c1, EndBlockLength;
   GetDimensions(dimensions, d1, d2, d3, d4, d5, d6, w1, w2, w3, w4, w5, w6, w7, c1, EndBlockLength);
   pBeam->put_W1(w1);
   pBeam->put_W2(w2);
   pBeam->put_W3(w3);
   pBeam->put_W4(w4);
   pBeam->put_W5(w5);
   pBeam->put_W6(w6);
   pBeam->put_W7(w7);
   pBeam->put_D1(d1);
   pBeam->put_D2(d2);
   pBeam->put_D3(d3);
   pBeam->put_D4(d4);
   pBeam->put_D5(d5);
   pBeam->put_D6(d6);
   pBeam->put_C1(c1);

   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_Height(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}
