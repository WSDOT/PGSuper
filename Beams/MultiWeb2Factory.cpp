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

// MultiWeb2Factory.cpp : Implementation of CMultiWeb2Factory
#include "stdafx.h"
#include "Beams.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "MultiWeb2Factory.h"
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
#include <EAF/EAFStatusCenter.h>
#include <IFace\AgeAdjustedMaterial.h>

#include <Beams\Helper.h>

#include <PgsExt\StatusItem.h>
#include <PgsExt/PoiMgr.h>

#include <PsgLib\BridgeDescription2.h>
#include <psgLib/SectionPropertiesCriteria.h>
#include <psgLib/SpecificationCriteria.h>
#include <psgLib/GirderLibraryEntry.h>

using namespace PGS::Beams;

INIT_BEAM_FACTORY_SINGLETON(MultiWeb2Factory)

MultiWeb2Factory::MultiWeb2Factory() : BeamFactory()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.emplace_back(_T("C1"));
   m_DimNames.emplace_back(_T("C2"));
   m_DimNames.emplace_back(_T("H1"));
   m_DimNames.emplace_back(_T("H2"));
   m_DimNames.emplace_back(_T("H3"));
   m_DimNames.emplace_back(_T("T1"));
   m_DimNames.emplace_back(_T("T2"));
   m_DimNames.emplace_back(_T("T3"));
   m_DimNames.emplace_back(_T("F1"));
   m_DimNames.emplace_back(_T("W2"));
   m_DimNames.emplace_back(_T("Wmin"));
   m_DimNames.emplace_back(_T("Wmax"));

   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 0.0,WBFL::Units::Measure::Inch)); // C1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 0.0,WBFL::Units::Measure::Inch)); // C2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(27.0,WBFL::Units::Measure::Inch)); // H1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 3.0,WBFL::Units::Measure::Inch)); // H2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 6.0,WBFL::Units::Measure::Inch)); // H3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 0.0,WBFL::Units::Measure::Inch)); // T1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 6.5,WBFL::Units::Measure::Inch)); // T2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 1.5,WBFL::Units::Measure::Inch)); // T3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 3.0,WBFL::Units::Measure::Inch)); // F1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(48.0,WBFL::Units::Measure::Inch)); // W2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 2.5,WBFL::Units::Measure::Feet)); // Wmin
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 3.0,WBFL::Units::Measure::Feet)); // Wmax


   // SI Units
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // C1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // C2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // H1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // H2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // H3
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // T1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // T2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // T3
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // F1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Meter); // Wmin
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Meter); // Wmax

   // US Units
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // C1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // C2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // H1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // H2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // H3
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // T1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // T2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // T3
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // F1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Feet); // Wmin
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Feet); // Wmax
}

void MultiWeb2Factory::CreateGirderSection(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const BeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   CComPtr<IMultiWebSection2> gdrSection;
   gdrSection.CoCreateInstance(CLSID_MultiWebSection2);
   CComPtr<IMultiWeb2> beam;
   gdrSection->get_Beam(&beam);

   DimensionAndPositionBeam(pBroker, dimensions, beam);

   gdrSection.QueryInterface(ppSection);
}

void MultiWeb2Factory::CreateSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
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

void MultiWeb2Factory::CreateSegmentShape(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   CComPtr<IMultiWeb2> beam;
   beam.CoCreateInstance(CLSID_MultiWeb2);
   
   DimensionAndPositionBeam(pBroker, dimensions, beam);

   beam.QueryInterface(ppShape);
}

Float64 MultiWeb2Factory::GetSegmentHeight(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   Float64 H1 = GetDimension(dimensions, _T("H1"));
   Float64 H2 = GetDimension(dimensions, _T("H2"));
   Float64 H3 = GetDimension(dimensions, _T("H3"));
   return H1 + H2 + H3;
}

void MultiWeb2Factory::ConfigureSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void MultiWeb2Factory::LayoutSectionChangePointsOfInterest(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);
   VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);
}

std::unique_ptr<DistFactorEngineer> MultiWeb2Factory::CreateDistFactorEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect) const
{
   return std::make_unique<MultiWebDistFactorEngineer>(MultiWebDistFactorEngineer::BeamType::MultiWebTee, pBroker, statusGroupID);
}

