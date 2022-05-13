///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

// SplicedIBeamFactory.cpp : Implementation of CSplicedIBeamFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "SplicedIBeamFactoryImpl.h"
#include "IBeamDistFactorEngineer.h"
#include "TimeStepLossEngineer.h"
#include "StrandMoverImpl.h"
#include <GeomModel\PrecastBeam.h>
#include <GenericBridge\Helpers.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>

#include <IFace\AgeAdjustedMaterial.h>
#include <Beams\Helper.h>

#include <PgsExt\StatusItem.h>

#include <PgsExt\BridgeDescription2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSplicedIBeamFactory
HRESULT CSplicedIBeamFactory::FinalConstruct()
{
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

   // Default beam is a W74G
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.000,WBFL::Units::Measure::Inch)); // C1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(2.875,WBFL::Units::Measure::Inch)); // D1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(2.625,WBFL::Units::Measure::Inch)); // D2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(2.000,WBFL::Units::Measure::Inch)); // D3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(6.000,WBFL::Units::Measure::Inch)); // D4
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(3.000,WBFL::Units::Measure::Inch)); // D5
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.000,WBFL::Units::Measure::Inch)); // D6
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(57.00,WBFL::Units::Measure::Inch)); // D7
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(6.000,WBFL::Units::Measure::Inch)); // T1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(6.000,WBFL::Units::Measure::Inch)); // T2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(16.50,WBFL::Units::Measure::Inch)); // W1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(2.000,WBFL::Units::Measure::Inch)); // W2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(9.500,WBFL::Units::Measure::Inch)); // W3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(0.000,WBFL::Units::Measure::Inch)); // W4

   // SI Units
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // C1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D3
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D4
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D5
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D6
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D7
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // T1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // T2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W3
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W4

   // US Units
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // C1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D3
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D4
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D5
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D6
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D7
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // T1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // T2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W3
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W4

   return S_OK;
}

void CSplicedIBeamFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   CComPtr<IFlangedGirderSection> gdrSection;
   gdrSection.CoCreateInstance(CLSID_FlangedGirderSection);

   CComPtr<IPrecastBeam> beam;
   gdrSection->get_Beam(&beam);

   DimensionAndPositionBeam(dimensions, overallHeight, bottomFlangeHeight, beam);

   gdrSection.QueryInterface(ppSection);
}

void CSplicedIBeamFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
{
   CComPtr<ISplicedGirderSegment> segment;
   segment.CoCreateInstance(CLSID_FlangedSplicedGirderSegment);

   ATLASSERT(segment != nullptr);

   // Build up the beam shape
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment    = pGirder->GetSegment(segmentKey.segmentIndex);

   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   CComPtr<IGirderSection> gdrSection;
   CreateGirderSection(pBroker,statusGroupID,dimensions,-1,-1,&gdrSection);

   // define end blocks at both ends
   segment->put_EndBlockLength(          etStart,pSegment->EndBlockLength[pgsTypes::metStart]);
   segment->put_EndBlockTransitionLength(etStart,pSegment->EndBlockTransitionLength[pgsTypes::metStart]);
   segment->put_EndBlockWidth(           etStart,pSegment->EndBlockWidth[pgsTypes::metStart]);

   segment->put_EndBlockLength(          etEnd,pSegment->EndBlockLength[pgsTypes::metEnd]);
   segment->put_EndBlockTransitionLength(etEnd,pSegment->EndBlockTransitionLength[pgsTypes::metEnd]);
   segment->put_EndBlockWidth(           etEnd,pSegment->EndBlockWidth[pgsTypes::metEnd]);


   // set the segment parameters
   pgsTypes::SegmentVariationType variationType = pSegment->GetVariationType();
   segment->put_VariationType((SegmentVariationType)variationType);

   for ( int i = 0; i < 4; i++ )
   {
      Float64 length, height, bottomFlangeDepth;
      pSegment->GetVariationParameters((pgsTypes::SegmentZoneType)i,false,&length,&height,&bottomFlangeDepth);
      segment->SetVariationParameters((SegmentZoneType)i,length,height,bottomFlangeDepth);
   }

   CComPtr<IAgeAdjustedMaterial> material;
   BuildAgeAdjustedGirderMaterialModel(pBroker,pSegment,segment,&material);
   CComQIPtr<IShape> shape(gdrSection);
   ATLASSERT(shape);
   segment->AddShape(shape,material,nullptr);

   CComQIPtr<ISuperstructureMemberSegment> ssmbrSegment(segment);
   ssmbrSegment.CopyTo(ppSegment);
}

