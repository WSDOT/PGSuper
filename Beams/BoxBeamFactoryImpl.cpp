///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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
#include "StrandMoverImpl.h"
#include <BridgeModeling\PrismaticGirderProfile.h>
#include <GeomModel\PrecastBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <PgsExt\BridgeDescription.h>

#include <IFace\StatusCenter.h>
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CBoxBeamFactoryImpl
void CBoxBeamFactoryImpl::CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetGirderLength(spanIdx,gdrIdx);

   double H1 = GetDimension(dimensions,_T("H1"));
   double H2 = GetDimension(dimensions,_T("H2"));
   double H3 = GetDimension(dimensions,_T("H3"));

   Float64 height = H1 + H2 + H3;

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

void CBoxBeamFactoryImpl::LayoutGirderLine(IBroker* pBroker,StatusGroupIDType statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* ssmbr)
{
   CComPtr<IBoxBeamEndBlockSegment> segment;
   HRESULT hr = segment.CoCreateInstance(CLSID_BoxBeamEndBlockSegment);

   // Length of the segments will be measured fractionally
   ssmbr->put_AreSegmentLengthsFractional(VARIANT_TRUE);
   segment->put_Length(-1.0);

   // Build up the beam shape
   GET_IFACE2(pBroker,ILibrary,pLib);
   GET_IFACE2(pBroker,IGirderData, pGirderData);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   segment->put_EndBlockLength(etStart,endBlockLength);
   segment->put_EndBlockLength(etEnd,endBlockLength);

   CComPtr<IGirderSection> gdrsection;
   CreateGirderSection(pBroker,statusGroupID,spanIdx,gdrIdx,dimensions,&gdrsection);

   CComQIPtr<IBoxBeamSection> section(gdrsection);

   CComPtr<IBoxBeam> boxBeamShape;
   section->get_Beam(&boxBeamShape);
   if ( gdrIdx == 0 && ExcludeExteriorBeamShearKeys() )
   {
      boxBeamShape->put_LeftBlockOut(VARIANT_FALSE);
   }

   if ( gdrIdx == pIBridgeDesc->GetSpan(spanIdx)->GetGirderCount()-1 && ExcludeExteriorBeamShearKeys()  )
   {
      boxBeamShape->put_RightBlockOut(VARIANT_FALSE);
   }

   boxBeamShape->put_UseOverallWidth(UseOverallWidth() ? VARIANT_TRUE : VARIANT_FALSE);

   segment->putref_BeamSection(section);

   // Beam materials
   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   CComPtr<IMaterial> material;
   material.CoCreateInstance(CLSID_Material);
   material->put_E(pMaterial->GetEcGdr(spanIdx,gdrIdx));
   material->put_Density(pMaterial->GetStrDensityGdr(spanIdx,gdrIdx));
   segment->putref_Material(material);

   ssmbr->AddSegment(segment);
}

void CBoxBeamFactoryImpl::LayoutSectionChangePointsOfInterest(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsPoiMgr* pPoiMgr)
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetGirderLength(span,gdr);

   pgsPointOfInterest poiStart(span,gdr,0.00);
   poiStart.AddStage(pgsTypes::CastingYard,POI_SECTCHANGE_RIGHTFACE | POI_TABULAR | POI_GRAPHICAL);
   poiStart.AddStage(pgsTypes::Lifting,    POI_SECTCHANGE_RIGHTFACE | POI_TABULAR | POI_GRAPHICAL);
   poiStart.AddStage(pgsTypes::Hauling,    POI_SECTCHANGE_RIGHTFACE | POI_TABULAR | POI_GRAPHICAL);

   pgsPointOfInterest poiEnd(span,gdr,gdrLength);
   poiEnd.AddStage(pgsTypes::CastingYard,POI_SECTCHANGE_LEFTFACE | POI_TABULAR | POI_GRAPHICAL);
   poiEnd.AddStage(pgsTypes::Lifting,    POI_SECTCHANGE_LEFTFACE | POI_TABULAR | POI_GRAPHICAL);
   poiEnd.AddStage(pgsTypes::Hauling,    POI_SECTCHANGE_LEFTFACE | POI_TABULAR | POI_GRAPHICAL);

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);

   // move bridge site poi to the start/end bearing
   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::GirderPlacement);
   stages.push_back(pgsTypes::TemporaryStrandRemoval);
   stages.push_back(pgsTypes::BridgeSite1);
   stages.push_back(pgsTypes::BridgeSite2);
   stages.push_back(pgsTypes::BridgeSite3);
   
   Float64 start_length = pBridge->GetGirderStartConnectionLength(span,gdr);
   Float64 end_length   = pBridge->GetGirderEndConnectionLength(span,gdr);
   poiStart.SetDistFromStart(start_length);
   poiEnd.SetDistFromStart(gdrLength-end_length);

   poiStart.RemoveStage(pgsTypes::CastingYard);
   poiStart.RemoveStage(pgsTypes::Lifting);
   poiStart.RemoveStage(pgsTypes::Hauling);
   poiStart.AddStages(stages,POI_SECTCHANGE_RIGHTFACE | POI_TABULAR | POI_GRAPHICAL);

   poiEnd.RemoveStage(pgsTypes::CastingYard);
   poiEnd.RemoveStage(pgsTypes::Lifting);
   poiEnd.RemoveStage(pgsTypes::Hauling);
   poiEnd.AddStages(stages,POI_SECTCHANGE_LEFTFACE | POI_TABULAR | POI_GRAPHICAL);

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);

   // put section breaks just on either side of the end blocks/void interface
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const GirderLibraryEntry* pGirderLib = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);
   Float64 endBlockLength = pGirderLib->GetDimension(_T("EndBlockLength"));

   if ( !IsZero(endBlockLength) )
   {
      Float64 delta = 1.5*pPoiMgr->GetTolerance();

      stages.push_back(pgsTypes::CastingYard);
      stages.push_back(pgsTypes::Lifting);
      stages.push_back(pgsTypes::Hauling);

      std::vector<pgsTypes::Stage> ebStages;
      if ( start_length < endBlockLength )
      {
         ebStages.push_back(pgsTypes::CastingYard); // end block is after brg... only add to cy stage
         ebStages.push_back(pgsTypes::Lifting);
         ebStages.push_back(pgsTypes::Hauling);
      }
      else
      {
         ebStages = stages; // all stages
      }

      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,endBlockLength,       POI_SECTCHANGE_LEFTFACE  | POI_TABULAR | POI_GRAPHICAL) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,endBlockLength+delta, POI_SECTCHANGE_RIGHTFACE | POI_TABULAR | POI_GRAPHICAL) );

      if ( end_length < endBlockLength )
      {
         ebStages.push_back(pgsTypes::CastingYard); // end block is after brg... only add to cy stage
         ebStages.push_back(pgsTypes::Lifting);
         ebStages.push_back(pgsTypes::Hauling);
      }
      else
      {
         ebStages = stages; // all stages
      }

      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,gdrLength - (endBlockLength+delta), POI_SECTCHANGE_LEFTFACE  | POI_TABULAR | POI_GRAPHICAL) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,gdrLength - endBlockLength,         POI_SECTCHANGE_RIGHTFACE | POI_TABULAR | POI_GRAPHICAL) );
   }
}

