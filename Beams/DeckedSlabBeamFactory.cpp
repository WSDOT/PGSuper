///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include "Beams.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "DeckedSlabBeamFactory.h"
#include <Beams/IBeamDistFactorEngineer.h>
#include "MultiWebDistFactorEngineer.h"
#include <Beams/PsBeamLossEngineer.h>
#include <Beams/TimeStepLossEngineer.h>
#include "StrandMoverImpl.h"
#include <GeomModel\PrecastBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <EAF/EAFStatusCenter.h>
#include <IFace\Intervals.h>

#include <IFace\AgeAdjustedMaterial.h>
#include <Beams\Helper.h>

#include <PsgLib\BridgeDescription2.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt/PoiMgr.h>

#include <psgLib/SectionPropertiesCriteria.h>
#include <psgLib/SpecificationCriteria.h>
#include <psgLib/GirderLibraryEntry.h>

using namespace PGS::Beams;

INIT_BEAM_FACTORY_SINGLETON(DeckedSlabBeamFactory)

DeckedSlabBeamFactory::DeckedSlabBeamFactory() :
   BeamFactory()
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

   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(77.75,WBFL::Units::Measure::Inch)); // A
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(9.0,WBFL::Units::Measure::Inch));   // B
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(12.0,WBFL::Units::Measure::Inch));  // C
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(1.75,WBFL::Units::Measure::Inch));  // F
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(6.0,WBFL::Units::Measure::Inch));   // W
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(8.0,WBFL::Units::Measure::Inch));   // Tt
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(7.0,WBFL::Units::Measure::Inch));   // Tb
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(1.0,WBFL::Units::Measure::Inch));   // Max Joint Spacing
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(36.0,WBFL::Units::Measure::Inch));  // End Block Length

   // SI Units
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // A 
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // B
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // C
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // F
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // Tt
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // Tb
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // Max joint size
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // End Block Length

   // US Units
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // A 
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // B
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // C
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // F
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // Tt
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // Tb
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // Max joint size
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // End Block Length
}

void DeckedSlabBeamFactory::CreateGirderSection(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const BeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
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

void DeckedSlabBeamFactory::CreateSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
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
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
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

void DeckedSlabBeamFactory::CreateSegmentShape(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
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

Float64 DeckedSlabBeamFactory::GetSegmentHeight(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   Float64 C = GetDimension(dimensions, _T("C"));
   Float64 Tt = GetDimension(dimensions, _T("Tt"));
   return C + Tt;
}

void DeckedSlabBeamFactory::ConfigureSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void DeckedSlabBeamFactory::LayoutSectionChangePointsOfInterest(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);
   VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);

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
      PoiIDType poiID = pPoiMgr->AddPointOfInterest(poiLeftFace1);
      ATLASSERT(poiID != INVALID_ID);
      poiLeftFace1 = pPoiMgr->GetPointOfInterest(poiID);
      poiRightFace1.SetDistFromStart(poiLeftFace1.GetDistFromStart(),true);
      VERIFY(pPoiMgr->AddPointOfInterest(poiRightFace1) != INVALID_ID);

      pgsPointOfInterest poiRightFace2(segmentKey, gdrLength - endBlockLength, POI_SECTCHANGE_RIGHTFACE);
      pgsPointOfInterest poiLeftFace2(segmentKey, gdrLength - endBlockLength, POI_SECTCHANGE_LEFTFACE);
      poiID = pPoiMgr->AddPointOfInterest(poiRightFace2);
      ATLASSERT(poiID != INVALID_ID);
      poiRightFace2 = pPoiMgr->GetPointOfInterest(poiID);
      poiLeftFace2.SetDistFromStart(poiRightFace2.GetDistFromStart(),true);
      VERIFY(pPoiMgr->AddPointOfInterest(poiLeftFace2) != INVALID_ID);
   }
}

std::unique_ptr<DistFactorEngineer> DeckedSlabBeamFactory::CreateDistFactorEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect) const
{
   return std::make_unique<MultiWebDistFactorEngineer>(MultiWebDistFactorEngineer::BeamType::DeckedSlabBeam, pBroker, statusGroupID);
}

