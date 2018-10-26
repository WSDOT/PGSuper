///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

// TaperedIBeamFactory.cpp : Implementation of CTaperedIBeamFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include "BeamFamilyCLSID.h"
#include "TaperedIBeamFactory.h"
#include "IBeamDistFactorEngineer.h"
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

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTaperedIBeamFactory
HRESULT CTaperedIBeamFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back("C1");
   m_DimNames.push_back("D1");
   m_DimNames.push_back("D2");
   m_DimNames.push_back("D3");
   m_DimNames.push_back("D4");
   m_DimNames.push_back("D5");
   m_DimNames.push_back("D6");
   m_DimNames.push_back("D7_Start");
   m_DimNames.push_back("D7_End");
   m_DimNames.push_back("T1");
   m_DimNames.push_back("T2");
   m_DimNames.push_back("W1");
   m_DimNames.push_back("W2");
   m_DimNames.push_back("W3");
   m_DimNames.push_back("W4");

//   std::sort(m_DimNames.begin(),m_DimNames.end());

   // Default beam is a WF74/83G
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.000,unitMeasure::Inch)); // C1
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // D1
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // D2
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // D3
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.125,unitMeasure::Inch)); // D4
   m_DefaultDims.push_back(::ConvertToSysUnits( 4.500,unitMeasure::Inch)); // D5
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // D6
   m_DefaultDims.push_back(::ConvertToSysUnits(52.375,unitMeasure::Inch)); // D7 start
   m_DefaultDims.push_back(::ConvertToSysUnits(61.000,unitMeasure::Inch)); // D7 end
   m_DefaultDims.push_back(::ConvertToSysUnits( 6.125,unitMeasure::Inch)); // T1
   m_DefaultDims.push_back(::ConvertToSysUnits( 6.125,unitMeasure::Inch)); // T2
   m_DefaultDims.push_back(::ConvertToSysUnits(18.4375,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits(13.125,unitMeasure::Inch)); // W3
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.000,unitMeasure::Inch)); // W4

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D5
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D6
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D7 start
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D7 end
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W4

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D5
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D6
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D7 start
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D7 end
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W4

   return S_OK;
}

void CTaperedIBeamFactory::CreateGirderSection(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IGirderSection** ppSection)
{
   CreateGirderSection(pBroker,statusGroupID,spanIdx,gdrIdx,pgsTypes::metStart,dimensions,ppSection);
}

void CTaperedIBeamFactory::CreateGirderSection(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,pgsTypes::MemberEndType end,const IBeamFactory::Dimensions& dimensions,IGirderSection** ppSection)
{
   CComPtr<IFlangedGirderSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_FlangedGirderSection);

   CComPtr<IPrecastBeam> beam;
   gdrsection->get_Beam(&beam);

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7s,d7e;
   Float64 w1,w2,w3,w4;
   Float64 t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7s,d7e,w1,w2,w3,w4,t1,t2,c1);
   beam->put_W1(w1);
   beam->put_W2(w2);
   beam->put_W3(w3);
   beam->put_W4(w4);
   beam->put_D1(d1);
   beam->put_D2(d2);
   beam->put_D3(d3);
   beam->put_D4(d4);
   beam->put_D5(d5);
   beam->put_D6(d6);

   if ( end == pgsTypes::metStart )
      beam->put_D7(d7s);
   else
      beam->put_D7(d7e);

   beam->put_T1(t1);
   beam->put_T2(t2);
   beam->put_C1(c1);

   gdrsection.QueryInterface(ppSection);
}

void CTaperedIBeamFactory::CreateGirderProfile(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetGirderLength(spanIdx,gdrIdx);

   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7s,d7e;
   Float64 w1,w2,w3,w4;
   Float64 t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7s,d7e,w1,w2,w3,w4,t1,t2,c1);

   Float64 height_start = d1 + d2 + d3 + d4 + d5 + d6 + d7s;
   Float64 height_end   = d1 + d2 + d3 + d4 + d5 + d6 + d7e;

   CComPtr<IPolyShape> shape;
   shape.CoCreateInstance(CLSID_PolyShape);
   shape->AddPoint(0,0); // top left corner of shape
   shape->AddPoint(length,0);
   shape->AddPoint(length,-height_end);
   shape->AddPoint(0,-height_start);

   shape->QueryInterface(ppShape);
}

