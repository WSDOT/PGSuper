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

// SplicedUBeamFactory.cpp : Implementation of CSplicedUBeamFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "SplicedUBeamFactory.h"
#include "UBeamDistFactorEngineer.h"
#include "TimeStepLossEngineer.h"
#include "StrandMoverImpl.h"
#include <sstream>
#include <algorithm>

#include "UBeamHelpers.h"

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
// CSplicedUBeamFactory
HRESULT CSplicedUBeamFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("D1"));
   m_DimNames.push_back(_T("D2"));
   m_DimNames.push_back(_T("D3"));
   m_DimNames.push_back(_T("D4"));
   m_DimNames.push_back(_T("D5"));
   m_DimNames.push_back(_T("D6"));
   m_DimNames.push_back(_T("D7"));
   m_DimNames.push_back(_T("T"));
   m_DimNames.push_back(_T("W1"));
   m_DimNames.push_back(_T("W2"));
   m_DimNames.push_back(_T("W3"));
   m_DimNames.push_back(_T("W4"));
   m_DimNames.push_back(_T("W5"));

   std::sort(m_DimNames.begin(),m_DimNames.end());

   // Default beam is a U54G4
   m_DefaultDims.push_back(::ConvertToSysUnits(54.00,unitMeasure::Inch)); // D1
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // D2
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // D3
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // D4
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // D5
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // D6
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // D7
   m_DefaultDims.push_back(::ConvertToSysUnits(7.000,unitMeasure::Inch)); // T
   m_DefaultDims.push_back(::ConvertToSysUnits(48.00,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits(63.425,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits(12.00,unitMeasure::Inch)); // W3
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // W4
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // W5

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D5
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D6
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D7
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W5

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D5
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D6
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D7
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W5

   return S_OK;
}

void CSplicedUBeamFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection)
{
   CComPtr<IUGirderSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_UGirderSection);
   CComPtr<IUBeam> beam;
   gdrsection->get_Beam(&beam);

   ConfigureShape(dimensions, beam);

   gdrsection.QueryInterface(ppSection);
}

void CSplicedUBeamFactory::CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetSegmentLength(segmentKey);

   Float64 w1, w2, w3, w4, w5;
   Float64 d1, d2, d3, d4, d5, d6, d7;
   Float64 t;
   GetDimensions(dimensions,d1, d2, d3, d4, d5, d6, d7, w1, w2, w3, w4, w5, t);

   Float64 height = d1;

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

void CSplicedUBeamFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,IStages* pStages,ISuperstructureMember* ssmbr)
{
   CComPtr<ISplicedGirderSegment> segment;
   segment.CoCreateInstance(CLSID_USplicedGirderSegment);

   ATLASSERT(segment != NULL);

   // Build up the beam shape
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment = pGirder->GetSegment(segmentKey.segmentIndex);

   bool bHasClosure[2] = {pSegment->GetLeftClosure() != NULL,pSegment->GetRightClosure() != NULL};

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
    // need to update WBFLGeometricBridge SplicedGirderSegment or create a new SplicedGirderSegment-type
    // for U-beams
   
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

void CSplicedUBeamFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr)
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 segment_length = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,          POI_SECTCHANGE_RIGHTFACE);
   pgsPointOfInterest poiEnd(  segmentKey,segment_length,POI_SECTCHANGE_LEFTFACE );

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);
}

void CSplicedUBeamFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   CComObject<CUBeamDistFactorEngineer>* pEngineer;
   CComObject<CUBeamDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CSplicedUBeamFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng)
{
   CComObject<CTimeStepLossEngineer>* pEngineer;
   CComObject<CTimeStepLossEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CSplicedUBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover)
{
   // build our shape so we can get higher-level info
   CComPtr<IUBeam> beam;
   beam.CoCreateInstance(CLSID_UBeam);

   ConfigureShape(dimensions, beam);

   // our goal is to build a parallelogram using the thin web dimension from top to bottom
   Float64 t;
   beam->get_T(&t);
   Float64 slope;
   GetSlope(beam, &slope);
   Float64 height;
   beam->get_Height(&height);
   Float64 w1;
   beam->get_W1(&w1);

   Float64 arc_slope = 1.0/slope;

   Float64 t_x_project = t*sqrt(slope*slope+1)/slope;

   CComPtr<IPolyShape> rgt_harp_poly;
   rgt_harp_poly.CoCreateInstance(CLSID_PolyShape);

   // travel counter clockwise around right web;
   Float64 x1 = w1/2.0;
   Float64 y1 = 0.0;

   Float64 x2 = x1 + height * arc_slope;
   Float64 y2 = height;

   Float64 x3 = x2 - t_x_project;
   Float64 y3 = y2;

   Float64 x4 = x1 - t_x_project;
   Float64 y4 = 0.0;

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
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);
   CComPtr<IStrandMover> sm = pStrandMover;

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(lft_shape, -arc_slope);
   ATLASSERT (SUCCEEDED(hr));
   hr = configurer->AddRegion(rgt_shape, arc_slope);
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

