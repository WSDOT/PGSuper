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
// CDeckedSlabBeamFactory
HRESULT CDeckedSlabBeamFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.emplace_back(_T("A"));
   m_DimNames.emplace_back(_T("B"));
   m_DimNames.emplace_back(_T("C"));
   m_DimNames.emplace_back(_T("F"));
   m_DimNames.emplace_back(_T("W"));
   m_DimNames.emplace_back(_T("Tt"));
   m_DimNames.emplace_back(_T("Tb"));
   m_DimNames.emplace_back(_T("Jmax"));
   m_DimNames.emplace_back(_T("EndBlockLength"));

   m_DefaultDims.emplace_back(::ConvertToSysUnits(77.75,unitMeasure::Inch)); // A
   m_DefaultDims.emplace_back(::ConvertToSysUnits(9.0,unitMeasure::Inch));   // B
   m_DefaultDims.emplace_back(::ConvertToSysUnits(12.0,unitMeasure::Inch));  // C
   m_DefaultDims.emplace_back(::ConvertToSysUnits(1.75,unitMeasure::Inch));  // F
   m_DefaultDims.emplace_back(::ConvertToSysUnits(6.0,unitMeasure::Inch));   // W
   m_DefaultDims.emplace_back(::ConvertToSysUnits(8.0,unitMeasure::Inch));   // Tt
   m_DefaultDims.emplace_back(::ConvertToSysUnits(7.0,unitMeasure::Inch));   // Tb
   m_DefaultDims.emplace_back(::ConvertToSysUnits(1.0,unitMeasure::Inch));   // Max Joint Spacing
   m_DefaultDims.emplace_back(::ConvertToSysUnits(36.0,unitMeasure::Inch));  // End Block Length

   // SI Units
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // A 
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // B
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // C
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // F
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // W
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // Tt
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // Tb
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // Max joint size
   m_DimUnits[0].emplace_back(&unitMeasure::Millimeter); // End Block Length

   // US Units
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // A 
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // B
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // C
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // F
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // W
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // Tt
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // Tb
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // Max joint size
   m_DimUnits[1].emplace_back(&unitMeasure::Inch); // End Block Length

   return S_OK;
}

void CDeckedSlabBeamFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   CComPtr<IDeckedSlabBeamSection> gdrSection;
   gdrSection.CoCreateInstance(CLSID_DeckedSlabBeamSection);
   CComPtr<IDeckedSlabBeam> beam;
   gdrSection->get_Beam(&beam);

   Float64 A,B,C,F,W,Tt,Tb,J,EndBlockLength;
   GetDimensions(dimensions,A,B,C,F,W,Tt,Tb,J,EndBlockLength);

   beam->put_A(A);
   beam->put_B(B);
   beam->put_C(C);
   beam->put_F(F);
   beam->put_W(W);
   beam->put_Tt(Tt);
   beam->put_Tb(Tb);

   PositionBeam(beam);

   gdrSection.QueryInterface(ppSection);
}

void CDeckedSlabBeamFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
{
   CComPtr<IDeckedSlabBeamEndBlockSegment> segment;
   segment.CoCreateInstance(CLSID_DeckedSlabBeamEndBlockSegment);

   // Build up the beam shape
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder     = pGroup->GetGirder(segmentKey.girderIndex);
   const CPrecastSegmentData* pSegment    = pGirder->GetSegment(segmentKey.segmentIndex);

   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   segment->put_EndBlockLength(etStart,endBlockLength);
   segment->put_EndBlockLength(etEnd,endBlockLength);

   CComPtr<IGirderSection> gdrSection;
   CreateGirderSection(pBroker,statusGroupID,dimensions,-1,-1,&gdrSection);
   CComQIPtr<IDeckedSlabBeamSection> section(gdrSection);

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

   CComQIPtr<IShape> shape(section);
   segment->AddShape(shape,material,nullptr);

   CComQIPtr<ISuperstructureMemberSegment> ssmbrSegment(segment);
   ssmbrSegment.CopyTo(ppSegment);
}