void CTaperedIBeamFactory::LayoutGirderLine(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* ssmbr)
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
      segment.CoCreateInstance(CLSID_TaperedGirderSegment);
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
      CreateGirderSection(pBroker,statusGroupID,spanIdx,gdrIdx,dimensions,&gdrSection);
      CComQIPtr<IPrismaticSegment> prisSegment(segment);
      CComQIPtr<IShape> shape(gdrSection);
      prisSegment->putref_Shape(shape);
   }
   else
   {
      for ( int i = 0; i < 2; i++ )
      {
         CComPtr<IGirderSection> gdrSection;
         CreateGirderSection(pBroker,statusGroupID,spanIdx,gdrIdx,(pgsTypes::MemberEndType)i,dimensions,&gdrSection);

         CComQIPtr<ITaperedGirderSegment> taperedSegment(segment);
         CComQIPtr<IFlangedGirderSection> flangedSection(gdrSection);

         taperedSegment->putref_FlangedGirderSection( (EndType)i, flangedSection );
      }
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

void CTaperedIBeamFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsPoiMgr* pPoiMgr)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const GirderLibraryEntry* pGirderEntry = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);

#if defined _DEBUG
   std::string strGirderName = pSpan->GetGirderTypes()->GetGirderName(gdr);
   ATLASSERT( strGirderName == pGirderEntry->GetName() );
#endif

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 gdrLength = pBridge->GetGirderLength(span,gdr);

   PoiAttributeType attrib = POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL;
   Float64 start_length = pBridge->GetGirderStartConnectionLength(span,gdr);
   Float64 end_length   = pBridge->GetGirderEndConnectionLength(span,gdr);

   std::vector<pgsTypes::Stage> stages;
   stages.push_back(pgsTypes::CastingYard);
   stages.push_back(pgsTypes::Lifting);
   stages.push_back(pgsTypes::Hauling);
   stages.push_back(pgsTypes::GirderPlacement);
   stages.push_back(pgsTypes::TemporaryStrandRemoval);
   stages.push_back(pgsTypes::BridgeSite1);
   stages.push_back(pgsTypes::BridgeSite2);
   stages.push_back(pgsTypes::BridgeSite3);
   
   for ( int i = 0; i < 11; i++ )
   {
      Float64 x = i*gdrLength/10;

      if ( x < start_length || gdrLength-end_length < x)
      {
         pgsPointOfInterest poi(span,gdr,x);
         poi.AddStage(pgsTypes::CastingYard,attrib);
         poi.AddStage(pgsTypes::Lifting,    attrib);
         poi.AddStage(pgsTypes::Hauling,    attrib);
         pPoiMgr->AddPointOfInterest( poi );
      }
      else
      {
         pgsPointOfInterest poi(stages,span,gdr,x,attrib);
         pPoiMgr->AddPointOfInterest(poi);
      }
   }
}

