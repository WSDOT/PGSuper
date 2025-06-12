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

// NUBeamFactory.cpp : Implementation of CNUBeamFactory
#include "stdafx.h"
#include "Beams.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "NUBeamFactory.h"
#include <Beams/IBeamDistFactorEngineer.h>
#include <Beams/PsBeamLossEngineer.h>
#include <Beams/TimeStepLossEngineer.h>
#include "StrandMoverImpl.h"
#include <GeomModel\NUBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\Intervals.h>
#include <IFace\AgeAdjustedMaterial.h>

#include <Beams\Helper.h>
#include <PgsExt/PoiMgr.h>

#include <PsgLib\BridgeDescription2.h>
#include <psgLib/SectionPropertiesCriteria.h>
#include <psgLib/SpecificationCriteria.h>
#include <psgLib/GirderLibraryEntry.h>

using namespace PGS::Beams;

INIT_BEAM_FACTORY_SINGLETON(NUBeamFactory)

NUBeamFactory::NUBeamFactory() : BeamFactory()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.emplace_back(_T("D1"));
   m_DimNames.emplace_back(_T("D2"));
   m_DimNames.emplace_back(_T("D3"));
   m_DimNames.emplace_back(_T("D4"));
   m_DimNames.emplace_back(_T("D5"));
   m_DimNames.emplace_back(_T("R1"));
   m_DimNames.emplace_back(_T("R2"));
   m_DimNames.emplace_back(_T("R3"));
   m_DimNames.emplace_back(_T("R4"));
   m_DimNames.emplace_back(_T("T"));
   m_DimNames.emplace_back(_T("W1"));
   m_DimNames.emplace_back(_T("W2"));
   m_DimNames.emplace_back(_T("C1"));

   // Default beam is Caltrans Super Girder
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 3.000,WBFL::Units::Measure::Inch)); // D1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 1.750,WBFL::Units::Measure::Inch)); // D2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(54.875,WBFL::Units::Measure::Inch)); // D3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 6.000,WBFL::Units::Measure::Inch)); // D4
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 6.375,WBFL::Units::Measure::Inch)); // D5
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(10.000,WBFL::Units::Measure::Inch)); // R1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(10.000,WBFL::Units::Measure::Inch)); // R2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 2.500,WBFL::Units::Measure::Inch)); // R3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 2.500,WBFL::Units::Measure::Inch)); // R4
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 7.000,WBFL::Units::Measure::Inch)); // T
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(48.000,WBFL::Units::Measure::Inch)); // W1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(43.000,WBFL::Units::Measure::Inch)); // W2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 1.000,WBFL::Units::Measure::Inch)); // C1

   // SI Units
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D3
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D4
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // D5
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // R1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // R2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // R3
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // R4
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // T
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // C1

   // US Units
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D3
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D4
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // D5
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // R1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // R2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // R3
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // R4
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // T
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // C1
}

void NUBeamFactory::CreateGirderSection(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const BeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   CComPtr<INUGirderSection> gdrSection;
   gdrSection.CoCreateInstance(CLSID_NUGirderSection);
   CComPtr<INUBeam> beam;
   gdrSection->get_Beam(&beam);

   DimensionAndPositionBeam(dimensions, beam);

   gdrSection.QueryInterface(ppSection);
}

void NUBeamFactory::CreateSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
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

void NUBeamFactory::CreateSegmentShape(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   CComPtr<INUBeam> beam;
   beam.CoCreateInstance(CLSID_NUBeam);

   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   DimensionAndPositionBeam(dimensions, beam);

   beam.QueryInterface(ppShape);
}

Float64 NUBeamFactory::GetSegmentHeight(std::shared_ptr<WBFL::EAF::Broker> pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   Float64 d1, d2, d3, d4, d5;
   Float64 r1, r2, r3, r4;
   Float64 t;
   Float64 w1, w2;
   Float64 c1;

   GetDimensions(dimensions, d1, d2, d3, d4, d5, r1, r2, r3, r4, t, w1, w2, c1);

   return d1 + d2 + d3 + d4 + d5;
}