void CSplicedIBeamFactory::CreateSegmentShape(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   const CSplicedGirderData*  pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   CComPtr<IPrecastBeam> beam;
   beam.CoCreateInstance(CLSID_PrecastBeam);

   Float64 Hg = GetSegmentHeight(pBroker, pSegment, Xs);
   Float64 Hbf = GetBottomFlangeDepth(pBroker, pSegment, Xs);

   DimensionAndPositionBeam(dimensions, Hg, Hbf, beam);

   // Adjust width of section for end blocks
   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 Ls = pBridge->GetSegmentLength(pSegment->GetSegmentKey());

   Float64 Wb, Wt;
   ::GetEndBlockWidth(Xs, Ls, (SectionBias)sectionBias, beam, pSegment->EndBlockWidth,pSegment->EndBlockLength,pSegment->EndBlockTransitionLength,&Wt,&Wb);
   ::AdjustForEndBlocks(beam, Wt, Wb);

   beam.QueryInterface(ppShape);
}

Float64 CSplicedIBeamFactory::GetSegmentHeight(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 Ls = pBridge->GetSegmentLength(pSegment->GetSegmentKey());

   std::array<Float64, 4> X;
   pBridge->ResolveSegmentVariation(pSegment, X);
   std::array<Float64, 4> Y;
   Y[pgsTypes::sztLeftPrismatic] = pSegment->GetVariationHeight(pgsTypes::sztLeftPrismatic);
   Y[pgsTypes::sztLeftTapered] = pSegment->GetVariationHeight(pgsTypes::sztLeftTapered);
   Y[pgsTypes::sztRightTapered] = pSegment->GetVariationHeight(pgsTypes::sztRightTapered);
   Y[pgsTypes::sztRightPrismatic] = pSegment->GetVariationHeight(pgsTypes::sztRightPrismatic);

   pgsTypes::SegmentVariationType variationType = pSegment->GetVariationType();
   bool bParabolas = IsParabolicVariation(variationType);

   Float64 Xl, Yl, Xr, Yr;
   ZoneType zone = ::GetControlPoints(Xs, Ls, X, Y, &Xl, &Yl, &Xr, &Yr);
   return ::GetSectionDepth(Xs, Xl, Yl, Xr, Yr, TransitionTypeFromZone(zone, Yl, Yr, bParabolas));
}

Float64 CSplicedIBeamFactory::GetBottomFlangeDepth(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 Ls = pBridge->GetSegmentLength(pSegment->GetSegmentKey());

   std::array<Float64, 4> X;
   pBridge->ResolveSegmentVariation(pSegment, X);
   std::array<Float64, 4> Y;
   Y[pgsTypes::sztLeftPrismatic] = pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztLeftPrismatic);
   Y[pgsTypes::sztLeftTapered] = pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztLeftTapered);
   Y[pgsTypes::sztRightTapered] = pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztRightTapered);
   Y[pgsTypes::sztRightPrismatic] = pSegment->GetVariationBottomFlangeDepth(pgsTypes::sztRightPrismatic);

   pgsTypes::SegmentVariationType variationType = pSegment->GetVariationType();
   bool bParabolas = IsParabolicVariation(variationType);

   Float64 Xl, Yl, Xr, Yr;
   ZoneType zone = ::GetControlPoints(Xs, Ls, X, Y, &Xl, &Yl, &Xr, &Yr);
   return ::GetSectionDepth(Xs, Xl, Yl, Xr, Yr, TransitionTypeFromZone(zone, Yl, Yr, bParabolas));
}

void CSplicedIBeamFactory::ConfigureSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void CSplicedIBeamFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc  = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup       = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder      = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment     = pGirder->GetSegment(segmentKey.segmentIndex);
   const GirderLibraryEntry*  pGirderEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   const CTimelineManager*    pTimelineMgr = pBridgeDesc->GetTimelineManager();

   SegmentIDType segID = pSegment->GetID();


#if defined _DEBUG
   std::_tstring strGirderName = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderName();
   ATLASSERT( strGirderName == pGirderEntry->GetName() );
