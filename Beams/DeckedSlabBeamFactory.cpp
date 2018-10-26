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

// DeckedSlabBeamFactory.cpp : Implementation of CDeckedSlabBeamFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "DeckedSlabBeamFactory.h"
#include "IBeamDistFactorEngineer.h"
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
#include <IFace\StatusCenter.h>
#include <IFace\Intervals.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDeckedSlabBeamFactory
HRESULT CDeckedSlabBeamFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("A"));
   m_DimNames.push_back(_T("B"));
   m_DimNames.push_back(_T("C"));
   m_DimNames.push_back(_T("F"));
   m_DimNames.push_back(_T("W"));
   m_DimNames.push_back(_T("Tt"));
   m_DimNames.push_back(_T("Tb"));
   m_DimNames.push_back(_T("Jmax"));
   m_DimNames.push_back(_T("EndBlockLength"));

   m_DefaultDims.push_back(::ConvertToSysUnits(77.75,unitMeasure::Inch)); // A
   m_DefaultDims.push_back(::ConvertToSysUnits(9.0,unitMeasure::Inch));   // B
   m_DefaultDims.push_back(::ConvertToSysUnits(12.0,unitMeasure::Inch));  // C
   m_DefaultDims.push_back(::ConvertToSysUnits(1.75,unitMeasure::Inch));  // F
   m_DefaultDims.push_back(::ConvertToSysUnits(6.0,unitMeasure::Inch));   // W
   m_DefaultDims.push_back(::ConvertToSysUnits(8.0,unitMeasure::Inch));   // Tt
   m_DefaultDims.push_back(::ConvertToSysUnits(7.0,unitMeasure::Inch));   // Tb
   m_DefaultDims.push_back(::ConvertToSysUnits(1.0,unitMeasure::Inch));   // Max Joint Spacing
   m_DefaultDims.push_back(::ConvertToSysUnits(36.0,unitMeasure::Inch));  // End Block Length

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // A 
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // B
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // F
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Tt
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Tb
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Max joint size
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // End Block Length

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // A 
   m_DimUnits[1].push_back(&unitMeasure::Inch); // B
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C
   m_DimUnits[1].push_back(&unitMeasure::Inch); // F
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Tt
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Tb
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Max joint size
   m_DimUnits[1].push_back(&unitMeasure::Inch); // End Block Length

   return S_OK;
}

void CDeckedSlabBeamFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection)
{
   CComPtr<IDeckedSlabBeamSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_DeckedSlabBeamSection);
   CComPtr<IDeckedSlabBeam> beam;
   gdrsection->get_Beam(&beam);

   Float64 A,B,C,F,W,Tt,Tb,J,EndBlockLength;
   GetDimensions(dimensions,A,B,C,F,W,Tt,Tb,J,EndBlockLength);

   beam->put_A(A);
   beam->put_B(B);
   beam->put_C(C);
   beam->put_F(F);
   beam->put_W(W);
   beam->put_Tt(Tt);
   beam->put_Tb(Tb);

   gdrsection.QueryInterface(ppSection);
}

void CDeckedSlabBeamFactory::CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetSegmentLength(segmentKey);

   Float64 height = GetDimension(dimensions,_T("C")) + GetDimension(dimensions,_T("Tt"));

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

void CDeckedSlabBeamFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMember* ssmbr)
{
   CComPtr<IDeckedSlabBeamEndBlockSegment> segment;
   segment.CoCreateInstance(CLSID_DeckedSlabBeamEndBlockSegment);

   // Build up the beam shape
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   segment->put_EndBlockLength(etStart,endBlockLength);
   segment->put_EndBlockLength(etEnd,endBlockLength);

   CComPtr<IGirderSection> gdrsection;
   CreateGirderSection(pBroker,statusGroupID,dimensions,-1,-1,&gdrsection);
   CComQIPtr<IDeckedSlabBeamSection> section(gdrsection);

   // if this is an exterior girder, remove the shear key block outs
   CComPtr<IDeckedSlabBeam> deckedSlabShape;
   section->get_Beam(&deckedSlabShape);
   if ( segmentKey.girderIndex == 0 )
   {
      deckedSlabShape->put_LeftBlockOut(VARIANT_FALSE);
   }

   if ( segmentKey.girderIndex == pGroup->GetGirderCount()-1 )
   {
      deckedSlabShape->put_RightBlockOut(VARIANT_FALSE);
   }

   // Beam materials
   GET_IFACE2(pBroker,IIntervals,pIntervals);
   GET_IFACE2(pBroker,IMaterials,pMaterial);
   CComPtr<IMaterial> material;
   material.CoCreateInstance(CLSID_Material);

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 E = pMaterial->GetSegmentEc(segmentKey,intervalIdx);
      Float64 D = pMaterial->GetSegmentWeightDensity(segmentKey,intervalIdx);

      material->put_E(intervalIdx,E);
      material->put_Density(intervalIdx,D);
   }

   CComQIPtr<IShape> shape(section);
   segment->AddShape(shape,material,NULL);

   ssmbr->AddSegment(segment);
}

void CDeckedSlabBeamFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);

   // put section breaks just on either side of the end blocks/void interface
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   Float64 endBlockLength = pGdrEntry->GetDimension(_T("EndBlockLength"));

   if ( !IsZero(endBlockLength) )
   {
      Float64 delta = 1.5*pPoiMgr->GetTolerance();

      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,endBlockLength,       POI_SECTCHANGE_LEFTFACE  ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,endBlockLength+delta, POI_SECTCHANGE_RIGHTFACE ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,gdrLength - (endBlockLength+delta), POI_SECTCHANGE_LEFTFACE  ) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(segmentKey,gdrLength - endBlockLength,         POI_SECTCHANGE_RIGHTFACE ) );
   }
}

void CDeckedSlabBeamFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   CComObject<CMultiWebDistFactorEngineer>* pEngineer;
   CComObject<CMultiWebDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);

   pEngineer->SetBeamType(CMultiWebDistFactorEngineer::btDeckedSlabBeam);

   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CDeckedSlabBeamFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng)
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
      pEngineer->Init(SingleT);
      pEngineer->SetBroker(pBroker,statusGroupID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
}

static void MakeRectangle(Float64 width, Float64 depth, Float64 xOffset, IShape** shape)
{
   CComPtr<IRectangle> harp_rect;
   HRESULT hr = harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   harp_rect->put_Width(width);
   harp_rect->put_Height(depth);

   Float64 hook_offset = 0.0;

   CComPtr<IPoint2d> hook;
   hook.CoCreateInstance(CLSID_Point2d);
   hook->Move(xOffset, depth/2.0);

   harp_rect->putref_HookPoint(hook);

   harp_rect->get_Shape(shape);
}

void CDeckedSlabBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover)
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   // set the shapes for harped strand bounds - only in the thinest part of the webs
   Float64 A,B,C,F,W,Tt,Tb,J,EndBlockLength;
   GetDimensions(dimensions,A,B,C,F,W,Tt,Tb,J,EndBlockLength);

   Float64 width = W;
   Float64 depth = C + Tt;
   Float64 bf_wid = A - 2*B;

   CComPtr<IRectangle> lft_harp_rect, rgt_harp_rect;
   hr = lft_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));
   hr = rgt_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   lft_harp_rect->put_Width(width);
   lft_harp_rect->put_Height(depth);
   rgt_harp_rect->put_Width(width);
   rgt_harp_rect->put_Height(depth);

   Float64 hook_offset = bf_wid/2.0 - W/2.0;

   CComPtr<IPoint2d> lft_hook, rgt_hook;
   lft_hook.CoCreateInstance(CLSID_Point2d);
   rgt_hook.CoCreateInstance(CLSID_Point2d);

   lft_hook->Move(-hook_offset, -depth/2.0);
   rgt_hook->Move( hook_offset, -depth/2.0);

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

   hr = configurer->SetHarpedStrandOffsetBounds(0, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));}

