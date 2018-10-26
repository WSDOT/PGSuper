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

// VoidedSlabFactory.cpp : Implementation of CVoidedSlabFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "VoidedSlabFactory.h"
#include "IBeamDistFactorEngineer.h"
#include "VoidedSlabDistFactorEngineer.h"
#include "UBeamDistFactorEngineer.h"
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
// CVoidedSlabFactory
HRESULT CVoidedSlabFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("H"));
   m_DimNames.push_back(_T("W"));
   m_DimNames.push_back(_T("Void_Diameter"));
   m_DimNames.push_back(_T("Void_Spacing"));
   m_DimNames.push_back(_T("Number_of_Voids"));
   m_DimNames.push_back(_T("Jmax"));

   m_DefaultDims.push_back(::ConvertToSysUnits(18.0,unitMeasure::Inch)); // H
   m_DefaultDims.push_back(::ConvertToSysUnits(48.0,unitMeasure::Inch)); // W
   m_DefaultDims.push_back(::ConvertToSysUnits(10.0,unitMeasure::Inch)); // Void Diameter
   m_DefaultDims.push_back(::ConvertToSysUnits(12.5,unitMeasure::Inch)); // Void Spacing
   m_DefaultDims.push_back(3);                                           // Number of Voids
   m_DefaultDims.push_back(::ConvertToSysUnits(1.0,unitMeasure::Inch));  // Max Joint Spacing

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H 
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Void Diameter
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Void Spacing
   m_DimUnits[0].push_back(NULL);                     // Number of Voids
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Max joint size

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H 
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Void Diameter
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Void Spacing
   m_DimUnits[1].push_back(NULL);               // Number of Voids
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Max joint size

   return S_OK;
}

void CVoidedSlabFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection)
{
   CComPtr<IVoidedSlabSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_VoidedSlabSection);
   CComPtr<IVoidedSlab> beam;
   gdrsection->get_Beam(&beam);

   Float64 H,W,D,S,J;
   WebIndexType N;
   GetDimensions(dimensions,H,W,D,S,N,J);

   beam->put_Height(H);
   beam->put_Width(W);
   beam->put_VoidDiameter(D);
   beam->put_VoidSpacing(S);
   beam->put_VoidCount(N);

   gdrsection.QueryInterface(ppSection);
}

void CVoidedSlabFactory::CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetSegmentLength(segmentKey);

   Float64 H,W,D,S,J;
   WebIndexType N;
   GetDimensions(dimensions,H,W,D,S,N,J);

   Float64 height = H;

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

void CVoidedSlabFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMember* ssmbr)
{
   CComPtr<IPrismaticSuperstructureMemberSegment> segment;
   segment.CoCreateInstance(CLSID_PrismaticSuperstructureMemberSegment);

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

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   for ( IntervalIndexType intervalIdx = 0; intervalIdx < nIntervals; intervalIdx++ )
   {
      Float64 E = pMaterial->GetSegmentEc(segmentKey,intervalIdx);
      Float64 D = pMaterial->GetSegmentWeightDensity(segmentKey,intervalIdx);

      material->put_E(intervalIdx,E);
      material->put_Density(intervalIdx,D);
   }

   CComQIPtr<IShape> shape(gdrsection);
   ATLASSERT(shape);
   segment->AddShape(shape,material,NULL);

   ssmbr->AddSegment(segment);
}

void CVoidedSlabFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr)
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);
}

void CVoidedSlabFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   GET_IFACE2(pBroker,IBridge,pBridge);

   // use passed value if not null
   pgsTypes::SupportedDeckType deckType = (pDeckType!=NULL) ? *pDeckType : pBridge->GetDeckType();
   
   if ( deckType == pgsTypes::sdtCompositeOverlay || deckType == pgsTypes::sdtNone )
   {
      CComObject<CVoidedSlabDistFactorEngineer>* pEngineer;
      CComObject<CVoidedSlabDistFactorEngineer>::CreateInstance(&pEngineer);
      pEngineer->SetBroker(pBroker,statusGroupID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
   else
   {
      // this is a type b section... type b's are the same as type c's which are U-beams
      ATLASSERT( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP );

      CComObject<CUBeamDistFactorEngineer>* pEngineer;
      CComObject<CUBeamDistFactorEngineer>::CreateInstance(&pEngineer);
      pEngineer->Init(true,true); // this is a type b cross section, and a spread slab
      pEngineer->SetBroker(pBroker,statusGroupID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
}

void CVoidedSlabFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng)
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
       
      // depends on # of voids
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(girderKey.groupIndex);
      const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(girderKey.girderIndex)->GetGirderLibraryEntry();

      Float64 nVoids = pGdrEntry->GetDimension(_T("Number_of_Voids"));

      if ( nVoids == 0 )
      {
         pEngineer->Init(SolidSlab);
      }
      else
      {
         pEngineer->Init(SingleT);
      }

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
   hook->Move(xOffset, -depth/2.0);

   harp_rect->putref_HookPoint(hook);

   harp_rect->get_Shape(shape);
}

void CVoidedSlabFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover)
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   CComQIPtr<IConfigureStrandMover> configurer(sm);

   // Set the shapes for harped strand bounds 
   // Voided slabs don't normally support harped strands, so the question
   Float64 H,W,D,S,J;
   WebIndexType N;
   GetDimensions(dimensions,H,W,D,S,N,J);

   Float64 width = W;
   Float64 depth = H;

   if (N==0)
   {
      // easy part, no voids
      Float64 hook_offset = 0.0;

      CComPtr<IShape> shape;
      MakeRectangle(width, depth, hook_offset, &shape);

      hr = configurer->AddRegion(shape, 0.0);
      ATLASSERT (SUCCEEDED(hr));
   }
   else
   {
      // multiple voids, put rectangles between them
      Float64 voids_w = (N-1)*S + D;
      Float64 end_width = (width-voids_w)/2.0;
      Float64 end_loc = (width-end_width)/2.0; 

      // rectangles at ends
      CComPtr<IShape> shapel, shaper;
      MakeRectangle(end_width, depth, -end_loc, &shapel);
      MakeRectangle(end_width, depth,  end_loc, &shaper);

      hr = configurer->AddRegion(shapel, 0.0);
      ATLASSERT (SUCCEEDED(hr));
      hr = configurer->AddRegion(shaper, 0.0);
      ATLASSERT (SUCCEEDED(hr));

      // retangles between voids
      voids_w = S - D;
      Float64 loc = -(end_loc - end_width/2.0 - D - voids_w/2.0);
      for(IndexType iv=0; iv<N-1; iv++)
      {

         CComPtr<IShape> shape;
         MakeRectangle(voids_w, depth, loc, &shape);

         hr = configurer->AddRegion(shape, 0.0);
         ATLASSERT (SUCCEEDED(hr));

         loc += S;
      }
   }

   // set vertical offset bounds and increments
   Float64 hptb  = hpTopFace     == IBeamFactory::BeamBottom ? hpTopLimit     - depth : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == IBeamFactory::BeamBottom ? hpBottomLimit  - depth : -hpBottomLimit;
   Float64 endtb = endTopFace    == IBeamFactory::BeamBottom ? endTopLimit    - depth : -endTopLimit;
   Float64 endbb = endBottomFace == IBeamFactory::BeamBottom ? endBottomLimit - depth : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(0, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}