#endif

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);
   Float64 start_brg_offset = pBridge->GetSegmentStartBearingOffset(segmentKey);
   Float64 end_brg_offset   = pBridge->GetSegmentEndBearingOffset(segmentKey);
   Float64 start_end_dist   = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 end_end_dist     = pBridge->GetSegmentEndEndDistance(segmentKey);

   Float64 start_offset = start_brg_offset - start_end_dist; // dist from CL Pier/TS to face of beam
   Float64 end_offset   = end_brg_offset - end_end_dist;

   //
   // POI at start and end of segment
   //

   // define these POI from time of construction to just before erection
   pgsPointOfInterest poiStart(segmentKey,0.00,POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,segment_length,POI_SECTCHANGE_LEFTFACE );

   VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);
   VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);


   //
   // end block transition points
   //
   LayoutIBeamEndBlockPointsOfInterest(segmentKey, pSegment, segment_length, pPoiMgr);

   // POI for transition points
   pgsTypes::SegmentVariationType variationType = pSegment->GetVariationType();
   if ( variationType == pgsTypes::svtNone )
   {
      // do nothing.... no extra POI required
   }
   else if ( variationType == pgsTypes::svtLinear || variationType == pgsTypes::svtParabolic )
   {
      Float64 xLeft  = pSegment->GetVariationLength(pgsTypes::sztLeftPrismatic);
      Float64 xRight = pSegment->GetVariationLength(pgsTypes::sztRightPrismatic);

      if (!IsZero(xLeft))
      {
         xLeft -= start_offset;
         xLeft = IsLE(xLeft,0.0) ? 0.0 : xLeft;
      }

      if (!IsZero(xRight))
      {
         xRight -= end_offset;
         xRight = IsLE(xRight,0.0) ? 0.0 : xRight;
      }

      pgsPointOfInterest poiStart( segmentKey, xLeft, POI_SECTCHANGE_TRANSITION);
      VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);

      pgsPointOfInterest poiEnd( segmentKey, segment_length-xRight, POI_SECTCHANGE_TRANSITION);
      VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);
   }
   else if ( variationType == pgsTypes::svtDoubleLinear || variationType == pgsTypes::svtDoubleParabolic )
   {
      Float64 xLeft  = pSegment->GetVariationLength(pgsTypes::sztLeftPrismatic);  // measured from CL Pier/TS
      Float64 xRight = pSegment->GetVariationLength(pgsTypes::sztRightPrismatic); // measured from CL Pier/TS

      if (!IsZero(xLeft))
      {
         xLeft -= start_offset;
         xLeft = IsLE(xLeft,0.0) ? 0.0 : xLeft;
      }

      if (!IsZero(xRight))
      {
         xRight -= end_offset;
         xRight = IsLE(xRight,0.0) ? 0.0 : xRight;
      }

      pgsPointOfInterest poiStart( segmentKey, xLeft, POI_SECTCHANGE_TRANSITION);
      VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);

      xLeft += pSegment->GetVariationLength(pgsTypes::sztLeftTapered);
      poiStart.SetDistFromStart(xLeft); // causes the attributes to be reset
      poiStart.SetNonReferencedAttributes(POI_SECTCHANGE_TRANSITION);
      VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);

      pgsPointOfInterest poiEnd( segmentKey, segment_length-xRight, POI_SECTCHANGE_TRANSITION );
      VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);

      xRight += pSegment->GetVariationLength(pgsTypes::sztRightTapered);
      poiEnd.SetDistFromStart(segment_length-xRight); // causes the attributes to be reset
      poiEnd.SetNonReferencedAttributes(POI_SECTCHANGE_TRANSITION);
      VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);
   }
   else
   {
      ATLASSERT(false); // General not supported
   }
}

void CSplicedIBeamFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng) const
{
   CComObject<CIBeamDistFactorEngineer>* pEngineer;
   CComObject<CIBeamDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CSplicedIBeamFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng) const
{
   CComObject<CTimeStepLossEngineer>* pEngineer;
   CComObject<CTimeStepLossEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CSplicedIBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions,  Float64 Hg,
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
   Float64 w1,w2,w3,w4;
   Float64 t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7,w1,w2,w3,w4,t1,t2,c1);

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

