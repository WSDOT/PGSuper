///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

// BulbTeeFactory.cpp : Implementation of CBulbTeeFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include "BeamFamilyCLSID.h"
#include "BulbTeeFactory.h"
#include "BulbTeeDistFactorEngineer.h"
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
// CBulbTeeFactory
HRESULT CBulbTeeFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back("D1");
   m_DimNames.push_back("D2");
   m_DimNames.push_back("D3");
   m_DimNames.push_back("D4");
   m_DimNames.push_back("D5");
   m_DimNames.push_back("D6");
   m_DimNames.push_back("D7");
   m_DimNames.push_back("D8");
   m_DimNames.push_back("T1");
   m_DimNames.push_back("T2");
   m_DimNames.push_back("W1");
   m_DimNames.push_back("W2");
   m_DimNames.push_back("W3");
   m_DimNames.push_back("W4");
   m_DimNames.push_back("Wmax");
   m_DimNames.push_back("Wmin");

   std::sort(m_DimNames.begin(),m_DimNames.end());

   // Default beam is a W74G                                              
   m_DefaultDims.push_back(::ConvertToSysUnits(2.875,unitMeasure::Inch)); // D1
   m_DefaultDims.push_back(::ConvertToSysUnits(2.625,unitMeasure::Inch)); // D2
   m_DefaultDims.push_back(::ConvertToSysUnits(2.000,unitMeasure::Inch)); // D3
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // D4
   m_DefaultDims.push_back(::ConvertToSysUnits(3.000,unitMeasure::Inch)); // D5
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // D6
   m_DefaultDims.push_back(::ConvertToSysUnits(57.00,unitMeasure::Inch)); // D7
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.00,unitMeasure::Inch)); // D8
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // T1
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Inch)); // T2
   m_DefaultDims.push_back(::ConvertToSysUnits(16.50,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits(2.000,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits(9.500,unitMeasure::Inch)); // W3
   m_DefaultDims.push_back(::ConvertToSysUnits(0.000,unitMeasure::Inch)); // W4
   m_DefaultDims.push_back(::ConvertToSysUnits(6.000,unitMeasure::Feet)); // Wmax
   m_DefaultDims.push_back(::ConvertToSysUnits(4.000,unitMeasure::Feet)); // Wmin

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D5
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D6
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D7
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D8
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W4
   m_DimUnits[0].push_back(&unitMeasure::Meter);      // Wmax
   m_DimUnits[0].push_back(&unitMeasure::Meter);      // Wmin

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D5
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D6
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D7
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D8
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W4
   m_DimUnits[1].push_back(&unitMeasure::Feet); // Wmax
   m_DimUnits[1].push_back(&unitMeasure::Feet); // Wmin

   return S_OK;
}

void CBulbTeeFactory::CreateGirderSection(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IGirderSection** ppSection)
{
   CComPtr<IBulbTeeSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_BulbTeeSection);
   CComPtr<IBulbTee> beam;
   gdrsection->get_Beam(&beam);

   double d1,d2,d3,d4,d5,d6,d7,d8;
   double w1,w2,w3,w4,wmin,wmax;
   double t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7,d8,w1,w2,w3,w4,wmin,wmax,t1,t2);
   beam->put_W1(w1);
   beam->put_W2(w2);
   beam->put_W3(w3);
   beam->put_W4(w4);

   if ( pBroker == NULL || spanIdx == INVALID_INDEX || gdrIdx == INVALID_INDEX )
   {
      beam->put_W5(wmax);
   }
   else
   {
#pragma Reminder("UPDATE: Assuming uniform spacing")
      // uniform spacing is required for this type of girder so maybe this is ok

      // use raw input here because requesting it from the bridge will cause an infite loop.
      // bridge agent calls this during validation
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
      const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      ATLASSERT(pBridgeDesc->GetGirderSpacingType() == pgsTypes::sbsConstantAdjacent);
      double spacing = pBridgeDesc->GetGirderSpacing();;

      // if this is a fixed width section, then set the spacing equal to the width
      if ( IsEqual(wmin,wmax) )
         spacing = wmax;
         
      beam->put_W5(spacing);
   }

   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_D3(d3);
   beam->put_D4(d4);
   beam->put_D5(d5);
   beam->put_D6(d6);
   beam->put_D7(d7);
   beam->put_T1(t1);
   beam->put_T2(t2);

   gdrsection.QueryInterface(ppSection);
}

