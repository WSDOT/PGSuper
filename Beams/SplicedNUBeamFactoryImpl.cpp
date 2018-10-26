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

// SplicedNUBeamFactory.cpp : Implementation of CSplicedNUBeamFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "SplicedNUBeamFactoryImpl.h"
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

#include <PgsExt\StatusItem.h>

#include <PgsExt\BridgeDescription2.h>

#include "PrivateHelpers.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CSplicedNUBeamFactory
HRESULT CSplicedNUBeamFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("D1"));
   m_DimNames.push_back(_T("D2"));
   m_DimNames.push_back(_T("D3"));
   m_DimNames.push_back(_T("D4"));
   m_DimNames.push_back(_T("D5"));
   m_DimNames.push_back(_T("R1"));
   m_DimNames.push_back(_T("R2"));
   m_DimNames.push_back(_T("R3"));
   m_DimNames.push_back(_T("R4"));
   m_DimNames.push_back(_T("T"));
   m_DimNames.push_back(_T("W1"));
   m_DimNames.push_back(_T("W2"));
   m_DimNames.push_back(_T("C1"));

   // Default beam is Caltrans Super Girder
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // D1
   m_DefaultDims.push_back(::ConvertToSysUnits( 1.750,unitMeasure::Inch)); // D2
   m_DefaultDims.push_back(::ConvertToSysUnits(54.875,unitMeasure::Inch)); // D3
   m_DefaultDims.push_back(::ConvertToSysUnits( 6.000,unitMeasure::Inch)); // D4
   m_DefaultDims.push_back(::ConvertToSysUnits( 6.375,unitMeasure::Inch)); // D5
   m_DefaultDims.push_back(::ConvertToSysUnits(10.000,unitMeasure::Inch)); // R1
   m_DefaultDims.push_back(::ConvertToSysUnits(10.000,unitMeasure::Inch)); // R2
   m_DefaultDims.push_back(::ConvertToSysUnits( 2.500,unitMeasure::Inch)); // R3
   m_DefaultDims.push_back(::ConvertToSysUnits( 2.500,unitMeasure::Inch)); // R4
   m_DefaultDims.push_back(::ConvertToSysUnits( 7.000,unitMeasure::Inch)); // T
   m_DefaultDims.push_back(::ConvertToSysUnits(48.000,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits(43.000,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits( 1.000,unitMeasure::Inch)); // C1

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D5
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // R1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // R2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // R3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // R4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C1

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D5
   m_DimUnits[1].push_back(&unitMeasure::Inch); // R1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // R2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // R3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // R4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C1

   return S_OK;
}

void CSplicedNUBeamFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection)
{
   CComPtr<INUGirderSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_NUGirderSection);
   CComPtr<INUBeam> beam;
   gdrsection->get_Beam(&beam);

   Float64 d1,d2,d3,d4,d5;
   Float64 r1,r2,r3,r4;
   Float64 t;
   Float64 w1,w2;
   Float64 c1;

   GetDimensions(dimensions,d1,d2,d3,d4,d5,r1,r2,r3,r4,t,w1,w2,c1);
   beam->put_W1(w1);
   beam->put_W2(w2);
   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_D3(d3);
   beam->put_D4(d4);
   beam->put_D5(d5);
   beam->put_T(t);
   beam->put_R1(r1);
   beam->put_R2(r2);
   beam->put_R3(r3);
   beam->put_R4(r4);
   beam->put_C1(c1);

   if ( 0 < bottomFlangeHeight )
   {
      d5 = bottomFlangeHeight;
      beam->put_D5(d5);
   }

   if ( 0 < overallHeight )
   {
      d3 += overallHeight - (d1+d2+d3+d4+d5);
      if ( 0 < d3 )
      {
         beam->put_D3(d3);
      }
   }

   gdrsection.QueryInterface(ppSection);
}

void CSplicedNUBeamFactory::CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetSegmentLength(segmentKey);

   Float64 d1,d2,d3,d4,d5;
   Float64 r1,r2,r3,r4;
   Float64 t;
   Float64 w1,w2;
   Float64 c1;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,r1,r2,r3,r4,t,w1,w2,c1);

   Float64 height = d1 + d2 + d3 + d4 + d5;

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

void CSplicedNUBeamFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMember* ssmbr)
{
   CComPtr<ISplicedGirderSegment> segment;
   segment.CoCreateInstance(CLSID_NUSplicedGirderSegment);

   ATLASSERT(segment != NULL);

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

   BuildSplicedGirderMaterialModel(pBroker,pSegment,segment,gdrSection);

   ssmbr->AddSegment(segment);
}

void CSplicedNUBeamFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr)
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
      {
         xLeft -= start_offset;
      }

      if ( !IsZero(xRight) )
      {
         xRight -= end_offset;
      }

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
      {
         xLeft -= start_offset;
      }

      if ( !IsZero(xRight) )
      {
         xRight -= end_offset;
      }

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

void CSplicedNUBeamFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   CComObject<CIBeamDistFactorEngineer>* pEngineer;
   CComObject<CIBeamDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CSplicedNUBeamFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng)
{
   CComObject<CTimeStepLossEngineer>* pEngineer;
   CComObject<CTimeStepLossEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CSplicedNUBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions,  Float64 Hg,
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

   Float64 d1,d2,d3,d4,d5;
   Float64 r1,r2,r3,r4;
   Float64 t;
   Float64 w1,w2;
   Float64 c1;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,r1,r2,r3,r4,t,w1,w2,c1);

   Float64 width = t;
   Float64 depth = (Hg < 0 ? d1 + d2 + d3 + d4 + d5 : Hg);

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


std::vector<std::_tstring> CSplicedNUBeamFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<Float64> CSplicedNUBeamFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CSplicedNUBeamFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CSplicedNUBeamFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg)
{
   Float64 d1,d2,d3,d4,d5;
   Float64 r1,r2,r3,r4;
   Float64 t;
   Float64 w1,w2;
   Float64 c1;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,r1,r2,r3,r4,t,w1,w2,c1);