const std::vector<std::_tstring>& CSplicedIBeamFactory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& CSplicedIBeamFactory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const WBFL::Units::Length*>& CSplicedIBeamFactory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CSplicedIBeamFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const
{
   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7;
   Float64 w1,w2,w3,w4;
   Float64 t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7,w1,w2,w3,w4,t1,t2,c1);

// C1  0
// D1  1
// D2  2
// D3  3
// D4  4
// D5  5
// D6  6
// D7  7
// T1  8
// T2  9
// W1  10
// W2  11
// W3  12
// W4  13

   if ( d1 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][1];
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
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][4];
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
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][7];
      std::_tostringstream os;
      os << _T("D7 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][10];
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
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][12];
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
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
      std::_tostringstream os;
      os << _T("T1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( t2 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
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

   if ( c1 > d4 )
   {
      std::_tostringstream os;
      os << _T("C1 must be greater than D4") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   return true;
}

void CSplicedIBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) const
{
   pSave->BeginUnit(_T("IBeamDimensions"),2.0);
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CSplicedIBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad) const
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

   for(const auto& name : m_DimNames)
   {
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
      {
         // failed to read dimension value...
         
         if ( dimVersion < 2 && parent_version < 3.0 && name == _T("C1") )
         {
            value = 0.0; // set the default value
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

bool CSplicedIBeamFactory::IsPrismatic(const IBeamFactory::Dimensions& dimensions) const
{
   return false;
}

bool CSplicedIBeamFactory::IsPrismatic(const CSegmentKey& segmentKey) const
{
   return false;
}

bool CSplicedIBeamFactory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return false;
}

std::_tstring CSplicedIBeamFactory::GetImage() const
{
   return std::_tstring(_T("SplicedIBeam.jpg"));
}

std::_tstring CSplicedIBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      strImage =  _T("IBeam_Slab_CIP.gif");
      break;

   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("IBeam_Slab_SIP.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CSplicedIBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CSplicedIBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CSplicedIBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CSplicedIBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("IBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("IBeam_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring CSplicedIBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("IBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("IBeam_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CSplicedIBeamFactory::GetCLSID() const
{
   return CLSID_SplicedIBeamFactory;
}

std::_tstring CSplicedIBeamFactory::GetName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CSplicedIBeamFactory::GetFamilyCLSID() const
{
   return CLSID_SplicedIBeamFamily;
}

std::_tstring CSplicedIBeamFactory::GetGirderFamilyName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CSplicedIBeamFactory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CSplicedIBeamFactory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CSplicedIBeamFactory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR CSplicedIBeamFactory::GetImageResourceName() const
{
   return _T("SPLICEDIBEAM");
}

HICON  CSplicedIBeamFactory::GetIcon()  const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_SPLICED_IBEAM) );
}

void CSplicedIBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                  Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,Float64& d6,Float64& d7,
                                  Float64& w1,Float64& w2,Float64& w3,Float64& w4,
                                  Float64& t1,Float64& t2, Float64& c1) const
{
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
   t1 = GetDimension(dimensions,_T("T1"));
   t2 = GetDimension(dimensions,_T("T2"));
   c1 = GetDimension(dimensions,_T("C1")); 
}

Float64 CSplicedIBeamFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name) const
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

pgsTypes::SupportedDeckTypes CSplicedIBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
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

pgsTypes::SupportedBeamSpacings CSplicedIBeamFactory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

bool CSplicedIBeamFactory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool CSplicedIBeamFactory::ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   return false;
}

pgsTypes::WorkPointLocations CSplicedIBeamFactory::GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::WorkPointLocations wpls;
   wpls.push_back(pgsTypes::wplTopGirder);
//   wpls.push_back(pgsTypes::wplBottomGirder);

   return wpls;
}

bool CSplicedIBeamFactory::IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation wpType) const
{
   pgsTypes::WorkPointLocations sbs = GetSupportedWorkPointLocations(spacingType);
   auto found = std::find(sbs.cbegin(), sbs.cend(), wpType);
   return found == sbs.end() ? false : true;
}


std::vector<pgsTypes::GirderOrientationType> CSplicedIBeamFactory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb/*, pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal*/ };
   return types;
}

bool CSplicedIBeamFactory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation == pgsTypes::Plumb ? true : false;
}

pgsTypes::GirderOrientationType CSplicedIBeamFactory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return pgsTypes::Plumb;
}

