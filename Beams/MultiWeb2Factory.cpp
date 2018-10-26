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

// MultiWeb2Factory.cpp : Implementation of CMultiWeb2Factory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "MultiWeb2Factory.h"
#include "MultiWebDistFactorEngineer.h"
#include "PsBeamLossEngineer.h"
#include "StrandMoverImpl.h"
#include <GeomModel\PrecastBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>

#include <PgsExt\BridgeDescription2.h>

#include <IFace\StatusCenter.h>
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CMultiWeb2Factory
HRESULT CMultiWeb2Factory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("C1"));
   m_DimNames.push_back(_T("C2"));
   m_DimNames.push_back(_T("H1"));
   m_DimNames.push_back(_T("H2"));
   m_DimNames.push_back(_T("H3"));
   m_DimNames.push_back(_T("T1"));
   m_DimNames.push_back(_T("T2"));
   m_DimNames.push_back(_T("T3"));
   m_DimNames.push_back(_T("F1"));
   m_DimNames.push_back(_T("W2"));
   m_DimNames.push_back(_T("Wmin"));
   m_DimNames.push_back(_T("Wmax"));

   m_DefaultDims.push_back(::ConvertToSysUnits( 0.0,unitMeasure::Inch)); // C1
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.0,unitMeasure::Inch)); // C2
   m_DefaultDims.push_back(::ConvertToSysUnits(27.0,unitMeasure::Inch)); // H1
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // H2
   m_DefaultDims.push_back(::ConvertToSysUnits( 6.0,unitMeasure::Inch)); // H3
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.0,unitMeasure::Inch)); // T1
   m_DefaultDims.push_back(::ConvertToSysUnits( 6.5,unitMeasure::Inch)); // T2
   m_DefaultDims.push_back(::ConvertToSysUnits( 1.5,unitMeasure::Inch)); // T3
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // F1
   m_DefaultDims.push_back(::ConvertToSysUnits(48.0,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits( 2.5,unitMeasure::Feet)); // Wmin
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Feet)); // Wmax


   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // F1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Meter); // Wmin
   m_DimUnits[0].push_back(&unitMeasure::Meter); // Wmax

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // F1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Feet); // Wmin
   m_DimUnits[1].push_back(&unitMeasure::Feet); // Wmax
   

   return S_OK;
}

void CMultiWeb2Factory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection)
{
   CComPtr<IMultiWebSection2> gdrsection;
   gdrsection.CoCreateInstance(CLSID_MultiWebSection2);
   CComPtr<IMultiWeb2> beam;
   gdrsection->get_Beam(&beam);

   Float64 c1,c2;
   Float64 h1,h2,h3;
   Float64 w2,wmin,wmax;
   Float64 t1,t2,t3;
   Float64 f1;
   GetDimensions(dimensions,h1,h2,h3,t1,t2,t3,f1,c1,c2,w2,wmin,wmax);

   beam->put_C1(c1);
   beam->put_C2(c2);
   beam->put_H1(h1);
   beam->put_H2(h2);
   beam->put_H3(h3);
   beam->put_T1(t1);
   beam->put_T2(t2);
   beam->put_T3(t3);
   beam->put_T4(0);
   beam->put_T5(0);
   beam->put_F1(f1);
   beam->put_F2(0);
   beam->put_W2(w2);
   beam->put_WebCount(2);

   // figure out the web spacing, w2, based on the girder spacing
   Float64 w1;
   if ( pBroker == NULL )
   {
      // just use the max
      w1 = wmax;
   }
   else
   {
#pragma Reminder("UPDATE: Assuming uniform spacing")
      // uniform spacing is required for this type of girder so maybe this is ok

      // use raw input here because requesting it from the bridge will cause an infite loop.
      // bridge agent calls this during validation
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      ATLASSERT(pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsConstantAdjacent);
      Float64 spacing = pBridgeDesc->GetGirderSpacing();;

      Float64 top_flange_max = 2*(wmax + t1+t2+t3) + w2;
      Float64 top_flange_min = 2*(wmin + t1+t2+t3) + w2;

      // if this is a fixed width section, then set the spacing equal to the width
      if ( IsEqual(top_flange_max,top_flange_min) )
         spacing = top_flange_max;

      w1 = (spacing - 2*(t1+t2+t3) - w2)/2;
   }
   beam->put_W1(w1);

   gdrsection.QueryInterface(ppSection);
}