// D1  0
// D2  1
// D3  2
// D4  3
// D5  4
// R1  5
// R2  6
// R3  7
// R4  8
// T   9
// W1  10
// W2  11
// C1  12
   
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
      os << _T("D2 must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d3 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D3 must be zero or greater") << std::ends;
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
      os << _T("D5 must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( r1 < 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][5];
      std::_tostringstream os;
      os << _T("R1 must be zero or greater") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( r2 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("R2 must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( r3 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("R3 must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( r4 < 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
      std::_tostringstream os;
      os << _T("R4 must be zero or greater") << pUnit->UnitTag() << std::ends;
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

   if ( w2 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("W2 must be greater than 0.0") << std::ends;
      *strErrMsg = os.str();
      return false;
   }
   
   if ( t <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
      std::_tostringstream os;
      os << _T("T must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( d1 < r3 )
   {
      std::_tostringstream os;
      os << _T("R3 must be less than or equal to D1") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( d5 < r4 + c1 )
   {
      std::_tostringstream os;
      os << _T("R4 + C1 must be less than or equal to D5") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   return true;
}

void CSplicedNUBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("NUBeamDimensions"),2.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CSplicedNUBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version;
   if ( pLoad->GetParentUnit() == _T("GirderLibraryEntry") )
   {
      parent_version = pLoad->GetParentVersion();
   }
   else
   {
      parent_version = pLoad->GetVersion();
   }

   IBeamFactory::Dimensions dimensions;
   std::vector<std::_tstring>::iterator iter;

   Float64 dimVersion = 1.0;
   if ( 14 <= parent_version )
   {
      if ( pLoad->BeginUnit(_T("NUBeamDimensions")) )
      {
         dimVersion = pLoad->GetVersion();
      }
      else
      {
         THROW_LOAD(InvalidFileFormat,pLoad);
      }
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
   {
      THROW_LOAD(InvalidFileFormat,pLoad);
   }

   return dimensions;
}

bool CSplicedNUBeamFactory::IsPrismatic(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   return false;
}

bool CSplicedNUBeamFactory::IsSymmetric(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   return false;
}

Float64 CSplicedNUBeamFactory::GetInternalSurfaceAreaOfVoids(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   return 0;
}

std::_tstring CSplicedNUBeamFactory::GetImage()
{
   return std::_tstring(_T("SplicedNUBeam.jpg"));
}

std::_tstring CSplicedNUBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedNUBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedNUBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedNUBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedNUBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedNUBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
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

CLSID CSplicedNUBeamFactory::GetCLSID()
{
   return CLSID_SplicedNUBeamFactory;
}

std::_tstring CSplicedNUBeamFactory::GetName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CSplicedNUBeamFactory::GetFamilyCLSID()
{
   return CLSID_SplicedIBeamFamily;
}

std::_tstring CSplicedNUBeamFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CSplicedNUBeamFactory::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CSplicedNUBeamFactory::GetPublisherContactInformation()
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CSplicedNUBeamFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CSplicedNUBeamFactory::GetImageResourceName()
{
   return _T("SPLICEDNUBEAM");
}

HICON  CSplicedNUBeamFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_SPLICED_NUBEAM) );
}

void CSplicedNUBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                   Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,
                                   Float64& r1,Float64& r2,Float64& r3,Float64& r4,
                                   Float64& t,
                                   Float64& w1,Float64& w2,
                                   Float64& c1)
{
   d1 = GetDimension(dimensions,_T("D1"));
   d2 = GetDimension(dimensions,_T("D2"));
   d3 = GetDimension(dimensions,_T("D3"));
   d4 = GetDimension(dimensions,_T("D4"));
   d5 = GetDimension(dimensions,_T("D5"));
   r1 = GetDimension(dimensions,_T("R1"));
   r2 = GetDimension(dimensions,_T("R2"));
   r3 = GetDimension(dimensions,_T("R3"));
   r4 = GetDimension(dimensions,_T("R4"));
   t  = GetDimension(dimensions,_T("T"));
   w1 = GetDimension(dimensions,_T("W1"));
   w2 = GetDimension(dimensions,_T("W2"));
   c1 = GetDimension(dimensions,_T("C1"));
}

Float64 CSplicedNUBeamFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name)
{
   Dimensions::const_iterator iter;
   for ( iter = dimensions.begin(); iter != dimensions.end(); iter++ )
   {
      const Dimension& dim = *iter;
      if ( name == dim.first )
      {
         return dim.second;
      }
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedDeckTypes CSplicedNUBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CSplicedNUBeamFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

pgsTypes::SupportedDiaphragmTypes CSplicedNUBeamFactory::GetSupportedDiaphragms()
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CSplicedNUBeamFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type)
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

void CSplicedNUBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing)
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

WebIndexType CSplicedNUBeamFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions)
{
   return 1;
}

Float64 CSplicedNUBeamFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   Float64 D1 = GetDimension(dimensions,_T("D1"));
   Float64 D2 = GetDimension(dimensions,_T("D2"));
   Float64 D3 = GetDimension(dimensions,_T("D3"));
   Float64 D4 = GetDimension(dimensions,_T("D4"));
   Float64 D5 = GetDimension(dimensions,_T("D5"));

   return D1 + D2 + D3 + D4 + D5;
}

Float64 CSplicedNUBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return Max(GetDimension(dimensions,_T("W1")),GetDimension(dimensions,_T("W2")));
}

bool CSplicedNUBeamFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CSplicedNUBeamFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

GirderIndexType CSplicedNUBeamFactory::GetMinimumBeamCount()
{
   return 2;
}

// ISplicedBeamFactory
bool CSplicedNUBeamFactory::SupportsVariableDepthSection()
{
   return true; // IBeams can be fixed depth or variable depth
}

LPCTSTR CSplicedNUBeamFactory::GetVariableDepthDimension()
{
   return _T("D3");
}

std::vector<pgsTypes::SegmentVariationType> CSplicedNUBeamFactory::GetSupportedSegmentVariations(bool bIsVariableDepthSection)
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

bool CSplicedNUBeamFactory::CanBottomFlangeDepthVary()
{
   return true;
}

LPCTSTR CSplicedNUBeamFactory::GetBottomFlangeDepthDimension()
{
   return _T("D5");
}

bool CSplicedNUBeamFactory::SupportsEndBlocks()
{
   return true;
}