void NUBeamFactory::ConfigureSegment(std::shared_ptr<WBFL::EAF::Broker> pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void NUBeamFactory::LayoutSectionChangePointsOfInterest(std::shared_ptr<WBFL::EAF::Broker> pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE  );

   VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);
   VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);
}

std::unique_ptr<DistFactorEngineer> NUBeamFactory::CreateDistFactorEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect) const
{
   return std::make_unique<IBeamDistFactorEngineer>(pBroker, statusGroupID);
}

std::unique_ptr<PsLossEngineerBase> NUBeamFactory::CreatePsLossEngineer(std::shared_ptr<WBFL::EAF::Broker> pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey) const
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

void NUBeamFactory::CreateStrandMover(const BeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  BeamFactory::BeamFace endTopFace, Float64 endTopLimit, BeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  BeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, BeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
{
   Float64 d1,d2,d3,d4,d5;
   Float64 r1,r2,r3,r4;
   Float64 t;
   Float64 w1,w2;
   Float64 c1;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,r1,r2,r3,r4,t,w1,w2,c1);

   // set the shape for harped strand bounds - only in the thinnest part of the web
   CComPtr<IRectangle> harp_rect;
   HRESULT hr = harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   Float64 width = t;
   Float64 depth = (Hg < 0 ? d1 + d2 + d3 + d4 + d5 : Hg);

   harp_rect->put_Width(width);
   harp_rect->put_Height(depth);

   CComPtr<IPoint2d> hook;
   hook.CoCreateInstance(CLSID_Point2d);
   hook->Move(0, -depth/2.0);

   harp_rect->putref_HookPoint(hook);

   CComPtr<IShape> shape;
   harp_rect->get_Shape(&shape);

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);
   CComPtr<IStrandMover> sm = pStrandMover;

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(shape, 0.0);
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

