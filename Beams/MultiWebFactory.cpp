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

// MultiWebFactory.cpp : Implementation of CMultiWebFactory
#include "stdafx.h"
#include "Beams.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "MultiWebFactory.h"
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
#include <IFace\Intervals.h>

#include <IFace\AgeAdjustedMaterial.h>
#include <Beams\Helper.h>

#include <PsgLib\BridgeDescription2.h>

#include <EAF/EAFStatusCenter.h>
#include <PgsExt\StatusItem.h>
#include <PgsExt/PoiMgr.h>

#include <psgLib/SectionPropertiesCriteria.h>
#include <psgLib/SpecificationCriteria.h>
#include <psgLib/GirderLibraryEntry.h>

using namespace PGS::Beams;

INIT_BEAM_FACTORY_SINGLETON(MultiWebFactory)

MultiWebFactory::MultiWebFactory() : BeamFactory()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.emplace_back(_T("D1"));
   m_DimNames.emplace_back(_T("D2"));
   m_DimNames.emplace_back(_T("T1"));
   m_DimNames.emplace_back(_T("T2"));
   m_DimNames.emplace_back(_T("W1"));
   m_DimNames.emplace_back(_T("Wmax"));
   m_DimNames.emplace_back(_T("Wmin"));

   std::sort(m_DimNames.begin(),m_DimNames.end());

   // Default beam is a 4' wide tri-beam
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 6.0,WBFL::Units::Measure::Inch)); // D1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(21.0,WBFL::Units::Measure::Inch)); // D2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(7.25,WBFL::Units::Measure::Inch)); // T1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(5.25,WBFL::Units::Measure::Inch)); // T2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(12.0,WBFL::Units::Measure::Inch)); // W1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(6.000,WBFL::Units::Measure::Feet)); // Wmax
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(4.000,WBFL::Units::Measure::Feet)); // Wmin


   // SI Units
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // T1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // T2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Meter);      // Wmax
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Meter);      // Wmin

   // US Units
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // T1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // T2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Feet); // Wmax
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Feet); // Wmin
}

void MultiWebFactory::CreateGirderSection(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const BeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   CComPtr<IMultiWebSection> gdrSection;
   gdrSection.CoCreateInstance(CLSID_MultiWebSection);
   CComPtr<IMultiWeb> beam;
   gdrSection->get_Beam(&beam);

   DimensionAndPositionBeam(pBroker, dimensions, beam);

   gdrSection.QueryInterface(ppSection);
}

void MultiWebFactory::CreateSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
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

   CComQIPtr<IShape> shape(gdrSection);
   ATLASSERT(shape);
   segment->AddShape(shape,material,nullptr);

   CComQIPtr<ISuperstructureMemberSegment> ssmbrSegment(segment);
   ssmbrSegment.CopyTo(ppSegment);
}

void MultiWebFactory::CreateSegmentShape(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();


   CComPtr<IMultiWeb> beam;
   beam.CoCreateInstance(CLSID_MultiWeb);

   DimensionAndPositionBeam(pBroker, dimensions, beam);

   beam.QueryInterface(ppShape);
}

Float64 MultiWebFactory::GetSegmentHeight(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   Float64 D1 = GetDimension(dimensions, _T("D1"));
   Float64 D2 = GetDimension(dimensions, _T("D2"));
   return D1 + D2;
}

void MultiWebFactory::ConfigureSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void MultiWebFactory::LayoutSectionChangePointsOfInterest(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);
}

std::unique_ptr<DistFactorEngineer> MultiWebFactory::CreateDistFactorEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect) const
{
   return std::make_unique<MultiWebDistFactorEngineer>(MultiWebDistFactorEngineer::BeamType::MultiWebTee, pBroker, statusGroupID);
}

std::unique_ptr<PsLossEngineerBase> MultiWebFactory::CreatePsLossEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      return std::make_unique<TimeStepLossEngineer>(pBroker,statusGroupID);
   }
   else
   {
      return std::make_unique<PsBeamLossEngineer>(PsBeamLossEngineer::BeamType::IBeam,pBroker, statusGroupID);
   }
}