void CTaperedIBeamFactory::CreateDistFactorEngineer(IBroker* pBroker,long statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   CComObject<CIBeamDistFactorEngineer>* pEngineer;
   CComObject<CIBeamDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CTaperedIBeamFactory::CreatePsLossEngineer(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,IPsLossEngineer** ppEng)
{
   CComObject<CPsBeamLossEngineer>* pEngineer;
   CComObject<CPsBeamLossEngineer>::CreateInstance(&pEngineer);
   pEngineer->Init(CPsLossEngineer::IBeam);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CTaperedIBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
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

   double c1;
   double d1,d2,d3,d4,d5,d6,d7s,d7e;
   double w1,w2,w3,w4;
   double t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7s,d7e,w1,w2,w3,w4,t1,t2,c1);

   double width = min(t1,t2);
   double depth = d1 + d2 + d3 + d4 + d5 + d6 + d7s;
#pragma Reminder("*** Review strand mover for beam with variable depth")

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


std::vector<std::string> CTaperedIBeamFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<double> CTaperedIBeamFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CTaperedIBeamFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CTaperedIBeamFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::string* strErrMsg)
{
   Float64 c1;
   Float64 d1,d2,d3,d4,d5,d6,d7s,d7e;
   Float64 w1,w2,w3,w4;
   Float64 t1,t2;
   GetDimensions(dimensions,d1,d2,d3,d4,d5,d6,d7s,d7e,w1,w2,w3,w4,t1,t2,c1);

// C1  0
// D1  1
// D2  2
// D3  3
// D4  4
// D5  5
// D6  6
// D7Start  7
// D7End    8
// T1  9
// T2  10
// W1  11
// W2  12
// W3  13
// W4  14

   if ( d1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][1];
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][4];
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

   if ( d7s <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][7];
      std::ostringstream os;
      os << "D7 at Start must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( d7e <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][8];
      std::ostringstream os;
      os << "D7 at End must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][11];
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][13];
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
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][9];
      std::ostringstream os;
      os << "T1 must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   
   
   if ( t2 <= 0.0 )
   {
      const unitLength* pUnit = m_DimUnits[bSIUnits ? 0 : 1][10];
      std::ostringstream os;
      os << "T2 must be greater than 0.0 " << pUnit->UnitTag() << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   if ( c1 < 0.0 )
   {
      std::ostringstream os;
      os << "C1 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( c1 > d4 )
   {
      std::ostringstream os;
      os << "C1 must be greater than D4" << std::ends;
      *strErrMsg = os.str();
      return false;
   }   

   return true;
}

void CTaperedIBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::string>::iterator iter;
   pSave->BeginUnit("TaperedIBeamDimensions",1.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::string name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CTaperedIBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version = pLoad->GetVersion();

   IBeamFactory::Dimensions dimensions;
   std::vector<std::string>::iterator iter;

   if ( !pLoad->BeginUnit("TaperedIBeamDimensions") )
      THROW_LOAD(InvalidFileFormat,pLoad);

   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::string name = *iter;
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      dimensions.push_back( std::make_pair(name,value) );
   }

   if ( !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   return dimensions;
}

bool CTaperedIBeamFactory::IsPrismatic(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();

   Float64 d7s = GetDimension(dimensions,"D7_Start");
   Float64 d7e = GetDimension(dimensions,"D7_End");

   bool bPrismatic = true;
   if ( IsEqual(d7s,d7e) )
   {
      // prismatic
      bPrismatic = true;
   }
   else
   {
      // non-prismatic
      bPrismatic = false;
   }

   return bPrismatic;
}

Float64 CTaperedIBeamFactory::GetVolume(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::CastingYard,POI_SECTCHANGE);
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

Float64 CTaperedIBeamFactory::GetSurfaceArea(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bReduceForPoorlyVentilatedVoids)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   GET_IFACE2(pBroker,IPointOfInterest,pPOI);

   std::vector<pgsPointOfInterest> vPOI = pPOI->GetPointsOfInterest(spanIdx,gdrIdx,pgsTypes::CastingYard,POI_SECTCHANGE);
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

std::string CTaperedIBeamFactory::GetImage()
{
   return std::string("TaperedIBeam.jpg");
}

std::string CTaperedIBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      strImage =  "IBeam_Slab_CIP.gif";
      break;

   case pgsTypes::sdtCompositeSIP:
      strImage =  "IBeam_Slab_SIP.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CTaperedIBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  "+Mn_IBeam_Composite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CTaperedIBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  "-Mn_IBeam_Composite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CTaperedIBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  "Vn_IBeam.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CTaperedIBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return "IBeam_Effective_Flange_Width_Interior_Girder_2008.gif";
   }
   else
   {
      return "IBeam_Effective_Flange_Width_Interior_Girder.gif";
   }
}

std::string CTaperedIBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return "IBeam_Effective_Flange_Width_Exterior_Girder_2008.gif";
   }
   else
   {
      return "IBeam_Effective_Flange_Width_Exterior_Girder.gif";
   }
}