void CBoxBeamFactoryImpl::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,
                                               const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,
                                               IDistFactorEngineer** ppEng)
{
   GET_IFACE2(pBroker,IBridge,pBridge);

   // use passed value if not null
   pgsTypes::SupportedDeckType deckType = (pDeckType!=NULL) ? *pDeckType : pBridge->GetDeckType();
   
   if ( deckType == pgsTypes::sdtCompositeOverlay || deckType == pgsTypes::sdtNone )
   {
      CComObject<CBoxBeamDistFactorEngineer>* pEngineer;
      CComObject<CBoxBeamDistFactorEngineer>::CreateInstance(&pEngineer);
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
      pEngineer->Init(true, false); // this is a type b cross section
      pEngineer->SetBroker(pBroker,statusGroupID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
}

void CBoxBeamFactoryImpl::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,IPsLossEngineer** ppEng)
{
    CComObject<CPsBeamLossEngineer>* pEngineer;
    CComObject<CPsBeamLossEngineer>::CreateInstance(&pEngineer);
    pEngineer->Init(BoxBeam);
    pEngineer->SetBroker(pBroker,statusGroupID);
    (*ppEng) = pEngineer;
    (*ppEng)->AddRef();
}


std::vector<std::_tstring> CBoxBeamFactoryImpl::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<double> CBoxBeamFactoryImpl::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CBoxBeamFactoryImpl::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CBoxBeamFactoryImpl::IsPrismatic(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));

   return IsZero(endBlockLength) ? true : false;
}

Float64 CBoxBeamFactoryImpl::GetVolume(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::CastingYard,POI_SECTCHANGE,POIFIND_OR);
   ATLASSERT( 2 <= vPOI.size() );
   Float64 V = 0;
   std::vector<pgsPointOfInterest>::iterator iter = vPOI.begin();
   pgsPointOfInterest prev_poi = *iter;
   Float64 prev_area = pSectProp2->GetAg(pgsTypes::CastingYard,prev_poi);
   iter++;

   for ( ; iter != vPOI.end(); iter++ )
   {
      pgsPointOfInterest poi = *iter;
      Float64 area = pSectProp2->GetAg(pgsTypes::CastingYard,poi);

      Float64 avg_area = (prev_area + area)/2;
      V += avg_area*(poi.GetDistFromStart() - prev_poi.GetDistFromStart());

      prev_poi = poi;
      prev_area = area;
   }

   return V;
}


CLSID CBoxBeamFactoryImpl::GetFamilyCLSID()
{
   return CLSID_BoxBeamFamily;
}

std::_tstring CBoxBeamFactoryImpl::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CBoxBeamFactoryImpl::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

HINSTANCE CBoxBeamFactoryImpl::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

double CBoxBeamFactoryImpl::GetDimension(const IBeamFactory::Dimensions& dimensions,
                                        const std::_tstring& name)
{
   IBeamFactory::Dimensions::const_iterator iter;
   for ( iter = dimensions.begin(); iter != dimensions.end(); iter++ )
   {
      const IBeamFactory::Dimension& dim = *iter;
      if ( name == dim.first )
         return dim.second;
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedDeckTypes CBoxBeamFactoryImpl::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CBoxBeamFactoryImpl::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);

   return sbs;
}


WebIndexType CBoxBeamFactoryImpl::GetNumberOfWebs(const IBeamFactory::Dimensions& dimensions)
{
   return 2;
}


Float64 CBoxBeamFactoryImpl::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   double H1 = GetDimension(dimensions,_T("H1"));
   double H2 = GetDimension(dimensions,_T("H2"));
   double H3 = GetDimension(dimensions,_T("H3"));

   return H1 + H2 + H3;
}



std::_tstring CBoxBeamFactoryImpl::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CBoxBeamFactoryImpl::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CBoxBeamFactoryImpl::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CBoxBeamFactoryImpl::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
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

std::_tstring CBoxBeamFactoryImpl::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
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

std::_tstring CBoxBeamFactoryImpl::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
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
