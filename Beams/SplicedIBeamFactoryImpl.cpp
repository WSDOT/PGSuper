///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>

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
   m_DimNames.push_back(_T("C1"));
   m_DimNames.push_back(_T("D1"));
   m_DimNames.push_back(_T("D2"));
   m_DimNames.push_back(_T("D3"));
   m_DimNames.push_back(_T("D4"));
   m_DimNames.push_back(_T("D5"));
   m_DimNames.push_back(_T("D6"));
   m_DimNames.push_back(_T("D7"));
   m_DimNames.push_back(_T("T1"));
   m_DimNames.push_back(_T("T2"));
   m_DimNames.push_back(_T("W1"));
   m_DimNames.push_back(_T("W2"));
   m_DimNames.push_back(_T("W3"));
   m_DimNames.push_back(_T("W4"));

   // Default beam is a W74G
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // C1
   m_DefaultDims.push_back(::ConvertToSysUnits(2.875,unitMeasure::Inch)); // D1
   m_DefaultDims.push_back(::ConvertToSysUnits(2.625,unitMeasure::Inch)); // D2
   m_DefaultDims.push_back(::ConvertToSysUnits(2.000,unitMeasure::Inch)); // D3
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // D4
   m_DefaultDims.push_back(::ConvertToSysUnits(3.000,unitMeasure::Inch)); // D5
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // D6
   m_DefaultDims.push_back(::ConvertToSysUnits(57.00,unitMeasure::Inch)); // D7
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // T1
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // T2
   m_DefaultDims.push_back(::ConvertToSysUnits(16.50,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits(2.000,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits(9.500,unitMeasure::Inch)); // W3
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // W4

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D5
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D6
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D7
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W4

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D5
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D6
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D7
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W4

   return S_OK;
}

void CSplicedIBeamFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection)
{
   CComPtr<IFlangedGirderSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_FlangedGirderSection);

   CComPtr<IPrecastBeam> beam;
   gdrsection->get_Beam(&beam);

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7;
   Float64 w1,w2,w3,w4;
   Float64 t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7,w1,w2,w3,w4,t1,t2,c1);
   beam->put_W1(w1);
   beam->put_W2(w2);
   beam->put_W3(w3);
   beam->put_W4(w4);
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

   if ( 0 < bottomFlangeHeight )
   {
      d4 = bottomFlangeHeight;
      beam->put_D4(d4);
   }

   if ( 0 < overallHeight )
   {
      d7 += overallHeight - (d1+d2+d3+d4+d5+d6+d7);
      if ( 0 < d7 )
         beam->put_D7(d7);
   }

   gdrsection.QueryInterface(ppSection);
}

void CSplicedIBeamFactory::CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
#pragma Reminder("OBSOLETE???") // this method may be obsolete... remove it if it is
   ATLASSERT(false); // this assert is to get my attention... is this method really needed?
   // The profile returned from this method is a rectangle... in generate the spliced IBeams
   // have a variable depth cross section so this method doesn't make any sense

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetSegmentLength(segmentKey);

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7;
   Float64 w1,w2,w3,w4;
   Float64 t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7,w1,w2,w3,w4,t1,t2,c1);

   Float64 height = d1 + d2 + d3 + d4 + d5 + d6 + d7;

   CComPtr<IRectangle> rect;
   rect.CoCreateInstance(CLSID_Rect);
   rect->put_Height(height);
   rect->put_Width(length);

   CComQIPtr<IXYPosition> position(rect);
   CComPtr<IPoint2d> topLeft;
   position->get_LocatorPoint(lpTopLeft,&topLeft);
   topLeft->Move(0,0);
   position->put_LocatorPoint(lpTopLeft,topLeft);

   rect->QueryInterface(ppShape);
}

void CSplicedIBeamFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,IStages* pStages,ISuperstructureMember* ssmbr)
{
   CComPtr<ISplicedGirderSegment> segment;
   segment.CoCreateInstance(CLSID_FlangedSplicedGirderSegment);

   ATLASSERT(segment != NULL);

   // Build up the beam shape
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData*    pGroup      = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment    = pGirder->GetSegment(segmentKey.segmentIndex);

   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   bool bHasClosure[2] = {pSegment->GetLeftClosure() != NULL,pSegment->GetRightClosure() != NULL};

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

   // Beam materials
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IMaterials,pMaterial);
   CComPtr<IMaterial> segmentMaterial, closureMaterial[2];
   segmentMaterial.CoCreateInstance(CLSID_Material);

   if ( bHasClosure[etStart] )
   {
      closureMaterial[etStart].CoCreateInstance(CLSID_Material);
   }

   if ( bHasClosure[etEnd] )
   {
      closureMaterial[etEnd].CoCreateInstance(CLSID_Material);
   }

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount(segmentKey);
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 E = pMaterial->GetSegmentAgeAdjustedEc(segmentKey,intervalIdx);
      Float64 D = pMaterial->GetSegmentWeightDensity(segmentKey,intervalIdx);

      StageIndexType stageIdx = pStages->GetStage(segmentKey,intervalIdx);

      segmentMaterial->put_E(stageIdx,E);
      segmentMaterial->put_Density(stageIdx,D);

      if ( bHasClosure[etStart] )
      {
         CClosureKey closureKey(segmentKey.groupIndex,segmentKey.girderIndex,segmentKey.segmentIndex-1);
         E = pMaterial->GetClosureJointAgeAdjustedEc(closureKey,intervalIdx);
         D = pMaterial->GetSegmentWeightDensity(closureKey,intervalIdx);
         closureMaterial[etStart]->put_E(stageIdx,E);
         closureMaterial[etStart]->put_Density(stageIdx,D);
      }

      if ( bHasClosure[etEnd] )
      {
         CClosureKey closureKey(segmentKey);
         E = pMaterial->GetClosureJointAgeAdjustedEc(closureKey,intervalIdx);
         D = pMaterial->GetSegmentWeightDensity(closureKey,intervalIdx);
         closureMaterial[etEnd]->put_E(stageIdx,E);
         closureMaterial[etEnd]->put_Density(stageIdx,D);
      }
   }

   CComQIPtr<IShape> shape(gdrSection);
   ATLASSERT(shape);
   segment->AddShape(shape,segmentMaterial,NULL);

   if ( bHasClosure[etStart] )
   {
      segment->put_ClosureJointForegroundMaterial(etStart,closureMaterial[etStart]);
      segment->put_ClosureJointBackgroundMaterial(etStart,NULL);
   }

   if ( bHasClosure[etEnd] )
   {
      segment->put_ClosureJointForegroundMaterial(etEnd,closureMaterial[etEnd]);
      segment->put_ClosureJointBackgroundMaterial(etEnd,NULL);
   }

   ssmbr->AddSegment(segment);
}

void CSplicedIBeamFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr)
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

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);


   //
   // end block transition points
   //

   // start of end block transition (wide end, tapering down to width of web)
   pgsPointOfInterest poiStartEndBlock1(segmentKey, pSegment->EndBlockLength[pgsTypes::metStart],POI_SECTCHANGE_TRANSITION);
   pPoiMgr->AddPointOfInterest(poiStartEndBlock1);

   // end of end block transition (end block is the width of the web)
   pgsPointOfInterest poiStartEndBlock2(segmentKey, pSegment->EndBlockLength[pgsTypes::metStart] + pSegment->EndBlockTransitionLength[pgsTypes::metStart],POI_SECTCHANGE_TRANSITION);
   pPoiMgr->AddPointOfInterest(poiStartEndBlock2);

   // end of end block transition (end block is the width of the web)
   pgsPointOfInterest poiEndEndBlock2(  segmentKey, segment_length - pSegment->EndBlockLength[pgsTypes::metEnd] - pSegment->EndBlockTransitionLength[pgsTypes::metEnd], POI_SECTCHANGE_TRANSITION);
   pPoiMgr->AddPointOfInterest(poiEndEndBlock2);

   // start of end block transition (wide end)
   pgsPointOfInterest poiEndEndBlock1(  segmentKey, segment_length - pSegment->EndBlockLength[pgsTypes::metEnd],POI_SECTCHANGE_TRANSITION);
   pPoiMgr->AddPointOfInterest(poiEndEndBlock1);

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

      if ( !IsZero(xLeft) )
         xLeft -= start_offset;

      if ( !IsZero(xRight) )
         xRight -= end_offset;

      pgsPointOfInterest poiStart( segmentKey, xLeft, POI_SECTCHANGE_TRANSITION);
      pPoiMgr->AddPointOfInterest( poiStart );

      pgsPointOfInterest poiEnd( segmentKey, segment_length-xRight, POI_SECTCHANGE_TRANSITION);
      pPoiMgr->AddPointOfInterest( poiEnd );
   }
   else if ( variationType == pgsTypes::svtDoubleLinear || variationType == pgsTypes::svtDoubleParabolic )
   {
      Float64 xLeft  = pSegment->GetVariationLength(pgsTypes::sztLeftPrismatic);  // measured from CL Pier/TS
      Float64 xRight = pSegment->GetVariationLength(pgsTypes::sztRightPrismatic); // measured from CL Pier/TS

      if ( !IsZero(xLeft) )
         xLeft -= start_offset;

      if ( !IsZero(xRight) )
         xRight -= end_offset;

      pgsPointOfInterest poiStart( segmentKey, xLeft, POI_SECTCHANGE_TRANSITION);
      pPoiMgr->AddPointOfInterest( poiStart );

      xLeft += pSegment->GetVariationLength(pgsTypes::sztLeftTapered);
      poiStart.SetDistFromStart(xLeft);
      pPoiMgr->AddPointOfInterest( poiStart );

      pgsPointOfInterest poiEnd( segmentKey, segment_length-xRight, POI_SECTCHANGE_TRANSITION );
      pPoiMgr->AddPointOfInterest( poiEnd );

      xRight += pSegment->GetVariationLength(pgsTypes::sztRightTapered);
      poiEnd.SetDistFromStart(segment_length-xRight);
      pPoiMgr->AddPointOfInterest( poiEnd );
   }
   else
   {
      ATLASSERT(false); // General not supported
   }
}

void CSplicedIBeamFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   CComObject<CIBeamDistFactorEngineer>* pEngineer;
   CComObject<CIBeamDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CSplicedIBeamFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng)
{
   CComObject<CTimeStepLossEngineer>* pEngineer;
   CComObject<CTimeStepLossEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CSplicedIBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover)
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
   Float64 height = d1 + d2 + d3 + d4 + d5 + d6 + d7;

   harp_rect->put_Width(width);
   harp_rect->put_Height(height);

   CComPtr<IPoint2d> hook;
   hook.CoCreateInstance(CLSID_Point2d);
   hook->Move(0, height/2.0);

   harp_rect->putref_HookPoint(hook);

   CComPtr<IShape> shape;
   harp_rect->get_Shape(&shape);

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));

   // set vertical offset bounds and increments
   Float64 hptb  = hpTopFace     == IBeamFactory::BeamBottom ? hpTopLimit     - height : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == IBeamFactory::BeamBottom ? hpBottomLimit  - height : -hpBottomLimit;
   Float64 endtb = endTopFace    == IBeamFactory::BeamBottom ? endTopLimit    - height : -endTopLimit;
   Float64 endbb = endBottomFace == IBeamFactory::BeamBottom ? endBottomLimit - height : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(height, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}


std::vector<std::_tstring> CSplicedIBeamFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<Float64> CSplicedIBeamFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CSplicedIBeamFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CSplicedIBeamFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg)
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][1];
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][4];
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][7];
      std::_tostringstream os;
      os << _T("D7 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][10];
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][12];
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
      std::_tostringstream os;
      os << _T("T1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( t2 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
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

void CSplicedIBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("IBeamDimensions"),2.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CSplicedIBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version;
   if ( pLoad->GetParentUnit() == _T("GirderLibraryEntry") )
      parent_version = pLoad->GetParentVersion();
   else
      parent_version = pLoad->GetVersion();

   IBeamFactory::Dimensions dimensions;
   std::vector<std::_tstring>::iterator iter;

   Float64 dimVersion = 1.0;
   if ( 14 <= parent_version )
   {
      if ( pLoad->BeginUnit(_T("IBeamDimensions")) )
         dimVersion = pLoad->GetVersion();
      else
         THROW_LOAD(InvalidFileFormat,pLoad);
   }

   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
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
      dimensions.push_back( std::make_pair(name,value) );
   }

   if ( 14 <= parent_version && !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   return dimensions;
}

bool CSplicedIBeamFactory::IsPrismatic(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   return false;
}

Float64 CSplicedIBeamFactory::GetVolume(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_SECTCHANGE) );
   ATLASSERT( 2 <= vPOI.size() );
   Float64 V = 0;
   std::vector<pgsPointOfInterest>::iterator iter( vPOI.begin() );
   pgsPointOfInterest prev_poi = *iter;
   Float64 prev_area;
   if ( spMode == pgsTypes::spmGross )
      prev_area = pSectProp->GetAg(releaseIntervalIdx,prev_poi);
   else
      prev_area = pSectProp->GetNetAg(releaseIntervalIdx,prev_poi);

   iter++;

#pragma Reminder("UPDATE: this will over-estimate volume for parabolically tapered beams")
   std::vector<pgsPointOfInterest>::const_iterator end(vPOI.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest poi = *iter;
      Float64 area;
      if ( spMode == pgsTypes::spmGross )
         area = pSectProp->GetAg(releaseIntervalIdx,poi);
      else
         area = pSectProp->GetNetAg(releaseIntervalIdx,poi);

      Float64 avg_area = (prev_area + area)/2;
      V += avg_area*(poi.GetDistFromStart() - prev_poi.GetDistFromStart());

      prev_poi = poi;
      prev_area = area;
   }

   return V;
}