std::unique_ptr<PsLossEngineerBase> DeckedSlabBeamFactory::CreatePsLossEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      return std::make_unique<TimeStepLossEngineer>(pBroker,statusGroupID);
   }
   else
   {
      return std::make_unique<PsBeamLossEngineer>(PsBeamLossEngineer::BeamType::SingleT,pBroker,statusGroupID);
   }
}

void DeckedSlabBeamFactory::CreateStrandMover(const BeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  BeamFactory::BeamFace endTopFace, Float64 endTopLimit, BeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  BeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, BeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   // set the shapes for harped strand bounds - only in the thinnest part of the webs
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
   Float64 hptb  = hpTopFace     == BeamFactory::BeamFace::Bottom ? hpTopLimit     - depth : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == BeamFactory::BeamFace::Bottom ? hpBottomLimit  - depth : -hpBottomLimit;
   Float64 endtb = endTopFace    == BeamFactory::BeamFace::Bottom ? endTopLimit    - depth : -endTopLimit;
   Float64 endbb = endBottomFace == BeamFactory::BeamFace::Bottom ? endBottomLimit - depth : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(0, depth, endtb, endbb, hptb, hpbb, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));}

const std::vector<std::_tstring>& DeckedSlabBeamFactory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& DeckedSlabBeamFactory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const WBFL::Units::Length*>& DeckedSlabBeamFactory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool DeckedSlabBeamFactory::ValidateDimensions(const BeamFactory::Dimensions& dimensions,bool bSI,std::_tstring* strErrMsg) const
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

void DeckedSlabBeamFactory::SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const BeamFactory::Dimensions& dimensions) const
{
   pSave->BeginUnit(_T("DeckedSlabBeamDimensions"),2.0);
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

BeamFactory::Dimensions DeckedSlabBeamFactory::LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const
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


   BeamFactory::Dimensions dimensions;

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

bool DeckedSlabBeamFactory::IsPrismatic(const BeamFactory::Dimensions& dimensions) const
{
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   return IsZero(endBlockLength) ? true : false;
}

bool DeckedSlabBeamFactory::IsPrismatic(const CSegmentKey& segmentKey) const
{
   auto pBroker = EAFGetBroker();

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData*  pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   return IsPrismatic(dimensions);
}

bool DeckedSlabBeamFactory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

std::_tstring DeckedSlabBeamFactory::GetImage() const
{
   return std::_tstring(_T("DeckedSlabBeam.gif"));
}

std::_tstring DeckedSlabBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage = _T("DeckedSlabBeam_Composite.gif");
      break;

   case pgsTypes::sdtNonstructuralOverlay:
      strImage = _T("DeckedSlabBeam_NSoverlay.gif");
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

std::_tstring DeckedSlabBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("+Mn_DeckedSlabBeam_Composite.gif");
      break;

   case pgsTypes::sdtNonstructuralOverlay:
   case pgsTypes::sdtNone:
      strImage =  _T("+Mn_DeckedSlabBeam_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring DeckedSlabBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("-Mn_DeckedSlabBeam_Composite.gif");
      break;

   case pgsTypes::sdtNonstructuralOverlay:
   case pgsTypes::sdtNone:
      strImage =  _T("-Mn_DeckedSlabBeam_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring DeckedSlabBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("Vn_DeckedSlabBeam_Composite.gif");
      break;

   case pgsTypes::sdtNonstructuralOverlay:
   case pgsTypes::sdtNone:
      strImage =  _T("Vn_DeckedSlabBeam_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring DeckedSlabBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if ( section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth || 
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition() )
   {
      return _T("DeckedSlabBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("DeckedSlabBeam_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring DeckedSlabBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if ( section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth || 
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition() )
   {
      return _T("DeckedSlabBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("DeckedSlabBeam_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID DeckedSlabBeamFactory::GetCLSID() const
{
   return CLSID_DeckedSlabBeamFactory;
}

CLSID DeckedSlabBeamFactory::GetFamilyCLSID() const
{
   return CLSID_DeckedSlabBeamFamily;
}

std::_tstring DeckedSlabBeamFactory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring DeckedSlabBeamFactory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE DeckedSlabBeamFactory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR DeckedSlabBeamFactory::GetImageResourceName() const
{
   return _T("DECKEDSLABBEAM");
}

HICON  DeckedSlabBeamFactory::GetIcon()  const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DECKEDSLAB) );
}

void DeckedSlabBeamFactory::GetDimensions(const BeamFactory::Dimensions& dimensions,
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

Float64 DeckedSlabBeamFactory::GetDimension(const BeamFactory::Dimensions& dimensions, const std::_tstring& name) const
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

pgsTypes::SupportedDeckTypes DeckedSlabBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
{
   pgsTypes::SupportedDeckTypes sdt;
   switch(sbs)
   {
   case pgsTypes::sbsUniformAdjacent:
   case pgsTypes::sbsGeneralAdjacent:
      sdt.push_back(pgsTypes::sdtCompositeOverlay);
      sdt.push_back(pgsTypes::sdtNonstructuralOverlay);
      sdt.push_back(pgsTypes::sdtNone);
      break;

   default:
      ATLASSERT(false);
   }
   return sdt;
}

pgsTypes::SupportedBeamSpacings DeckedSlabBeamFactory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);

   return sbs;
}

bool DeckedSlabBeamFactory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool DeckedSlabBeamFactory::ConvertBeamSpacing(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   return false;
}

pgsTypes::WorkPointLocations DeckedSlabBeamFactory::GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::WorkPointLocations wpls;
   wpls.push_back(pgsTypes::wplTopGirder);
//   wpls.push_back(pgsTypes::wplBottomGirder);

   return wpls;
}

bool DeckedSlabBeamFactory::IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation wpType) const
{
   pgsTypes::WorkPointLocations sbs = GetSupportedWorkPointLocations(spacingType);
   auto found = std::find(sbs.cbegin(), sbs.cend(),wpType);
   return found == sbs.end() ? false : true;
}

std::vector<pgsTypes::GirderOrientationType> DeckedSlabBeamFactory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb,pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal,pgsTypes::Balanced };
   return types;
}