std::unique_ptr<PsLossEngineerBase> MultiWeb2Factory::CreatePsLossEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey) const
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
   {
      return std::make_unique<TimeStepLossEngineer>(pBroker,statusGroupID);
   }
   else
   {
      return std::make_unique<PsBeamLossEngineer>(PsBeamLossEngineer::BeamType::IBeam,pBroker,statusGroupID);
   }
}

void MultiWeb2Factory::CreateStrandMover(const BeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  BeamFactory::BeamFace endTopFace, Float64 endTopLimit, BeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  BeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, BeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
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
   Float64 depth = (Hg < 0 ? h1 + h2 + h3 : Hg);

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
   ATLASSERT (SUCCEEDED(hr));
}

const std::vector<std::_tstring>& MultiWeb2Factory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& MultiWeb2Factory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const WBFL::Units::Length*>& MultiWeb2Factory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool MultiWeb2Factory::ValidateDimensions(const BeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const
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

void MultiWeb2Factory::SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const BeamFactory::Dimensions& dimensions) const
{
   pSave->BeginUnit(_T("MultiWeb2Dimensions"),1.0);
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

BeamFactory::Dimensions MultiWeb2Factory::LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const
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
      if (pLoad->BeginUnit(_T("MultiWeb2Dimensions")))
      {
         dimVersion = pLoad->GetVersion();
      }
      else
      {
         THROW_LOAD(InvalidFileFormat, pLoad);
      }
   }

   for (const auto& name : m_DimNames)
   {
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
      dimensions.emplace_back(name,value);
   }

   if (14 <= parent_version && !pLoad->EndUnit())
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   return dimensions;
}

bool MultiWeb2Factory::IsPrismatic(const BeamFactory::Dimensions& dimensions) const
{
   return true;
}

bool MultiWeb2Factory::IsPrismatic(const CSegmentKey& segmentKey) const
{
   return true;
}

bool MultiWeb2Factory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

std::_tstring MultiWeb2Factory::GetImage() const
{
   return std::_tstring(_T("MultiWeb2.gif"));
}

std::_tstring MultiWeb2Factory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring MultiWeb2Factory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring MultiWeb2Factory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring MultiWeb2Factory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring MultiWeb2Factory::GetInteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if ( section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth || 
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition() )
   {
      return _T("DoubleTee_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("DoubleTee_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring MultiWeb2Factory::GetExteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if ( section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth || 
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition() )
   {
      return _T("DoubleTee_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("DoubleTee_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID MultiWeb2Factory::GetCLSID() const
{
   return CLSID_MultiWeb2Factory;
}

CLSID MultiWeb2Factory::GetFamilyCLSID() const
{
   return CLSID_RibbedBeamFamily;
}

std::_tstring MultiWeb2Factory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring MultiWeb2Factory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE MultiWeb2Factory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR MultiWeb2Factory::GetImageResourceName() const
{
   return _T("MultiWeb2");
}

HICON  MultiWeb2Factory::GetIcon()  const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_MULTIWEB2) );
}

void MultiWeb2Factory::GetDimensions(const BeamFactory::Dimensions& dimensions,
                                      Float64& h1,Float64& h2,Float64& h3,
                                      Float64& t1,Float64& t2,Float64& t3,
                                      Float64& f1,
                                      Float64& c1,Float64& c2,
                                      Float64& w2,Float64& wmin,Float64& wmax) const
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

Float64 MultiWeb2Factory::GetDimension(const BeamFactory::Dimensions& dimensions,const std::_tstring& name) const
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

pgsTypes::SupportedDeckTypes MultiWeb2Factory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
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

pgsTypes::SupportedBeamSpacings MultiWeb2Factory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsConstantAdjacent);
   return sbs;
}

bool MultiWeb2Factory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool MultiWeb2Factory::ConvertBeamSpacing(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
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

pgsTypes::WorkPointLocations MultiWeb2Factory::GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::WorkPointLocations wpls;
   wpls.push_back(pgsTypes::wplTopGirder);
//   wpls.push_back(pgsTypes::wplBottomGirder);

   return wpls;
}