const std::vector<std::_tstring>& NUBeamFactory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& NUBeamFactory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const WBFL::Units::Length*>& NUBeamFactory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool NUBeamFactory::ValidateDimensions(const Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const
{
   Float64 d1,d2,d3,d4,d5;
   Float64 r1,r2,r3,r4;
   Float64 t;
   Float64 w1,w2;
   Float64 c1;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,r1,r2,r3,r4,t,w1,w2,c1);

// D1  0
// D2  1
// D3  2
// D4  3
// D5  4
// R1  5
// R2  6
// R3  7
// R4  8
// T   9
// W1  10
// W2  11
// C1  12
   
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
      os << _T("D2 must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d3 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D3 must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d4 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][3];
      std::_tostringstream os;
      os << _T("D4 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d5 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("D5 must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( r1 < 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][5];
      std::_tostringstream os;
      os << _T("R1 must be zero or greater") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( r2 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("R2 must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( r3 < 0.0 )
   {
      std::_tostringstream os;
      os << _T("R3 must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( r4 < 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
      std::_tostringstream os;
      os << _T("R4 must be zero or greater") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][10];
      std::_tostringstream os;
      os << _T("W1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( w2 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("W2 must be greater than 0.0") << std::ends;
      *strErrMsg = os.str();
      return false;
   }
   
   if ( t <= 0.0 )
   {
      const WBFL::Units::Length* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
      std::_tostringstream os;
      os << _T("T must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( d1 < r3 )
   {
      std::_tostringstream os;
      os << _T("R3 must be less than or equal to D1") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( d5 < r4 + c1 )
   {
      std::_tostringstream os;
      os << _T("R4 + C1 must be less than or equal to D5") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   return true;
}

void NUBeamFactory::SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const BeamFactory::Dimensions& dimensions) const
{
   // version 2.... added C1
   pSave->BeginUnit(_T("NUBeamDimensions"),2.0);
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

BeamFactory::Dimensions NUBeamFactory::LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const
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

   if (14 <= parent_version && !pLoad->BeginUnit(_T("NUBeamDimensions")))
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   Float64 version = pLoad->GetVersion();

   for(const auto& name : m_DimNames)
   {
      Float64 value;

      if ( name == _T("C1") && version < 2 )
      {
         // C1 didn't exist before version 2
         value = 0.0;
      }
      else
      {
         pLoad->Property(name.c_str(),&value);
      }

      dimensions.emplace_back(name,value);
   }

   if (14 <= parent_version && !pLoad->EndUnit())
   {
      THROW_LOAD(InvalidFileFormat, pLoad);
   }

   return dimensions;
}

bool NUBeamFactory::IsPrismatic(const BeamFactory::Dimensions& dimensions) const
{
   return true;
}

bool NUBeamFactory::IsPrismatic(const CSegmentKey& segmentKey) const
{
   return true;
}

bool NUBeamFactory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

std::_tstring NUBeamFactory::GetImage() const
{
   return std::_tstring(_T("NUBeam.jpg"));
}

std::_tstring NUBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      strImage =  _T("IBeam_Slab_CIP.gif");
      break;

   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("IBeam_Slab_SIP.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring NUBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("+Mn_IBeam_Composite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring NUBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("-Mn_IBeam_Composite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring NUBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("Vn_IBeam.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring NUBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if ( section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth || 
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition() )
   {
      return _T("IBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("IBeam_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring NUBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(std::shared_ptr<WBFL::EAF::Broker> pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if ( section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth || 
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition() )
   {
      return _T("IBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("IBeam_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID NUBeamFactory::GetCLSID() const
{
   return CLSID_NUBeamFactory;
}

CLSID NUBeamFactory::GetFamilyCLSID() const
{
   return CLSID_WFBeamFamily;
}

std::_tstring NUBeamFactory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring NUBeamFactory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE NUBeamFactory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR NUBeamFactory::GetImageResourceName() const
{
   return _T("NUBEAM");
}

HICON  NUBeamFactory::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_NUBEAM) );
}

void NUBeamFactory::GetDimensions(const BeamFactory::Dimensions& dimensions,
                                   Float64& d1,Float64& d2,Float64& d3,Float64& d4,Float64& d5,
                                   Float64& r1,Float64& r2,Float64& r3,Float64& r4,
                                   Float64& t,
                                   Float64& w1,Float64& w2,
                                   Float64& c1) const
{
   d1 = GetDimension(dimensions,_T("D1"));
   d2 = GetDimension(dimensions,_T("D2"));
   d3 = GetDimension(dimensions,_T("D3"));
   d4 = GetDimension(dimensions,_T("D4"));
   d5 = GetDimension(dimensions,_T("D5"));
   r1 = GetDimension(dimensions,_T("R1"));
   r2 = GetDimension(dimensions,_T("R2"));
   r3 = GetDimension(dimensions,_T("R3"));
   r4 = GetDimension(dimensions,_T("R4"));
   t  = GetDimension(dimensions,_T("T"));
   w1 = GetDimension(dimensions,_T("W1"));
   w2 = GetDimension(dimensions,_T("W2"));
   c1 = GetDimension(dimensions,_T("C1"));
}

Float64 NUBeamFactory::GetDimension(const BeamFactory::Dimensions& dimensions,const std::_tstring& name) const
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

pgsTypes::SupportedDeckTypes NUBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
{
   pgsTypes::SupportedDeckTypes sdt;
   switch(sbs)
   {
   case pgsTypes::sbsUniform:
   case pgsTypes::sbsGeneral:
      sdt.push_back(pgsTypes::sdtCompositeCIP);
      sdt.push_back(pgsTypes::sdtCompositeSIP);
      break;

   default:
      ATLASSERT(false);
   }
   return sdt;
}

pgsTypes::SupportedBeamSpacings NUBeamFactory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

bool NUBeamFactory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool NUBeamFactory::ConvertBeamSpacing(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   return false;
}

pgsTypes::WorkPointLocations NUBeamFactory::GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::WorkPointLocations wpls;
   wpls.push_back(pgsTypes::wplTopGirder);
   wpls.push_back(pgsTypes::wplBottomGirder);

   return wpls;
}

bool NUBeamFactory::IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation wpType) const
{
   pgsTypes::WorkPointLocations sbs = GetSupportedWorkPointLocations(spacingType);
   auto found = std::find(sbs.cbegin(), sbs.cend(),wpType);
   return found == sbs.end() ? false : true;
}

std::vector<pgsTypes::GirderOrientationType> NUBeamFactory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb/*, pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal*/ };
   return types;
}

bool NUBeamFactory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation == pgsTypes::Plumb ? true : false;
}

pgsTypes::GirderOrientationType NUBeamFactory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return pgsTypes::Plumb;
}

pgsTypes::SupportedDiaphragmTypes NUBeamFactory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes NUBeamFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

void NUBeamFactory::GetAllowableSpacingRange(const BeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));

   Float64 gw = Max(W1, W2);


   if ( sdt == pgsTypes::sdtCompositeCIP || sdt == pgsTypes::sdtCompositeSIP )
   {
      if ( sbs == pgsTypes::sbsUniform || sbs == pgsTypes::sbsGeneral )
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
      ATLASSERT(false); // shouldn't get here
   }
}

WebIndexType NUBeamFactory::GetWebCount(const BeamFactory::Dimensions& dimensions) const
{
   return 1;
}

Float64 NUBeamFactory::GetBeamHeight(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 D1 = GetDimension(dimensions,_T("D1"));
   Float64 D2 = GetDimension(dimensions,_T("D2"));
   Float64 D3 = GetDimension(dimensions,_T("D3"));
   Float64 D4 = GetDimension(dimensions,_T("D4"));
   Float64 D5 = GetDimension(dimensions,_T("D5"));

   return D1 + D2 + D3 + D4 + D5;
}

Float64 NUBeamFactory::GetBeamWidth(const BeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   return Max(GetDimension(dimensions,_T("W1")),GetDimension(dimensions,_T("W2")));
}

void NUBeamFactory::GetBeamTopWidth(const BeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const
{
   Float64 W1 = GetDimension(dimensions,_T("W1"));

   Float64 top = W1;
   top /= 2.0;

   *pLeftWidth = top;
   *pRightWidth = top;
}

bool NUBeamFactory::IsShearKey(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void NUBeamFactory::GetShearKeyAreas(const BeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool NUBeamFactory::HasLongitudinalJoints() const
{
   return false;
}

bool NUBeamFactory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool NUBeamFactory::HasTopFlangeThickening() const
{
   return false;
}

bool NUBeamFactory::CanPrecamber() const
{
   return true;
}

GirderIndexType NUBeamFactory::GetMinimumBeamCount() const
{
   return 2;
}

void NUBeamFactory::DimensionAndPositionBeam(const BeamFactory::Dimensions& dimensions, INUBeam* pBeam) const
{
   Float64 d1, d2, d3, d4, d5;
   Float64 r1, r2, r3, r4;
   Float64 t;
   Float64 w1, w2;
   Float64 c1;

   GetDimensions(dimensions, d1, d2, d3, d4, d5, r1, r2, r3, r4, t, w1, w2, c1);
   pBeam->put_W1(w1);
   pBeam->put_W2(w2);
   pBeam->put_D1(d1);
   pBeam->put_D2(d2);
   pBeam->put_D3(d3);
   pBeam->put_D4(d4);
   pBeam->put_D5(d5);
   pBeam->put_T(t);
   pBeam->put_R1(r1);
   pBeam->put_R2(r2);
   pBeam->put_R3(r3);
   pBeam->put_R4(r4);
   pBeam->put_C1(c1);


   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_Height(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}
