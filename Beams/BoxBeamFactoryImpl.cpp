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

// BoxBeamFactory.cpp : Implementation of CBoxBeamFactoryImpl
#include "stdafx.h"
#include "Beams.h"
#include <Plugins\Beams.h>

#include <Plugins\BeamFamilyCLSID.h>

#include "BoxBeamFactoryImpl.h"
#include "BoxBeamDistFactorEngineer.h"
#include <Beams/UBeamDistFactorEngineer.h>
#include <Beams/PsBeamLossEngineer.h>
#include <Beams/TimeStepLossEngineer.h>
#include "StrandMoverImpl.h"
#include <GeomModel\PrecastBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <EAF/EAFStatusCenter.h>

#include <PgsExt\StatusItem.h>
#include <PgsExt/PoiMgr.h>

#include <Beams\Helper.h>

#include <PsgLib\BridgeDescription2.h>
#include <psgLib/SectionPropertiesCriteria.h>
#include <psgLib/SpecificationCriteria.h>
#include <psgLib/GirderLibraryEntry.h>

using namespace PGS::Beams;


void BoxBeamFactoryImpl::CreateGirderSection(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const BeamFactory::Dimensions& dimensions, Float64 overallHeight, Float64 bottomFlangeHeight, IGirderSection** ppSection) const
{
   CComPtr<IBoxBeamSection> gdrSection;
   gdrSection.CoCreateInstance(CLSID_BoxBeamSection);
   CComPtr<IBoxBeam> beam;
   gdrSection->get_Beam(&beam);

   DimensionBeam(dimensions, beam);

   gdrSection.QueryInterface(ppSection);
}

void BoxBeamFactoryImpl::CreateSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusItemIDType statusID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
{
   CComPtr<IBoxBeamEndBlockSegment> segment;
   HRESULT hr = segment.CoCreateInstance(CLSID_BoxBeamEndBlockSegment);

   // Build up the beam shape
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const CSplicedGirderData* pGirder = pGroup->GetGirder(segmentKey.girderIndex);
   const GirderLibraryEntry* pGdrEntry = pGirder->GetGirderLibraryEntry();

   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   segment->put_EndBlockLength(etStart,endBlockLength);
   segment->put_EndBlockLength(etEnd,endBlockLength);

   CComPtr<IGirderSection> gdrSection;
   CreateGirderSection(pBroker,statusID,dimensions,-1,-1,&gdrSection);

   CComQIPtr<IBoxBeamSection> section(gdrSection);
   CComPtr<IBoxBeam> beam;
   section->get_Beam(&beam);

   ConfigureGirderShape(pGirder->GetSegment(segmentKey.segmentIndex), dimensions, beam);

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

   segment->AddShape(shape,material,nullptr);

   CComQIPtr<ISuperstructureMemberSegment> ssmbrSegment(segment);
   ssmbrSegment.CopyTo(ppSegment);
}

void BoxBeamFactoryImpl::ConfigureSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void BoxBeamFactoryImpl::CreateSegmentShape(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias,IShape** ppShape) const
{
   // also, no need to create girder section, can just create the shape
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   CComPtr<IBoxBeam> beam;
   beam.CoCreateInstance(CLSID_BoxBeam);
   ConfigureGirderShape(pSegment, dimensions, beam);

   Float64 endBlockLength = GetDimension(dimensions, _T("EndBlockLength"));
   GET_IFACE2(pBroker,IBridge, pBridge);
   Float64 Lg = pBridge->GetSegmentLength(pSegment->GetSegmentKey());
   if (IsInEndBlock(Xs,sectionBias,endBlockLength,Lg))
   {
      // Xs is in the end block region
      beam->put_VoidCount(0);
   }


   beam.QueryInterface(ppShape);
}

Float64 BoxBeamFactoryImpl::GetSegmentHeight(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   Float64 H1 = GetDimension(dimensions, _T("H1"));
   Float64 H2 = GetDimension(dimensions, _T("H2"));
   Float64 H3 = GetDimension(dimensions, _T("H3"));
   return H1 + H2 + H3;
}