Float64 CSplicedIBeamFactory::GetSurfaceArea(IBroker* pBroker,const CSegmentKey& segmentKey,bool bReduceForPoorlyVentilatedVoids)
{
   // compute surface area along length of member
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_SECTCHANGE) );
   ATLASSERT( 2 <= vPOI.size() );
   Float64 S = 0;
   std::vector<pgsPointOfInterest>::iterator iter( vPOI.begin() );
   pgsPointOfInterest prev_poi = *iter;
   Float64 prev_perimeter = pSectProp->GetPerimeter(prev_poi);
   iter++;

   std::vector<pgsPointOfInterest>::const_iterator end(vPOI.end());
   for ( ; iter != end; iter++ )
   {
      pgsPointOfInterest poi = *iter;
      Float64 perimeter = pSectProp->GetPerimeter(poi);

#pragma Reminder("UPDATE: this will over-estimate surface area for parabolically tapered beams")
      Float64 avg_perimeter = (prev_perimeter + perimeter)/2;
      S += avg_perimeter*(poi.GetDistFromStart() - prev_poi.GetDistFromStart());

      prev_poi = poi;
      prev_perimeter = perimeter;
   }

   // Add area for both ends
   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   Float64 start_area;
   if ( spMode == pgsTypes::spmGross )
      start_area = pSectProp->GetAg(releaseIntervalIdx,vPOI.front());
   else
      start_area = pSectProp->GetNetAg(releaseIntervalIdx,vPOI.front());

   Float64 end_area;
   if ( spMode == pgsTypes::spmGross )
      end_area = pSectProp->GetAg(releaseIntervalIdx,vPOI.back());
   else
      end_area = pSectProp->GetNetAg(releaseIntervalIdx,vPOI.back());

   S += (start_area + end_area);

   return S;
}

std::_tstring CSplicedIBeamFactory::GetImage()
{
   return std::_tstring(_T("SplicedIBeam.jpg"));
}

std::_tstring CSplicedIBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedIBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedIBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedIBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedIBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedIBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
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

CLSID CSplicedIBeamFactory::GetCLSID()
{
   return CLSID_SplicedIBeamFactory;
}

CLSID CSplicedIBeamFactory::GetFamilyCLSID()
{
   return CLSID_SplicedIBeamFamily;
}

std::_tstring CSplicedIBeamFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CSplicedIBeamFactory::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

HINSTANCE CSplicedIBeamFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CSplicedIBeamFactory::GetImageResourceName()
{
   return _T("SPLICEDIBEAM");
}

HICON  CSplicedIBeamFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_SPLICED_IBEAM) );
}

void CSplicedIBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                  Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,Float64& d6,Float64& d7,
                                  Float64& w1,Float64& w2,Float64& w3,Float64& w4,
                                  Float64& t1,Float64& t2, Float64& c1)
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

Float64 CSplicedIBeamFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name)
{
   Dimensions::const_iterator iter;
   for ( iter = dimensions.begin(); iter != dimensions.end(); iter++ )
   {
      const Dimension& dim = *iter;
      if ( name == dim.first )
         return dim.second;
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedDeckTypes CSplicedIBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CSplicedIBeamFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

pgsTypes::SupportedDiaphragmTypes CSplicedIBeamFactory::GetSupportedDiaphragms()
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CSplicedIBeamFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type)
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

void CSplicedIBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing)
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

WebIndexType CSplicedIBeamFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions)
{
   return 1;
}

Float64 CSplicedIBeamFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
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

Float64 CSplicedIBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
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

bool CSplicedIBeamFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CSplicedIBeamFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

GirderIndexType CSplicedIBeamFactory::GetMinimumBeamCount()
{
   return 2;
}

// ISplicedBeamFactory
bool CSplicedIBeamFactory::SupportsVariableDepthSection()
{
   return true; // IBeams can be fixed depth or variable depth
}

LPCTSTR CSplicedIBeamFactory::GetVariableDepthDimension()
{
   return _T("D7");
}

std::vector<pgsTypes::SegmentVariationType> CSplicedIBeamFactory::GetSupportedSegmentVariations(bool bIsVariableDepthSection)
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

bool CSplicedIBeamFactory::CanBottomFlangeDepthVary()
{
   return true;
}

LPCTSTR CSplicedIBeamFactory::GetBottomFlangeDepthDimension()
{
   return _T("D4");
}

bool CSplicedIBeamFactory::SupportsEndBlocks()
{
   return true;
}