pgsTypes::SupportedDiaphragmTypes CSplicedIBeamFactory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CSplicedIBeamFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

void CSplicedIBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 T1 = GetDimension(dimensions,_T("T1"));
   Float64 T2 = GetDimension(dimensions,_T("T2"));
   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 W3 = GetDimension(dimensions,_T("W3"));
   Float64 W4 = GetDimension(dimensions,_T("W4"));

   Float64 top_w = T1 + 2.0*(W1+W2);
   Float64 bot_w = T2 + 2.0*(W3+W4);

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

WebIndexType CSplicedIBeamFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions) const
{
   return 1;
}

Float64 CSplicedIBeamFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
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

Float64 CSplicedIBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 W3 = GetDimension(dimensions,_T("W3"));
   Float64 W4 = GetDimension(dimensions,_T("W4"));
   Float64 T1 = GetDimension(dimensions,_T("T1"));
   Float64 T2 = GetDimension(dimensions,_T("T2"));

   Float64 top = 2*(W1+W2) + T1;
   Float64 bot = 2*(W3+W4) + T2;

   return Max(top,bot);
}

void CSplicedIBeamFactory::GetBeamTopWidth(const IBeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const
{
   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 T1 = GetDimension(dimensions,_T("T1"));

   Float64 top = 2*(W1+W2) + T1;
   top /= 2.0;

   *pLeftWidth = top;
   *pRightWidth = top;
}

bool CSplicedIBeamFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void CSplicedIBeamFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool CSplicedIBeamFactory::HasLongitudinalJoints() const
{
   return false;
}

bool CSplicedIBeamFactory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool CSplicedIBeamFactory::HasTopFlangeThickening() const
{
   return false;
}

bool CSplicedIBeamFactory::CanPrecamber() const
{
   return false;
}

GirderIndexType CSplicedIBeamFactory::GetMinimumBeamCount() const
{
   return 2;
}

// ISplicedBeamFactory
bool CSplicedIBeamFactory::SupportsVariableDepthSection() const
{
   return true; // IBeams can be fixed depth or variable depth
}

LPCTSTR CSplicedIBeamFactory::GetVariableDepthDimension() const
{
   return _T("D7");
}

std::vector<pgsTypes::SegmentVariationType> CSplicedIBeamFactory::GetSupportedSegmentVariations(bool bIsVariableDepthSection) const
{
   std::vector<pgsTypes::SegmentVariationType> variations;
   if ( bIsVariableDepthSection )
   {
      variations.push_back(pgsTypes::svtLinear);
      variations.push_back(pgsTypes::svtParabolic);
      variations.push_back(pgsTypes::svtDoubleLinear);
      variations.push_back(pgsTypes::svtDoubleParabolic);
   }
   else
   {
      variations.push_back(pgsTypes::svtNone);
   }
   return variations;
}

bool CSplicedIBeamFactory::CanBottomFlangeDepthVary() const
{
   return true;
}

LPCTSTR CSplicedIBeamFactory::GetBottomFlangeDepthDimension() const
{
   return _T("D4");
}

bool CSplicedIBeamFactory::SupportsEndBlocks() const
{
   return true;
}

void CSplicedIBeamFactory::DimensionAndPositionBeam(const IBeamFactory::Dimensions& dimensions, Float64 Hg, Float64 Hbf, IPrecastBeam* pBeam) const
{
   Float64 c1;
   Float64 d1, d2, d3, d4, d5, d6, d7;
   Float64 w1, w2, w3, w4;
   Float64 t1, t2;
   GetDimensions(dimensions, d1, d2, d3, d4, d5, d6, d7, w1, w2, w3, w4, t1, t2, c1);
   pBeam->put_W1(w1);
   pBeam->put_W2(w2);
   pBeam->put_W3(w3);
   pBeam->put_W4(w4);
   pBeam->put_D1(d1);
   pBeam->put_D2(d2);
   pBeam->put_D3(d3);
   pBeam->put_D4(d4);
   pBeam->put_D5(d5);
   pBeam->put_D6(d6);
   pBeam->put_D7(d7);
   pBeam->put_T1(t1);
   pBeam->put_T2(t2);
   pBeam->put_C1(c1);

   ::AdjustForVariableDepth(pBeam, Hg, Hbf);

   Float64 H;
   pBeam->get_Height(&H);

   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Move(0, -H);
}
