///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include "TxDOTSpreadSlabBeamDistFactorEngineer.h"
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

#include <IFace\AgeAdjustedMaterial.h>
#include <Beams\Helper.h>

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
   m_DimNames.emplace_back(_T("H"));
   m_DimNames.emplace_back(_T("W"));
   m_DimNames.emplace_back(_T("Void_Diameter"));
   m_DimNames.emplace_back(_T("Void_Spacing"));
   m_DimNames.emplace_back(_T("Number_of_Voids"));
   m_DimNames.emplace_back(_T("Jmax"));

   m_DefaultDims.emplace_back(::ConvertToSysUnits(18.0,unitMeasure::Inch)); // H
   m_DefaultDims.emplace_back(::ConvertToSysUnits(48.0,unitMeasure::Inch)); // W
   m_DefaultDims.emplace_back(::ConvertToSysUnits(10.0,unitMeasure::Inch)); // Void Diameter
   m_DefaultDims.emplace_back(::ConvertToSysUnits(12.5,unitMeasure::Inch)); // Void Spacing
   m_DefaultDims.emplace_back(3);                                           // Number of Voids
   m_DefaultDims.emplace_back(::ConvertToSysUnits(1.0,unitMeasure::Inch));  // Max Joint Spacing

   // SI Units
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // H 
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // Void Diameter
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // Void Spacing
   m_DimUnits[0].emplace_back(nullptr);                     // Number of Voids
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // Max joint size

   // US Units
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // H 
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // Void Diameter
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // Void Spacing
   m_DimUnits[1].emplace_back(nullptr);               // Number of Voids
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // Max joint size

   return S_OK;
}

void CVoidedSlabFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   CComPtr<IVoidedSlabSection> gdrSection;
   gdrSection.CoCreateInstance(CLSID_VoidedSlabSection);
   CComPtr<IVoidedSlab> beam;
   gdrSection->get_Beam(&beam);

   DimensionAndPositionBeam(dimensions, beam);

   gdrSection.QueryInterface(ppSection);
}

void CVoidedSlabFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
{
   CComPtr<IPrismaticSuperstructureMemberSegment> segment;
   segment.CoCreateInstance(CLSID_PrismaticSuperstructureMemberSegment);

   // Build up the beam shape
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment    = pGirder->GetSegment(segmentKey.segmentIndex);

   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   CComPtr<IGirderSection> gdrSection;
   CreateGirderSection(pBroker,statusGroupID,dimensions,-1,-1,&gdrSection);

   // Beam materials
   GET_IFACE2(pBroker,ILossParameters,pLossParams);
   CComPtr<IMaterial> material;
   if ( pLossParams->GetLossMethod() == pgsTypes::TIME_STEP )
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

   CComQIPtr<IShape> shape(gdrSection);
   ATLASSERT(shape);
   segment->AddShape(shape,material,nullptr);

   CComQIPtr<ISuperstructureMemberSegment> ssmbrSegment(segment);
   ssmbrSegment.CopyTo(ppSegment);
}

void CVoidedSlabFactory::CreateSegmentShape(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   CComPtr<IVoidedSlab> beam;
   beam.CoCreateInstance(CLSID_VoidedSlab);

   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   DimensionAndPositionBeam(dimensions, beam);

   beam.QueryInterface(ppShape);
}

Float64 CVoidedSlabFactory::GetSegmentHeight(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   Float64 H = GetDimension(dimensions, _T("H"));
   return H;
}

void CVoidedSlabFactory::ConfigureSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void CVoidedSlabFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);
}

void CVoidedSlabFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng) const
{
   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   // use passed value if not null
   pgsTypes::SupportedDeckType deckType = (pDeckType!=nullptr) ? *pDeckType : pDeck->GetDeckType();
   pgsTypes::SupportedBeamSpacing spacingType = (pSpacingType!=nullptr) ? *pSpacingType : pBridgeDesc->GetGirderSpacingType();

   if (spacingType==pgsTypes::sbsUniformAdjacent || spacingType==pgsTypes::sbsGeneralAdjacent || spacingType==pgsTypes::sbsConstantAdjacent)
   {
      CComObject<CVoidedSlabDistFactorEngineer>* pEngineer;
      CComObject<CVoidedSlabDistFactorEngineer>::CreateInstance(&pEngineer);
      pEngineer->SetBroker(pBroker,statusGroupID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
   else
   {
      GET_IFACE2(pBroker, ILibrary,       pLib);
      GET_IFACE2(pBroker, ISpecification, pSpec);
      const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

      int lldf_method = pSpecEntry->GetLiveLoadDistributionMethod();
      if (lldf_method == LLDF_TXDOT)
      {
         CComObject<CTxDOTSpreadSlabBeamDistFactorEngineer>* pEngineer;
         CComObject<CTxDOTSpreadSlabBeamDistFactorEngineer>::CreateInstance(&pEngineer);
         pEngineer->SetBroker(pBroker, statusGroupID);
         (*ppEng) = pEngineer;
         (*ppEng)->AddRef();
      }
      else
      {
         // this is a type b section... type b's are the same as type c's which are U-beams
         ATLASSERT(deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP);

         CComObject<CUBeamDistFactorEngineer>* pEngineer;
         CComObject<CUBeamDistFactorEngineer>::CreateInstance(&pEngineer);
         pEngineer->Init(true, true); // this is a type b cross section, and a spread slab
         pEngineer->SetBroker(pBroker, statusGroupID);
         (*ppEng) = pEngineer;
         (*ppEng)->AddRef();
      }
   }
}

void CVoidedSlabFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng) const
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

void CVoidedSlabFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
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
   Float64 depth = (Hg < 0 ? H : Hg);

   if (N==0)
   {
      // easy part, no voids
      Float64 hook_offset = 0.0;

      CComPtr<IShape> shape;
      MakeRectangle(width, depth, hook_offset, 0.0, &shape);

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
      MakeRectangle(end_width, depth, -end_loc, 0.0, &shapel);
      MakeRectangle(end_width, depth,  end_loc, 0.0, &shaper);

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
         MakeRectangle(voids_w, depth, loc, 0.0, &shape);

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

   hr = configurer->SetHarpedStrandOffsetBounds(0, depth, endtb, endbb, hptb, hpbb, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}

const std::vector<std::_tstring>& CVoidedSlabFactory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& CVoidedSlabFactory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const unitLength*>& CVoidedSlabFactory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CVoidedSlabFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSI,std::_tstring* strErrMsg) const
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

void CVoidedSlabFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) const
{
   pSave->BeginUnit(_T("VoidedSlabDimensions"),2.0);
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CVoidedSlabFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad) const
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
      if (pLoad->BeginUnit(_T("VoidedSlabDimensions")))
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
      dimensions.emplace_back(name,value);
   }

   if (14 <= parent_version && !pLoad->EndUnit())
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   return dimensions;
}

bool CVoidedSlabFactory::IsPrismatic(const IBeamFactory::Dimensions& dimensions) const
{
   return true;
}

bool CVoidedSlabFactory::IsPrismatic(const CSegmentKey& segmentKey) const
{
   return true;
}

bool CVoidedSlabFactory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

Float64 CVoidedSlabFactory::GetInternalSurfaceAreaOfVoids(IBroker* pBroker,const CSegmentKey& segmentKey) const
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

std::_tstring CVoidedSlabFactory::GetImage() const
{
   return std::_tstring(_T("VoidedSlab.jpg"));
}

std::_tstring CVoidedSlabFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CVoidedSlabFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CVoidedSlabFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CVoidedSlabFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CVoidedSlabFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CVoidedSlabFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
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

CLSID CVoidedSlabFactory::GetCLSID() const
{
   return CLSID_VoidedSlabFactory;
}

std::_tstring CVoidedSlabFactory::GetName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CVoidedSlabFactory::GetFamilyCLSID() const
{
   return CLSID_SlabBeamFamily;
}

std::_tstring CVoidedSlabFactory::GetGirderFamilyName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CVoidedSlabFactory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CVoidedSlabFactory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CVoidedSlabFactory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR CVoidedSlabFactory::GetImageResourceName() const
{
   return _T("VOIDEDSLAB");
}

