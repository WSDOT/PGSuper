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

// TxDotDoubleTFactory.cpp : Implementation of CTxDotDoubleTFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "TxDotDoubleTFactory.h"
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

#include <IFace\AgeAdjustedMaterial.h>
#include <Beams\Helper.h>

#include <PgsExt\BridgeDescription2.h>

#include <IFace\StatusCenter.h>
#include <PgsExt\StatusItem.h>

#include <psgLib/SectionPropertiesCriteria.h>
#include <psgLib/SpecificationCriteria.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CTxDotDoubleTFactory
HRESULT CTxDotDoubleTFactory::FinalConstruct()
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
   m_DimNames.emplace_back(_T("W1"));
   m_DimNames.emplace_back(_T("W2"));
   m_DimNames.emplace_back(_T("J"));

   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 0.0,WBFL::Units::Measure::Inch)); // C1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 0.0,WBFL::Units::Measure::Inch)); // C2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(27.0,WBFL::Units::Measure::Inch)); // H1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 3.0,WBFL::Units::Measure::Inch)); // H2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 6.0,WBFL::Units::Measure::Inch)); // H3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 0.0,WBFL::Units::Measure::Inch)); // T1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 6.5,WBFL::Units::Measure::Inch)); // T2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 1.5,WBFL::Units::Measure::Inch)); // T3
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 3.0,WBFL::Units::Measure::Inch)); // F1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(24.0,WBFL::Units::Measure::Inch)); // W1
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits(48.0,WBFL::Units::Measure::Inch)); // W2
   m_DefaultDims.emplace_back(WBFL::Units::ConvertToSysUnits( 1.0,WBFL::Units::Measure::Inch)); // J

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
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W1
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // W2
   m_DimUnits[0].emplace_back(&WBFL::Units::Measure::Millimeter); // J

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
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W1
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // W2
   m_DimUnits[1].emplace_back(&WBFL::Units::Measure::Inch); // J
   

   return S_OK;
}

void CTxDotDoubleTFactory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,const IBeamFactory::Dimensions& dimensions,Float64 overallHeight,Float64 bottomFlangeHeight,IGirderSection** ppSection) const
{
   CComPtr<IMultiWebSection2> gdrSection;
   gdrSection.CoCreateInstance(CLSID_MultiWebSection2);
   CComPtr<IMultiWeb2> beam;
   gdrSection->get_Beam(&beam);

   DimensionAndPositionBeam(nullptr, dimensions, beam);

   gdrSection.QueryInterface(ppSection);
}

void CTxDotDoubleTFactory::CreateSegment(IBroker* pBroker,StatusGroupIDType statusGroupID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
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

   CComQIPtr<IMultiWebSection2> section(gdrSection);
   CComPtr<IMultiWeb2> beam;
   section->get_Beam(&beam);

   DimensionAndPositionBeam(pSegment, dimensions, beam);

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

void CTxDotDoubleTFactory::CreateSegmentShape(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias, IShape** ppShape) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();

   CComPtr<IMultiWeb2> beam;
   beam.CoCreateInstance(CLSID_MultiWeb2);

   DimensionAndPositionBeam(pSegment, dimensions, beam);

   beam.QueryInterface(ppShape);
}

Float64 CTxDotDoubleTFactory::GetSegmentHeight(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   Float64 H1 = GetDimension(dimensions, _T("H1"));
   Float64 H2 = GetDimension(dimensions, _T("H2"));
   Float64 H3 = GetDimension(dimensions, _T("H3"));
   return H1 + H2 + H3;
}

void CTxDotDoubleTFactory::ConfigureSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void CTxDotDoubleTFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,POI_SECTCHANGE_RIGHTFACE );
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE );

   VERIFY(pPoiMgr->AddPointOfInterest(poiStart) != INVALID_ID);
   VERIFY(pPoiMgr->AddPointOfInterest(poiEnd) != INVALID_ID);
}

