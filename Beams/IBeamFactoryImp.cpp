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

// BeamFactory.cpp : Implementation of CIBeamFactory
#include "stdafx.h"
#include "Beams.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include <Beams/IBeamFactoryImp.h>
#include <Beams/IBeamDistFactorEngineer.h>
#include <Beams/PsBeamLossEngineer.h>
#include <Beams/TimeStepLossEngineer.h>
#include "StrandMoverImpl.h"
#include <GeomModel\PrecastBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <GenericBridge\Helpers.h>

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


#define D1 0
#define D2 1
#define D3 2
#define D4 3
#define D5 4
#define D6 5
#define H  6
#define T1 7
#define T2 8
#define W1 9
#define W2 10
#define W3 11
#define W4 12
#define W5 13
#define C1 14
#define EBW 15
#define EBL 16
#define EBT 17

INIT_BEAM_FACTORY_SINGLETON(IBeamFactory)

IBeamFactory::IBeamFactory() : BeamFactory()
{
   // Initialize with default values... This are not necessarily valid dimensions
   // use emplace_back instead of push_back to construct the string inside the container instead of creating an unnamed temporary that is copied into the container
   m_DimNames.emplace_back(_T("D1"));
   m_DimNames.emplace_back(_T("D2"));
   m_DimNames.emplace_back(_T("D3"));
   m_DimNames.emplace_back(_T("D4"));
   m_DimNames.emplace_back(_T("D5"));
   m_DimNames.emplace_back(_T("D6"));
   m_DimNames.emplace_back(_T("H"));
   m_DimNames.emplace_back(_T("T1"));
   m_DimNames.emplace_back(_T("T2"));
   m_DimNames.emplace_back(_T("W1"));
   m_DimNames.emplace_back(_T("W2"));
   m_DimNames.emplace_back(_T("W3"));
   m_DimNames.emplace_back(_T("W4"));
   m_DimNames.emplace_back(_T("W5"));
   m_DimNames.emplace_back(_T("C1"));
   m_DimNames.emplace_back(_T("EndBlockWidth"));
   m_DimNames.emplace_back(_T("EndBlockLength"));
   m_DimNames.emplace_back(_T("EndBlockTransition"));

   // Default beam is a W74G
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(2.875,WBFL::Units::Measure::Inch)); // D1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(2.625,WBFL::Units::Measure::Inch)); // D2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(2.000,WBFL::Units::Measure::Inch)); // D3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(6.000,WBFL::Units::Measure::Inch)); // D4
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(3.000,WBFL::Units::Measure::Inch)); // D5
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.000,WBFL::Units::Measure::Inch)); // D6
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(74.00,WBFL::Units::Measure::Inch)); // H
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(6.000,WBFL::Units::Measure::Inch)); // T1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(6.000,WBFL::Units::Measure::Inch)); // T2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.000, WBFL::Units::Measure::Inch)); // W1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(16.50,WBFL::Units::Measure::Inch)); // W2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(2.000,WBFL::Units::Measure::Inch)); // W3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.000, WBFL::Units::Measure::Inch)); // W4
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(9.500, WBFL::Units::Measure::Inch)); // W5
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.000, WBFL::Units::Measure::Inch)); // C1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.000,WBFL::Units::Measure::Inch)); // EndBlockWidth
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.000,WBFL::Units::Measure::Feet)); // EndBlockLength
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.000,WBFL::Units::Measure::Feet)); // EndBlockTransition

   // SI Units
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D3
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D4
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D5
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D6
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // H
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // T1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // T2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W3
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W4
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W5
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // C1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // EndBlockWidth
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // EndBlockLength
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // EndBlockTransition

   // US Units
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D3
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D4
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D5
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D6
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // H
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // T1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // T2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W3
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W4
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W5
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // C1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // EndBlockWidth
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Feet); // EndBlockLength
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Feet); // EndBlockTransition
}

void IBeamFactory::CreateGirderSection(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const BeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   CComPtr<IFlangedGirderSection2> gdrSection;
   gdrSection.CoCreateInstance(CLSID_FlangedGirderSection2);

   CComPtr<IPrecastBeam2> beam;
   gdrSection->get_Beam(&beam);

   DimensionAndPositionBeam(dimensions, beam);

   gdrSection.QueryInterface(ppSection);
}

