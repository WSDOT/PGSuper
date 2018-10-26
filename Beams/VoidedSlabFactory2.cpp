///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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

// VoidedSlab2Factory.cpp : Implementation of CVoidedSlab2Factory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include "BeamFamilyCLSID.h"
#include "VoidedSlabFactory2.h"
#include "IBeamDistFactorEngineer.h"
#include "VoidedSlab2DistFactorEngineer.h"
#include "PsBeamLossEngineer.h"
#include "StrandMoverImpl.h"
#include <BridgeModeling\PrismaticGirderProfile.h>
#include <GeomModel\PrecastBeam.h>
#include <MathEx.h>
#include <sstream>
#include <algorithm>

#include <IFace\Project.h>
#include <IFace\Bridge.h>
#include <IFace\StatusCenter.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVoidedSlab2Factory
HRESULT CVoidedSlab2Factory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("H"));
   m_DimNames.push_back(_T("W"));
   m_DimNames.push_back(_T("Number_of_Voids"));
   m_DimNames.push_back(_T("D1"));
   m_DimNames.push_back(_T("D2"));
   m_DimNames.push_back(_T("H1"));
   m_DimNames.push_back(_T("H2"));
   m_DimNames.push_back(_T("S1"));
   m_DimNames.push_back(_T("S2"));
   m_DimNames.push_back(_T("C1"));
   m_DimNames.push_back(_T("C2"));
   m_DimNames.push_back(_T("C3"));
   m_DimNames.push_back(_T("Jmax"));
   m_DimNames.push_back(_T("EndBlockLength"));

   m_DefaultDims.push_back(::ConvertToSysUnits(18.0,unitMeasure::Inch)); // H
   m_DefaultDims.push_back(::ConvertToSysUnits(48.0,unitMeasure::Inch)); // W
   m_DefaultDims.push_back(3);                                           // Number of Voids
   m_DefaultDims.push_back(::ConvertToSysUnits(10.0,unitMeasure::Inch)); // D1
   m_DefaultDims.push_back(::ConvertToSysUnits(10.0,unitMeasure::Inch)); // D2
   m_DefaultDims.push_back(::ConvertToSysUnits(12.0,unitMeasure::Inch)); // H1
   m_DefaultDims.push_back(::ConvertToSysUnits(12.0,unitMeasure::Inch)); // H2
   m_DefaultDims.push_back(::ConvertToSysUnits(12.5,unitMeasure::Inch)); // S1
   m_DefaultDims.push_back(::ConvertToSysUnits(12.5,unitMeasure::Inch)); // S2
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // C1
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // C2
   m_DefaultDims.push_back(::ConvertToSysUnits( 1.0,unitMeasure::Inch)); // C3
   m_DefaultDims.push_back(::ConvertToSysUnits(1.0,unitMeasure::Inch));  // Max Joint Spacing
   m_DefaultDims.push_back(::ConvertToSysUnits(36.0,unitMeasure::Inch)); // End Block Length

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H 
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W
   m_DimUnits[0].push_back(NULL);                     // Number of Voids
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // D2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // S1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // S2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Max joint size
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // End Block Length

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H 
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W
   m_DimUnits[1].push_back(NULL);               // Number of Voids
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // D2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // S1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // S2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Max joint size
   m_DimUnits[1].push_back(&unitMeasure::Inch); // End Block Length

   return S_OK;
}

void CVoidedSlab2Factory::CreateGirderSection(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IGirderSection** ppSection)
{
   CComPtr<IVoidedSlabSection2> gdrsection;
   gdrsection.CoCreateInstance(CLSID_VoidedSlabSection2);
   CComPtr<IVoidedSlab2> beam;
   gdrsection->get_Beam(&beam);

   double H,W,D1,D2,H1,H2,S1,S2,C1,C2,C3,J,EndBlockLength;
   long N;
   GetDimensions(dimensions,H,W,D1,D2,H1,H2,S1,S2,C1,C2,C3,N,J,EndBlockLength);

   beam->put_Height(H);
   beam->put_Width(W);
   beam->put_C1(C1);
   beam->put_C2(C2);
   beam->put_C3(C3);
   beam->put_ExteriorVoidDiameter(D1);
   beam->put_InteriorVoidDiameter(D2);
   beam->put_ExteriorVoidElevation(H1);
   beam->put_InteriorVoidElevation(H2);
   beam->put_ExteriorVoidSpacing(S1);
   beam->put_InteriorVoidSpacing(S2);
   beam->put_VoidCount(N);

   gdrsection.QueryInterface(ppSection);
}