void CTxDotDoubleTFactory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedBeamSpacing* pSpacingType,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng) const
{
   CComObject<CMultiWebDistFactorEngineer>* pEngineer;
   CComObject<CMultiWebDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);

   pEngineer->SetBeamType(CMultiWebDistFactorEngineer::btMultiWebTee);

   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CTxDotDoubleTFactory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng) const
{
   GET_IFACE2(pBroker, ILossParameters, pLossParams);
   if ( pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP )
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

void CTxDotDoubleTFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions,  Float64 Hg,
                                  IBeamFactory::BeamFace endTopFace, Float64 endTopLimit, IBeamFactory::BeamFace endBottomFace, Float64 endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, Float64 hpTopLimit, IBeamFactory::BeamFace hpBottomFace, Float64 hpBottomLimit, 
                                  Float64 endIncrement, Float64 hpIncrement, IStrandMover** strandMover) const
{
   Float64 h1,h2,h3;
   Float64 c1,c2;
   Float64 w1,w2,j;
   Float64 t1,t2,t3;
   Float64 f1;
   GetDimensions(dimensions,h1,h2,h3,t1,t2,t3,f1,c1,c2,w1,w2,j);

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
   Float64 hptb  = hpTopFace     == IBeamFactory::BeamBottom ? hpTopLimit     - depth : -hpTopLimit;
   Float64 hpbb  = hpBottomFace  == IBeamFactory::BeamBottom ? hpBottomLimit  - depth : -hpBottomLimit;
   Float64 endtb = endTopFace    == IBeamFactory::BeamBottom ? endTopLimit    - depth : -endTopLimit;
   Float64 endbb = endBottomFace == IBeamFactory::BeamBottom ? endBottomLimit - depth : -endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(0, depth, endtb, endbb, hptb, hpbb, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}

const std::vector<std::_tstring>& CTxDotDoubleTFactory::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& CTxDotDoubleTFactory::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const WBFL::Units::Length*>& CTxDotDoubleTFactory::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CTxDotDoubleTFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg) const
{
   Float64 h1,h2,h3;
   Float64 c1,c2;
   Float64 w1,w2,j;
   Float64 t1,t2,t3;
   Float64 f1;
   GetDimensions(dimensions,h1,h2,h3,t1,t2,t3,f1,c1,c2,w1,w2,j);

 // C1
 // C2
 // H1
 // H2
 // H3
 // T1
 // T2
 // T3
 // F1
 // W1
 // W2
 // J

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

   if ( w1 <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("W1 must be greater than 0.0")<< std::ends;
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

   if ( j < 0.0 )
   {
      std::_tostringstream os;
      os << _T("J must be zero or greater")<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   // relations
   if ( w1 < f1 )
   {
      std::_tostringstream os;
      os << _T("W1 must be greater than F1")<< std::ends;
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

void CTxDotDoubleTFactory::SaveSectionDimensions(WBFL::System::IStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions) const
{
   pSave->BeginUnit(_T("TxDOTDoubleTeeDimensions"),1.0);
   for(const auto& name : m_DimNames)
   {
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CTxDotDoubleTFactory::LoadSectionDimensions(WBFL::System::IStructuredLoad* pLoad) const
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
      if (pLoad->BeginUnit(_T("TxDOTDoubleTeeDimensions")))
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
         
         if ( dimVersion < 2 && parent_version < 3.0 && (name == _T("C1") || name == _T("C2")) )
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

bool CTxDotDoubleTFactory::IsPrismatic(const IBeamFactory::Dimensions& dimensions) const
{
   return true;
}

bool CTxDotDoubleTFactory::IsPrismatic(const CSegmentKey& segmentKey) const
{
   return true;
}

bool CTxDotDoubleTFactory::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

std::_tstring CTxDotDoubleTFactory::GetImage() const
{
   return std::_tstring(_T("TxDotDoubleT.gif"));
}

std::_tstring CTxDotDoubleTFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CTxDotDoubleTFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CTxDotDoubleTFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CTxDotDoubleTFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CTxDotDoubleTFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
   {
      return _T("DoubleTee_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("DoubleTee_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring CTxDotDoubleTFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   const auto& specification_criteria = pSpecEntry->GetSpecificationCriteria();
   const auto& section_properties_criteria = pSpecEntry->GetSectionPropertiesCriteria();
   if (section_properties_criteria.EffectiveFlangeWidthMethod == pgsTypes::efwmTribWidth ||
      WBFL::LRFD::BDSManager::Edition::FourthEditionWith2008Interims <= specification_criteria.GetEdition())
   {
      return _T("DoubleTee_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("DoubleTee_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CTxDotDoubleTFactory::GetCLSID() const
{
   return CLSID_TxDotDoubleTFactory;
}

std::_tstring CTxDotDoubleTFactory::GetName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CTxDotDoubleTFactory::GetFamilyCLSID() const
{
   return CLSID_DoubleTeeBeamFamily;
}

std::_tstring CTxDotDoubleTFactory::GetGirderFamilyName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CTxDotDoubleTFactory::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CTxDotDoubleTFactory::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CTxDotDoubleTFactory::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

LPCTSTR CTxDotDoubleTFactory::GetImageResourceName() const
{
   return _T("TxDotDoubleT");
}

HICON  CTxDotDoubleTFactory::GetIcon() const
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TXDOTDOUBLET) );
}

void CTxDotDoubleTFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                      Float64& h1,Float64& h2,Float64& h3,
                                      Float64& t1,Float64& t2,Float64& t3,
                                      Float64& f1,
                                      Float64& c1,Float64& c2,
                                      Float64& w1,Float64& w2,Float64& j) const
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
   w1 = GetDimension(dimensions,_T("W1"));
   w2 = GetDimension(dimensions,_T("W2"));
   j = GetDimension(dimensions,_T("J"));
}

Float64 CTxDotDoubleTFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name) const
{
   for (auto const& dim : dimensions)
   {
      if (name == dim.first)
      {
         return dim.second;
      }
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedDeckTypes CTxDotDoubleTFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
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

pgsTypes::SupportedBeamSpacings CTxDotDoubleTFactory::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);

   return sbs;
}

bool CTxDotDoubleTFactory::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(), spacingType);
   return found == sbs.end() ? false : true;
}

bool CTxDotDoubleTFactory::ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
{
   if (spacingType == pgsTypes::sbsUniform || spacingType == pgsTypes::sbsConstantAdjacent)
   {
      *pNewSpacingType = pgsTypes::sbsUniformAdjacent;
      *pNewSpacing = spacing;
      *pNewTopWidth = 0.0;
      return true;
   }
   return false;
}

pgsTypes::WorkPointLocations CTxDotDoubleTFactory::GetSupportedWorkPointLocations(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::WorkPointLocations wpls;
   wpls.push_back(pgsTypes::wplTopGirder);
//   wpls.push_back(pgsTypes::wplBottomGirder);

   return wpls;
}

bool CTxDotDoubleTFactory::IsSupportedWorkPointLocation(pgsTypes::SupportedBeamSpacing spacingType, pgsTypes::WorkPointLocation wpType) const
{
   pgsTypes::WorkPointLocations sbs = GetSupportedWorkPointLocations(spacingType);
   auto found = std::find(sbs.cbegin(), sbs.cend(), wpType);
   return found == sbs.end() ? false : true;
}

std::vector<pgsTypes::GirderOrientationType> CTxDotDoubleTFactory::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb, pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal,pgsTypes::Balanced };
   return types;
}