void IBeamFactory::CreateSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
{
   CComPtr<ISuperstructureMemberSegment> segment;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   bool bPrismatic = IsPrismatic(segmentKey);
   if ( bPrismatic )
   {
      // prismatic
      segment.CoCreateInstance(CLSID_PrismaticSuperstructureMemberSegment);
   }
   else
   {
      // non-prismatic
      segment.CoCreateInstance(CLSID_FlangedGirderEndBlockSegment);
   }

   ATLASSERT(segment != nullptr);

   // Build up the beam shape
   Float64 ebWidth, ebLength, ebTransition;
   ebWidth      = GetDimension(dimensions,_T("EndBlockWidth"));
   ebLength     = GetDimension(dimensions,_T("EndBlockLength"));
   ebTransition = GetDimension(dimensions,_T("EndBlockTransition"));

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


   if ( bPrismatic )
   {
      CComQIPtr<IPrismaticSuperstructureMemberSegment> prisSegment(segment);
      ATLASSERT(prisSegment);

      CComQIPtr<IShape> shape(gdrSection);
      ATLASSERT(shape);

      prisSegment->AddShape(shape,material,nullptr);
   }
   else
   {
      CComQIPtr<IFlangedGirderEndBlockSegment> ebSegment(segment);

      // define end blocks at both ends
      ebSegment->put_EndBlockLength(etStart,ebLength);
      ebSegment->put_EndBlockTransitionLength(etStart,ebTransition);
      ebSegment->put_EndBlockWidth(etStart,ebWidth);

      ebSegment->put_EndBlockLength(etEnd,ebLength);
      ebSegment->put_EndBlockTransitionLength(etEnd,ebTransition);
      ebSegment->put_EndBlockWidth(etEnd,ebWidth);

      CComQIPtr<IShape> shape(gdrSection);
      ATLASSERT(shape);

      ebSegment->AddShape(shape,material,nullptr);
   }

   segment.CopyTo(ppSegment);
}

void IBeamFactory::CreateSegmentShape(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   
   CComPtr<IPrecastBeam2> beam;
   beam.CoCreateInstance(CLSID_PrecastBeam2);

   DimensionAndPositionBeam(dimensions, beam);

   // Adjust width of section for end blocks
   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 Ls = pBridge->GetSegmentLength(pSegment->GetSegmentKey());

   Float64 w = GetDimension(dimensions, _T("EndBlockWidth"));
   Float64 l = GetDimension(dimensions, _T("EndBlockLength"));
   Float64 lt = GetDimension(dimensions, _T("EndBlockTransition"));

   std::array<Float64, 2> ebWidth{ w,w };
   std::array<Float64, 2> ebLength{ l,l };
   std::array<Float64, 2> ebTransition{ lt,lt };

   Float64 Wb, Wt;
   ::GetEndBlockWidth(Xs, Ls, (SectionBias)sectionBias, beam, ebWidth, ebLength, ebTransition, &Wt, &Wb);
   ::AdjustForEndBlocks(beam, Wt, Wb);

   beam.QueryInterface(ppShape);
}

Float64 IBeamFactory::GetSegmentHeight(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   return GetDimension(dimensions, _T("H"));
}