bool DeckedSlabBeamFactory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType DeckedSlabBeamFactory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes DeckedSlabBeamFactory::GetSupportedDiaphragms() const
{
   // only supports adjacent spacing so there can only be precast diaphragms
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes DeckedSlabBeamFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

void DeckedSlabBeamFactory::GetAllowableSpacingRange(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   // we only allow adjacent spacing for this girder type so allowable spacing range is the joint spacing
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 J  = GetDimension(dimensions,_T("Jmax"));

   if ( sdt == pgsTypes::sdtCompositeOverlay || sdt == pgsTypes::sdtNone || sdt == pgsTypes::sdtCompositeOverlay || sdt == pgsTypes::sdtNonstructuralOverlay )
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

WebIndexType DeckedSlabBeamFactory::GetWebCount(const BeamFactory::Dimensions& dimensions) const
{
   return 2;
}

Float64 DeckedSlabBeamFactory::GetBeamHeight(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("C")) + GetDimension(dimensions,_T("Tt"));
}

Float64 DeckedSlabBeamFactory::GetBeamWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("A"));
}

void DeckedSlabBeamFactory::GetBeamTopWidth(const BeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const
{
   Float64 A = GetDimension(dimensions, _T("A"));
   Float64 F = GetDimension(dimensions, _T("F"));

   Float64 top = A - 2 * F;
   top /= 2.0;

   *pLeftWidth = top;
   *pRightWidth = top;
}

bool DeckedSlabBeamFactory::IsShearKey(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void DeckedSlabBeamFactory::GetShearKeyAreas(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool DeckedSlabBeamFactory::HasLongitudinalJoints() const
{
   return false;
}

bool DeckedSlabBeamFactory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool DeckedSlabBeamFactory::HasTopFlangeThickening() const
{
   return false;
}

bool DeckedSlabBeamFactory::CanPrecamber() const
{
   return false;
}

GirderIndexType DeckedSlabBeamFactory::GetMinimumBeamCount() const
{
   return 1;
}

void DeckedSlabBeamFactory::PositionBeam(IDeckedSlabBeam* pBeam) const
{
   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_Height(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}