void CMultiWeb2Factory::CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetSegmentLength(segmentKey);

   Float64 c1,c2;
   Float64 h1,h2,h3;
   Float64 w2,wmin,wmax;
   Float64 t1,t2,t3;
   Float64 f1;
   GetDimensions(dimensions,h1,h2,h3,t1,t2,t3,f1,c1,c2,w2,wmin,wmax);

   Float64 height = h1 + h2 + h3;

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

void CMultiWeb2Factory::LayoutGirderLine(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMember* ssmbr)
{
   CComPtr<IPrismaticSegment> segment;
   segment.CoCreateInstance(CLSID_PrismaticSegment);

   // Build up the beam shape
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,ISegmentData,pSegmentData);

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

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 E = pMaterial->GetSegmentAgeAdjustedEc(segmentKey,intervalIdx);
      Float64 D = pMaterial->GetSegmentWeightDensity(segmentKey,intervalIdx);

      material->put_E(intervalIdx,E);
      material->put_Density(intervalIdx,D);
   }

   CComQIPtr<IShape> shape(gdrsection);
   ATLASSERT(shape);
   segment->AddShape(shape,material,NULL);

   ssmbr->AddSegment(segment);
}

void CMultiWeb2Factory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr)
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);
}

void CMultiWeb2Factory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   
   CComObject<CMultiWebDistFactorEngineer>* pEngineer;
   CComObject<CMultiWebDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);

   pEngineer->SetBeamType(CMultiWebDistFactorEngineer::btMultiWebTee);

   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CMultiWeb2Factory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng)
{
    CComObject<CPsBeamLossEngineer>* pEngineer;
    CComObject<CPsBeamLossEngineer>::CreateInstance(&pEngineer);
    pEngineer->Init(IBeam);
    pEngineer->SetBroker(pBroker,statusGroupID);
    (*ppEng) = pEngineer;
    (*ppEng)->AddRef();
}

void CMultiWeb2Factory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover)
{
   Float64 h1,h2,h3;
   Float64 c1,c2;
   Float64 w2,wmin,wmax;
   Float64 t1,t2,t3;
   Float64 f1;
   GetDimensions(dimensions,h1,h2,h3,t1,t2,t3,f1,c1,c2,w2,wmin,wmax);

   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   Float64 width = t2;
   Float64 depth = h1 + h2 + h3;

   CComPtr<IRectangle> lft_harp_rect, rgt_harp_rect;
   hr = lft_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));
   hr = rgt_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   lft_harp_rect->put_Width(width);
   lft_harp_rect->put_Height(depth);
   rgt_harp_rect->put_Width(width);
   rgt_harp_rect->put_Height(depth);

   Float64 hook_offset = w2/2.0 + t2/2.0 + t3;

   CComPtr<IPoint2d> lft_hook, rgt_hook;
   lft_hook.CoCreateInstance(CLSID_Point2d);
   rgt_hook.CoCreateInstance(CLSID_Point2d);

   lft_hook->Move(-hook_offset, depth/2.0);
   rgt_hook->Move( hook_offset, depth/2.0);

   lft_harp_rect->putref_HookPoint(lft_hook);
   rgt_harp_rect->putref_HookPoint(rgt_hook);

   CComPtr<IShape> lft_shape, rgt_shape;
   lft_harp_rect->get_Shape(&lft_shape);
   rgt_harp_rect->get_Shape(&rgt_shape);

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(lft_shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));
   hr = configurer->AddRegion(rgt_shape, 0.0);
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