bool CTxDotDoubleTFactory::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType CTxDotDoubleTFactory::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes CTxDotDoubleTFactory::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CTxDotDoubleTFactory::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

void CTxDotDoubleTFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, pgsTypes::SupportedBeamSpacing sbs, Float64* minSpacing, Float64* maxSpacing) const
{
   // we only allow adjacent spacing for this girder type so allowable spacing range is the joint spacing
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   Float64  J = GetDimension(dimensions,_T("J"));

   if ( sdt == pgsTypes::sdtCompositeOverlay ||  sdt == pgsTypes::sdtNone )
   {
      if(sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent )
      {
         *minSpacing = 0.0;
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

WebIndexType CTxDotDoubleTFactory::GetWebCount(const IBeamFactory::Dimensions& dimensions) const
{
   return 2;
}

Float64 CTxDotDoubleTFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 H1 = GetDimension(dimensions,_T("H1"));
   Float64 H2 = GetDimension(dimensions,_T("H2"));
   Float64 H3 = GetDimension(dimensions,_T("H3"));

   return H1 + H2 + H3;
}

Float64 CTxDotDoubleTFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 T1 = GetDimension(dimensions,_T("T1"));
   Float64 T2 = GetDimension(dimensions,_T("T2"));
   Float64 T3 = GetDimension(dimensions,_T("T3"));

   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));

   return 2*(T1+T2+T3+W1) + W2;
}

