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

// UBeam2Factory.cpp : Implementation of CUBeam2Factory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include <Plugins\BeamFamilyCLSID.h>
#include "UBeam2Factory.h"
#include "UBeamDistFactorEngineer.h"
#include "PsBeamLossEngineer.h"
#include "StrandMoverImpl.h"
#include <BridgeModeling\PrismaticGirderProfile.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <PgsExt\BridgeDescription.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CUBeam2Factory
HRESULT CUBeam2Factory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("C1"));
   m_DimNames.push_back(_T("D1"));
   m_DimNames.push_back(_T("D2"));
   m_DimNames.push_back(_T("D3"));
   m_DimNames.push_back(_T("D4"));
   m_DimNames.push_back(_T("D5"));
   m_DimNames.push_back(_T("D6"));
   m_DimNames.push_back(_T("W1"));
   m_DimNames.push_back(_T("W2"));
   m_DimNames.push_back(_T("W3"));
   m_DimNames.push_back(_T("W4"));
   m_DimNames.push_back(_T("W5"));
   m_DimNames.push_back(_T("W6"));
   m_DimNames.push_back(_T("W7"));
   m_DimNames.push_back(_T("EndBlockLength"));

   std::sort(m_DimNames.begin(),m_DimNames.end());

   // Default beam is a TXDOT U40
   m_DefaultDims.push_back(::ConvertToSysUnits(0.750,unitMeasure::Inch)); // C1
   m_DefaultDims.push_back(::ConvertToSysUnits(40.00,unitMeasure::Inch)); // D1
   m_DefaultDims.push_back(::ConvertToSysUnits( 8.25,unitMeasure::Inch)); // D2
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.00,unitMeasure::Inch)); // D3
   m_DefaultDims.push_back(::ConvertToSysUnits(0.875,unitMeasure::Inch)); // D4
   m_DefaultDims.push_back(::ConvertToSysUnits(5.875,unitMeasure::Inch)); // D5
   m_DefaultDims.push_back(::ConvertToSysUnits(21.625,unitMeasure::Inch)); // D6
   m_DefaultDims.push_back(::ConvertToSysUnits(55.00,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits(89.00,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.00,unitMeasure::Inch)); // W3
   m_DefaultDims.push_back(::ConvertToSysUnits(0.375,unitMeasure::Inch)); // W4
   m_DefaultDims.push_back(::ConvertToSysUnits( 8.25,unitMeasure::Inch)); // W5
   m_DefaultDims.push_back(::ConvertToSysUnits(15.75,unitMeasure::Inch)); // W6
   m_DefaultDims.push_back(::ConvertToSysUnits( 1.75,unitMeasure::Inch)); // W7
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.0,unitMeasure::Inch)); // End Block Length

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D5
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D6
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W5
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W6
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W7
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // End Block Length

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D5
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D6
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W5
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W6
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W7
   m_DimUnits[1].push_back(&unitMeasure::Inch); // End Block Length

   return S_OK;
}

void CUBeam2Factory::CreateGirderSection(IBroker* pBroker,StatusGroupIDType statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IGirderSection** ppSection)
{
   CComPtr<IUGirderSection2> gdrsection;
   gdrsection.CoCreateInstance(CLSID_UGirderSection2);
   CComPtr<IUBeam2> beam;
   gdrsection->get_Beam(&beam);

   ConfigureShape(dimensions, beam);

   gdrsection.QueryInterface(ppSection);
}