void CBulbTeeFactory::CreateGirderProfile(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetGirderLength(spanIdx,gdrIdx);

   double d1,d2,d3,d4,d5,d6,d7,d8;
   double w1,w2,w3,w4,wmin,wmax;
   double t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7,d8,w1,w2,w3,w4,wmin,wmax,t1,t2);

   Float64 height = d1 + d2 + d3 + d4 + d5 + d6 + d7;
   
   CComPtr<IPolyShape> polyShape;
   polyShape.CoCreateInstance(CLSID_PolyShape);
   
   polyShape->AddPoint(0,0);
   polyShape->AddPoint(length,0);
   polyShape->AddPoint(length,height+d8);
   polyShape->AddPoint(7*length/8,height+0.5625*d8);
   polyShape->AddPoint(6*length/8,height+0.2500*d8);
   polyShape->AddPoint(5*length/8,height+0.0625*d8);
   polyShape->AddPoint(4*length/8,height+0.0000*d8);
   polyShape->AddPoint(3*length/8,height+0.0625*d8);
   polyShape->AddPoint(2*length/8,height+0.2500*d8);
   polyShape->AddPoint(1*length/8,height+0.5625*d8);
   polyShape->AddPoint(0,height+d8);

   CComQIPtr<IXYPosition> position(polyShape);
   CComPtr<IPoint2d> topLeft;
   position->get_LocatorPoint(lpTopLeft,&topLeft);
   topLeft->Move(0,0);
   position->put_LocatorPoint(lpTopLeft,topLeft);

   polyShape->QueryInterface(ppShape);
}

void CBulbTeeFactory::LayoutGirderLine(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* ssmbr)
{
   CComPtr<ISegment> segment;

   bool bPrismatic = IsPrismatic(pBroker,spanIdx,gdrIdx);
   if ( bPrismatic )
   {
      // prismatic
      segment.CoCreateInstance(CLSID_PrismaticSegment);
   }
   else
   {
      // non-prismatic
      segment.CoCreateInstance(CLSID_ThickenedFlangeBulbTeeSegment);
   }

   ATLASSERT(segment != NULL);

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


   if ( bPrismatic )
   {
      CComPtr<IGirderSection> gdrSection;
      CreateGirderSection(pBroker,agentID,spanIdx,gdrIdx,dimensions,&gdrSection);
      CComQIPtr<IPrismaticSegment> prisSegment(segment);
      CComQIPtr<IShape> shape(gdrSection);
      prisSegment->putref_Shape(shape);
   }
   else
   {
      CComPtr<IGirderSection> gdrSection;
      CreateGirderSection(pBroker,agentID,spanIdx,gdrIdx,dimensions,&gdrSection);

      CComQIPtr<IThickenedFlangeBulbTeeSegment> bulbTeeSegment(segment);
      CComQIPtr<IBulbTeeSection> bulbTeeSection(gdrSection);

      Float64 flangeThickening = GetDimension(dimensions,"D8");

      bulbTeeSegment->putref_BulbTeeSection(bulbTeeSection,flangeThickening);
   }

   // Beam materials
   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   double Ecgdr = pMaterial->GetEcGdr(spanIdx,gdrIdx);
   double density = pMaterial->GetStrDensityGdr(spanIdx,gdrIdx);

   CComPtr<IMaterial> material;
   material.CoCreateInstance(CLSID_Material);
   material->put_E(Ecgdr);
   material->put_Density(density);
   segment->putref_Material(material);

   ssmbr->AddSegment(segment);
}

void CBulbTeeFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsPoiMgr* pPoiMgr)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetGirderLength(span,gdr);

   pgsPointOfInterest poiStart(pgsTypes::CastingYard,span,gdr,0.00,POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL);
   pgsPointOfInterest poiEnd(pgsTypes::CastingYard,span,gdr,gdrLength,POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL);
   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);

   // move bridge site poi to the start/end bearing
   std::set<pgsTypes::Stage> stages;
   stages.insert(pgsTypes::GirderPlacement);
   stages.insert(pgsTypes::TemporaryStrandRemoval);
   stages.insert(pgsTypes::BridgeSite1);
   stages.insert(pgsTypes::BridgeSite2);
   stages.insert(pgsTypes::BridgeSite3);
   

   Float64 start_length = pBridge->GetGirderStartConnectionLength(span,gdr);
   Float64 end_length   = pBridge->GetGirderEndConnectionLength(span,gdr);

   poiStart.SetDistFromStart(start_length);
   poiEnd.SetDistFromStart(gdrLength-end_length);

   poiStart.RemoveStage(pgsTypes::CastingYard);
   poiStart.AddStages(stages);

   poiEnd.RemoveStage(pgsTypes::CastingYard);
   poiEnd.AddStages(stages);

   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);

   if ( !IsPrismatic(pBroker,span,gdr) )
   {
      stages.insert(pgsTypes::CastingYard);
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,1*gdrLength/8,POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,2*gdrLength/8,POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,3*gdrLength/8,POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,4*gdrLength/8,POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,5*gdrLength/8,POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,6*gdrLength/8,POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,7*gdrLength/8,POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL) );
   }
}