CLSID CTaperedIBeamFactory::GetCLSID()
{
   return CLSID_TaperedIBeamFactory;
}

CLSID CTaperedIBeamFactory::GetFamilyCLSID()
{
   return CLSID_WFBeamFamily;
}

std::string CTaperedIBeamFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::string( OLE2A(pszUserType) );
}

std::string CTaperedIBeamFactory::GetPublisher()
{
   return std::string("WSDOT");
}

HINSTANCE CTaperedIBeamFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CTaperedIBeamFactory::GetImageResourceName()
{
   return _T("TAPEREDIBEAM");
}

HICON  CTaperedIBeamFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_IBEAM) );
}

void CTaperedIBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                  double& d1,double& d2,double& d3,double& d4,double& d5,double& d6,double& d7s,double& d7e,
                                  double& w1,double& w2,double& w3,double& w4,
                                  double& t1,double& t2, double& c1)
{
   d1 = GetDimension(dimensions,"D1");
   d2 = GetDimension(dimensions,"D2");
   d3 = GetDimension(dimensions,"D3");
   d4 = GetDimension(dimensions,"D4");
   d5 = GetDimension(dimensions,"D5");
   d6 = GetDimension(dimensions,"D6");
   d7s = GetDimension(dimensions,"D7_Start");
   d7e = GetDimension(dimensions,"D7_End");
   w1 = GetDimension(dimensions,"W1");
   w2 = GetDimension(dimensions,"W2");
   w3 = GetDimension(dimensions,"W3");
   w4 = GetDimension(dimensions,"W4");
   t1 = GetDimension(dimensions,"T1");
   t2 = GetDimension(dimensions,"T2");
   c1 = GetDimension(dimensions,"C1"); 
}

double CTaperedIBeamFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::string& name)
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

pgsTypes::SupportedDeckTypes CTaperedIBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CTaperedIBeamFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);
   return sbs;
}

void CTaperedIBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, double* minSpacing, double* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   double T1 = GetDimension(dimensions,"T1");
   double T2 = GetDimension(dimensions,"T2");
   double W1 = GetDimension(dimensions,"W1");
   double W2 = GetDimension(dimensions,"W2");
   double W3 = GetDimension(dimensions,"W3");
   double W4 = GetDimension(dimensions,"W4");

   double top_w = T1 + 2.0*(W1+W2);
   double bot_w = T2 + 2.0*(W3+W4);

   double gw = max(top_w, bot_w);


   if ( sdt == pgsTypes::sdtCompositeCIP || sdt == pgsTypes::sdtCompositeSIP )
   {
      if(sbs == pgsTypes::sbsUniform || sbs == pgsTypes::sbsGeneral)
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

long CTaperedIBeamFactory::GetNumberOfWebs(const IBeamFactory::Dimensions& dimensions)
{
   return 1;
}

Float64 CTaperedIBeamFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   double D1 = GetDimension(dimensions,"D1");
   double D2 = GetDimension(dimensions,"D2");
   double D3 = GetDimension(dimensions,"D3");
   double D4 = GetDimension(dimensions,"D4");
   double D5 = GetDimension(dimensions,"D5");
   double D6 = GetDimension(dimensions,"D6");
   double D7_start = GetDimension(dimensions,"D7_Start");
   double D7_end   = GetDimension(dimensions,"D7_End");

   return D1 + D2 + D3 + D4 + D5 + D6 + (endType == pgsTypes::metStart ? D7_start : D7_end);
}

Float64 CTaperedIBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   double W1 = GetDimension(dimensions,"W1");
   double W2 = GetDimension(dimensions,"W2");
   double W3 = GetDimension(dimensions,"W3");
   double W4 = GetDimension(dimensions,"W4");
   double T1 = GetDimension(dimensions,"T1");
   double T2 = GetDimension(dimensions,"T2");

   double top = 2*(W1+W2) + T1;
   double bot = 2*(W3+W4) + T2;

   return max(top,bot);
}

bool CTaperedIBeamFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CTaperedIBeamFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}