void CVoidedSlab2Factory::CreateGirderProfile(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetGirderLength(spanIdx,gdrIdx);

   Float64 height = GetDimension(dimensions,_T("H"));

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

void CVoidedSlab2Factory::LayoutGirderLine(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* ssmbr)
{
   CComPtr<IVoidedSlabEndBlockSegment> segment;
   segment.CoCreateInstance(CLSID_VoidedSlabEndBlockSegment);


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
   CComQIPtr<IVoidedSlabSection2> section(gdrsection);

   // if this is an exterior girder, remove the shear key block outs
   CComPtr<IVoidedSlab2> voidedSlabShape;
   section->get_Beam(&voidedSlabShape);
   if ( gdrIdx == 0 )
   {
      voidedSlabShape->put_LeftBlockOut(VARIANT_FALSE);
   }

   if ( gdrIdx == pIBridgeDesc->GetSpan(spanIdx)->GetGirderCount()-1 )
   {
      voidedSlabShape->put_RightBlockOut(VARIANT_FALSE);
   }
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

void CVoidedSlab2Factory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsPoiMgr* pPoiMgr)
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

void CVoidedSlab2Factory::CreateDistFactorEngineer(IBroker* pBroker,long statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   CComObject<CVoidedSlab2DistFactorEngineer>* pEngineer;
   CComObject<CVoidedSlab2DistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CVoidedSlab2Factory::CreatePsLossEngineer(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,IPsLossEngineer** ppEng)
{
   CComObject<CPsBeamLossEngineer>* pEngineer;
   CComObject<CPsBeamLossEngineer>::CreateInstance(&pEngineer);
    
   // depends on # of voids
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(spanIdx);
   const GirderLibraryEntry* pGirderLib = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);

   double nVoids = pGirderLib->GetDimension(_T("Number_of_Voids"));

   if ( nVoids == 0 )
      pEngineer->Init(CPsLossEngineer::SolidSlab);
   else
      pEngineer->Init(CPsLossEngineer::SingleT);

   pEngineer->SetBroker(pBroker,statusGroupID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

static void MakeRectangle(double width, double depth, double xOffset, IShape** shape)
{
   CComPtr<IRectangle> harp_rect;
   HRESULT hr = harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   harp_rect->put_Width(width);
   harp_rect->put_Height(depth);

   double hook_offset = 0.0;

   CComPtr<IPoint2d> hook;
   hook.CoCreateInstance(CLSID_Point2d);
   hook->Move(xOffset, depth/2.0);

   harp_rect->putref_HookPoint(hook);

   harp_rect->get_Shape(shape);
}

void CVoidedSlab2Factory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, double endTopLimit, IBeamFactory::BeamFace endBottomFace, double endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, double hpTopLimit, IBeamFactory::BeamFace hpBottomFace, double hpBottomLimit, 
                                  double endIncrement, double hpIncrement, IStrandMover** strandMover)
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   CComQIPtr<IConfigureStrandMover> configurer(sm);

   // Set the shapes for harped strand bounds 
   // Voided slabs don't normally support harped strands, so the question
   double H,W,D1,D2,H1,H2,S1,S2,C1,C2,C3,J,EndBlockLength;
   long N;
   GetDimensions(dimensions,H,W,D1,D2,H1,H2,S1,S2,C1,C2,C3,N,J,EndBlockLength);

   double width = W;
   double depth = H;

   if (N==0)
   {
      // easy part, no voids
      double hook_offset = 0.0;

      CComPtr<IShape> shape;
      MakeRectangle(width, depth, hook_offset, &shape);

      hr = configurer->AddRegion(shape, 0.0);
      ATLASSERT (SUCCEEDED(hr));
   }
   else
   {
      // multiple voids, put rectangles between them
      long nIntVoids, nExtVoids;
      if ( N == 0 )
      {
         nIntVoids = 0;
         nExtVoids = 0;
      }
      else if ( N == 1 )
      {
         nIntVoids = 0;
         nExtVoids = 1;
      }
      else
      {
         nExtVoids = 2;
         nIntVoids = N - nExtVoids;
      }

      Float64 t_ext;
      if ( nIntVoids == 0 )
         t_ext = (width - (nExtVoids-1)*S1 - D1)/2;
      else
         t_ext = (width - (nIntVoids-1)*S2 - 2*S1 - D1)/2;

      // thickness of interior _T("web") (between interior voids)
      Float64 t_int;
      if ( nIntVoids == 0 )
         t_int = 0;
      else
         t_int = S2 - D2;

      // thickness of _T("web") between interior and exterior voids)
      Float64 t_ext_int;
      if ( nIntVoids == 0 )
         t_ext_int = S1 - D1;
      else
         t_ext_int = S1 - D1/2 - D2/2;

      double end_loc = (width-t_ext)/2.0; 

      // rectangles at ends
      CComPtr<IShape> shapel, shaper;
      MakeRectangle(t_ext, depth - C1 - C2, -end_loc, &shapel);
      MakeRectangle(t_ext, depth - C1 - C2,  end_loc, &shaper);

      hr = configurer->AddRegion(shapel, 0.0);
      ATLASSERT (SUCCEEDED(hr));
      
      hr = configurer->AddRegion(shaper, 0.0);
      ATLASSERT (SUCCEEDED(hr));

      if ( nIntVoids == 0 && nExtVoids == 2 )
      {
         CComPtr<IShape> shape;
         MakeRectangle(t_ext_int, depth, 0.00, &shape);
         hr = configurer->AddRegion(shape, 0.0);
         ATLASSERT (SUCCEEDED(hr));
      }
      else if ( 0 < nIntVoids )
      {
         // rectangles between interior and exterior voids
         Float64 loc = width/2 - t_ext - D1 - t_ext_int/2;
         shapel.Release();
         shaper.Release();

         MakeRectangle(t_ext_int, depth, -loc, &shapel);
         MakeRectangle(t_ext_int, depth,  loc, &shaper);

         hr = configurer->AddRegion(shapel, 0.0);
         ATLASSERT (SUCCEEDED(hr));
         
         hr = configurer->AddRegion(shaper, 0.0);
         ATLASSERT (SUCCEEDED(hr));

         // retangles between interior voids
         loc = width/2 - t_ext - D1 - t_ext_int - D2 - t_int/2;
         loc *= -1;
         for ( Uint16 i = 1; i < nIntVoids; i++ )
         {
            CComPtr<IShape> shape;
            MakeRectangle(t_int,depth,loc,&shape);
            hr = configurer->AddRegion(shape, 0.0);
            ATLASSERT (SUCCEEDED(hr));

            loc += S2;
         }
      }
   }

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

std::vector<std::_tstring> CVoidedSlab2Factory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<double> CVoidedSlab2Factory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CVoidedSlab2Factory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CVoidedSlab2Factory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSI,std::_tstring* strErrMsg)
{
   double H,W,D1,D2,H1,H2,S1,S2,C1,C2,C3,J,EndBlockLength;
   long N;
   GetDimensions(dimensions,H,W,D1,D2,H1,H2,S1,S2,C1,C2,C3,N,J,EndBlockLength);

   if ( H <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("Height must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W <= 0.0 )
   {
      std::_tostringstream os;
      os << _T("Width must be a positive value") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( N < 0 )
   {
      std::_tostringstream os;
      os << _T("Invalid Number of Voids") << std::ends;
      *strErrMsg = os.str();
      return false;
   }
   else if (N == 0)
   {
      if ( !IsZero(D1) || !IsZero(D2) )
      {
         std::_tostringstream os;
         os << _T("Void Diameter Must Be Zero If No Voids") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( !IsZero(S1) || !IsZero(S2) )
      {
         std::_tostringstream os;
         os << _T("Invalid - Void Spacing Must Be Zero If No Voids") << std::ends;
         *strErrMsg = os.str();
         return false;
      }
   }
   else // (N > 0)
   {
      if ( N == 1 && !IsZero(S1) )
      {
         std::_tostringstream os;
         os << _T("Invalid - Exterior Void Spacing Must Be Zero") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( (N == 1 || N == 2) && !IsZero(D2) )
      {
         std::_tostringstream os;
         os << _T("Invalid - Interior Void Diameter Must Be Zero") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( (N == 1 || N == 2) && !IsZero(S2) )
      {
         std::_tostringstream os;
         os << _T("Invalid - Interior Void Spacing Must Be Zero") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( D1 <= 0.0 )
      {
         std::_tostringstream os;
         os << _T("Exterior Void Diameter Must Be Greater Than Zero") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( H <= D1 )
      {
         std::_tostringstream os;
         os << _T("Exterior Void Diameter must be less than slab height") << std::ends;
         *strErrMsg = os.str();
         return false;
      }


      if ( 2 < N )
      {
         if ( D2 <= 0.0 )
         {
            std::_tostringstream os;
            os << _T("Interior Void Diameter Must Be Greater Than Zero") << std::ends;
            *strErrMsg = os.str();
            return false;
         }

         if ( H <= D2 )
         {
            std::_tostringstream os;
            os << _T("Interior Void Diameter must be less than slab height") << std::ends;
            *strErrMsg = os.str();
            return false;
         }
      }

      if ( N == 1 )
      {
         if ( W <= D1 )
         {
            std::_tostringstream os;
            os << _T("Void Diameter must be less than slab width") << std::ends;
            *strErrMsg = os.str();
            return false;
         }

         if ( W <= D1 )
         {
            std::_tostringstream os;
            os << _T("Void Diameter must be less than slab width") << std::ends;
            *strErrMsg = os.str();
            return false;
         }
      }
      else
      {
         if ( N == 2 )
         {
            if ( W <= S1 + D1 )
            {
               std::_tostringstream os;
               os << _T("Slab must be wider than width occupied by voids") << std::ends;
               *strErrMsg = os.str();
               return false;
            }

            if ( S1 <= D1 )
            {
               std::_tostringstream os;
               os << _T("Void spacing must be greater than void diameter") << std::ends;
               *strErrMsg = os.str();
               return false;
            }
         }
         else
         {
            if ( W <= (N-3)*S2 + 2*S1 + D1 )
            {
               std::_tostringstream os;
               os << _T("Slab must be wider than width occupied by voids") << std::ends;
               *strErrMsg = os.str();
               return false;
            }

            if ( S1 <= D1/2+D2/2 )
            {
               std::_tostringstream os;
               os << _T("Void spacing must be greater than void diameter") << std::ends;
               *strErrMsg = os.str();
               return false;
            }

            if ( 3 < N && S2 <= D2 )
            {
               std::_tostringstream os;
               os << _T("Void spacing must be greater than void diameter") << std::ends;
               *strErrMsg = os.str();
               return false;
            }
         }
      }
   }

   if ( J < 0.0 )
   {
      std::_tostringstream os;
      os << _T("Maximum joint size must be zero or greater") << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   return true;
}

void CVoidedSlab2Factory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("VoidedSlab2Dimensions"),3.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CVoidedSlab2Factory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version = pLoad->GetVersion();

   IBeamFactory::Dimensions dimensions;
   std::vector<std::_tstring>::iterator iter;

   Float64 dimVersion = 1.0;
   if ( 14 <= parent_version )
   {
      if ( pLoad->BeginUnit(_T("VoidedSlab2Dimensions")) )
         dimVersion = pLoad->GetVersion();
      else
         THROW_LOAD(InvalidFileFormat,pLoad);
   }

   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = 0;
      if ( !pLoad->Property(name.c_str(),&value) )
      {
         if ( name == _T("C3") && dimVersion < 3)
         {
            // C3 was introduced in dimVersion 3
            // if version is less, it is ok to fail and continue
            value = 0;
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

bool CVoidedSlab2Factory::IsPrismatic(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));

   return IsZero(endBlockLength) ? true : false;
}

Float64 CVoidedSlab2Factory::GetVolume(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
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

Float64 CVoidedSlab2Factory::GetSurfaceArea(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bReduceForPoorlyVentilatedVoids)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   Float64 perimeter = pSectProp2->GetPerimeter(pgsPointOfInterest(spanIdx,gdrIdx,0.00));
   
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetGirderLength(spanIdx,gdrIdx);

   Float64 solid_slab_surface_area = perimeter*Lg;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 D1 = GetDimension(dimensions,_T("D1"));
   Float64 D2 = GetDimension(dimensions,_T("D2"));
   Float64 endBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
   long    N = (long)GetDimension(dimensions,_T("Number_of_Voids"));
   long nIntVoids, nExtVoids;
   if ( N == 0 )
   {
      nIntVoids = 0;
      nExtVoids = 0;
   }
   else if ( N == 1 )
   {
      nExtVoids = 1;
      nIntVoids = 0;
   }
   else
   {
      nExtVoids = 2;
      nIntVoids = N - nExtVoids;
   }

   Float64 void_surface_area = (Lg-2*endBlockLength)*(nExtVoids*M_PI*D1 + nIntVoids*M_PI*D2);

   if ( bReduceForPoorlyVentilatedVoids )
      void_surface_area *= 0.50;

   Float64 surface_area = solid_slab_surface_area + void_surface_area;

   return surface_area;
}

std::_tstring CVoidedSlab2Factory::GetImage()
{
   return std::_tstring(_T("VoidedSlab2.jpg"));
}

std::_tstring CVoidedSlab2Factory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage = _T("VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage = _T("VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlab2Factory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("+Mn_VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("+Mn_VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlab2Factory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("-Mn_VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("-Mn_VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlab2Factory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  _T("Vn_VoidedSlab_Composite.gif");
      break;

   case pgsTypes::sdtNone:
      strImage =  _T("Vn_VoidedSlab_Noncomposite.gif");
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlab2Factory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("VoidedSlab_Effective_Flange_Width_Interior_Girder_2008.gif");
   }
   else
   {
      return _T("VoidedSlab_Effective_Flange_Width_Interior_Girder.gif");
   }
}

std::_tstring CVoidedSlab2Factory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return _T("VoidedSlab_Effective_Flange_Width_Exterior_Girder_2008.gif");
   }
   else
   {
      return _T("VoidedSlab_Effective_Flange_Width_Exterior_Girder.gif");
   }
}

CLSID CVoidedSlab2Factory::GetCLSID()
{
   return CLSID_VoidedSlab2Factory;
}

CLSID CVoidedSlab2Factory::GetFamilyCLSID()
{
   return CLSID_SlabBeamFamily;
}

std::_tstring CVoidedSlab2Factory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CVoidedSlab2Factory::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

HINSTANCE CVoidedSlab2Factory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CVoidedSlab2Factory::GetImageResourceName()
{
   return _T("VOIDEDSLAB2");
}

HICON  CVoidedSlab2Factory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_VOIDEDSLAB2) );
}

void CVoidedSlab2Factory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                      double& H,double& W,double& D1,double& D2,double& H1,double& H2,double& S1,double& S2,double& C1,double& C2,double& C3,long& N,double& J,double& EndBlockLength)
{
   H = GetDimension(dimensions,_T("H"));
   W = GetDimension(dimensions,_T("W"));
   D1 = GetDimension(dimensions,_T("D1"));
   D2 = GetDimension(dimensions,_T("D2"));
   H1 = GetDimension(dimensions,_T("H1"));
   H2 = GetDimension(dimensions,_T("H2"));
   S1 = GetDimension(dimensions,_T("S1"));
   S2 = GetDimension(dimensions,_T("S2"));
   C1 = GetDimension(dimensions,_T("C1"));
   C2 = GetDimension(dimensions,_T("C2"));
   C3 = GetDimension(dimensions,_T("C3"));
   N = (long)GetDimension(dimensions,_T("Number_of_Voids"));
   J = GetDimension(dimensions,_T("Jmax"));
   EndBlockLength = GetDimension(dimensions,_T("EndBlockLength"));
}

double CVoidedSlab2Factory::GetDimension(const IBeamFactory::Dimensions& dimensions,
                                        const std::_tstring& name)
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

pgsTypes::SupportedDeckTypes CVoidedSlab2Factory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CVoidedSlab2Factory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);

   return sbs;
}

void CVoidedSlab2Factory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, double* minSpacing, double* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   double gw = GetDimension(dimensions,_T("W"));
   double J  = GetDimension(dimensions,_T("Jmax"));

   if ( sdt == pgsTypes::sdtCompositeOverlay || sdt == pgsTypes::sdtNone )
   {
      if(sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent)
      {
         *minSpacing = gw;
         *maxSpacing = gw+J;
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

long CVoidedSlab2Factory::GetNumberOfWebs(const IBeamFactory::Dimensions& dimensions)
{
   long nv = (long)GetDimension(dimensions,_T("Number_of_Voids"));
   return nv+1;
}

Float64 CVoidedSlab2Factory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("H"));
}

Float64 CVoidedSlab2Factory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("W"));
}

bool CVoidedSlab2Factory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CVoidedSlab2Factory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}