void IBeamFactory::ConfigureSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void IBeamFactory::LayoutSectionChangePointsOfInterest(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGirderEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();

#if defined _DEBUG
   std::_tstring strGirderName = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderName();
   ATLASSERT( strGirderName == pGirderEntry->GetName() );
#endif

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   Float64 ebLength, ebTransition;
   ebLength     = GetDimension(pGirderEntry->GetDimensions(),_T("EndBlockLength"));
   ebTransition = GetDimension(pGirderEntry->GetDimensions(),_T("EndBlockTransition"));

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);
   VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);

   // end block transition points
   if (0 < (ebLength + ebTransition))
   {
      if (IsZero(ebTransition))
      {
         // there is an abrupt section change
         pgsPointOfInterest poiStartEndBlock1(segmentKey, ebLength, POI_SECTCHANGE_LEFTFACE);
         VERIFY(pPoiMgr->AddPointOfInterest(poiStartEndBlock1) != INVALID_ID);

         pgsPointOfInterest poiStartEndBlock2(segmentKey, ebLength, POI_SECTCHANGE_RIGHTFACE);
         VERIFY(pPoiMgr->AddPointOfInterest(poiStartEndBlock2) != INVALID_ID);

         pgsPointOfInterest poiEndEndBlock1(segmentKey, gdrLength - ebLength, POI_SECTCHANGE_LEFTFACE);
         VERIFY(pPoiMgr->AddPointOfInterest(poiEndEndBlock1) != INVALID_ID);

         pgsPointOfInterest poiEndEndBlock2(segmentKey, gdrLength - ebLength, POI_SECTCHANGE_RIGHTFACE);
         VERIFY(pPoiMgr->AddPointOfInterest(poiEndEndBlock2) != INVALID_ID);
      }
      else
      {
         // there is a smooth taper over the transition length
         pgsPointOfInterest poiStartEndBlock1(segmentKey, ebLength, POI_SECTCHANGE_TRANSITION);
         pgsPointOfInterest poiStartEndBlock2(segmentKey, ebLength + ebTransition, POI_SECTCHANGE_TRANSITION);
         pgsPointOfInterest poiEndEndBlock2(segmentKey, gdrLength - ebLength - ebTransition, POI_SECTCHANGE_TRANSITION);
         pgsPointOfInterest poiEndEndBlock1(segmentKey, gdrLength - ebLength, POI_SECTCHANGE_TRANSITION);

         VERIFY(pPoiMgr->AddPointOfInterest(poiStartEndBlock1) != INVALID_ID);
         VERIFY(pPoiMgr->AddPointOfInterest(poiStartEndBlock2) != INVALID_ID);
         VERIFY(pPoiMgr->AddPointOfInterest(poiEndEndBlock2) != INVALID_ID);
         VERIFY(pPoiMgr->AddPointOfInterest(poiEndEndBlock1) != INVALID_ID);
      }
   }
}

std::unique_ptr<DistFactorEngineer> IBeamFactory::CreateDistFactorEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect) const
{
   return std::make_unique<IBeamDistFactorEngineer>(pBroker, statusGroupID);
}

std::unique_ptr<PsLossEngineerBase> IBeamFactory::CreatePsLossEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      return std::make_unique<TimeStepLossEngineer>(pBroker,statusGroupID);
   }
   else
   {
      return std::make_unique<PsBeamLossEngineer>(PsBeamLossEngineer::BeamType::IBeam,pBroker,statusGroupID);
   }
}

void IBeamFactory::CreateStrandMover(const BeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  BeamFactory::BeamFace endTopFace, Float64 endTopLimit, BeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  BeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, BeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   // set the shape for harped strand bounds - only in the thinnest part of the web
   CComPtr<IRectangle> harp_rect;
   hr = harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,h;
   Float64 w1,w2,w3,w4,w5;
   Float64 t1,t2;
   Float64 ebWidth,ebLength,ebTransition;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,h,w1,w2,w3,w4,w5,t1,t2,c1,ebWidth,ebLength,ebTransition);

   Float64 width = Min(t1,t2);
   Float64 depth = (Hg < 0 ? h : Hg);

   harp_rect->put_Width(width);
   harp_rect->put_Height(depth);

   // hook point is in the middle of the rectangle
   // we need to position the rectangle such that (0,0) is at the top center
   // move the hook point to (0,-depth/2)
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
   Float64 hptb  = hpTopFace     == BeamFactory::BeamFace::Bottom ? hpTopLimit     - depth : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == BeamFactory::BeamFace::Bottom ? hpBottomLimit  - depth : -hpBottomLimit;
   Float64 endtb = endTopFace    == BeamFactory::BeamFace::Bottom ? endTopLimit    - depth : -endTopLimit;
   Float64 endbb = endBottomFace == BeamFactory::BeamFace::Bottom ? endBottomLimit - depth : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(0, depth, endtb, endbb, hptb, hpbb, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);

   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}