HICON  CVoidedSlabFactory::GetIcon()  const
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
                                       Float64& J) const
{
   H = GetDimension(dimensions,_T("H"));
   W = GetDimension(dimensions,_T("W"));
   D = GetDimension(dimensions,_T("Void_Diameter"));
   S = GetDimension(dimensions,_T("Void_Spacing"));
   N = (WebIndexType)GetDimension(dimensions,_T("Number_of_Voids"));
   J = GetDimension(dimensions,_T("Jmax"));
}

Float64 CVoidedSlabFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,
                                        const std::_tstring& name) const
{
   for ( const auto& dim : dimensions)
   {
      if (name == dim.first)
      {
         return dim.second;
      }
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedDeckTypes CVoidedSlabFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
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
      sdt.push_back(pgsTypes::sdtCompositeCIP);
      sdt.push_back(pgsTypes::sdtCompositeOverlay);
      //sdt.push_back(pgsTypes::sdtNonstructuralOverlay);
      sdt.push_back(pgsTypes::sdtNone);
      break;

   default:
      ATLASSERT(false);
   }
   return sdt;
}

pgsTypes::SupportedBeamSpacings CVoidedSlabFactory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

bool CVoidedSlabFactory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool CVoidedSlabFactory::ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   if (spacingType == pgsTypes::sbsUniform)
   {
      // we have uniform spacing, but can only get here if there no deck... we actually want uniform adjacent
      *pNewSpacingType = pgsTypes::sbsUniformAdjacent;
      *pNewSpacing = spacing;
      *pNewTopWidth = 0.0;
      return true;
   }
   else if (spacingType == pgsTypes::sbsGeneral)
   {
      // we have general spacing, but can only get here if there no deck... we actually want general adjacent
      *pNewSpacingType = pgsTypes::sbsGeneralAdjacent;
      *pNewSpacing = spacing;
      *pNewTopWidth = 0.0;
      return true;
   }
   return false;
}

std::vector<pgsTypes::GirderOrientationType> CVoidedSlabFactory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb, pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal };
   return types;
}

bool CVoidedSlabFactory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType CVoidedSlabFactory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes CVoidedSlabFactory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CVoidedSlabFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

void CVoidedSlabFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 gw = GetDimension(dimensions,_T("W"));
   Float64 J  = GetDimension(dimensions,_T("Jmax"));

   if(sbs == pgsTypes::sbsUniform || sbs == pgsTypes::sbsGeneral)
   {
      *minSpacing = gw;
      *maxSpacing = MAX_GIRDER_SPACING;
   }
   else if(sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent)
   {
      // for this spacing type, we have joint spacing... spacing range is the range of joint width
      *minSpacing = 0;
      *maxSpacing = J;
   }
   else
   {
      ATLASSERT(false); // shouldn't get here
   }
}

WebIndexType CVoidedSlabFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions) const
{
   long nv = (long)GetDimension(dimensions,_T("Number_of_Voids"));
   return nv+1;
}

Float64 CVoidedSlabFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("H"));
}

Float64 CVoidedSlabFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("W"));
}

bool CVoidedSlabFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void CVoidedSlabFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool CVoidedSlabFactory::HasLongitudinalJoints() const
{
   return false;
}

bool CVoidedSlabFactory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool CVoidedSlabFactory::HasTopFlangeThickening() const
{
   return false;
}

bool CVoidedSlabFactory::CanPrecamber() const
{
   return false;
}

GirderIndexType CVoidedSlabFactory::GetMinimumBeamCount() const
{
   return 1;
}

void CVoidedSlabFactory::DimensionAndPositionBeam(const IBeamFactory::Dimensions& dimensions, IVoidedSlab* pBeam) const
{
   Float64 H, W, D, S, J;
   WebIndexType N;
   GetDimensions(dimensions, H, W, D, S, N, J);

   pBeam->put_Height(H);
   pBeam->put_Width(W);
   pBeam->put_VoidDiameter(D);
   pBeam->put_VoidSpacing(S);
   pBeam->put_VoidCount(N);

   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_Height(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}