void BoxBeamFactoryImpl::LayoutSectionChangePointsOfInterest(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE);
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE );

   VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);
   VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);

   // put section breaks just on either side of the end blocks/void interface
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
   const GirderLibraryEntry* pGirderLib = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
   Float64 endBlockLength = pGirderLib->GetDimension(_T("EndBlockLength"));

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

std::unique_ptr<DistFactorEngineer> BoxBeamFactoryImpl::CreateDistFactorEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,
                                               const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect) const
{
   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   // use passed value if not null
   pgsTypes::SupportedDeckType deckType = (pDeckType!=nullptr) ? *pDeckType : pDeck->GetDeckType();

   if ( deckType == pgsTypes::sdtCompositeOverlay || deckType == pgsTypes::sdtNone )
   {
      return std::make_unique<BoxBeamDistFactorEngineer>(pBroker, statusGroupID);
   }
   else
   {
      // this is a type b section... type b's are the same as type c's which are U-beams
      ATLASSERT( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP );
      return std::make_unique<UBeamDistFactorEngineer>(pBroker, statusGroupID);
   }
   return nullptr;
}

std::unique_ptr<PsLossEngineerBase> BoxBeamFactoryImpl::CreatePsLossEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusItemIDType statusGroupID,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      return std::make_unique<TimeStepLossEngineer>(pBroker,statusGroupID);
   }
   else
   {
      return std::make_unique<PsBeamLossEngineer>(PsBeamLossEngineer::BeamType::BoxBeam,pBroker,statusGroupID);
   }
}

const std::vector<std::_tstring>& BoxBeamFactoryImpl::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& BoxBeamFactoryImpl::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const WBFL::Units::Length*>& BoxBeamFactoryImpl::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool BoxBeamFactoryImpl::IsPrismatic(const BeamFactory::Dimensions& dimensions) const
{
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   return IsZero(endBlockLength) ? true : false;
}

bool BoxBeamFactoryImpl::IsPrismatic(const CSegmentKey& segmentKey) const
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

bool BoxBeamFactoryImpl::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

CLSID BoxBeamFactoryImpl::GetFamilyCLSID() const
{
   return CLSID_BoxBeamFamily;
}

std::_tstring BoxBeamFactoryImpl::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring BoxBeamFactoryImpl::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE BoxBeamFactoryImpl::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

Float64 BoxBeamFactoryImpl::GetDimension(const BeamFactory::Dimensions& dimensions,const std::_tstring& name) const
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

pgsTypes::SupportedDeckTypes BoxBeamFactoryImpl::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
{
   pgsTypes::SupportedDeckTypes sdt;
   switch( sbs )
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

pgsTypes::SupportedBeamSpacings BoxBeamFactoryImpl::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);

   return sbs;
}

bool BoxBeamFactoryImpl::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(),spacingType);
   return found == sbs.end() ? false : true;
}

bool BoxBeamFactoryImpl::ConvertBeamSpacing(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
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

pgsTypes::WorkPointLocations BoxBeamFactoryImpl::GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::WorkPointLocations wpls;
   wpls.push_back(pgsTypes::wplTopGirder);
   wpls.push_back(pgsTypes::wplBottomGirder);

   return wpls;
}

bool BoxBeamFactoryImpl::IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType,pgsTypes::WorkPointLocation wpType) const
{
   pgsTypes::WorkPointLocations sbs = GetSupportedWorkPointLocations(spacingType);
   auto found = std::find(sbs.cbegin(), sbs.cend(),wpType);
   return found == sbs.end() ? false : true;
}

std::vector<pgsTypes::GirderOrientationType> BoxBeamFactoryImpl::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb,pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal,pgsTypes::Balanced};
   return types;
}