const std::vector<std::_tstring>& IBeamFactory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& IBeamFactory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const WBFL::Units::Length*>& IBeamFactory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool IBeamFactory::ValidateDimensions(const BeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const
{
   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,h;
   Float64 w1,w2,w3,w4,w5;
   Float64 t1,t2;
   Float64 ebWidth,ebLength,ebTransition;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,h,w1,w2,w3,w4,w5,t1,t2,c1,ebWidth,ebLength,ebTransition);

   if ( d1 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D1 must be a positive value") << std::ends;
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

   if ( d4 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D4 must be a positive value") << std::ends;
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

   if ( h <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][H];
      std::_tostringstream os;
      os << _T("H must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if (h < (d1+d2+d3+d4+d5+d6))
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][H];
      std::_tostringstream os;
      os << _T("H must be greater than sum of the flange depth dimensions D1 through D6 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("W1 must be a positive value") << std::ends;
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

   if ( w3 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("W3 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if (w4 < 0.0)
   {
      std::_tostringstream os;
      os << _T("W4 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if (w5 < 0.0)
   {
      std::_tostringstream os;
      os << _T("W5 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( t1 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][T1];
      std::_tostringstream os;
      os << _T("T1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( t2 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][T2];
      std::_tostringstream os;
      os << _T("T2 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( c1 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("C1 must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d6 < c1 )
   {
      std::_tostringstream os;
      os << _T("C1 must be less than D6") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   return true;
}

void IBeamFactory::SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const BeamFactory::Dimensions& dimensions) const
{
   pSave->BeginUnit(_T("IBeamDimensions"),3.0); // bumped version number for 2.0 to 3.0 when changing to PrecastBeam2 object
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

BeamFactory::Dimensions IBeamFactory::LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const
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
      if (pLoad->BeginUnit(_T("IBeamDimensions")))
      {
         dimVersion = pLoad->GetVersion();
      }
      else
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   if (2 < dimVersion)
   { 
      // reading version 3 dimensions
      for (const auto& name : m_DimNames)
      {
         Float64 value;
         if (!pLoad->Property(name.c_str(), &value))
         {
            THROW_LOAD(InvalidFileFormat, pLoad);
         }
         dimensions.emplace_back(name, value);
      }
   }
   else
   {
      // read the old dimension names
      std::vector<std::_tstring> dimNames;
      dimNames.emplace_back(_T("C1"));
      dimNames.emplace_back(_T("D1"));
      dimNames.emplace_back(_T("D2"));
      dimNames.emplace_back(_T("D3"));
      dimNames.emplace_back(_T("D4"));
      dimNames.emplace_back(_T("D5"));
      dimNames.emplace_back(_T("D6"));
      dimNames.emplace_back(_T("D7"));
      dimNames.emplace_back(_T("T1"));
      dimNames.emplace_back(_T("T2"));
      dimNames.emplace_back(_T("W1"));
      dimNames.emplace_back(_T("W2"));
      dimNames.emplace_back(_T("W3"));
      dimNames.emplace_back(_T("W4"));
      dimNames.emplace_back(_T("EndBlockWidth"));
      dimNames.emplace_back(_T("EndBlockLength"));
      dimNames.emplace_back(_T("EndBlockTransition"));

      for (const auto& name : dimNames)
      {
         Float64 value;
         if (!pLoad->Property(name.c_str(), &value))
         {
            // failed to read dimension value...

            if (dimVersion < 2 && parent_version < 3.0 && name == _T("C1"))
            {
               value = 0.0; // set the default value
            }
            else if (dimVersion < 2 && parent_version < 14 && (name == _T("EndBlockWidth") || name == _T("EndBlockLength") || name == _T("EndBlockTransition")))
            {
               value = 0.0;
            }
            else
            {
               THROW_LOAD(InvalidFileFormat, pLoad);
            }
         }
         dimensions.emplace_back(name, value);
      }

      dimensions = ConvertIBeamDimensions(dimensions);
      ATLASSERT(m_DimNames.size() == dimensions.size());
   }

   if (14 <= parent_version && !pLoad->EndUnit())
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   return dimensions;
}

bool IBeamFactory::IsPrismatic(const BeamFactory::Dimensions& dimensions) const
{
   Float64 ebWidth, ebLength, ebTransition;
   ebWidth      = GetDimension(dimensions,_T("EndBlockWidth"));
   ebLength     = GetDimension(dimensions,_T("EndBlockLength"));
   ebTransition = GetDimension(dimensions,_T("EndBlockTransition"));

   bool bPrismatic = true;
   if ( IsZero(ebWidth) || (IsZero(ebLength) && IsZero(ebTransition)) )
   {
      // prismatic
      bPrismatic = true;
   }
   else
   {
      // non-prismatic
      bPrismatic = false;
   }

   return bPrismatic;
}

bool IBeamFactory::IsPrismatic(const CSegmentKey& segmentKey) const
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

bool IBeamFactory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

std::_tstring IBeamFactory::GetImage() const
{
   return std::_tstring(_T("IBeam.png"));
}

std::_tstring IBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      strImage =  _T("IBeam_Slab_CIP.gif");
      break;

   case pgsTypes::sdtCompositeSIP:
      strImage = _T("IBeam_Slab_SIP.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring IBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("+Mn_IBeam_Composite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring IBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("-Mn_IBeam_Composite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring IBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("Vn_IBeam.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring IBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
   {
      return _T("IBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("IBeam_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring IBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
   {
      return _T("IBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("IBeam_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID IBeamFactory::GetCLSID() const
{
   return CLSID_WFBeamFactory;
}

CLSID IBeamFactory::GetFamilyCLSID() const
{
   return CLSID_WFBeamFamily;
}

std::_tstring IBeamFactory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring IBeamFactory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE IBeamFactory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR IBeamFactory::GetImageResourceName() const
{
   return _T("IBEAM");
}

HICON  IBeamFactory::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_IBEAM) );
}

void IBeamFactory::GetDimensions(const BeamFactory::Dimensions& dimensions,
                                  Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,Float64& d6,Float64& h,
                                  Float64& w1,Float64& w2,Float64& w3,Float64& w4,Float64& w5,
                                  Float64& t1,Float64& t2, Float64& c1,
                                  Float64& ebWidth,Float64& ebLength,Float64& ebTransition) const
{
   d1 = dimensions[D1].second;
   d2 = dimensions[D2].second;
   d3 = dimensions[D3].second;
   d4 = dimensions[D4].second;
   d5 = dimensions[D5].second;
   d6 = dimensions[D6].second;
   h = dimensions[H].second;
   w1 = dimensions[W1].second;
   w2 = dimensions[W2].second;
   w3 = dimensions[W3].second;
   w4 = dimensions[W4].second;
   w5 = dimensions[W5].second;
   t1 = dimensions[T1].second;
   t2 = dimensions[T2].second;
   c1 = dimensions[C1].second;

   ebWidth = dimensions[EBW].second;
   ebLength = dimensions[EBL].second;
   ebTransition = dimensions[EBT].second;

   ATLASSERT(IsEqual(d1, GetDimension(dimensions, _T("D1"))));
   ATLASSERT(IsEqual(d2, GetDimension(dimensions, _T("D2"))));
   ATLASSERT(IsEqual(d3, GetDimension(dimensions, _T("D3"))));
   ATLASSERT(IsEqual(d4, GetDimension(dimensions, _T("D4"))));
   ATLASSERT(IsEqual(d5, GetDimension(dimensions, _T("D5"))));
   ATLASSERT(IsEqual(d6, GetDimension(dimensions, _T("D6"))));
   ATLASSERT(IsEqual(h, GetDimension(dimensions, _T("H"))));
   ATLASSERT(IsEqual(w1, GetDimension(dimensions, _T("W1"))));
   ATLASSERT(IsEqual(w2, GetDimension(dimensions, _T("W2"))));
   ATLASSERT(IsEqual(w3, GetDimension(dimensions, _T("W3"))));
   ATLASSERT(IsEqual(w4, GetDimension(dimensions, _T("W4"))));
   ATLASSERT(IsEqual(w5, GetDimension(dimensions, _T("W5"))));
   ATLASSERT(IsEqual(t1, GetDimension(dimensions, _T("T1"))));
   ATLASSERT(IsEqual(t2, GetDimension(dimensions, _T("T2"))));
   ATLASSERT(IsEqual(c1, GetDimension(dimensions, _T("C1"))));
   ATLASSERT(IsEqual(ebWidth, GetDimension(dimensions, _T("EndBlockWidth"))));
   ATLASSERT(IsEqual(ebLength, GetDimension(dimensions, _T("EndBlockLength"))));
   ATLASSERT(IsEqual(ebTransition, GetDimension(dimensions, _T("EndBlockTransition"))));
}

Float64 IBeamFactory::GetDimension(const BeamFactory::Dimensions& dimensions,const std::_tstring& name) const
{
   for (const auto& dim : dimensions)
   {
      if (name == dim.first)
      {
         return dim.second;
      }
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedDeckTypes IBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
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

pgsTypes::SupportedBeamSpacings IBeamFactory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

bool IBeamFactory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool IBeamFactory::ConvertBeamSpacing(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   // nothing to convert
   return false;
}

pgsTypes::WorkPointLocations IBeamFactory::GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::WorkPointLocations wpls;
   wpls.push_back(pgsTypes::wplTopGirder);

   if (IsSpreadSpacing(spacingType))
   {
      wpls.push_back(pgsTypes::wplBottomGirder);
   }

   return wpls;
}

bool IBeamFactory::IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation wpType) const
{
   pgsTypes::WorkPointLocations sbs = GetSupportedWorkPointLocations(spacingType);
   auto found = std::find(sbs.cbegin(), sbs.cend(),wpType);
   return found == sbs.end() ? false : true;
}

std::vector<pgsTypes::GirderOrientationType> IBeamFactory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb/*, pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal*/ };
   return types;
}

bool IBeamFactory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation == pgsTypes::Plumb ? true : false;
}

pgsTypes::GirderOrientationType IBeamFactory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return pgsTypes::Plumb;
}

pgsTypes::SupportedDiaphragmTypes IBeamFactory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes IBeamFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

void IBeamFactory::GetAllowableSpacingRange(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 t1 = GetDimension(dimensions,_T("T1"));
   Float64 t2 = GetDimension(dimensions,_T("T2"));
   Float64 w1 = GetDimension(dimensions,_T("W1"));
   Float64 w2 = GetDimension(dimensions,_T("W2"));
   Float64 w3 = GetDimension(dimensions,_T("W3"));
   Float64 w4 = GetDimension(dimensions, _T("W4"));
   Float64 w5 = GetDimension(dimensions, _T("W5"));

   Float64 top_w = t1 + 2.0*(w1+w2+w3);
   Float64 bot_w = t2 + 2.0*(w4+w5);

   Float64 gw = Max(top_w, bot_w);


   if ( sdt == pgsTypes::sdtCompositeCIP || sdt == pgsTypes::sdtCompositeSIP )
   {
      if(sbs == pgsTypes::sbsUniform || sbs == pgsTypes::sbsGeneral)
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

WebIndexType IBeamFactory::GetWebCount(const BeamFactory::Dimensions& dimensions) const
{
   return 1;
}

Float64 IBeamFactory::GetBeamHeight(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("H"));
}

Float64 IBeamFactory::GetBeamWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 w1 = GetDimension(dimensions, _T("W1"));
   Float64 w2 = GetDimension(dimensions, _T("W2"));
   Float64 w3 = GetDimension(dimensions, _T("W3"));
   Float64 w4 = GetDimension(dimensions, _T("W4"));
   Float64 w5 = GetDimension(dimensions, _T("W5"));
   Float64 t1 = GetDimension(dimensions, _T("T1"));
   Float64 t2 = GetDimension(dimensions, _T("T2"));

   Float64 top = 2*(w1+w2+w3) + t1;
   Float64 bot = 2*(w4+w5) + t2;

   return Max(top,bot);
}

void IBeamFactory::GetBeamTopWidth(const BeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const
{
   Float64 w1 = GetDimension(dimensions, _T("W1"));
   Float64 w2 = GetDimension(dimensions, _T("W2"));
   Float64 w3 = GetDimension(dimensions, _T("W3"));
   Float64 t1 = GetDimension(dimensions, _T("T1"));

   Float64 top = 2*(w1+w2+w3) + t1;

   top /= 2.0;

   *pLeftWidth = top;
   *pRightWidth = top;
}

bool IBeamFactory::IsShearKey(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void IBeamFactory::GetShearKeyAreas(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool IBeamFactory::HasLongitudinalJoints() const
{
   return false;
}

bool IBeamFactory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool IBeamFactory::HasTopFlangeThickening() const
{
   return false;
}

bool IBeamFactory::CanPrecamber() const
{
   return true;
}

GirderIndexType IBeamFactory::GetMinimumBeamCount() const
{
   return 2;
}

void IBeamFactory::DimensionAndPositionBeam(const BeamFactory::Dimensions& dimensions, IPrecastBeam2* pBeam) const
{
   Float64 c1;
   Float64 d1, d2, d3, d4, d5, d6, h;
   Float64 w1, w2, w3, w4, w5;
   Float64 t1, t2;
   Float64 ebWidth, ebLength, ebTransition;
   GetDimensions(dimensions, d1, d2, d3, d4, d5, d6, h, w1, w2, w3, w4, w5, t1, t2, c1, ebWidth, ebLength, ebTransition);
   pBeam->put_W1(w1);
   pBeam->put_W2(w2);
   pBeam->put_W3(w3);
   pBeam->put_W4(w4);
   pBeam->put_W5(w5);
   pBeam->put_D1(d1);
   pBeam->put_D2(d2);
   pBeam->put_D3(d3);
   pBeam->put_D4(d4);
   pBeam->put_D5(d5);
   pBeam->put_D6(d6);
   pBeam->put_H(h);
   pBeam->put_T1(t1);
   pBeam->put_T2(t2);
   pBeam->put_C1(c1);

   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -h);
}