void CDeckedSlabBeamFactory::CreateSegmentShape(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   CComPtr<IDeckedSlabBeam> beam;
   beam.CoCreateInstance(CLSID_DeckedSlabBeam);

   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   const CSegmentKey& segmentKey(pSegment->GetSegmentKey());

   Float64 A, B, C, F, W, Tt, Tb, J, endBlockLength;
   GetDimensions(dimensions, A, B, C, F, W, Tt, Tb, J, endBlockLength);

   beam->put_A(A);
   beam->put_B(B);
   beam->put_C(C);
   beam->put_F(F);
   beam->put_W(W);
   beam->put_Tt(Tt);
   beam->put_Tb(Tb);

   if (segmentKey.girderIndex == 0)
   {
      beam->put_LeftBlockOut(VARIANT_FALSE);
   }

   const CGirderGroupData* pGroup = pSegment->GetGirder()->GetGirderGroup();
   if (segmentKey.girderIndex == pGroup->GetGirderCount() - 1)
   {
      beam->put_RightBlockOut(VARIANT_FALSE);
   }

   GET_IFACE2(pBroker, IBridge, pBridge);
   Float64 Lg = pBridge->GetSegmentLength(segmentKey);
   if (IsInEndBlock(Xs, sectionBias, endBlockLength, Lg))
   {
      // Xs is in the end block region
      beam->put_VoidCount(0);
   }

   PositionBeam(beam);

   beam.QueryInterface(ppShape);
}

Float64 CDeckedSlabBeamFactory::GetSegmentHeight(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   Float64 C = GetDimension(dimensions, _T("C"));
   Float64 Tt = GetDimension(dimensions, _T("Tt"));
   return C + Tt;
}

void CDeckedSlabBeamFactory::ConfigureSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void CDeckedSlabBeamFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
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
      pgsPointOfInterest poiLeftFace1(segmentKey, endBlockLength, POI_SECTCHANGE_LEFTFACE);
      pgsPointOfInterest poiRightFace1(segmentKey, endBlockLength, POI_SECTCHANGE_RIGHTFACE);
      poiLeftFace1.CanMerge(false);
      poiRightFace1.CanMerge(false);
      PoiIDType poiID = pPoiMgr->AddPointOfInterest(poiLeftFace1);
      poiLeftFace1 = pPoiMgr->GetPointOfInterest(poiID);
      poiRightFace1.SetDistFromStart(poiLeftFace1.GetDistFromStart(),true);
      pPoiMgr->AddPointOfInterest(poiRightFace1);

      pgsPointOfInterest poiRightFace2(segmentKey, gdrLength - endBlockLength, POI_SECTCHANGE_RIGHTFACE);
      pgsPointOfInterest poiLeftFace2(segmentKey, gdrLength - endBlockLength, POI_SECTCHANGE_LEFTFACE);
      poiRightFace2.CanMerge(false);
      poiLeftFace2.CanMerge(false);
      poiID = pPoiMgr->AddPointOfInterest(poiRightFace2);
      poiRightFace2 = pPoiMgr->GetPointOfInterest(poiID);
      poiLeftFace2.SetDistFromStart(poiRightFace2.GetDistFromStart(),true);
      pPoiMgr->AddPointOfInterest(poiLeftFace2);
   }
}

void CDeckedSlabBeamFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng) const
{
   CComObject<CMultiWebDistFactorEngineer>* pEngineer;
   CComObject<CMultiWebDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);

   pEngineer->SetBeamType(CMultiWebDistFactorEngineer::btDeckedSlabBeam);

   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CDeckedSlabBeamFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng) const
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

void CDeckedSlabBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   // set the shapes for harped strand bounds - only in the thinest part of the webs
   Float64 A,B,C,F,W,Tt,Tb,J,EndBlockLength;
   GetDimensions(dimensions,A,B,C,F,W,Tt,Tb,J,EndBlockLength);

   Float64 width = W;
   Float64 depth = (Hg < 0 ? C + Tt : Hg);
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

   hr = configurer->SetHarpedStrandOffsetBounds(0, depth, endtb, endbb, hptb, hpbb, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));}

const std::vector<std::_tstring>& CDeckedSlabBeamFactory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& CDeckedSlabBeamFactory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const unitLength*>& CDeckedSlabBeamFactory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CDeckedSlabBeamFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSI,std::_tstring* strErrMsg) const
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

void CDeckedSlabBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) const
{
   pSave->BeginUnit(_T("DeckedSlabBeamDimensions"),2.0);
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CDeckedSlabBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad) const
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
      if (pLoad->BeginUnit(_T("DeckedSlabBeamDimensions")))
      {
         dimVersion = pLoad->GetVersion();
      }
      else
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   for ( const auto& name : m_DimNames)
   {
      Float64 value;
      if (!pLoad->Property(name.c_str(), &value))
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }

      dimensions.emplace_back(name,value);
   }

   if (14 <= parent_version && !pLoad->EndUnit())
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   return dimensions;
}

bool CDeckedSlabBeamFactory::IsPrismatic(const IBeamFactory::Dimensions& dimensions) const
{
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   return IsZero(endBlockLength) ? true : false;
}

bool CDeckedSlabBeamFactory::IsPrismatic(const CSegmentKey& segmentKey) const
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   return IsPrismatic(dimensions);
}

bool CDeckedSlabBeamFactory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

std::_tstring CDeckedSlabBeamFactory::GetImage() const
{
   return std::_tstring(_T("DeckedSlabBeam.gif"));
}

std::_tstring CDeckedSlabBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CDeckedSlabBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CDeckedSlabBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CDeckedSlabBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CDeckedSlabBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CDeckedSlabBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
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

CLSID CDeckedSlabBeamFactory::GetCLSID() const
{
   return CLSID_DeckedSlabBeamFactory;
}

std::_tstring CDeckedSlabBeamFactory::GetName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CDeckedSlabBeamFactory::GetFamilyCLSID() const
{
   return CLSID_DeckedSlabBeamFamily;
}

std::_tstring CDeckedSlabBeamFactory::GetGirderFamilyName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CDeckedSlabBeamFactory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CDeckedSlabBeamFactory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CDeckedSlabBeamFactory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR CDeckedSlabBeamFactory::GetImageResourceName() const
{
   return _T("DECKEDSLABBEAM");
}

HICON  CDeckedSlabBeamFactory::GetIcon()  const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DECKEDSLAB) );
}

void CDeckedSlabBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                      Float64& A,Float64& B,Float64& C,Float64& F,Float64& W,Float64& Tt,Float64& Tb,Float64& Jmax,Float64& EndBlockLength) const
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

Float64 CDeckedSlabBeamFactory::GetDimension(const IBeamFactory::Dimensions& dimensions, const std::_tstring& name) const
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

pgsTypes::SupportedDeckTypes CDeckedSlabBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
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

pgsTypes::SupportedBeamSpacings CDeckedSlabBeamFactory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);

   return sbs;
}

bool CDeckedSlabBeamFactory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool CDeckedSlabBeamFactory::ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   return false;
}

std::vector<pgsTypes::GirderOrientationType> CDeckedSlabBeamFactory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb,pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal };
   return types;
}

bool CDeckedSlabBeamFactory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType CDeckedSlabBeamFactory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes CDeckedSlabBeamFactory::GetSupportedDiaphragms() const
{
   // only supports adjacent spacing so there can only be precast diaphragms
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CDeckedSlabBeamFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

void CDeckedSlabBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   // we only allow adjacent spacing for this girder type so allowable spacing range is the joint spacing
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 J  = GetDimension(dimensions,_T("Jmax"));

   if ( sdt == pgsTypes::sdtCompositeOverlay || sdt == pgsTypes::sdtNone )
   {
      if(sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent)
      {
         *minSpacing = 0;
         *maxSpacing = J;
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

WebIndexType CDeckedSlabBeamFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions) const
{
   return 2;
}

Float64 CDeckedSlabBeamFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("C")) + GetDimension(dimensions,_T("Tt"));
}

Float64 CDeckedSlabBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("A"));
}

bool CDeckedSlabBeamFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void CDeckedSlabBeamFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool CDeckedSlabBeamFactory::HasLongitudinalJoints() const
{
   return false;
}

bool CDeckedSlabBeamFactory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool CDeckedSlabBeamFactory::HasTopFlangeThickening() const
{
   return false;
}

bool CDeckedSlabBeamFactory::CanPrecamber() const
{
   return false;
}

GirderIndexType CDeckedSlabBeamFactory::GetMinimumBeamCount() const
{
   return 1;
}

void CDeckedSlabBeamFactory::PositionBeam(IDeckedSlabBeam* pBeam) const
{
   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_Height(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}