void CUBeam2Factory::CreateGirderProfile(IBroker* pBroker,StatusGroupIDType statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetGirderLength(spanIdx,gdrIdx);

   double w1, w2, w3, w4, w5, w6, w7;
   double d1, d2, d3, d4, d5, d6;
   double c1, EndBlockLength;
   GetDimensions(dimensions,d1, d2, d3, d4, d5, d6, w1, w2, w3, w4, w5, w6, w7, c1, EndBlockLength);

   Float64 height = d1;

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

void CUBeam2Factory::ConfigureShape(const IBeamFactory::Dimensions& dimensions, IUBeam2* beam)
{
   double w1, w2, w3, w4, w5, w6, w7;
   double d1, d2, d3, d4, d5, d6;
   double c1, EndBlockLength;
   GetDimensions(dimensions,d1, d2, d3, d4, d5, d6, w1, w2, w3, w4, w5, w6, w7, c1, EndBlockLength);
   beam->put_W1(w1);
   beam->put_W2(w2);
   beam->put_W3(w3);
   beam->put_W4(w4);
   beam->put_W5(w5);
   beam->put_W6(w6);
   beam->put_W7(w7);
   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_D3(d3);
   beam->put_D4(d4);
   beam->put_D5(d5);
   beam->put_D6(d6);
   beam->put_C1(c1);
}



void CUBeam2Factory::LayoutGirderLine(IBroker* pBroker,StatusGroupIDType statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* ssmbr)
{
   CComPtr<IUGirderSection2EndBlockSegment> segment;
   segment.CoCreateInstance(CLSID_UGirderSection2EndBlockSegment);

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

   CComQIPtr<IUGirderSection2> section(gdrsection);
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

void CUBeam2Factory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsPoiMgr* pPoiMgr)
{
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
         ebStages.push_back(pgsTypes::Lifting); // end block is after brg... only add to cy stage
         ebStages.push_back(pgsTypes::Hauling); // end block is after brg... only add to cy stage
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
         ebStages.push_back(pgsTypes::Lifting); // end block is after brg... only add to cy stage
         ebStages.push_back(pgsTypes::Hauling); // end block is after brg... only add to cy stage
      }
      else
      {
         ebStages = stages; // all stages
      }

      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,gdrLength - (endBlockLength+delta), POI_SECTCHANGE_LEFTFACE  | POI_TABULAR | POI_GRAPHICAL) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,gdrLength - endBlockLength,         POI_SECTCHANGE_RIGHTFACE | POI_TABULAR | POI_GRAPHICAL) );
   }
}

void CUBeam2Factory::CreateDistFactorEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   CComObject<CUBeamDistFactorEngineer>* pEngineer;
   CComObject<CUBeamDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CUBeam2Factory::CreatePsLossEngineer(IBroker* pBroker,StatusGroupIDType statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,IPsLossEngineer** ppEng)
{
   CComObject<CPsBeamLossEngineer>* pEngineer;
   CComObject<CPsBeamLossEngineer>::CreateInstance(&pEngineer);
   pEngineer->Init(UBeam);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CUBeam2Factory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, double endTopLimit, IBeamFactory::BeamFace endBottomFace, double endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, double hpTopLimit, IBeamFactory::BeamFace hpBottomFace, double hpBottomLimit, 
                                  double endIncrement, double hpIncrement, IStrandMover** strandMover)
{
   // build our shape so we can get higher-level info
   CComPtr<IUBeam2> beam;
   beam.CoCreateInstance(CLSID_UBeam2);

   ConfigureShape(dimensions, beam);

   // our goal is to build a parallelogram using the thin web dimension from top to bottom
   double t;
   beam->get_T(&t);
   double slope;
   beam->get_Slope(1, &slope);
   double height;
   beam->get_Height(&height);
   double w1;
   beam->get_W1(&w1);

   double arc_slope = 1.0/slope;

   double t_x_project = t*sqrt(slope*slope+1)/slope;

   CComPtr<IPolyShape> rgt_harp_poly;
   rgt_harp_poly.CoCreateInstance(CLSID_PolyShape);

   // travel counter clockwise around right web;
   double x1 = w1/2.0;
   double y1 = 0.0;

   double x2 = x1 + height * arc_slope;
   double y2 = height;

   double x3 = x2 - t_x_project;
   double y3 = y2;

   double x4 = x1 - t_x_project;
   double y4 = 0.0;

   rgt_harp_poly->AddPoint(x1,y1);
   rgt_harp_poly->AddPoint(x2,y2);
   rgt_harp_poly->AddPoint(x3,y3);
   rgt_harp_poly->AddPoint(x4,y4);
   rgt_harp_poly->AddPoint(x1,y1);

   // left side is same with negative x's
   CComPtr<IPolyShape> lft_harp_poly;
   lft_harp_poly.CoCreateInstance(CLSID_PolyShape);

   lft_harp_poly->AddPoint(-x1,y1);
   lft_harp_poly->AddPoint(-x2,y2);
   lft_harp_poly->AddPoint(-x3,y3);
   lft_harp_poly->AddPoint(-x4,y4);
   lft_harp_poly->AddPoint(-x1,y1);

   CComPtr<IShape> lft_shape, rgt_shape;
   lft_harp_poly->get_Shape(&lft_shape);
   rgt_harp_poly->get_Shape(&rgt_shape);

   // now make our strand mover and fill it up
   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);
   CComPtr<IStrandMover> sm = pStrandMover;

   CComQIPtr<IConfigureStrandMover> configurer(sm);
   HRESULT hr = configurer->AddRegion(lft_shape, -arc_slope);
   ATLASSERT (SUCCEEDED(hr));
   hr = configurer->AddRegion(rgt_shape, arc_slope);
   ATLASSERT (SUCCEEDED(hr));

   // set vertical offset bounds and increments
   double hptb = hpTopFace==IBeamFactory::BeamBottom ? hpTopLimit : height-hpTopLimit;
   double hpbb = hpBottomFace==IBeamFactory::BeamBottom ? hpBottomLimit : height-hpBottomLimit;
   double endtb = endTopFace==IBeamFactory::BeamBottom ? endTopLimit : height-endTopLimit;
   double endbb = endBottomFace==IBeamFactory::BeamBottom ? endBottomLimit : height-endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(height, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));
}