bool BoxBeamFactoryImpl::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType BoxBeamFactoryImpl::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes BoxBeamFactoryImpl::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes BoxBeamFactoryImpl::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

WebIndexType BoxBeamFactoryImpl::GetWebCount(const BeamFactory::Dimensions& dimensions) const
{
   return 2;
}


Float64 BoxBeamFactoryImpl::GetBeamHeight(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 H1 = GetDimension(dimensions,_T("H1"));
   Float64 H2 = GetDimension(dimensions,_T("H2"));
   Float64 H3 = GetDimension(dimensions,_T("H3"));

   return H1 + H2 + H3;
}

std::_tstring BoxBeamFactoryImpl::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      strImage = _T("BoxBeam_Composite_CIP.gif");
      break;

   case pgsTypes::sdtCompositeSIP:
      strImage = _T("BoxBeam_Composite_SIP.gif");
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage = _T("BoxBeam_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage = _T("BoxBeam_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring BoxBeamFactoryImpl::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("+Mn_SpreadBoxBeam_Composite.gif");
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("+Mn_BoxBeam_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("+Mn_BoxBeam_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring BoxBeamFactoryImpl::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("-Mn_SpreadBoxBeam_Composite.gif");
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("-Mn_BoxBeam_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("-Mn_BoxBeam_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring BoxBeamFactoryImpl::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("Vn_SpreadBoxBeam_Composite.gif");
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("Vn_BoxBeam_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("Vn_BoxBeam_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring BoxBeamFactoryImpl::GetInteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();

   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
         WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
      {
         strImage =  _T("SpreadBoxBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
      }
      else
      {
         strImage =  _T("SpreadBoxBeam_Effective_Flange_Width_Interior_Girder.gif");
      }
      break;

   case pgsTypes::sdtCompositeOverlay:
      if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
         WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
      {
         strImage =  _T("BoxBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
      }
      else
      {
         strImage =  _T("BoxBeam_Effective_Flange_Width_Interior_Girder.gif");
      }
      break;

   case pgsTypes::sdtNone:
   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring BoxBeamFactoryImpl::GetExteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();

   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
         WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
      {
         strImage =  _T("SpreadBoxBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
      }
      else
      {
         strImage =  _T("SpreadBoxBeam_Effective_Flange_Width_Exterior_Girder.gif");
      }
      break;

   case pgsTypes::sdtCompositeOverlay:
      if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
         WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
      {
         strImage =  _T("BoxBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
      }
      else
      {
         strImage =  _T("BoxBeam_Effective_Flange_Width_Exterior_Girder.gif");
      }
      break;

   case pgsTypes::sdtNone:
   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

bool BoxBeamFactoryImpl::HasTopFlangeThickening() const
{
   return false;
}

bool BoxBeamFactoryImpl::CanPrecamber() const
{
   return false;
}

GirderIndexType BoxBeamFactoryImpl::GetMinimumBeamCount() const
{
   return 1;
}

void BoxBeamFactoryImpl::DimensionBeam(const BeamFactory::Dimensions& dimensions, IBoxBeam* pBeam) const
{
   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_Height(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}

void BoxBeamFactoryImpl::ConfigureGirderShape(const CPrecastSegmentData* pSegment,const BeamFactory::Dimensions& dimensions, IBoxBeam* pBeam) const
{
   DimensionBeam(dimensions, pBeam);

   const CSegmentKey& segmentKey(pSegment->GetSegmentKey());
   if (segmentKey.girderIndex == 0 && ExcludeExteriorBeamShearKeys(dimensions))
   {
      pBeam->put_LeftBlockOut(VARIANT_FALSE);
   }

   const CGirderGroupData* pGroup = pSegment->GetGirder()->GetGirderGroup();
   if (segmentKey.girderIndex == pGroup->GetGirderCount() - 1 && ExcludeExteriorBeamShearKeys(dimensions))
   {
      pBeam->put_RightBlockOut(VARIANT_FALSE);
   }
}
