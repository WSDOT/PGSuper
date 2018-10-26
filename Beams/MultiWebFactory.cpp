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

// MultiWebFactory.cpp : Implementation of CMultiWebFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "MultiWebFactory.h"
#include "MultiWebDistFactorEngineer.h"
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

#include <PgsExt\BridgeDescription2.h>

#include <IFace\StatusCenter.h>
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CMultiWebFactory
HRESULT CMultiWebFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("D1"));
   m_DimNames.push_back(_T("D2"));
   m_DimNames.push_back(_T("T1"));
   m_DimNames.push_back(_T("T2"));
   m_DimNames.push_back(_T("W1"));
   m_DimNames.push_back(_T("Wmax"));
   m_DimNames.push_back(_T("Wmin"));

   std::sort(m_DimNames.begin(),m_DimNames.end());

   // Default beam is a 4' wide tri-beam
   m_DefaultDims.push_back(::ConvertToSysUnits( 6.0,unitMeasure::Inch)); // D1
   m_DefaultDims.push_back(::ConvertToSysUnits(21.0,unitMeasure::Inch)); // D2
   m_DefaultDims.push_back(::ConvertToSysUnits(7.25,unitMeasure::Inch)); // T1
   m_DefaultDims.push_back(::ConvertToSysUnits(5.25,unitMeasure::Inch)); // T2
   m_DefaultDims.push_back(::ConvertToSysUnits(12.0,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Feet)); // Wmax
   m_DefaultDims.push_back(::ConvertToSysUnits(4.000,unitMeasure::Feet)); // Wmin


   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Meter);      // Wmax
   m_DimUnits[0].push_back(&unitMeasure::Meter);      // Wmin

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Feet); // Wmax
   m_DimUnits[1].push_back(&unitMeasure::Feet); // Wmin
   

   return S_OK;
}

void CMultiWebFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection)
{
   CComPtr<IMultiWebSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_MultiWebSection);
   CComPtr<IMultiWeb> beam;
   gdrsection->get_Beam(&beam);

   Float64 d1,d2;
   Float64 w,wmin,wmax;
   Float64 t1,t2;
   WebIndexType nWebs;
   GetDimensions(dimensions,d1,d2,w,wmin,wmax,t1,t2,nWebs);

   beam->put_W2(w);
   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_T1(t1);
   beam->put_T2(t2);
   beam->put_WebCount(nWebs);

   // figure out the overhang, w1, based on the spacing
   Float64 w1;
   if ( pBroker == NULL )
   {
      // just use the max
      w1 = (wmax - nWebs*t1 - (nWebs-1)*w)/2;
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
      Float64 spacing = pBridgeDesc->GetGirderSpacing();

      // if this is a fixed width section, then set the spacing equal to the width
      if ( IsEqual(wmax,wmin) )
         spacing = wmax;

      w1 = (spacing - nWebs*t1 - (nWebs-1)*w)/2;
   }
   beam->put_W1(w1);

   // origin of multi-web is top center... we need to reposition it so that bottom center is at (0,0)
   CComQIPtr<IXYPosition> position(beam);
   CComPtr<IPoint2d> tc;
   position->get_LocatorPoint(lpTopCenter,&tc);

   CComPtr<IPoint2d> bc;
   position->get_LocatorPoint(lpBottomCenter,&bc);

   position->MoveEx(bc,tc);

   gdrsection.QueryInterface(ppSection);
}

void CMultiWebFactory::CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetSegmentLength(segmentKey);

   Float64 d1,d2;
   Float64 w,wmin,wmax;
   Float64 t1,t2;
   WebIndexType nWebs;
   GetDimensions(dimensions,d1,d2,w,wmin,wmax,t1,t2,nWebs);

   Float64 height = d1 + d2;

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

void CMultiWebFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,IStages* pStages,ISuperstructureMember* ssmbr)
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
      Float64 E = pMaterial->GetSegmentAgeAdjustedEc(segmentKey,intervalIdx);
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

void CMultiWebFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr)
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);
}

void CMultiWebFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   CComObject<CMultiWebDistFactorEngineer>* pEngineer;
   CComObject<CMultiWebDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);

   pEngineer->SetBeamType(CMultiWebDistFactorEngineer::btMultiWebTee);

   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CMultiWebFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng)
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

void CMultiWebFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover)
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   // set the shapes for harped strand bounds - only in the thinest part of the webs
   Float64 d1,d2;
   Float64 w,wmin,wmax;
   Float64 t1,t2;
   WebIndexType nWebs;
   GetDimensions(dimensions,d1,d2,w,wmin,wmax,t1,t2,nWebs);
   ATLASSERT(nWebs==3);

   Float64 width = Min(t1,t2);
   Float64 depth = d1 + d2;

   CComPtr<IRectangle> lft_harp_rect, rgt_harp_rect, ctr_harp_rect;
   hr = lft_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));
   hr = rgt_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));
   hr = ctr_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   lft_harp_rect->put_Width(width);
   lft_harp_rect->put_Height(depth);
   rgt_harp_rect->put_Width(width);
   rgt_harp_rect->put_Height(depth);
   ctr_harp_rect->put_Width(width);
   ctr_harp_rect->put_Height(depth);

   Float64 hook_offset = w + t1;

   CComPtr<IPoint2d> lft_hook, rgt_hook, ctr_hook;
   lft_hook.CoCreateInstance(CLSID_Point2d);
   rgt_hook.CoCreateInstance(CLSID_Point2d);
   ctr_hook.CoCreateInstance(CLSID_Point2d);

   lft_hook->Move(-hook_offset, depth/2.0);
   rgt_hook->Move( hook_offset, depth/2.0);
   ctr_hook->Move(         0.0, depth/2.0);

   lft_harp_rect->putref_HookPoint(lft_hook);
   rgt_harp_rect->putref_HookPoint(rgt_hook);
   ctr_harp_rect->putref_HookPoint(ctr_hook);

   CComPtr<IShape> lft_shape, rgt_shape, ctr_shape;
   lft_harp_rect->get_Shape(&lft_shape);
   rgt_harp_rect->get_Shape(&rgt_shape);
   ctr_harp_rect->get_Shape(&ctr_shape);

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(lft_shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));
   hr = configurer->AddRegion(rgt_shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));
   hr = configurer->AddRegion(ctr_shape, 0.0);
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