std::vector<std::_tstring> CDeckedSlabBeamFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<Float64> CDeckedSlabBeamFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CDeckedSlabBeamFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CDeckedSlabBeamFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSI,std::_tstring* strErrMsg)
{
   Float64 A,B,C,F,W,Tt,Tb,J,EndBlockLength;
   GetDimensions(dimensions,A,B,C,F,W,Tt,Tb,J,EndBlockLength);

   if ( B <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("B must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("W must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( A < 2*(B+W) )
   {
      std::_tostringstream os;
      os << _T("A must be greater or equal to 2*(B+W)") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( C <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("C must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( Tt <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("Tt must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( C < Tb )
   {
      std::_tostringstream os;
      os << _T("C must be a greater or equal to Tb") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F < 0.0 )
   {
      std::_tostringstream os;
      os << _T("F must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( J < 0.0 )
   {
      std::_tostringstream os;
      os << _T("Maximum joint size must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   return true;
}

void CDeckedSlabBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("DeckedSlabBeamDimensions"),2.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CDeckedSlabBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
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
      if ( pLoad->BeginUnit(_T("DeckedSlabBeamDimensions")) )
         dimVersion = pLoad->GetVersion();
      else
         THROW_LOAD(InvalidFileFormat,pLoad);
   }

   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      dimensions.push_back( std::make_pair(name,value) );
   }

   if ( 14 <= parent_version && !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   return dimensions;
}

bool CDeckedSlabBeamFactory::IsPrismatic(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));

   return IsZero(endBlockLength) ? true : false;
}

Float64 CDeckedSlabBeamFactory::GetInternalSurfaceAreaOfVoids(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetSegmentLength(segmentKey);

   // void perimeter
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 A = GetDimension(dimensions,_T("A"));
   Float64 B = GetDimension(dimensions,_T("B"));
   Float64 C = GetDimension(dimensions,_T("C"));
   Float64 W = GetDimension(dimensions,_T("W"));
   Float64 Tb = GetDimension(dimensions,_T("Tb"));
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));

   Float64 void_perimeter = (A-2*(B+W)) * (C-Tb);

   Float64 void_surface_area = (Lg-2*endBlockLength)*void_perimeter;

   return void_surface_area;
}

std::_tstring CDeckedSlabBeamFactory::GetImage()
{
   return std::_tstring(_T("DeckedSlabBeam.gif"));
}

std::_tstring CDeckedSlabBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage = _T("DeckedSlabBeam_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage = _T("DeckedSlabBeam_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CDeckedSlabBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("+Mn_DeckedSlabBeam_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("+Mn_DeckedSlabBeam_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CDeckedSlabBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("-Mn_DeckedSlabBeam_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("-Mn_DeckedSlabBeam_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CDeckedSlabBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("Vn_DeckedSlabBeam_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("Vn_DeckedSlabBeam_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CDeckedSlabBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("DeckedSlabBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("DeckedSlabBeam_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring CDeckedSlabBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("DeckedSlabBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("DeckedSlabBeam_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CDeckedSlabBeamFactory::GetCLSID()
{
   return CLSID_DeckedSlabBeamFactory;
}

std::_tstring CDeckedSlabBeamFactory::GetName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CDeckedSlabBeamFactory::GetFamilyCLSID()
{
   return CLSID_DeckedSlabBeamFamily;
}

std::_tstring CDeckedSlabBeamFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CDeckedSlabBeamFactory::GetPublisher()
{
   return std::_tstring(_T("TxDOT"));
}

HINSTANCE CDeckedSlabBeamFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CDeckedSlabBeamFactory::GetImageResourceName()
{
   return _T("DECKEDSLABBEAM");
}

HICON  CDeckedSlabBeamFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DECKEDSLAB) );
}

void CDeckedSlabBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                      Float64& A,Float64& B,Float64& C,Float64& F,Float64& W,Float64& Tt,Float64& Tb,Float64& Jmax,Float64& EndBlockLength)
{
   A = GetDimension(dimensions,_T("A"));
   B = GetDimension(dimensions,_T("B"));
   C = GetDimension(dimensions,_T("C"));
   W = GetDimension(dimensions,_T("W"));
   Tt = GetDimension(dimensions,_T("Tt"));
   Tb = GetDimension(dimensions,_T("Tb"));
   F  = GetDimension(dimensions,_T("F"));
   Jmax = GetDimension(dimensions,_T("Jmax"));
   EndBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
}

Float64 CDeckedSlabBeamFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,
                                        const std::_tstring& name)
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

pgsTypes::SupportedDeckTypes CDeckedSlabBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
{
   pgsTypes::SupportedDeckTypes sdt;
   switch(sbs)
   {
   case pgsTypes::sbsUniformAdjacent:
   case pgsTypes::sbsGeneralAdjacent:
      sdt.push_back(pgsTypes::sdtCompositeOverlay);
      sdt.push_back(pgsTypes::sdtNone);
      break;

   default:
      ATLASSERT(false);
   }
   return sdt;
}

pgsTypes::SupportedBeamSpacings CDeckedSlabBeamFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);

   return sbs;
}

pgsTypes::SupportedDiaphragmTypes CDeckedSlabBeamFactory::GetSupportedDiaphragms()
{
   // only supports adjacent spacing so there can only be precast diaphragms
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CDeckedSlabBeamFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type)
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

void CDeckedSlabBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 A = GetDimension(dimensions,_T("A"));
   Float64 J  = GetDimension(dimensions,_T("Jmax"));

   if ( sdt == pgsTypes::sdtCompositeOverlay || sdt == pgsTypes::sdtNone )
   {
      if(sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent)
      {
         *minSpacing = A;
         *maxSpacing = A+J;
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

WebIndexType CDeckedSlabBeamFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions)
{
   return 2;
}

Float64 CDeckedSlabBeamFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("C")) + GetDimension(dimensions,_T("Tt"));
}

Float64 CDeckedSlabBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("A"));
}

bool CDeckedSlabBeamFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CDeckedSlabBeamFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

GirderIndexType CDeckedSlabBeamFactory::GetMinimumBeamCount()
{
   return 1;
}