void CBulbTeeFactory::CreateDistFactorEngineer(IBroker* pBroker,long agentID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   GET_IFACE2(pBroker, ISpecification,    pSpec);
   GET_IFACE2(pBroker, ILibrary,          pLib);
   GET_IFACE2(pBroker, IBridge,           pBridge);
   GET_IFACE2(pBroker, IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CDeckDescription* pDeck = pBridgeDesc->GetDeckDescription();

   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   Int16 method = pSpecEntry->GetLiveLoadDistributionMethod();

   // use passed values if not null
   pgsTypes::SupportedDeckType deckType = (pDeckType!=NULL) ? *pDeckType : pDeck->DeckType;

   ATLASSERT(deckType == pgsTypes::sdtNone || deckType == pgsTypes::sdtCompositeOverlay); // no spread bulb t's

   pgsTypes::AdjacentTransverseConnectivity connect = (pConnect!=NULL) ? *pConnect : pDeck->TransverseConnectivity;

   // for composite attached beams, we want to use WSDOT type k
   bool useIBeam = ( method==LLDF_WSDOT && connect == pgsTypes::atcConnectedAsUnit && deckType == pgsTypes::sdtCompositeOverlay );

   CComObject<CBulbTeeDistFactorEngineer>* pEngineer;
   CComObject<CBulbTeeDistFactorEngineer>::CreateInstance(&pEngineer);

   pEngineer->Init(useIBeam);
   pEngineer->SetBroker(pBroker,agentID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CBulbTeeFactory::CreatePsLossEngineer(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,IPsLossEngineer** ppEng)
{
    CComObject<CPsBeamLossEngineer>* pEngineer;
    CComObject<CPsBeamLossEngineer>::CreateInstance(&pEngineer);
    pEngineer->Init(CPsLossEngineer::IBeam);
    pEngineer->SetBroker(pBroker,agentID);
    (*ppEng) = pEngineer;
    (*ppEng)->AddRef();
}

void CBulbTeeFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, double endTopLimit, IBeamFactory::BeamFace endBottomFace, double endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, double hpTopLimit, IBeamFactory::BeamFace hpBottomFace, double hpBottomLimit, 
                                  double endIncrement, double hpIncrement, IStrandMover** strandMover)
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;


   // set the shape for harped strand bounds - only in the thinest part of the web
   CComPtr<IRectangle> harp_rect;
   hr = harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   double d1,d2,d3,d4,d5,d6,d7,d8;
   double w1,w2,w3,w4,wmin,wmax;
   double t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7,d8,w1,w2,w3,w4,wmin,wmax,t1,t2);

   double width = min(t1,t2);
   double depth = d1 + d2 + d3 + d4 + d5 + d6 + d7;

   harp_rect->put_Width(width);
   harp_rect->put_Height(depth);

   CComPtr<IPoint2d> hook;
   hook.CoCreateInstance(CLSID_Point2d);
   hook->Move(0, depth/2.0);

   harp_rect->putref_HookPoint(hook);

   CComPtr<IShape> shape;
   harp_rect->get_Shape(&shape);

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   hr = configurer->AddRegion(shape, 0.0);
   ATLASSERT (SUCCEEDED(hr));

   // set vertical offset bounds and increments
   double hptb = hpTopFace==IBeamFactory::BeamBottom ? hpTopLimit : depth-hpTopLimit;
   double hpbb = hpBottomFace==IBeamFactory::BeamBottom ? hpBottomLimit : depth-hpBottomLimit;
   double endtb = endTopFace==IBeamFactory::BeamBottom ? endTopLimit : depth-endTopLimit;
   double endbb = endBottomFace==IBeamFactory::BeamBottom ? endBottomLimit : depth-endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(depth, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}

std::vector<std::string> CBulbTeeFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<double> CBulbTeeFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CBulbTeeFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CBulbTeeFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::string* strErrMsg)
{
   double d1,d2,d3,d4,d5,d6,d7,d8;
   double w1,w2,w3,w4,wmin,wmax;
   double t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7,d8,w1,w2,w3,w4,wmin,wmax,t1,t2);

