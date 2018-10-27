///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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
#include <Plugins\Beams.h>

#include <Plugins\BeamFamilyCLSID.h>

#include "BoxBeamFactoryImpl.h"
#include "BoxBeamDistFactorEngineer.h"
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
#include <PgsExt\BridgeDescription2.h>
#include <IFace\Intervals.h>

#include <IFace\StatusCenter.h>
#include <PgsExt\StatusItem.h>

#include <Beams\Helper.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CBoxBeamFactoryImpl

void CBoxBeamFactoryImpl::CreateGirderSection(IBroker* pBroker, StatusItemIDType statusID, const IBeamFactory::Dimensions& dimensions, Float64 overallHeight, Float64 bottomFlangeHeight, IGirderSection** ppSection) const
{
   CComPtr<IBoxBeamSection> gdrSection;
   gdrSection.CoCreateInstance(CLSID_BoxBeamSection);
   CComPtr<IBoxBeam> beam;
   gdrSection->get_Beam(&beam);

   DimensionBeam(dimensions, beam);

   gdrSection.QueryInterface(ppSection);
}

void CBoxBeamFactoryImpl::CreateSegment(IBroker* pBroker,StatusItemIDType statusID,const CSegmentKey& segmentKey,ISuperstructureMemberSegment** ppSegment) const
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

void CBoxBeamFactoryImpl::ConfigureSegment(IBroker* pBroker, StatusItemIDType statusID, const CSegmentKey& segmentKey, ISuperstructureMemberSegment* pSSMbrSegment) const
{
   // do nothing... all the configuration was done in CreateSegment
}

void CBoxBeamFactoryImpl::CreateSegmentShape(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs, pgsTypes::SectionBias sectionBias,IShape** ppShape) const
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

Float64 CBoxBeamFactoryImpl::GetSegmentHeight(IBroker* pBroker, const CPrecastSegmentData* pSegment, Float64 Xs) const
{
   const CSplicedGirderData* pGirder = pSegment->GetGirder();
   const GirderLibraryEntry* pGirderEntry = pGirder->GetGirderLibraryEntry();
   const auto& dimensions = pGirderEntry->GetDimensions();
   Float64 H1 = GetDimension(dimensions, _T("H1"));
   Float64 H2 = GetDimension(dimensions, _T("H2"));
   Float64 H3 = GetDimension(dimensions, _T("H3"));
   return H1 + H2 + H3;
}

void CBoxBeamFactoryImpl::LayoutSectionChangePointsOfInterest(IBroker* pBroker,const CSegmentKey& segmentKey,pgsPoiMgr* pPoiMgr) const
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetSegmentLength(segmentKey);

   pgsPointOfInterest poiStart(segmentKey,0.00,   POI_SECTCHANGE_RIGHTFACE);
   pgsPointOfInterest poiEnd(segmentKey,gdrLength,POI_SECTCHANGE_LEFTFACE );

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);

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

void CBoxBeamFactoryImpl::CreateDistFactorEngineer(IBroker* pBroker,StatusItemIDType statusID,const pgsTypes::SupportedBeamSpacing* pSpacingType,
                                               const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,
                                               IDistFactorEngineer** ppEng) const
{
   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription2* pDeck = pBridgeDesc->GetDeckDescription();

   // use passed value if not null
   pgsTypes::SupportedDeckType deckType = (pDeckType!=nullptr) ? *pDeckType : pDeck->GetDeckType();

   if ( deckType == pgsTypes::sdtCompositeOverlay || deckType == pgsTypes::sdtNone )
   {
      CComObject<CBoxBeamDistFactorEngineer>* pEngineer;
      CComObject<CBoxBeamDistFactorEngineer>::CreateInstance(&pEngineer);
      pEngineer->SetBroker(pBroker,statusID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
   else
   {
      // this is a type b section... type b's are the same as type c's which are U-beams
      ATLASSERT( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP );

      CComObject<CUBeamDistFactorEngineer>* pEngineer;
      CComObject<CUBeamDistFactorEngineer>::CreateInstance(&pEngineer);
      pEngineer->Init(true, false); // this is a type b cross section
      pEngineer->SetBroker(pBroker,statusID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
}

void CBoxBeamFactoryImpl::CreatePsLossEngineer(IBroker* pBroker,StatusItemIDType statusGroupID,const CGirderKey& girderKey,IPsLossEngineer** ppEng) const
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
       pEngineer->Init(BoxBeam);
       pEngineer->SetBroker(pBroker,statusGroupID);
       (*ppEng) = pEngineer;
       (*ppEng)->AddRef();
   }
}

const std::vector<std::_tstring>& CBoxBeamFactoryImpl::GetDimensionNames() const
{
   return m_DimNames;
}

const std::vector<Float64>& CBoxBeamFactoryImpl::GetDefaultDimensions() const
{
   return m_DefaultDims;
}

const std::vector<const unitLength*>& CBoxBeamFactoryImpl::GetDimensionUnits(bool bSIUnits) const
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CBoxBeamFactoryImpl::IsPrismatic(const IBeamFactory::Dimensions& dimensions) const
{
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   return IsZero(endBlockLength) ? true : false;
}

bool CBoxBeamFactoryImpl::IsPrismatic(const CSegmentKey& segmentKey) const
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

bool CBoxBeamFactoryImpl::IsSymmetric(const CSegmentKey& segmentKey) const
{
   return true;
}

std::_tstring CBoxBeamFactoryImpl::GetName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

CLSID CBoxBeamFactoryImpl::GetFamilyCLSID() const
{
   return CLSID_BoxBeamFamily;
}

std::_tstring CBoxBeamFactoryImpl::GetGirderFamilyName() const
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CBoxBeamFactoryImpl::GetPublisher() const
{
   return std::_tstring(_T("WSDOT"));
}

std::_tstring CBoxBeamFactoryImpl::GetPublisherContactInformation() const
{
   return std::_tstring(_T("http://www.wsdot.wa.gov/eesc/bridge"));
}

HINSTANCE CBoxBeamFactoryImpl::GetResourceInstance() const
{
   return _Module.GetResourceInstance();
}

Float64 CBoxBeamFactoryImpl::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name) const
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

pgsTypes::SupportedDeckTypes CBoxBeamFactoryImpl::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs) const
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