void MultiWebFactory::CreateStrandMover(const BeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  BeamFactory::BeamFace endTopFace, Float64 endTopLimit, BeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  BeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, BeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   // set the shapes for harped strand bounds - only in the thinnest part of the webs
   Float64 d1,d2;
   Float64 w,wmin,wmax;
   Float64 t1,t2;
   WebIndexType nWebs;
   GetDimensions(dimensions,d1,d2,w,wmin,wmax,t1,t2,nWebs);
   ATLASSERT(nWebs==3);

   Float64 width = Min(t1,t2);
   Float64 depth = (Hg < 0 ? d1 + d2 : Hg);

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

   lft_hook->Move(-hook_offset, -depth/2.0);
   rgt_hook->Move( hook_offset, -depth/2.0);
   ctr_hook->Move(         0.0, -depth/2.0);

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
   Float64 hptb  = hpTopFace     == BeamFactory::BeamFace::Bottom ? hpTopLimit     - depth : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == BeamFactory::BeamFace::Bottom ? hpBottomLimit  - depth : -hpBottomLimit;
   Float64 endtb = endTopFace    == BeamFactory::BeamFace::Bottom ? endTopLimit    - depth : -endTopLimit;
   Float64 endbb = endBottomFace == BeamFactory::BeamFace::Bottom ? endBottomLimit - depth : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(0, depth, endtb, endbb, hptb, hpbb, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}

const std::vector<std::_tstring>& MultiWebFactory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& MultiWebFactory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const WBFL::Units::Length*>& MultiWebFactory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool MultiWebFactory::ValidateDimensions(const BeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const
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
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
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
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][4];
      std::_tostringstream os;
      os << _T("W1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   
   if ( t1 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][2];
      std::_tostringstream os;
      os << _T("T1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( t2 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][3];
      std::_tostringstream os;
      os << _T("T2 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   Float64 web_wid = nWebs*t1 + (nWebs-1)*w1;
   if (wmin < web_wid)
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][5];
      std::_tostringstream os;
      os << _T("Wmin must be greater than the width of the webs, which is ")<< WBFL::Units::ConvertFromSysUnits(web_wid,*pUnit) << _T(" ") << pUnit->UnitTag()<< std::ends;
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

void MultiWebFactory::SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const BeamFactory::Dimensions& dimensions) const
{
   pSave->BeginUnit(_T("MultiWebDimensions"),1.0);
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

BeamFactory::Dimensions MultiWebFactory::LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const
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

   if (14 <= parent_version && !pLoad->BeginUnit(_T("MultiWebDimensions")))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   for( const auto& name : m_DimNames)
   {
      Float64 value;
      pLoad->Property(name.c_str(),&value);
      dimensions.emplace_back(name,value);
   }

   if (14 <= parent_version && !pLoad->EndUnit())
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   return dimensions;
}

bool MultiWebFactory::IsPrismatic(const BeamFactory::Dimensions& dimensions) const
{
   return true;
}

bool MultiWebFactory::IsPrismatic(const CSegmentKey& segmentKey) const
{
   return true;
}

bool MultiWebFactory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

std::_tstring MultiWebFactory::GetImage() const
{
   return std::_tstring(_T("MultiWeb.jpg"));
}

std::_tstring MultiWebFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring MultiWebFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring MultiWebFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring MultiWebFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring MultiWebFactory::GetInteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();

   if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth || 
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition() )
   {
      return _T("TripleTee_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("TripleTee_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring MultiWebFactory::GetExteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
   {
      return _T("TripleTee_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("TripleTee_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID MultiWebFactory::GetCLSID() const
{
   return CLSID_MultiWebFactory;
}

CLSID MultiWebFactory::GetFamilyCLSID() const
{
   return CLSID_RibbedBeamFamily;
}

std::_tstring MultiWebFactory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring MultiWebFactory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE MultiWebFactory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR MultiWebFactory::GetImageResourceName() const
{
   return _T("MultiWeb");
}

HICON  MultiWebFactory::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_MULTIWEB) );
}

void MultiWebFactory::GetDimensions(const BeamFactory::Dimensions& dimensions,
                                  Float64& d1,Float64& d2,
                                  Float64& w,Float64& wmin,Float64& wmax,
                                  Float64& t1,Float64& t2,
                                  WebIndexType& nWebs) const
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

Float64 MultiWebFactory::GetDimension(const BeamFactory::Dimensions& dimensions,const std::_tstring& name) const
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

pgsTypes::SupportedDeckTypes MultiWebFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
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

pgsTypes::SupportedBeamSpacings MultiWebFactory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsConstantAdjacent);
   return sbs;
}