void CTxDotDoubleTFactory::GetBeamTopWidth(const IBeamFactory::Dimensions& dimensions, pgsTypes::MemberEndType endType, Float64* pLeftWidth, Float64* pRightWidth) const
{
   Float64 T1 = GetDimension(dimensions,_T("T1"));
   Float64 T2 = GetDimension(dimensions,_T("T2"));
   Float64 T3 = GetDimension(dimensions,_T("T3"));
   Float64 W1 = GetDimension(dimensions,_T("W1"));
   Float64 W2 = GetDimension(dimensions,_T("W2"));
   Float64 C2 = GetDimension(dimensions,_T("C2"));

   Float64 top = 2*(T1+T2+T3+W1-C2) + W2;
   top /= 2.0;

   *pLeftWidth = top;
   *pRightWidth = top;
}

bool CTxDotDoubleTFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType) const
{
   return false;
}

void CTxDotDoubleTFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint) const
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}

bool CTxDotDoubleTFactory::HasLongitudinalJoints() const
{
   return false;
}

bool CTxDotDoubleTFactory::IsLongitudinalJointStructural(pgsTypes::SupportedDeckType deckType,pgsTypes::AdjacentTransverseConnectivity connectivity) const
{
   return false;
}

bool CTxDotDoubleTFactory::HasTopFlangeThickening() const
{
   return false;
}

bool CTxDotDoubleTFactory::CanPrecamber() const
{
   return false;
}

GirderIndexType CTxDotDoubleTFactory::GetMinimumBeamCount() const
{
   return 1;
}

void CTxDotDoubleTFactory::DimensionAndPositionBeam(const CPrecastSegmentData* pSegment,const IBeamFactory::Dimensions& dimensions, IMultiWeb2* pBeam) const
{
   Float64 c1, c2;
   Float64 h1, h2, h3;
   Float64 w1, w2, j;
   Float64 t1, t2, t3;
   Float64 f1;
   GetDimensions(dimensions, h1, h2, h3, t1, t2, t3, f1, c1, c2, w1, w2, j);

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
   pBeam->put_W1(w1);
   pBeam->put_W2(w2);
   pBeam->put_WebCount(2);

   if (pSegment)
   {
      // if this is an exterior girder, remove the shear key block outs
      const CSegmentKey& segmentKey(pSegment->GetSegmentKey());
      if (segmentKey.girderIndex == 0)
      {
         pBeam->put_LeftBlockOut(VARIANT_FALSE);
      }

      const CGirderGroupData* pGroup = pSegment->GetGirder()->GetGirderGroup();
      if (segmentKey.girderIndex == pGroup->GetGirderCount() - 1)
      {
         pBeam->put_RightBlockOut(VARIANT_FALSE);
      }
   }


   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_Height(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}