pgsTypes::SupportedBeamSpacings CBoxBeamFactoryImpl::GetSupportedBeamSpacings() const
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);

   return sbs;
}

bool CBoxBeamFactoryImpl::IsSupportedBeamSpacing(pgsTypes::SupportedBeamSpacing spacingType) const
{
   pgsTypes::SupportedBeamSpacings sbs = GetSupportedBeamSpacings();
   auto found = std::find(sbs.cbegin(), sbs.cend(),spacingType);
   return found == sbs.end() ? false : true;
}

bool CBoxBeamFactoryImpl::ConvertBeamSpacing(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedBeamSpacing spacingType, Float64 spacing, pgsTypes::SupportedBeamSpacing* pNewSpacingType, Float64* pNewSpacing, Float64* pNewTopWidth) const
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

std::vector<pgsTypes::GirderOrientationType> CBoxBeamFactoryImpl::GetSupportedGirderOrientation() const
{
   std::vector<pgsTypes::GirderOrientationType> types{ pgsTypes::Plumb,pgsTypes::StartNormal,pgsTypes::MidspanNormal,pgsTypes::EndNormal };
   return types;
}

bool CBoxBeamFactoryImpl::IsSupportedGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return true;
}

pgsTypes::GirderOrientationType CBoxBeamFactoryImpl::ConvertGirderOrientation(pgsTypes::GirderOrientationType orientation) const
{
   return orientation;
}

pgsTypes::SupportedDiaphragmTypes CBoxBeamFactoryImpl::GetSupportedDiaphragms() const
{
   pgsTypes::SupportedDiaphragmTypes diaphragmTypes;
   diaphragmTypes.push_back(pgsTypes::dtPrecast);
   diaphragmTypes.push_back(pgsTypes::dtCastInPlace);
   return diaphragmTypes;
}

pgsTypes::SupportedDiaphragmLocationTypes CBoxBeamFactoryImpl::GetSupportedDiaphragmLocations(pgsTypes::DiaphragmType type) const
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

WebIndexType CBoxBeamFactoryImpl::GetWebCount(const IBeamFactory::Dimensions& dimensions) const
{
   return 2;
}


Float64 CBoxBeamFactoryImpl::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType) const
{
   Float64 H1 = GetDimension(dimensions,_T("H1"));
   Float64 H2 = GetDimension(dimensions,_T("H2"));
   Float64 H3 = GetDimension(dimensions,_T("H3"));

   return H1 + H2 + H3;
}

std::_tstring CBoxBeamFactoryImpl::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CBoxBeamFactoryImpl::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CBoxBeamFactoryImpl::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CBoxBeamFactoryImpl::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType) const
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

std::_tstring CBoxBeamFactoryImpl::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
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
         strImage =  _T("SpreadBoxBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
      }
      else
      {
         strImage =  _T("SpreadBoxBeam_Effective_Flange_Width_Interior_Girder.gif");
      }
      break;

   case pgsTypes::sdtCompositeOverlay:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
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

std::_tstring CBoxBeamFactoryImpl::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType) const
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
         strImage =  _T("SpreadBoxBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
      }
      else
      {
         strImage =  _T("SpreadBoxBeam_Effective_Flange_Width_Exterior_Girder.gif");
      }
      break;

   case pgsTypes::sdtCompositeOverlay:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
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

bool CBoxBeamFactoryImpl::HasTopFlangeThickening() const
{
   return false;
}

bool CBoxBeamFactoryImpl::CanPrecamber() const
{
   return false;
}

GirderIndexType CBoxBeamFactoryImpl::GetMinimumBeamCount() const
{
   return 1;
}

void CBoxBeamFactoryImpl::DimensionBeam(const IBeamFactory::Dimensions& dimensions, IBoxBeam* pBeam) const
{
   // Hook point is at bottom center of bounding box.
   // Adjust hook point so top center of bounding box is at (0,0)
   Float64 Hg;
   pBeam->get_Height(&Hg);

   CComPtr<IPoint2d> hookPt;
   pBeam->get_HookPoint(&hookPt);
   hookPt->Offset(0, -Hg);
}

void CBoxBeamFactoryImpl::ConfigureGirderShape(const CPrecastSegmentData* pSegment,const IBeamFactory::Dimensions& dimensions, IBoxBeam* pBeam) const
{
   DimensionBeam(dimensions, pBeam);

   const CSegmentKey& segmentKey(pSegment->GetSegmentKey());
   if (segmentKey.girderIndex == 0 && ExcludeExteriorBeamShearKeys())
   {
      pBeam->put_LeftBlockOut(VARIANT_FALSE);
   }

   const CGirderGroupData* pGroup = pSegment->GetGirder()->GetGirderGroup();
   if (segmentKey.girderIndex == pGroup->GetGirderCount() - 1 && ExcludeExteriorBeamShearKeys())
   {
      pBeam->put_RightBlockOut(VARIANT_FALSE);
   }

   pBeam->put_UseOverallWidth(UseOverallWidth() ? VARIANT_TRUE : VARIANT_FALSE);
}