std::vector<std::_tstring> CMultiWebFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<Float64> CMultiWebFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CMultiWebFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CMultiWebFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg)
{
   Float64 d1,d2;
   Float64 w1,wmin,wmax;
   Float64 t1,t2;
   WebIndexType nWebs;
   GetDimensions(dimensions,d1,d2,w1,wmin,wmax,t1,t2,nWebs);

 // D1    0
 // D2    1
 // T1    2
 // T2    3
 // W1    4
 // Wmax  5
 // Wmin  6
   
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

   if ( w1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][4];
      std::_tostringstream os;
      os << _T("W1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   
   if ( t1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][2];
      std::_tostringstream os;
      os << _T("T1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( t2 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][3];
      std::_tostringstream os;
      os << _T("T2 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   Float64 web_wid = nWebs*t1 + (nWebs-1)*w1;
   if (wmin < web_wid)
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][5];
      std::_tostringstream os;
      os << _T("Wmin must be greater than the width of the webs, which is ")<< ::ConvertFromSysUnits(web_wid,*pUnit) << _T(" ") << pUnit->UnitTag()<< std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if (wmin > wmax)
   {
      std::_tostringstream os;
      os << _T("Wmin must be less than or equal to Wmax") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   return true;
}

void CMultiWebFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("MultiWebDimensions"),1.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CMultiWebFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version;
   if ( pLoad->GetParentUnit() == _T("GirderLibraryEntry") )
      parent_version = pLoad->GetParentVersion();
   else
      parent_version = pLoad->GetVersion();


   IBeamFactory::Dimensions dimensions;

   if ( 14 <= parent_version && !pLoad->BeginUnit(_T("MultiWebDimensions")) )
      THROW_LOAD(InvalidFileFormat,pLoad);

   std::vector<std::_tstring>::iterator iter;
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value;
      pLoad->Property(name.c_str(),&value);
      dimensions.push_back( std::make_pair(name,value) );
   }

   if ( 14 <= parent_version && !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   return dimensions;
}

bool CMultiWebFactory::IsPrismatic(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   return true;
}

Float64 CMultiWebFactory::GetVolume(IBroker* pBroker,const CSegmentKey& segmentKey)
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

Float64 CMultiWebFactory::GetSurfaceArea(IBroker* pBroker,const CSegmentKey& segmentKey,bool bReduceForPoorlyVentilatedVoids)
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

std::_tstring CMultiWebFactory::GetImage()
{
   return std::_tstring(_T("MultiWeb.jpg"));
}

std::_tstring CMultiWebFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("TripleTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("TripleTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CMultiWebFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("+Mn_TripleTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("+Mn_TripleTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CMultiWebFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("-Mn_TripleTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("-Mn_TripleTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CMultiWebFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("Vn_TripleTee_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("Vn_TripleTee_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CMultiWebFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("TripleTee_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("TripleTee_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring CMultiWebFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("TripleTee_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("TripleTee_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CMultiWebFactory::GetCLSID()
{
   return CLSID_MultiWebFactory;
}

CLSID CMultiWebFactory::GetFamilyCLSID()
{
   return CLSID_RibbedBeamFamily;
}

std::_tstring CMultiWebFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CMultiWebFactory::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

HINSTANCE CMultiWebFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CMultiWebFactory::GetImageResourceName()
{
   return _T("MultiWeb");
}

HICON  CMultiWebFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_MULTIWEB) );
}

void CMultiWebFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                  Float64& d1,Float64& d2,
                                  Float64& w,Float64& wmin,Float64& wmax,
                                  Float64& t1,Float64& t2,
                                  WebIndexType& nWebs)
{
   d1 = GetDimension(dimensions,_T("D1"));
   d2 = GetDimension(dimensions,_T("D2"));
   w  = GetDimension(dimensions,_T("W1"));
   wmin = GetDimension(dimensions,_T("Wmin"));
   wmax = GetDimension(dimensions,_T("Wmax"));
   t1 = GetDimension(dimensions,_T("T1"));
   t2 = GetDimension(dimensions,_T("T2"));
   nWebs = 3;
}

Float64 CMultiWebFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name)
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

pgsTypes::SupportedDeckTypes CMultiWebFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CMultiWebFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsConstantAdjacent);
   return sbs;
}

pgsTypes::SupportedDiaphragmTypes CMultiWebFactory::GetSupportedDiaphragms()
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CMultiWebFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type)
{
   pgsTypes::SupportedDiaphragmLocationTypes locations;
   switch(type)
   {
   case pgsTypes::dtPrecast :
      locations.push_back(pgsTypes::dltInternal);
      break;

   case pgsTypes::dtCastInPlace :
      locations.push_back(pgsTypes::dltExternal);
      break;

   default:
      ATLASSERT(false);
   }

   return locations;
}

void CMultiWebFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 gw_min = GetDimension(dimensions,_T("Wmin"));
   Float64 gw_max = GetDimension(dimensions,_T("Wmax"));

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

WebIndexType CMultiWebFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions)
{
   return 3;
}

Float64 CMultiWebFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   Float64 D1 = GetDimension(dimensions,_T("D1"));
   Float64 D2 = GetDimension(dimensions,_T("D2"));

   return D1 + D2;
}

Float64 CMultiWebFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("Wmax"));
}

bool CMultiWebFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CMultiWebFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

GirderIndexType CMultiWebFactory::GetMinimumBeamCount()
{
   return 1;
}
