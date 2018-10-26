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

// NUBeamFactory.cpp : Implementation of CNUBeamFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "NUBeamFactory.h"
#include "IBeamDistFactorEngineer.h"
#include "PsBeamLossEngineer.h"
#include "TimeStepLossEngineer.h"
#include "StrandMoverImpl.h"
#include <GeomModel\NUBeam.h>
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
// CNUBeamFactory
HRESULT CNUBeamFactory::FinalConstruct()
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

void CNUBeamFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection)
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

   gdrsection.QueryInterface(ppSection);
}

void CNUBeamFactory::CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
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

void CNUBeamFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,IStages* pStages,ISuperstructureMember* ssmbr)
{
   CComPtr<IPrismaticSegment> segment;
   segment.CoCreateInstance(CLSID_PrismaticSegment);

   // Build up the beam shape
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   CComPtr<IGirderSection> gdrsection;
   CreateGirderSection(pBroker,statusGroupID,dimensions,-1,-1,&gdrsection);

   // Beam materials
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IMaterials,pMaterial);
   CComPtr<IMaterial> material;
   material.CoCreateInstance(CLSID_Material);

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount(segmentKey);
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 E = pMaterial->GetSegmentEc(segmentKey,intervalIdx);
      Float64 D = pMaterial->GetSegmentWeightDensity(segmentKey,intervalIdx);

      StageIndexType stageIdx = pStages->GetStage(segmentKey,intervalIdx);

      material->put_E(stageIdx,E);
      material->put_Density(stageIdx,D);
   }

   CComQIPtr<IShape> shape(gdrsection);
   ATLASSERT(shape);
   segment->AddShape(shape,material,NULL);

   ssmbr->AddSegment(segment);
}

void CNUBeamFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr)
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);
}

void CNUBeamFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   CComObject<CIBeamDistFactorEngineer>* pEngineer;
   CComObject<CIBeamDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CNUBeamFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng)
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

void CNUBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover)
{
   Float64 d1,d2,d3,d4,d5;
   Float64 r1,r2,r3,r4;
   Float64 t;
   Float64 w1,w2;
   Float64 c1;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,r1,r2,r3,r4,t,w1,w2,c1);

   // set the shape for harped strand bounds - only in the thinest part of the web
   CComPtr<IRectangle> harp_rect;
   HRESULT hr = harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   Float64 width = t;
   Float64 depth = d1 + d2 + d3 + d4 + d5;

   harp_rect->put_Width(width);
   harp_rect->put_Height(depth);

   CComPtr<IPoint2d> hook;
   hook.CoCreateInstance(CLSID_Point2d);
   hook->Move(0, depth/2.0);

   harp_rect->putref_HookPoint(hook);

   CComPtr<IShape> shape;
   harp_rect->get_Shape(&shape);

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);
   CComPtr<IStrandMover> sm = pStrandMover;

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));

   // set vertical offset bounds and increments
   Float64 hptb  = hpTopFace     == IBeamFactory::BeamBottom ? hpTopLimit     - depth : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == IBeamFactory::BeamBottom ? hpBottomLimit  - depth : -hpBottomLimit;
   Float64 endtb = endTopFace    == IBeamFactory::BeamBottom ? endTopLimit    - depth : -endTopLimit;
   Float64 endbb = endBottomFace == IBeamFactory::BeamBottom ? endBottomLimit - depth : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(depth, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}


std::vector<std::_tstring> CNUBeamFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<Float64> CNUBeamFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CNUBeamFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CNUBeamFactory::ValidateDimensions(const Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg)
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

void CNUBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   // version 2.... added C1
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

IBeamFactory::Dimensions CNUBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version;
   if ( pLoad->GetParentUnit() == _T("GirderLibraryEntry") )
      parent_version = pLoad->GetParentVersion();
   else
      parent_version = pLoad->GetVersion();


   IBeamFactory::Dimensions dimensions;

   if ( 14 <= parent_version && !pLoad->BeginUnit(_T("NUBeamDimensions")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   Float64 version = pLoad->GetVersion();

   std::vector<std::_tstring>::iterator iter;
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value;

      if ( name == _T("C1") && version < 2 )
      {
         // C1 didn't exist before version 2
         value = 0.0;
      }
      else
      {
         pLoad->Property(name.c_str(),&value);
      }

      dimensions.push_back( std::make_pair(name,value) );
   }

   if ( 14 <= parent_version && !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   return dimensions;
}

bool CNUBeamFactory::IsPrismatic(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   return true;
}

Float64 CNUBeamFactory::GetVolume(IBroker* pBroker,const CSegmentKey& segmentKey)
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

Float64 CNUBeamFactory::GetSurfaceArea(IBroker* pBroker,const CSegmentKey& segmentKey,bool bReduceForPoorlyVentilatedVoids)
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

std::_tstring CNUBeamFactory::GetImage()
{
   return std::_tstring(_T("NUBeam.jpg"));
}

std::_tstring CNUBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CNUBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CNUBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CNUBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CNUBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
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

std::_tstring CNUBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
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

CLSID CNUBeamFactory::GetCLSID()
{
   return CLSID_NUBeamFactory;
}

CLSID CNUBeamFactory::GetFamilyCLSID()
{
   return CLSID_WFBeamFamily;
}

std::_tstring CNUBeamFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CNUBeamFactory::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

HINSTANCE CNUBeamFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CNUBeamFactory::GetImageResourceName()
{
   return _T("NUBEAM");
}

HICON  CNUBeamFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_NUBEAM) );
}

void CNUBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
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

Float64 CNUBeamFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name)
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

pgsTypes::SupportedDeckTypes CNUBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CNUBeamFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

pgsTypes::SupportedDiaphragmTypes CNUBeamFactory::GetSupportedDiaphragms()
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CNUBeamFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type)
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

void CNUBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
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

WebIndexType CNUBeamFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions)
{
   return 1;
}

Float64 CNUBeamFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   Float64 D1 = GetDimension(dimensions,_T("D1"));
   Float64 D2 = GetDimension(dimensions,_T("D2"));
   Float64 D3 = GetDimension(dimensions,_T("D3"));
   Float64 D4 = GetDimension(dimensions,_T("D4"));
   Float64 D5 = GetDimension(dimensions,_T("D5"));

   return D1 + D2 + D3 + D4 + D5;
}

Float64 CNUBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return Max(GetDimension(dimensions,_T("W1")),GetDimension(dimensions,_T("W2")));
}

bool CNUBeamFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CNUBeamFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

GirderIndexType CNUBeamFactory::GetMinimumBeamCount()
{
   return 2;
}