bool MultiWebFactory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool MultiWebFactory::ConvertBeamSpacing(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   if (spacingType == pgsTypes::sbsUniform || spacingType == pgsTypes::sbsUniformAdjacent)
   {
      *pNewSpacingType = pgsTypes::sbsConstantAdjacent;
      *pNewSpacing = spacing;
      *pNewTopWidth = 0.0;
      return true;
   }
   return false;
}

pgsTypes::WorkPointLocations MultiWebFactory::GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::WorkPointLocations wpls;
   wpls.push_back(pgsTypes::wplTopGirder);
//   wpls.push_back(pgsTypes::wplBottomGirder);

   return wpls;
}

bool MultiWebFactory::IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation wpType) const
{
   pgsTypes::WorkPointLocations sbs = GetSupportedWorkPointLocations(spacingType);
   auto found = std::find(sbs.cbegin(), sbs.cend(),wpType);
   return found == sbs.end() ? false : true;
}

std::vector<pgsTypes::GirderOrientationType> MultiWebFactory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb, pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal,pgsTypes::Balanced};
   return types;
}

bool MultiWebFactory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType MultiWebFactory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes MultiWebFactory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes MultiWebFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

void MultiWebFactory::GetAllowableSpacingRange(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
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

WebIndexType MultiWebFactory::GetWebCount(const BeamFactory::Dimensions& dimensions) const
{
   return 3;
}

Float64 MultiWebFactory::GetBeamHeight(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 D1 = GetDimension(dimensions,_T("D1"));
   Float64 D2 = GetDimension(dimensions,_T("D2"));

   return D1 + D2;
}

Float64 MultiWebFactory::GetBeamWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return GetDimension(dimensions,_T("Wmax"));
}

void MultiWebFactory::GetBeamTopWidth(const BeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const
{
   Float64 Wmin = GetDimension(dimensions,_T("Wmin"));

   Float64 top = Wmin;
   top /= 2.0;

   *pLeftWidth = top;
   *pRightWidth = top;
}

bool MultiWebFactory::IsShearKey(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void MultiWebFactory::GetShearKeyAreas(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool MultiWebFactory::HasLongitudinalJoints() const
{
   return false;
}

bool MultiWebFactory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool MultiWebFactory::HasTopFlangeThickening() const
{
   return false;
}

bool MultiWebFactory::CanPrecamber() const
{
   return false;
}

GirderIndexType MultiWebFactory::GetMinimumBeamCount() const
{
   return 1;
}

void MultiWebFactory::DimensionAndPositionBeam(std::shared_ptr<WBFL::EAF::Broker> pBroker, const BeamFactory::Dimensions& dimensions, IMultiWeb* pBeam) const
{
   Float64 d1, d2;
   Float64 w, wmin, wmax;
   Float64 t1, t2;
   WebIndexType nWebs;
   GetDimensions(dimensions, d1, d2, w, wmin, wmax, t1, t2, nWebs);

   pBeam->put_W2(w);
   pBeam->put_D1(d1);
   pBeam->put_D2(d2);
   pBeam->put_T1(t1);
   pBeam->put_T2(t2);
   pBeam->put_WebCount(nWebs);

   // figure out the overhang, w1, based on the spacing
   Float64 w1;
   if (pBroker == nullptr)
   {
      // just use the max
      w1 = (wmax - nWebs*t1 - (nWebs - 1)*w) / 2;
   }
   else
   {
      // NOTE: Assuming uniform spacing
      // uniform spacing is required for this type of girder so maybe this is ok

      // use raw input here because requesting it from the bridge will cause an infite loop.
      // bridge agent calls this during validation
      GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      ATLASSERT(pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsConstantAdjacent);
      Float64 spacing = pBridgeDesc->GetGirderSpacing();

      // if this is a fixed width section, then set the spacing equal to the width
      if (IsEqual(wmax, wmin))
      {
         spacing = wmax;
      }

      w1 = (spacing - nWebs*t1 - (nWebs - 1)*w) / 2;
   }
   pBeam->put_W1(w1);

   // origin of multi-web is top center... this is what we want
}