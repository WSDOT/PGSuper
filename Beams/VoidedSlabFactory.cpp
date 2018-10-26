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

// VoidedSlabFactory.cpp : Implementation of CVoidedSlabFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include "BeamFamilyCLSID.h"
#include "VoidedSlabFactory.h"
#include "IBeamDistFactorEngineer.h"
#include "VoidedSlabDistFactorEngineer.h"
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
#include <IFace\StatusCenter.h>

#include <PgsExt\BridgeDescription.h>
#include <PgsExt\StatusItem.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CVoidedSlabFactory
HRESULT CVoidedSlabFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back(_T("H"));
   m_DimNames.push_back(_T("W"));
   m_DimNames.push_back(_T("Void_Diameter"));
   m_DimNames.push_back(_T("Void_Spacing"));
   m_DimNames.push_back(_T("Number_of_Voids"));
   m_DimNames.push_back(_T("Jmax"));

   m_DefaultDims.push_back(::ConvertToSysUnits(18.0,unitMeasure::Inch)); // H
   m_DefaultDims.push_back(::ConvertToSysUnits(48.0,unitMeasure::Inch)); // W
   m_DefaultDims.push_back(::ConvertToSysUnits(10.0,unitMeasure::Inch)); // Void Diameter
   m_DefaultDims.push_back(::ConvertToSysUnits(12.5,unitMeasure::Inch)); // Void Spacing
   m_DefaultDims.push_back(3);                                           // Number of Voids
   m_DefaultDims.push_back(::ConvertToSysUnits(1.0,unitMeasure::Inch));  // Max Joint Spacing

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H 
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Void Diameter
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Void Spacing
   m_DimUnits[0].push_back(NULL);                     // Number of Voids
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Max joint size

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H 
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Void Diameter
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Void Spacing
   m_DimUnits[1].push_back(NULL);               // Number of Voids
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Max joint size

   return S_OK;
}

void CVoidedSlabFactory::CreateGirderSection(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IGirderSection** ppSection)
{
   CComPtr<IVoidedSlabSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_VoidedSlabSection);
   CComPtr<IVoidedSlab> beam;
   gdrsection->get_Beam(&beam);

   double H,W,D,S,J;
   long N;
   GetDimensions(dimensions,H,W,D,S,N,J);

   beam->put_Height(H);
   beam->put_Width(W);
   beam->put_VoidDiameter(D);
   beam->put_VoidSpacing(S);
   beam->put_VoidCount(N);

   gdrsection.QueryInterface(ppSection);
}

void CVoidedSlabFactory::CreateGirderProfile(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetGirderLength(spanIdx,gdrIdx);

   double H,W,D,S,J;
   long N;
   GetDimensions(dimensions,H,W,D,S,N,J);

   Float64 height = H;

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

void CVoidedSlabFactory::LayoutGirderLine(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* ssmbr)
{
   CComPtr<IPrismaticSegment> segment;
   segment.CoCreateInstance(CLSID_PrismaticSegment);

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

   CComPtr<IGirderSection> gdrsection;
   CreateGirderSection(pBroker,statusGroupID,spanIdx,gdrIdx,dimensions,&gdrsection);
   CComQIPtr<IShape> shape(gdrsection);
   segment->putref_Shape(shape);

   // Beam materials
   GET_IFACE2(pBroker,IBridgeMaterial,pMaterial);
   CComPtr<IMaterial> material;
   material.CoCreateInstance(CLSID_Material);
   material->put_E(pMaterial->GetEcGdr(spanIdx,gdrIdx));
   material->put_Density(pMaterial->GetStrDensityGdr(spanIdx,gdrIdx));
   segment->putref_Material(material);

   ssmbr->AddSegment(segment);
}

void CVoidedSlabFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsPoiMgr* pPoiMgr)
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
}