std::vector<std::_tstring> CUBeam2Factory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<double> CUBeam2Factory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CUBeam2Factory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CUBeam2Factory::ValidateDimensions(const Dimensions& dimensions,bool bSIUnits,std::_tstring* strErrMsg)
{
   double w1, w2, w3, w4, w5, w6, w7;
   double d1, d2, d3, d4, d5, d6;
   double c1, EndBlockLength;
   GetDimensions(dimensions,d1, d2, d3, d4, d5, d6, w1, w2, w3, w4, w5, w6, w7, c1, EndBlockLength);

// D1  0
// D2  1
// D3  2
// D4  3
// D5  4
// D6  5
// W1  6
// W2  7
// W3  8
// W4  9
// W5  10
// W6  11
// W7  12

   if ( d1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][1];
      std::_tostringstream os;
      os << _T("D1 must be greater than 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d2 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][2];
      std::_tostringstream os;
      os << _T("D2 must be greater than 0.0") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d3 < 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][3];
      std::_tostringstream os;
      os << _T("D3 must be greater than or equal to 0.0") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d1 < d2+d3 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][1];
      std::_tostringstream os;
      os << _T("D1 must be greater than or equal to D2 + D3 (") << ::ConvertFromSysUnits(d2+d3,*pUnit) << _T(" ") << pUnit->UnitTag() << _T(")") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][7];
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

   if ( w3 < 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
      std::_tostringstream os;
      os << _T("W3 must be greater than or equal to 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( c1 < 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][0];
      std::_tostringstream os;
      os << _T("C1 must be greater than or equal to 0.0 ") << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( c1 > w5 )
   {
      std::_tostringstream os;
      os << _T("C1 must be less than or equal to W5") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( c1 > d5 )
   {
      std::_tostringstream os;
      os << _T("C1 must be less than or equal to D5") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( w1 > w2-2*(w4+w5) )
   {
      std::_tostringstream os;
      os << _T("W1 must be less than w2 - 2 * (w4+w5)") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   


   // build our shape so we can get higher-level info
   CComPtr<IUBeam2> beam;
   beam.CoCreateInstance(CLSID_UBeam2);

   ConfigureShape(dimensions, beam);

   // our goal is to build a parallelogram using the thin web dimension from top to bottom
   double t;
   beam->get_T(&t);
   if ( t<=0.0 )
   {
      std::_tostringstream os;
      os << _T("Dimensions are such that the web thickness is zero.") << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   
   return true;
}

void CUBeam2Factory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   // Version 2.0 - Added EndBlockLength
   pSave->BeginUnit(_T("UBeam2Dimensions"),2.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CUBeam2Factory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version = pLoad->GetVersion();

   IBeamFactory::Dimensions dimensions;
   std::vector<std::_tstring>::iterator iter;

   Float64 dimVersion = 1.0;
   if ( 14 <= parent_version )
   {
      if ( pLoad->BeginUnit(_T("UBeam2Dimensions")) )
         dimVersion = pLoad->GetVersion();
      else
         THROW_LOAD(InvalidFileFormat,pLoad);
   }

   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
      {
         // failed to read dimension value...
         
         if ( dimVersion < 2)
         {
            if( parent_version < 3.0 && name == _T("C1") )
            {
               value = 0.0; // set the default value
            }
            else if (name == _T("EndBlockLength"))
            {
               value = 0.0;
            }
            else
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
         else
         {
            THROW_LOAD(InvalidFileFormat,pLoad);
         }
      }
      dimensions.push_back( std::make_pair(name,value) );
   }

   if ( 14 <= parent_version && !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   return dimensions;
}

bool CUBeam2Factory::IsPrismatic(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));

   return IsZero(endBlockLength) ? true : false;
}

Float64 CUBeam2Factory::GetVolume(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
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

Float64 CUBeam2Factory::GetSurfaceArea(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bReduceForPoorlyVentilatedVoids)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::CastingYard,POI_SECTCHANGE,POIFIND_OR);
   ATLASSERT( 2 <= vPOI.size() );
   Float64 As = 0;
   std::vector<pgsPointOfInterest>::iterator iter = vPOI.begin();
   pgsPointOfInterest prev_poi = *iter;
   Float64 prev_area = pSectProp2->GetPerimeter(prev_poi);
   iter++;

   for ( ; iter != vPOI.end(); iter++ )
   {
      pgsPointOfInterest poi = *iter;
      Float64 area = pSectProp2->GetPerimeter(poi);

      Float64 avg_area = (prev_area + area)/2;
      As += avg_area*(poi.GetDistFromStart() - prev_poi.GetDistFromStart());

      prev_poi = poi;
      prev_area = area;
   }

   // add area of both ends of the girder
   Float64 start_area = pSectProp2->GetAg(pgsTypes::CastingYard,vPOI.front());
   Float64 end_area   = pSectProp2->GetAg(pgsTypes::CastingYard,vPOI.back());
   As += (start_area + end_area);

   return As;
}

std::_tstring CUBeam2Factory::GetImage()
{
   return std::_tstring(_T("UBeam2.jpg"));
}

std::_tstring CUBeam2Factory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      strImage =  _T("UBeam_Slab_CIP.gif");
      break;

   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("UBeam_Slab_SIP.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CUBeam2Factory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("+Mn_UBeam_Composite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CUBeam2Factory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("-Mn_UBeam_Composite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CUBeam2Factory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("Vn_UBeam.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CUBeam2Factory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("UBeam_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("UBeam_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring CUBeam2Factory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("UBeam_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("UBeam_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CUBeam2Factory::GetCLSID()
{
   return CLSID_UBeam2Factory;
}

CLSID CUBeam2Factory::GetFamilyCLSID()
{
   return CLSID_UBeamFamily;
}

std::_tstring CUBeam2Factory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CUBeam2Factory::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

HINSTANCE CUBeam2Factory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CUBeam2Factory::GetImageResourceName()
{
   return _T("UBEAM2");
}

HICON  CUBeam2Factory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TEXASU) );
}

void CUBeam2Factory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                  double& d1,double& d2,double& d3,double& d4,double& d5,double& d6,
                                  double& w1,double& w2,double& w3,double& w4,double& w5,double& w6,double& w7,
                                  double& c1,double& EndBlockLength)
{
   d1 = GetDimension(dimensions,_T("D1"));
   d2 = GetDimension(dimensions,_T("D2"));
   d3 = GetDimension(dimensions,_T("D3"));
   d4 = GetDimension(dimensions,_T("D4"));
   d5 = GetDimension(dimensions,_T("D5"));
   d6 = GetDimension(dimensions,_T("D6"));
   w1 = GetDimension(dimensions,_T("W1"));
   w2 = GetDimension(dimensions,_T("W2"));
   w3 = GetDimension(dimensions,_T("W3"));
   w4 = GetDimension(dimensions,_T("W4"));
   w5 = GetDimension(dimensions,_T("W5"));
   w6 = GetDimension(dimensions,_T("W6"));
   w7 = GetDimension(dimensions,_T("W7"));
   c1 = GetDimension(dimensions,_T("C1"));
   EndBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
}

double CUBeam2Factory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::_tstring& name)
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

pgsTypes::SupportedDeckTypes CUBeam2Factory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CUBeam2Factory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);
   return sbs;
}

void CUBeam2Factory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, double* minSpacing, double* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   double W1 = GetDimension(dimensions,_T("W1"));
   double W2 = GetDimension(dimensions,_T("W2"));

   double gw = max(W1, W2);


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

WebIndexType CUBeam2Factory::GetNumberOfWebs(const IBeamFactory::Dimensions& dimensions)
{
   return 2;
}

Float64 CUBeam2Factory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("D1"));
}

Float64 CUBeam2Factory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("W2"));
}

bool CUBeam2Factory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CUBeam2Factory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}