std::vector<std::_tstring> CSplicedUBeamFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<Float64> CSplicedUBeamFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CSplicedUBeamFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CSplicedUBeamFactory::ValidateDimensions(const Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg)
{
   Float64 w1, w2, w3, w4, w5;
   Float64 d1, d2, d3, d4, d5, d6, d7;
   Float64 t;
   GetDimensions(dimensions,d1, d2, d3, d4, d5, d6, d7, w1, w2, w3, w4, w5, t);

// D1  0
// D2  1
// D3  2
// D4  3
// D5  4
// D6  5
// D7  6
// T   7
// W1  8 
// W2  9
// W3  10
// W4  11 
// W5  12 
   
   if ( d1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
      std::_tostringstream os;
      os << _T("D1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d2 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][1];
      std::_tostringstream os;
      os << _T("D2 must be greater than 0.0") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d3 < 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][2];
      std::_tostringstream os;
      os << _T("D3 must be greater than or equal to 0.0") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d1 < d2+d3 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
      std::_tostringstream os;
      os << _T("D1 must be greater than or equal to D2 + D3 (") << ::ConvertFromSysUnits(d2+d3,*pUnit) << _T(" ") << pUnit->UnitTag() << _T(")") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][10];
      std::_tostringstream os;
      os << _T("W3 must be greater than or equal to 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( t <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][7];
      std::_tostringstream os;
      os << _T("T must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   

   return true;
}

void CSplicedUBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("UBeamDimensions"),1.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CSplicedUBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
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
      if ( pLoad->BeginUnit(_T("UBeamDimensions")) )
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
         if ( dimVersion < 2 && (parent_version == 1.2 || parent_version == 1.3) )
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

   if ( parent_version == 1.3 )
   {
      // It seems that we published our U-Girder shape too soon. The dimensions of the shape
      // have changed. Technically, this constitues a new shape, however we don't want to support
      // basically identical shapes (the new shape can degenerate to the old one).
      //
      // HACKS:
      // The new shape has more dimensions than the old one, hence setting the values of the unknown
      // dimensions to 0.0 above. The correct behavior is to throw an exception, so this functionality is
      // maintained for all other versions.
      //
      // Need to compute W4 as the slope adjusted web width (measured horizontally)
      Float64 W1 = GetDimension(dimensions,std::_tstring(_T("W1")));
      Float64 W2 = GetDimension(dimensions,std::_tstring(_T("W2")));
      Float64 W4 = GetDimension(dimensions,std::_tstring(_T("W4")));
      Float64 T  = GetDimension(dimensions,std::_tstring(_T("T")));
      Float64 D1 = GetDimension(dimensions,std::_tstring(_T("D1")));
      Float64 D6 = GetDimension(dimensions,std::_tstring(_T("D6")));

      Float64 slope = ComputeSlope(T,D1,D6,W1,W2,W4);
      Float64 W5 = W4 - T*sqrt(slope*slope + 1)/slope;
      W5 = IsZero(W5) ? 0.0 : W5; // Deaden any noise

      // Set W4 = 0
      Dimensions::iterator iter;
      for ( iter = dimensions.begin(); iter != dimensions.end(); iter++ )
      {
         Dimension& dim = *iter;
         if ( dim.first == std::_tstring(_T("W4")) )
         {
            dim.second = 0.0;
            break;
         }
      }

      dimensions.push_back(Dimension(_T("W5"),W5));
   }


   return dimensions;
}

bool CSplicedUBeamFactory::IsPrismatic(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   return false;
}

Float64 CSplicedUBeamFactory::GetVolume(IBroker* pBroker,const CSegmentKey& segmentKey)
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

Float64 CSplicedUBeamFactory::GetSurfaceArea(IBroker* pBroker,const CSegmentKey& segmentKey,bool bReduceForPoorlyVentilatedVoids)
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

std::_tstring CSplicedUBeamFactory::GetImage()
{
   return std::_tstring(_T("UBeam.jpg"));
}

std::_tstring CSplicedUBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedUBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedUBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedUBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CSplicedUBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("UBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("UBeam_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring CSplicedUBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("UBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("UBeam_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CSplicedUBeamFactory::GetCLSID()
{
   return CLSID_SplicedUBeamFactory;
}

CLSID CSplicedUBeamFactory::GetFamilyCLSID()
{
   return CLSID_SplicedUBeamFamily;
}

std::_tstring CSplicedUBeamFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CSplicedUBeamFactory::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

HINSTANCE CSplicedUBeamFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CSplicedUBeamFactory::GetImageResourceName()
{
   return _T("UBEAM");
}

HICON  CSplicedUBeamFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_SPLICED_UBEAM) );
}

void CSplicedUBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                  Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,Float64& d6,Float64& d7,
                                  Float64& w1,Float64& w2,Float64& w3,Float64& w4,Float64& w5,
                                  Float64& t)
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
   w5 = GetDimension(dimensions,_T("W5"));
   t  = GetDimension(dimensions,_T("T"));
}

Float64 CSplicedUBeamFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name)
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

pgsTypes::SupportedDeckTypes CSplicedUBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CSplicedUBeamFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

pgsTypes::SupportedDiaphragmTypes CSplicedUBeamFactory::GetSupportedDiaphragms()
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CSplicedUBeamFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type)
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

void CSplicedUBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
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


void CSplicedUBeamFactory::ConfigureShape(const IBeamFactory::Dimensions& dimensions, IUBeam* beam)
{
   Float64 w1, w2, w3, w4, w5;
   Float64 d1, d2, d3, d4, d5, d6, d7;
   Float64 t;
   GetDimensions(dimensions,d1, d2, d3, d4, d5, d6, d7, w1, w2, w3, w4, w5, t);
   beam->put_W1(w1);
   beam->put_W2(w2);
   beam->put_W3(w3);
   beam->put_W4(w4);
   beam->put_W5(w5);
   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_D3(d3);
   beam->put_D4(d4);
   beam->put_D5(d5);
   beam->put_D6(d6);
   beam->put_D7(d7);
   beam->put_T(t);
}

WebIndexType CSplicedUBeamFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions)
{
   return 2;
}

Float64 CSplicedUBeamFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("D1"));
}

Float64 CSplicedUBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("W2"));
}

bool CSplicedUBeamFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CSplicedUBeamFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

GirderIndexType CSplicedUBeamFactory::GetMinimumBeamCount()
{
   return 1;
}

// ISplicedBeamFactory
bool CSplicedUBeamFactory::SupportsVariableDepthSection()
{
   return false; // section cannot be varied
}

LPCTSTR CSplicedUBeamFactory::GetVariableDepthDimension()
{
   ATLASSERT(false); // should never get here because U-beams don't support variable depth
   return _T("???");
}

std::vector<pgsTypes::SegmentVariationType> CSplicedUBeamFactory::GetSupportedSegmentVariations(bool bIsVariableDepthSection)
{
   ATLASSERT(bIsVariableDepthSection == false);
   std::vector<pgsTypes::SegmentVariationType> variations;
   variations.push_back(pgsTypes::svtNone);
   return variations;
}

bool CSplicedUBeamFactory::CanBottomFlangeDepthVary()
{
   return false; // the bottom flange depth cannot vary
}

LPCTSTR CSplicedUBeamFactory::GetBottomFlangeDepthDimension()
{
   ATLASSERT(false); // should never get here because U-beams don't support variable depth
   return _T("???");
}

bool CSplicedUBeamFactory::SupportsEndBlocks()
{
   return false;
}