std::vector<std::_tstring> CMultiWeb2Factory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<Float64> CMultiWeb2Factory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CMultiWeb2Factory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CMultiWeb2Factory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg)
{
   Float64 h1,h2,h3;
   Float64 c1,c2;
   Float64 w2,wmin,wmax;
   Float64 t1,t2,t3;
   Float64 f1;
   GetDimensions(dimensions,h1,h2,h3,t1,t2,t3,f1,c1,c2,w2,wmin,wmax);

 // C1
 // C2
 // H1
 // H2
 // H3
 // T1
 // T2
 // T3
 // F1
 // W2
 // Wmin
 // Wmax

   // values that must be postive
   if ( h1 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("H1 must be greater than 0.0")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( h3 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("H3 must be greater than 0.0")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( t2 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("T2 must be greater than 0.0")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( wmin <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("Wmin must be greater than 0.0")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( wmax <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("Wmax must be greater than 0.0")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   // values that cant be negative
   if ( f1 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("F1 must be zero or greater")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( h2 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("H2 must be zero or greater")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( c1 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("C1 must be zero or greater")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( c2 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("C2 must be zero or greater")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( t1 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("T1 must be zero or greater")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( t3 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("T3 must be zero or greater")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   // relations
   if ( wmin < f1 )
   {
      std::_tostringstream os;
      os << _T("Wmin must be greater than F1")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w2 < 2*f1 )
   {
      std::_tostringstream os;
      os << _T("W2 must be greater than 2*F1")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( wmax < wmin )
   {
      std::_tostringstream os;
      os << _T("Wmax must be greater than Wmin")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( c1 > t2/2.0 )
   {
      std::_tostringstream os;
      os << _T("T2 must be greater than 2 * C1")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( c2 > h3 )
   {
      std::_tostringstream os;
      os << _T("C2 must be less than H3")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   return true;
}

void CMultiWeb2Factory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("MultiWeb2Dimensions"),1.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CMultiWeb2Factory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
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
      if ( pLoad->BeginUnit(_T("MultiWeb2Dimensions")) )
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

bool CMultiWeb2Factory::IsPrismatic(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   return true;
}

Float64 CMultiWeb2Factory::GetVolume(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   pgsTypes::SectionPropertyMode spMode = pSectProp->GetSectionPropertiesMode();

   GET_IFACE2(pBroker,IIntervals,pIntervals);
   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(segmentKey);

   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_SECTCHANGE,POIFIND_OR) );
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

Float64 CMultiWeb2Factory::GetSurfaceArea(IBroker* pBroker,const CSegmentKey& segmentKey,bool bReduceForPoorlyVentilatedVoids)
{
   // compute surface area along length of member
   GET_IFACE2(pBroker,ISectionProperties,pSectProp);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   std::vector<pgsPointOfInterest> vPOI( pPOI->GetPointsOfInterest(segmentKey,POI_SECTCHANGE,POIFIND_OR) );
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

std::_tstring CMultiWeb2Factory::GetImage()
{
   return std::_tstring(_T("MultiWeb2.gif"));
}

std::_tstring CMultiWeb2Factory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("DoubleTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("DoubleTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CMultiWeb2Factory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("+Mn_DoubleTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("+Mn_DoubleTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CMultiWeb2Factory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("-Mn_DoubleTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("-Mn_DoubleTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CMultiWeb2Factory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("Vn_DoubleTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("Vn_DoubleTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CMultiWeb2Factory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("DoubleTee_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("DoubleTee_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring CMultiWeb2Factory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("DoubleTee_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("DoubleTee_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CMultiWeb2Factory::GetCLSID()
{
   return CLSID_MultiWeb2Factory;
}

CLSID CMultiWeb2Factory::GetFamilyCLSID()
{
   return CLSID_RibbedBeamFamily;
}

std::_tstring CMultiWeb2Factory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CMultiWeb2Factory::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

HINSTANCE CMultiWeb2Factory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CMultiWeb2Factory::GetImageResourceName()
{
   return _T("MultiWeb2");
}

HICON  CMultiWeb2Factory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_MULTIWEB2) );
}

void CMultiWeb2Factory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                      Float64& h1,Float64& h2,Float64& h3,
                                      Float64& t1,Float64& t2,Float64& t3,
                                      Float64& f1,
                                      Float64& c1,Float64& c2,
                                      Float64& w2,Float64& wmin,Float64& wmax)
{
   c1 = GetDimension(dimensions,_T("C1"));
   c2 = GetDimension(dimensions,_T("C2"));
   h1 = GetDimension(dimensions,_T("H1"));
   h2 = GetDimension(dimensions,_T("H2"));
   h3 = GetDimension(dimensions,_T("H3"));
   t1 = GetDimension(dimensions,_T("T1"));
   t2 = GetDimension(dimensions,_T("T2"));
   t3 = GetDimension(dimensions,_T("T3"));
   f1 = GetDimension(dimensions,_T("F1"));
   w2 = GetDimension(dimensions,_T("W2"));
   wmin = GetDimension(dimensions,_T("Wmin"));
   wmax = GetDimension(dimensions,_T("Wmax"));
}

Float64 CMultiWeb2Factory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name)
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

pgsTypes::SupportedDeckTypes CMultiWeb2Factory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
{
   pgsTypes::SupportedDeckTypes sdt;
   switch(sbs)
   {
   case pgsTypes::sbsConstantAdjacent:
      sdt.push_back(pgsTypes::sdtCompositeOverlay);
      sdt.push_back(pgsTypes::sdtNone);
      break;

   default:
      ATLASSERT(false);
   }
   return sdt;
}

pgsTypes::SupportedBeamSpacings CMultiWeb2Factory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsConstantAdjacent);
   return sbs;
}

void CMultiWeb2Factory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 T1 = GetDimension(dimensions,_T("T1"));
   Float64 T2 = GetDimension(dimensions,_T("T2"));
   Float64 T3 = GetDimension(dimensions,_T("T3"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 Wmin = GetDimension(dimensions,_T("Wmin"));
   Float64 Wmax = GetDimension(dimensions,_T("Wmax"));

   Float64 mid_width = W2 + 2.0*(T1 + T2 + T3);

   Float64 gw_min =  mid_width + 2.0 * Wmin;
   Float64 gw_max =  mid_width + 2.0 * Wmax;

   if ( sdt == pgsTypes::sdtNone || sdt == pgsTypes::sdtCompositeOverlay )
   {
      if ( sbs == pgsTypes::sbsConstantAdjacent )
      {
         *minSpacing = gw_min;
         *maxSpacing = gw_max;
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

WebIndexType CMultiWeb2Factory::GetWebCount(const IBeamFactory::Dimensions& dimensions)
{
   return 2;
}

Float64 CMultiWeb2Factory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   Float64 H1 = GetDimension(dimensions,_T("H1"));
   Float64 H2 = GetDimension(dimensions,_T("H2"));
   Float64 H3 = GetDimension(dimensions,_T("H3"));

   return H1 + H2 + H3;
}

Float64 CMultiWeb2Factory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   Float64 T1 = GetDimension(dimensions,_T("T1"));
   Float64 T2 = GetDimension(dimensions,_T("T2"));
   Float64 T3 = GetDimension(dimensions,_T("T3"));

   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 Wmax = GetDimension(dimensions,_T("Wmax"));

   return 2*(T1+T2+T3+Wmax) + W2;
}


bool CMultiWeb2Factory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CMultiWeb2Factory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

GirderIndexType CMultiWeb2Factory::GetMinimumBeamCount()
{
   return 1;
}