std::vector<std::_tstring> CVoidedSlabFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<Float64> CVoidedSlabFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CVoidedSlabFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CVoidedSlabFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSI,std::_tstring* strErrMsg)
{
   Float64 H,W,D,S,J;
   WebIndexType N;
   GetDimensions(dimensions,H,W,D,S,N,J);

   if ( H <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("Height must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("Width must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( N < 0.0 )
   {
      std::_tostringstream os;
      os << _T("Invalid Number of Voids") << std::ends;
      *strErrMsg = os.str();
      return false;
   }
   else if (N == 0)
   {
      if ( D != 0.0 )
      {
         std::_tostringstream os;
         os << _T("Void Diameter Must Be Zero If No Voids") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( S != 0.0 )
      {
         std::_tostringstream os;
         os << _T("Invalid - Void Spacing Must Be Zero If No Voids") << std::ends;
         *strErrMsg = os.str();
         return false;
      }
   }
   else // (N > 0)
   {
      if ( D <= 0.0 )
      {
         std::_tostringstream os;
         os << _T("Void Diameter Must Be Greater Than Zero") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( D >= H )
      {
         std::_tostringstream os;
         os << _T("Void Diameter must be less than slab height") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if (N == 1)
      {
         if ( S != 0.0 )
         {
            std::_tostringstream os;
            os << _T("Invalid - Void Spacing Must Be Zero If Only One Void") << std::ends;
            *strErrMsg = os.str();
            return false;
         }

         if ( D >= W )
         {
            std::_tostringstream os;
            os << _T("Void Diameter must be less than slab width") << std::ends;
            *strErrMsg = os.str();
            return false;
         }

      }
      else // (N > 1)
      {
         if ( S < D )
         {
            std::_tostringstream os;
            os << _T("Void Spacing must be greater than Void Diameter") << std::ends;
            *strErrMsg = os.str();
            return false;
         }

         if ( W <= (N-1)*S + D)
         {
            std::_tostringstream os;
            os << _T("Slab must be wider than width occupied by voids") << std::ends;
            *strErrMsg = os.str();
            return false;
         }
      }
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

void CVoidedSlabFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("VoidedSlabDimensions"),2.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CVoidedSlabFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
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
      if ( pLoad->BeginUnit(_T("VoidedSlabDimensions")) )
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
         
         // if this is before dimension data block versio 2 and the
         // dimension is Jmax, the fail to read is expected
         if ( dimVersion < 2 && parent_version < 8.0 && name == _T("Jmax") )
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

bool CVoidedSlabFactory::IsPrismatic(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   return true;
}

Float64 CVoidedSlabFactory::GetInternalSurfaceAreaOfVoids(IBroker* pBroker,const CSegmentKey& segmentKey)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetSegmentLength(segmentKey);

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGdrEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   Float64 D = GetDimension(dimensions,_T("Void_Diameter"));
   long    N = (long)GetDimension(dimensions,_T("Number_of_Voids"));
   Float64 void_surface_area = Lg*N*M_PI*D;
   return void_surface_area;
}

std::_tstring CVoidedSlabFactory::GetImage()
{
   return std::_tstring(_T("VoidedSlab.jpg"));
}

std::_tstring CVoidedSlabFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      strImage = _T("VoidedSlab_Composite_CIP.gif");
      break;

   case pgsTypes::sdtCompositeSIP:
      strImage = _T("VoidedSlab_Composite_SIP.gif");
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage = _T("VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage = _T("VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlabFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("+Mn_SpreadVoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("+Mn_VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("+Mn_VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlabFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("-Mn_SpreadVoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("-Mn_VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("-Mn_VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlabFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("Vn_SpreadVoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("Vn_VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("Vn_VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlabFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
      {
         strImage =  _T("SpreadVoidedSlab_Effective_Flange_Width_Interior_Girder_2008.gif");
      }
      else
      {
         strImage =  _T("SpreadVoidedSlab_Effective_Flange_Width_Interior_Girder.gif");
      }
      break;

   case pgsTypes::sdtCompositeOverlay:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
      {
         return _T("VoidedSlab_Effective_Flange_Width_Interior_Girder_2008.gif");
      }
      else
      {
         return _T("VoidedSlab_Effective_Flange_Width_Interior_Girder.gif");
      }
      break;

   case pgsTypes::sdtNone:
   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlabFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
      {
         strImage =  _T("SpreadVoidedSlab_Effective_Flange_Width_Exterior_Girder_2008.gif");
      }
      else
      {
         strImage =  _T("SpreadVoidedSlab_Effective_Flange_Width_EXterior_Girder.gif");
      }
      break;

   case pgsTypes::sdtCompositeOverlay:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
      {
         return _T("VoidedSlab_Effective_Flange_Width_Exterior_Girder_2008.gif");
      }
      else
      {
         return _T("VoidedSlab_Effective_Flange_Width_Exterior_Girder.gif");
      }
      break;

   case pgsTypes::sdtNone:
   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

CLSID CVoidedSlabFactory::GetCLSID()
{
   return CLSID_VoidedSlabFactory;
}

std::_tstring CVoidedSlabFactory::GetName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CVoidedSlabFactory::GetFamilyCLSID()
{
   return CLSID_SlabBeamFamily;
}

std::_tstring CVoidedSlabFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CVoidedSlabFactory::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CVoidedSlabFactory::GetPublisherContactInformation()
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CVoidedSlabFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CVoidedSlabFactory::GetImageResourceName()
{
   return _T("VOIDEDSLAB");
}

HICON  CVoidedSlabFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_VOIDEDSLAB) );
}

void CVoidedSlabFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                       Float64& H,
                                       Float64& W,
                                       Float64& D,
                                       Float64& S,
                                       WebIndexType& N,
                                       Float64& J)
{
   H = GetDimension(dimensions,_T("H"));
   W = GetDimension(dimensions,_T("W"));
   D = GetDimension(dimensions,_T("Void_Diameter"));
   S = GetDimension(dimensions,_T("Void_Spacing"));
   N = (WebIndexType)GetDimension(dimensions,_T("Number_of_Voids"));
   J = GetDimension(dimensions,_T("Jmax"));
}

Float64 CVoidedSlabFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,
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

pgsTypes::SupportedDeckTypes CVoidedSlabFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
{
   pgsTypes::SupportedDeckTypes sdt;
   switch(sbs)
   {
   case pgsTypes::sbsUniform:
   case pgsTypes::sbsGeneral:
      sdt.push_back(pgsTypes::sdtCompositeCIP);
      sdt.push_back(pgsTypes::sdtCompositeSIP);
      break;

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

pgsTypes::SupportedBeamSpacings CVoidedSlabFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

pgsTypes::SupportedDiaphragmTypes CVoidedSlabFactory::GetSupportedDiaphragms()
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CVoidedSlabFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type)
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

void CVoidedSlabFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 gw = GetDimension(dimensions,_T("W"));
   Float64 J  = GetDimension(dimensions,_T("Jmax"));

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
      if (sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent)
      {
         if ( sdt == pgsTypes::sdtCompositeOverlay || sdt == pgsTypes::sdtNone )
         {
            *minSpacing = gw;
            *maxSpacing = gw+J;
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
}

WebIndexType CVoidedSlabFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions)
{
   long nv = (long)GetDimension(dimensions,_T("Number_of_Voids"));
   return nv+1;
}

Float64 CVoidedSlabFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("H"));
}

Float64 CVoidedSlabFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("W"));
}

bool CVoidedSlabFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CVoidedSlabFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

GirderIndexType CVoidedSlabFactory::GetMinimumBeamCount()
{
   return 1;
}