bool MultiWeb2Factory::IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation wpType) const
{
   pgsTypes::WorkPointLocations sbs = GetSupportedWorkPointLocations(spacingType);
   auto found = std::find(sbs.cbegin(), sbs.cend(),wpType);
   return found == sbs.end() ? false : true;
}

std::vector<pgsTypes::GirderOrientationType> MultiWeb2Factory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb, pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal,pgsTypes::Balanced };
   return types;
}

bool MultiWeb2Factory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType MultiWeb2Factory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes MultiWeb2Factory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes MultiWeb2Factory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

void MultiWeb2Factory::GetAllowableSpacingRange(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
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

WebIndexType MultiWeb2Factory::GetWebCount(const BeamFactory::Dimensions& dimensions) const
{
   return 2;
}

Float64 MultiWeb2Factory::GetBeamHeight(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 H1 = GetDimension(dimensions,_T("H1"));
   Float64 H2 = GetDimension(dimensions,_T("H2"));
   Float64 H3 = GetDimension(dimensions,_T("H3"));

   return H1 + H2 + H3;
}

Float64 MultiWeb2Factory::GetBeamWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 T1 = GetDimension(dimensions,_T("T1"));
   Float64 T2 = GetDimension(dimensions,_T("T2"));
   Float64 T3 = GetDimension(dimensions,_T("T3"));

   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 Wmax = GetDimension(dimensions,_T("Wmax"));

   return 2*(T1+T2+T3+Wmax) + W2;
}

void MultiWeb2Factory::GetBeamTopWidth(const BeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const
{
   Float64 T1 = GetDimension(dimensions,_T("T1"));
   Float64 T2 = GetDimension(dimensions,_T("T2"));
   Float64 T3 = GetDimension(dimensions,_T("T3"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 C2 = GetDimension(dimensions,_T("C2"));
   Float64 Wmin = GetDimension(dimensions,_T("Wmin"));

   Float64 top = 2*(T1+T2+T3+Wmin-C2) + W2;

   top /= 2.0;

   *pLeftWidth = top;
   *pRightWidth = top;
}

bool MultiWeb2Factory::IsShearKey(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void MultiWeb2Factory::GetShearKeyAreas(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool MultiWeb2Factory::HasLongitudinalJoints() const
{
   return false;
}

bool MultiWeb2Factory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool MultiWeb2Factory::HasTopFlangeThickening() const
{
   return false;
}

bool MultiWeb2Factory::CanPrecamber() const
{
   return false;
}

GirderIndexType MultiWeb2Factory::GetMinimumBeamCount() const
{
   return 1;
}

void MultiWeb2Factory::DimensionAndPositionBeam(std::shared_ptr<WBFL::EAF::Broker> pBroker,const BeamFactory::Dimensions& dimensions, IMultiWeb2* pBeam) const
{
   Float64 c1, c2;
   Float64 h1, h2, h3;
   Float64 w2, wmin, wmax;
   Float64 t1, t2, t3;
   Float64 f1;
   GetDimensions(dimensions, h1, h2, h3, t1, t2, t3, f1, c1, c2, w2, wmin, wmax);

   pBeam->put_C1(c1);
   pBeam->put_C2(c2);
   pBeam->put_H1(h1);
   pBeam->put_H2(h2);
   pBeam->put_H3(h3);
   pBeam->put_T1(t1);
   pBeam->put_T2(t2);
   pBeam->put_T3(t3);
   pBeam->put_T4(0);
   pBeam->put_T5(0);
   pBeam->put_F1(f1);
   pBeam->put_F2(0);
   pBeam->put_W2(w2);
   pBeam->put_WebCount(2);


   // figure out the web spacing, w2, based on the girder spacing
   Float64 w1;
   if (pBroker == nullptr)
   {
      // just use the max
      w1 = wmax;
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

      Float64 top_flange_max = 2 * (wmax + t1 + t2 + t3) + w2;
      Float64 top_flange_min = 2 * (wmin + t1 + t2 + t3) + w2;

      // if this is a fixed width section, then set the spacing equal to the width
      if (IsEqual(top_flange_max, top_flange_min))
      {
         spacing = top_flange_max;
      }

      w1 = (spacing - 2 * (t1 + t2 + t3) - w2) / 2;
   }
   pBeam->put_W1(w1);

   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_Height(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}