void CVoidedSlabFactory::CreateDistFactorEngineer(IBroker* pBroker,long statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   GET_IFACE2(pBroker,IBridge,pBridge);

   // use passed value if not null
   pgsTypes::SupportedDeckType deckType = (pDeckType!=NULL) ? *pDeckType : pBridge->GetDeckType();
   
   if ( deckType == pgsTypes::sdtCompositeOverlay || deckType == pgsTypes::sdtNone )
   {
      CComObject<CVoidedSlabDistFactorEngineer>* pEngineer;
      CComObject<CVoidedSlabDistFactorEngineer>::CreateInstance(&pEngineer);
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
      pEngineer->Init(true,true); // this is a type b cross section, and a spread slab
      pEngineer->SetBroker(pBroker,statusGroupID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
}

void CVoidedSlabFactory::CreatePsLossEngineer(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,IPsLossEngineer** ppEng)
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

void CVoidedSlabFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
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
   double H,W,D,S,J;
   long N;
   GetDimensions(dimensions,H,W,D,S,N,J);

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
      double voids_w = (N-1)*S + D;
      double end_width = (width-voids_w)/2.0;
      double end_loc = (width-end_width)/2.0; 

      // rectangles at ends
      CComPtr<IShape> shapel, shaper;
      MakeRectangle(end_width, depth, -end_loc, &shapel);
      MakeRectangle(end_width, depth,  end_loc, &shaper);

      hr = configurer->AddRegion(shapel, 0.0);
      ATLASSERT (SUCCEEDED(hr));
      hr = configurer->AddRegion(shaper, 0.0);
      ATLASSERT (SUCCEEDED(hr));

      // retangles between voids
      voids_w = S - D;
      double loc = -(end_loc - end_width/2.0 - D - voids_w/2.0);
      for(long iv=0; iv<N-1; iv++)
      {

         CComPtr<IShape> shape;
         MakeRectangle(voids_w, depth, loc, &shape);

         hr = configurer->AddRegion(shape, 0.0);
         ATLASSERT (SUCCEEDED(hr));

         loc += S;
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

std::vector<std::_tstring> CVoidedSlabFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<double> CVoidedSlabFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CVoidedSlabFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CVoidedSlabFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSI,std::_tstring* strErrMsg)
{
   double H,W,D,S,J;
   long N;
   GetDimensions(dimensions,H,W,D,S,N,J);

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

   if ( N < 0.0 )
   {
      std::_tostringstream os;
      os << _T("Invalid Number of Voids") << std::ends;
      *strErrMsg = os.str();
      return false;
   }
   else if (N == 0)
   {
      if ( D != 0.0 )
      {
         std::_tostringstream os;
         os << _T("Void Diameter Must Be Zero If No Voids") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( S != 0.0 )
      {
         std::_tostringstream os;
         os << _T("Invalid - Void Spacing Must Be Zero If No Voids") << std::ends;
         *strErrMsg = os.str();
         return false;
      }
   }
   else // (N > 0)
   {
      if ( D <= 0.0 )
      {
         std::_tostringstream os;
         os << _T("Void Diameter Must Be Greater Than Zero") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if ( D >= H )
      {
         std::_tostringstream os;
         os << _T("Void Diameter must be less than slab height") << std::ends;
         *strErrMsg = os.str();
         return false;
      }

      if (N == 1)
      {
         if ( S != 0.0 )
         {
            std::_tostringstream os;
            os << _T("Invalid - Void Spacing Must Be Zero If Only One Void") << std::ends;
            *strErrMsg = os.str();
            return false;
         }

         if ( D >= W )
         {
            std::_tostringstream os;
            os << _T("Void Diameter must be less than slab width") << std::ends;
            *strErrMsg = os.str();
            return false;
         }

      }
      else // (N > 1)
      {
         if ( S < D )
         {
            std::_tostringstream os;
            os << _T("Void Spacing must be greater than Void Diameter") << std::ends;
            *strErrMsg = os.str();
            return false;
         }

         if ( W <= (N-1)*S + D)
         {
            std::_tostringstream os;
            os << _T("Slab must be wider than width occupied by voids") << std::ends;
            *strErrMsg = os.str();
            return false;
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

void CVoidedSlabFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::_tstring>::iterator iter;
   pSave->BeginUnit(_T("VoidedSlabDimensions"),2.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::_tstring name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CVoidedSlabFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version = pLoad->GetVersion();

   IBeamFactory::Dimensions dimensions;
   std::vector<std::_tstring>::iterator iter;

   Float64 dimVersion = 1.0;
   if ( 14 <= parent_version )
   {
      if ( pLoad->BeginUnit(_T("VoidedSlabDimensions")) )
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
         
         // if this is before dimension data block versio 2 and the
         // dimension is Jmax, the fail to read is expected
         if ( dimVersion < 2 && parent_version < 8.0 && name == _T("Jmax") )
         {
            value = 0.0; // set the default value
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

bool CVoidedSlabFactory::IsPrismatic(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   return true;
}

Float64 CVoidedSlabFactory::GetVolume(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
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

Float64 CVoidedSlabFactory::GetSurfaceArea(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bReduceForPoorlyVentilatedVoids)
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
   Float64 D = GetDimension(dimensions,_T("Void_Diameter"));
   long    N = (long)GetDimension(dimensions,_T("Number_of_Voids"));
   Float64 void_surface_area = Lg*N*M_PI*D;

   if ( bReduceForPoorlyVentilatedVoids )
      void_surface_area *= 0.50;

   Float64 surface_area = solid_slab_surface_area + void_surface_area;

   return surface_area;
}

std::_tstring CVoidedSlabFactory::GetImage()
{
   return std::_tstring(_T("VoidedSlab.jpg"));
}

std::_tstring CVoidedSlabFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      strImage = _T("VoidedSlab_Composite_CIP.gif");
      break;

   case pgsTypes::sdtCompositeSIP:
      strImage = _T("VoidedSlab_Composite_SIP.gif");
      break;

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

std::_tstring CVoidedSlabFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("+Mn_SpreadVoidedSlab_Composite.gif");
      break;

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

std::_tstring CVoidedSlabFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("-Mn_SpreadVoidedSlab_Composite.gif");
      break;

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

std::_tstring CVoidedSlabFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::_tstring strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  _T("Vn_SpreadVoidedSlab_Composite.gif");
      break;

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

std::_tstring CVoidedSlabFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
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
         strImage =  _T("SpreadVoidedSlab_Effective_Flange_Width_Interior_Girder_2008.gif");
      }
      else
      {
         strImage =  _T("SpreadVoidedSlab_Effective_Flange_Width_Interior_Girder.gif");
      }
      break;

   case pgsTypes::sdtCompositeOverlay:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
      {
         return _T("VoidedSlab_Effective_Flange_Width_Interior_Girder_2008.gif");
      }
      else
      {
         return _T("VoidedSlab_Effective_Flange_Width_Interior_Girder.gif");
      }
      break;

   case pgsTypes::sdtNone:
   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::_tstring CVoidedSlabFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
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
         strImage =  _T("SpreadVoidedSlab_Effective_Flange_Width_Exterior_Girder_2008.gif");
      }
      else
      {
         strImage =  _T("SpreadVoidedSlab_Effective_Flange_Width_EXterior_Girder.gif");
      }
      break;

   case pgsTypes::sdtCompositeOverlay:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
      {
         return _T("VoidedSlab_Effective_Flange_Width_Exterior_Girder_2008.gif");
      }
      else
      {
         return _T("VoidedSlab_Effective_Flange_Width_Exterior_Girder.gif");
      }
      break;

   case pgsTypes::sdtNone:
   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

CLSID CVoidedSlabFactory::GetCLSID()
{
   return CLSID_VoidedSlabFactory;
}

CLSID CVoidedSlabFactory::GetFamilyCLSID()
{
   return CLSID_SlabBeamFamily;
}

std::_tstring CVoidedSlabFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::_tstring( OLE2T(pszUserType) );
}

std::_tstring CVoidedSlabFactory::GetPublisher()
{
   return std::_tstring(_T("WSDOT"));
}

HINSTANCE CVoidedSlabFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CVoidedSlabFactory::GetImageResourceName()
{
   return _T("VOIDEDSLAB");
}

HICON  CVoidedSlabFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_VOIDEDSLAB) );
}

void CVoidedSlabFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                       double& H,
                                       double& W,
                                       double& D,
                                       double& S,
                                       long& N,
                                       double& J)
{
   H = GetDimension(dimensions,_T("H"));
   W = GetDimension(dimensions,_T("W"));
   D = GetDimension(dimensions,_T("Void_Diameter"));
   S = GetDimension(dimensions,_T("Void_Spacing"));
   N = (long)GetDimension(dimensions,_T("Number_of_Voids"));
   J = GetDimension(dimensions,_T("Jmax"));
}

double CVoidedSlabFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,
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

pgsTypes::SupportedDeckTypes CVoidedSlabFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
{
   pgsTypes::SupportedDeckTypes sdt;
   switch(sbs)
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

pgsTypes::SupportedBeamSpacings CVoidedSlabFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);

   return sbs;
}

void CVoidedSlabFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, double* minSpacing, double* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   double gw = GetDimension(dimensions,_T("W"));
   double J  = GetDimension(dimensions,_T("Jmax"));

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
      if (sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent)
      {
         if ( sdt == pgsTypes::sdtCompositeOverlay || sdt == pgsTypes::sdtNone )
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
}

long CVoidedSlabFactory::GetNumberOfWebs(const IBeamFactory::Dimensions& dimensions)
{
   long nv = (long)GetDimension(dimensions,_T("Number_of_Voids"));
   return nv+1;
}

Float64 CVoidedSlabFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("H"));
}

Float64 CVoidedSlabFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,_T("W"));
}

bool CVoidedSlabFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CVoidedSlabFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}