// 0  D1  
// 1  D2
// 2  D3
// 3  D4
// 4  D5
// 5  D6
// 6  D7
// 7  D8
// 8  T1
// 9  T2
// 10 W1
// 11 W2
// 12 W3
// 13 W4
// 14 Wmax
// 15 Wmin

   if ( d1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
      std::ostringstream os;
      os << "D1 must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d2 < 0.0 )
   {
      std::ostringstream os;
      os << "D2 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d3 < 0.0 )
   {
      std::ostringstream os;
      os << "D3 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d4 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][3];
      std::ostringstream os;
      os << "D4 must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d5 < 0.0 )
   {
      std::ostringstream os;
      os << "D5 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d6 < 0.0 )
   {
      std::ostringstream os;
      os << "D6 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d7 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][6];
      std::ostringstream os;
      os << "D7 must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d8 < 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][7];
      std::ostringstream os;
      os << "D8 must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][10];
      std::ostringstream os;
      os << "W1 must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( w2 < 0.0 )
   {
      std::ostringstream os;
      os << "W2 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w3 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][12];
      std::ostringstream os;
      os << "W3 must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( w4 < 0.0 )
   {
      std::ostringstream os;
      os << "W4 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }
   
   if ( t1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
      std::ostringstream os;
      os << "T1 must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( t2 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
      std::ostringstream os;
      os << "T2 must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if (wmin>wmax)
   {
      std::ostringstream os;
      os << "Wmin must be greater than or equal to Wmax" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   double inp_toler = ::ConvertToSysUnits(2.0, unitMeasure::Millimeter);

   double min_topflange = t2+2.0*(w3+w4);
   if ( wmin + inp_toler < min_topflange )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][15];
      double mf_u = ::ConvertFromSysUnits(min_topflange,*pUnit);

      std::ostringstream os;
      os << "Wmin must be greater than or equal to bottom flange width = "<<mf_u<< pUnit->UnitTag() <<" = T2 + 2.0*(W3+W4)" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   min_topflange = t1+2.0*(w1+w2);
   if ( wmin + inp_toler < min_topflange  )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][15];
      double mf_u = ::ConvertFromSysUnits(min_topflange,*pUnit);

      std::ostringstream os;
      os << "Wmin must be greater than or equal to T1 + 2.0*(W1 + W2) = "<<mf_u<< pUnit->UnitTag() <<" = " << std::ends;
      *strErrMsg = os.str();
      return false;
   }


   return true;
}

void CBulbTeeFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::string>::iterator iter;
   pSave->BeginUnit("BulbTeeDimensions",2.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::string name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CBulbTeeFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   IBeamFactory::Dimensions dimensions;

   Float64 parent_version = pLoad->GetVersion();

   if ( 14 <= parent_version && !pLoad->BeginUnit("BulbTeeDimensions") )
      THROW_LOAD(InvalidFileFormat,pLoad);

   Float64 dimVersion = pLoad->GetVersion();

   std::vector<std::string>::iterator iter;
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::string name = *iter;
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
      {
         if ( (parent_version < 14 || (14 <= parent_version && dimVersion < 2)) && name == "D8" )
            value = 0;
         else
            THROW_LOAD(InvalidFileFormat,pLoad);
      }
      dimensions.push_back( std::make_pair(name,value) );
   }

   if ( 14 <= parent_version && !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   return dimensions;
}

bool CBulbTeeFactory::IsPrismatic(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 d8 = GetDimension(dimensions,"D8");

   return IsZero(d8) ? true : false;
}

Float64 CBulbTeeFactory::GetVolume(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(pgsTypes::CastingYard,spanIdx,gdrIdx,POI_SECTCHANGE);
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

Float64 CBulbTeeFactory::GetSurfaceArea(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bReduceForPoorlyVentilatedVoids)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(pgsTypes::CastingYard,spanIdx,gdrIdx,POI_SECTCHANGE);
   ATLASSERT( 2 <= vPOI.size() );
   Float64 S = 0;
   std::vector<pgsPointOfInterest>::iterator iter = vPOI.begin();
   pgsPointOfInterest prev_poi = *iter;
   Float64 prev_perimeter = pSectProp2->GetPerimeter(prev_poi);
   iter++;

   for ( ; iter != vPOI.end(); iter++ )
   {
      pgsPointOfInterest poi = *iter;
      Float64 perimeter = pSectProp2->GetPerimeter(poi);

      Float64 avg_perimeter = (prev_perimeter + perimeter)/2;
      S += avg_perimeter*(poi.GetDistFromStart() - prev_poi.GetDistFromStart());

      prev_poi = poi;
      prev_perimeter = perimeter;
   }

   return S;
}

std::string CBulbTeeFactory::GetImage()
{
   return std::string("BulbTee.jpg");
}

std::string CBulbTeeFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  "BulbTee_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "BulbTee_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CBulbTeeFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  "+Mn_BulbTee_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "+Mn_BulbTee_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CBulbTeeFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  "-Mn_BulbTee_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "-Mn_BulbTee_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CBulbTeeFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  "Vn_BulbTee_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "Vn_BulbTee_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CBulbTeeFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   return "BulbTee_Effective_Flange_Width_Interior_Girder.gif";
}

std::string CBulbTeeFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return "BulbTee_Effective_Flange_Width_Exterior_Girder_2008.gif";
   }
   else
   {
      return "BulbTee_Effective_Flange_Width_Exterior_Girder.gif";
   }
}

CLSID CBulbTeeFactory::GetCLSID()
{
   return CLSID_BulbTeeFactory;
}

CLSID CBulbTeeFactory::GetFamilyCLSID()
{
   return CLSID_DeckBulbTeeBeamFamily;
}

std::string CBulbTeeFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::string( OLE2A(pszUserType) );
}

std::string CBulbTeeFactory::GetPublisher()
{
   return std::string("WSDOT");
}

HINSTANCE CBulbTeeFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CBulbTeeFactory::GetImageResourceName()
{
   return _T("BULBTEE");
}

HICON  CBulbTeeFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_BULBTEE) );
}

void CBulbTeeFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                  double& d1,double& d2,double& d3,double& d4,double& d5,double& d6,double& d7,double& d8,
                                  double& w1,double& w2,double& w3,double& w4,double& wmin,double& wmax,
                                  double& t1,double& t2)
{
   d1 = GetDimension(dimensions,"D1");
   d2 = GetDimension(dimensions,"D2");
   d3 = GetDimension(dimensions,"D3");
   d4 = GetDimension(dimensions,"D4");
   d5 = GetDimension(dimensions,"D5");
   d6 = GetDimension(dimensions,"D6");
   d7 = GetDimension(dimensions,"D7");
   d8 = GetDimension(dimensions,"D8");
   w1 = GetDimension(dimensions,"W1");
   w2 = GetDimension(dimensions,"W2");
   w3 = GetDimension(dimensions,"W3");
   w4 = GetDimension(dimensions,"W4");
   wmin = GetDimension(dimensions,"Wmin");
   wmax = GetDimension(dimensions,"Wmax");
   t1 = GetDimension(dimensions,"T1");
   t2 = GetDimension(dimensions,"T2");
}

double CBulbTeeFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::string& name)
{
   Dimensions::const_iterator iter;
   for ( iter = dimensions.begin(); iter != dimensions.end(); iter++ )
   {
      const Dimension& dim = *iter;
      if ( name == dim.first )
         return dim.second;
   }

   ATLASSERT(false); // should never get here
   return -99999;
}

pgsTypes::SupportedDeckTypes CBulbTeeFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CBulbTeeFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsConstantAdjacent);
   return sbs;
}

void CBulbTeeFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, double* minSpacing, double* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   double Wmax = GetDimension(dimensions,"Wmax");
   double Wmin = GetDimension(dimensions,"Wmin");

   if (sdt == pgsTypes::sdtCompositeOverlay || sdt == pgsTypes::sdtNone)
   {
      if (sbs == pgsTypes::sbsConstantAdjacent)
      {
         *minSpacing = Wmin;
         *maxSpacing = Wmax;
      }
      else
      {
         ATLASSERT(0);
      }
   }
   else
   {
      ATLASSERT(0);
   };
}

long CBulbTeeFactory::GetNumberOfWebs(const IBeamFactory::Dimensions& dimensions)
{
   return 1;
}

Float64 CBulbTeeFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   double D1 = GetDimension(dimensions,"D1");
   double D2 = GetDimension(dimensions,"D2");
   double D3 = GetDimension(dimensions,"D3");
   double D4 = GetDimension(dimensions,"D4");
   double D5 = GetDimension(dimensions,"D5");
   double D6 = GetDimension(dimensions,"D6");
   double D7 = GetDimension(dimensions,"D7");

   return D1 + D2 + D3 + D4 + D5 + D6 + D7;
}

Float64 CBulbTeeFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,"Wmax");
}
