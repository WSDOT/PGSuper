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

// DeckedSlabBeamFactory.cpp : Implementation of CDeckedSlabBeamFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include "BeamFamilyCLSID.h"
#include "DeckedSlabBeamFactory.h"
#include "IBeamDistFactorEngineer.h"
#include "MultiWebDistFactorEngineer.h"
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
// CDeckedSlabBeamFactory
HRESULT CDeckedSlabBeamFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back("A");
   m_DimNames.push_back("B");
   m_DimNames.push_back("C");
   m_DimNames.push_back("F");
   m_DimNames.push_back("W");
   m_DimNames.push_back("Tt");
   m_DimNames.push_back("Tb");
   m_DimNames.push_back("Jmax");
   m_DimNames.push_back("EndBlockLength");

   m_DefaultDims.push_back(::ConvertToSysUnits(77.75,unitMeasure::Inch)); // A
   m_DefaultDims.push_back(::ConvertToSysUnits(9.0,unitMeasure::Inch));   // B
   m_DefaultDims.push_back(::ConvertToSysUnits(12.0,unitMeasure::Inch));  // C
   m_DefaultDims.push_back(::ConvertToSysUnits(1.75,unitMeasure::Inch));  // F
   m_DefaultDims.push_back(::ConvertToSysUnits(6.0,unitMeasure::Inch));   // W
   m_DefaultDims.push_back(::ConvertToSysUnits(8.0,unitMeasure::Inch));   // Tt
   m_DefaultDims.push_back(::ConvertToSysUnits(7.0,unitMeasure::Inch));   // Tb
   m_DefaultDims.push_back(::ConvertToSysUnits(1.0,unitMeasure::Inch));   // Max Joint Spacing
   m_DefaultDims.push_back(::ConvertToSysUnits(36.0,unitMeasure::Inch));  // End Block Length

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // A 
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // B
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // F
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Tt
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Tb
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Max joint size
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // End Block Length

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // A 
   m_DimUnits[1].push_back(&unitMeasure::Inch); // B
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C
   m_DimUnits[1].push_back(&unitMeasure::Inch); // F
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Tt
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Tb
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Max joint size
   m_DimUnits[1].push_back(&unitMeasure::Inch); // End Block Length

   return S_OK;
}

void CDeckedSlabBeamFactory::CreateGirderSection(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IGirderSection** ppSection)
{
   CComPtr<IDeckedSlabBeamSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_DeckedSlabBeamSection);
   CComPtr<IDeckedSlabBeam> beam;
   gdrsection->get_Beam(&beam);

   double A,B,C,F,W,Tt,Tb,J,EndBlockLength;
   GetDimensions(dimensions,A,B,C,F,W,Tt,Tb,J,EndBlockLength);

   beam->put_A(A);
   beam->put_B(B);
   beam->put_C(C);
   beam->put_F(F);
   beam->put_W(W);
   beam->put_Tt(Tt);
   beam->put_Tb(Tb);

   gdrsection.QueryInterface(ppSection);
}

void CDeckedSlabBeamFactory::CreateGirderProfile(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetGirderLength(spanIdx,gdrIdx);

   Float64 height = GetDimension(dimensions,"C") + GetDimension(dimensions,"Tt");

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

void CDeckedSlabBeamFactory::LayoutGirderLine(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* ssmbr)
{
   CComPtr<IDeckedSlabBeamEndBlockSegment> segment;
   segment.CoCreateInstance(CLSID_DeckedSlabBeamEndBlockSegment);

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

   Float64 endBlockLength = GetDimension(dimensions,"EndBlockLength");
   segment->put_EndBlockLength(etStart,endBlockLength);
   segment->put_EndBlockLength(etEnd,endBlockLength);

   CComPtr<IGirderSection> gdrsection;
   CreateGirderSection(pBroker,statusGroupID,spanIdx,gdrIdx,dimensions,&gdrsection);
   CComQIPtr<IDeckedSlabBeamSection> section(gdrsection);

   // if this is an exterior girder, remove the shear key block outs
   CComPtr<IDeckedSlabBeam> deckedSlabShape;
   section->get_Beam(&deckedSlabShape);
   if ( gdrIdx == 0 )
   {
      deckedSlabShape->put_LeftBlockOut(VARIANT_FALSE);
   }

   if ( gdrIdx == pIBridgeDesc->GetSpan(spanIdx)->GetGirderCount()-1 )
   {
      deckedSlabShape->put_RightBlockOut(VARIANT_FALSE);
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

void CDeckedSlabBeamFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsPoiMgr* pPoiMgr)
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

   // put section breaks just on either side of the end blocks/void interface
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const CSpanData* pSpan = pBridgeDesc->GetSpan(span);
   const GirderLibraryEntry* pGirderLib = pSpan->GetGirderTypes()->GetGirderLibraryEntry(gdr);
   Float64 endBlockLength = pGirderLib->GetDimension("EndBlockLength");

   if ( !IsZero(endBlockLength) )
   {
      Float64 delta = 1.5*pPoiMgr->GetTolerance();

      stages.insert(pgsTypes::CastingYard);

      std::set<pgsTypes::Stage> ebStages;
      if ( start_length < endBlockLength )
         ebStages.insert(pgsTypes::CastingYard); // end block is after brg... only add to cy stage
      else
         ebStages = stages; // all stages

      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,endBlockLength,       POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,endBlockLength+delta,                  POI_TABULAR | POI_GRAPHICAL) );

      if ( end_length < endBlockLength )
         ebStages.insert(pgsTypes::CastingYard);
      else
         ebStages = stages;

      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,gdrLength - (endBlockLength+delta),                  POI_TABULAR | POI_GRAPHICAL) );
      pPoiMgr->AddPointOfInterest( pgsPointOfInterest(stages,span,gdr,gdrLength - endBlockLength,         POI_SECTCHANGE | POI_TABULAR | POI_GRAPHICAL) );
   }
}

void CDeckedSlabBeamFactory::CreateDistFactorEngineer(IBroker* pBroker,long statusGroupID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   CComObject<CMultiWebDistFactorEngineer>* pEngineer;
   CComObject<CMultiWebDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,statusGroupID);

   pEngineer->SetBeamType(CMultiWebDistFactorEngineer::btDeckedSlabBeam);

   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CDeckedSlabBeamFactory::CreatePsLossEngineer(IBroker* pBroker,long statusGroupID,SpanIndexType spanIdx,GirderIndexType gdrIdx,IPsLossEngineer** ppEng)
{
   CComObject<CPsBeamLossEngineer>* pEngineer;
   CComObject<CPsBeamLossEngineer>::CreateInstance(&pEngineer);
    
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

void CDeckedSlabBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, double endTopLimit, IBeamFactory::BeamFace endBottomFace, double endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, double hpTopLimit, IBeamFactory::BeamFace hpBottomFace, double hpBottomLimit, 
                                  double endIncrement, double hpIncrement, IStrandMover** strandMover)
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   // set the shapes for harped strand bounds - only in the thinest part of the webs
   double A,B,C,F,W,Tt,Tb,J,EndBlockLength;
   GetDimensions(dimensions,A,B,C,F,W,Tt,Tb,J,EndBlockLength);

   double width = W;
   double depth = C + Tt;
   double bf_wid = A - 2*B;

   CComPtr<IRectangle> lft_harp_rect, rgt_harp_rect;
   hr = lft_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));
   hr = rgt_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   lft_harp_rect->put_Width(width);
   lft_harp_rect->put_Height(depth);
   rgt_harp_rect->put_Width(width);
   rgt_harp_rect->put_Height(depth);

   double hook_offset = bf_wid/2.0 - W/2.0;

   CComPtr<IPoint2d> lft_hook, rgt_hook;
   lft_hook.CoCreateInstance(CLSID_Point2d);
   rgt_hook.CoCreateInstance(CLSID_Point2d);

   lft_hook->Move(-hook_offset, depth/2.0);
   rgt_hook->Move( hook_offset, depth/2.0);

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
   double hptb = hpTopFace==IBeamFactory::BeamBottom ? hpTopLimit : depth-hpTopLimit;
   double hpbb = hpBottomFace==IBeamFactory::BeamBottom ? hpBottomLimit : depth-hpBottomLimit;
   double endtb = endTopFace==IBeamFactory::BeamBottom ? endTopLimit : depth-endTopLimit;
   double endbb = endBottomFace==IBeamFactory::BeamBottom ? endBottomLimit : depth-endBottomLimit;

   hr = configurer->SetHarpedStrandOffsetBounds(depth, hptb, hpbb, endtb, endbb, endIncrement, hpIncrement);
   ATLASSERT (SUCCEEDED(hr));

   hr = sm.CopyTo(strandMover);
   ATLASSERT (SUCCEEDED(hr));}

std::vector<std::string> CDeckedSlabBeamFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<double> CDeckedSlabBeamFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CDeckedSlabBeamFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CDeckedSlabBeamFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSI,std::string* strErrMsg)
{
   double A,B,C,F,W,Tt,Tb,J,EndBlockLength;
   GetDimensions(dimensions,A,B,C,F,W,Tt,Tb,J,EndBlockLength);

   if ( B <= 0.0 )
   {
      std::ostringstream os;
      os << "B must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W <= 0.0 )
   {
      std::ostringstream os;
      os << "W must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( A < 2*(B+W) )
   {
      std::ostringstream os;
      os << "A must be greater or equal to 2*(B+W)" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( C <= 0.0 )
   {
      std::ostringstream os;
      os << "C must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( Tt <= 0.0 )
   {
      std::ostringstream os;
      os << "Tt must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( C < Tb )
   {
      std::ostringstream os;
      os << "C must be a greater or equal to Tb" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F < 0.0 )
   {
      std::ostringstream os;
      os << "F must be zero or greater" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( J < 0.0 )
   {
      std::ostringstream os;
      os << "Maximum joint size must be zero or greater" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   return true;
}

void CDeckedSlabBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::string>::iterator iter;
   pSave->BeginUnit("DeckedSlabBeamDimensions",2.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::string name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CDeckedSlabBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version = pLoad->GetVersion();

   IBeamFactory::Dimensions dimensions;
   std::vector<std::string>::iterator iter;

   Float64 dimVersion = 1.0;
   if ( 14 <= parent_version )
   {
      if ( pLoad->BeginUnit("DeckedSlabBeamDimensions") )
         dimVersion = pLoad->GetVersion();
      else
         THROW_LOAD(InvalidFileFormat,pLoad);
   }

   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::string name = *iter;
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
         THROW_LOAD(InvalidFileFormat,pLoad);

      dimensions.push_back( std::make_pair(name,value) );
   }

   if ( 14 <= parent_version && !pLoad->EndUnit() )
      THROW_LOAD(InvalidFileFormat,pLoad);

   return dimensions;
}

bool CDeckedSlabBeamFactory::IsPrismatic(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 endBlockLength = GetDimension(dimensions,"EndBlockLength");

   return IsZero(endBlockLength) ? true : false;
}

Float64 CDeckedSlabBeamFactory::GetVolume(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
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

Float64 CDeckedSlabBeamFactory::GetSurfaceArea(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bReduceForPoorlyVentilatedVoids)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   Float64 perimeter = pSectProp2->GetPerimeter(pgsPointOfInterest(spanIdx,gdrIdx,0.00));
   
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetGirderLength(spanIdx,gdrIdx);

   Float64 decked_slab_surface_area = perimeter*Lg;

   // void perimeter
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 A = GetDimension(dimensions,"A");
   Float64 B = GetDimension(dimensions,"B");
   Float64 C = GetDimension(dimensions,"C");
   Float64 W = GetDimension(dimensions,"W");
   Float64 Tb = GetDimension(dimensions,"Tb");
   Float64 endBlockLength = GetDimension(dimensions,"EndBlockLength");

   Float64 void_perimeter = (A-2*(B+W)) * (C-Tb);

   Float64 void_surface_area = (Lg-2*endBlockLength)*void_perimeter;

   if ( bReduceForPoorlyVentilatedVoids )
      void_surface_area *= 0.50;

   Float64 surface_area = decked_slab_surface_area + void_surface_area;

   return surface_area;
}

std::string CDeckedSlabBeamFactory::GetImage()
{
   return std::string("DeckedSlabBeam.gif");
}

std::string CDeckedSlabBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage = "DeckedSlabBeam_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage = "DeckedSlabBeam_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CDeckedSlabBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  "+Mn_DeckedSlabBeam_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "+Mn_DeckedSlabBeam_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CDeckedSlabBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  "-Mn_DeckedSlabBeam_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "-Mn_DeckedSlabBeam_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CDeckedSlabBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  "Vn_DeckedSlabBeam_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "Vn_DeckedSlabBeam_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CDeckedSlabBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return "DeckedSlabBeam_Effective_Flange_Width_Interior_Girder_2008.gif";
   }
   else
   {
      return "DeckedSlabBeam_Effective_Flange_Width_Interior_Girder.gif";
   }
}

std::string CDeckedSlabBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return "DeckedSlabBeam_Effective_Flange_Width_Exterior_Girder_2008.gif";
   }
   else
   {
      return "DeckedSlabBeam_Effective_Flange_Width_Exterior_Girder.gif";
   }
}

CLSID CDeckedSlabBeamFactory::GetCLSID()
{
   return CLSID_DeckedSlabBeamFactory;
}

CLSID CDeckedSlabBeamFactory::GetFamilyCLSID()
{
   return CLSID_DeckedSlabBeamFamily;
}

std::string CDeckedSlabBeamFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::string( OLE2A(pszUserType) );
}

std::string CDeckedSlabBeamFactory::GetPublisher()
{
   return std::string("TxDOT");
}

HINSTANCE CDeckedSlabBeamFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CDeckedSlabBeamFactory::GetImageResourceName()
{
   return _T("DECKEDSLABBEAM");
}

HICON  CDeckedSlabBeamFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_DECKEDSLAB) );
}

void CDeckedSlabBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                      double& A,double& B,double& C,double& F,double& W,double& Tt,double& Tb,double& Jmax,double& EndBlockLength)
{
   A = GetDimension(dimensions,"A");
   B = GetDimension(dimensions,"B");
   C = GetDimension(dimensions,"C");
   W = GetDimension(dimensions,"W");
   Tt = GetDimension(dimensions,"Tt");
   Tb = GetDimension(dimensions,"Tb");
   F  = GetDimension(dimensions,"F");
   Jmax = GetDimension(dimensions,"Jmax");
   EndBlockLength = GetDimension(dimensions,"EndBlockLength");
}

double CDeckedSlabBeamFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,
                                        const std::string& name)
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

pgsTypes::SupportedDeckTypes CDeckedSlabBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CDeckedSlabBeamFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);

   return sbs;
}

void CDeckedSlabBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, double* minSpacing, double* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   double A = GetDimension(dimensions,"A");
   double J  = GetDimension(dimensions,"Jmax");

   if ( sdt == pgsTypes::sdtCompositeOverlay || sdt == pgsTypes::sdtNone )
   {
      if(sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent)
      {
         *minSpacing = A;
         *maxSpacing = A+J;
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

long CDeckedSlabBeamFactory::GetNumberOfWebs(const IBeamFactory::Dimensions& dimensions)
{
   return 2;
}

Float64 CDeckedSlabBeamFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,"C") + GetDimension(dimensions,"Tt");
}

Float64 CDeckedSlabBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   return GetDimension(dimensions,"A");
}

bool CDeckedSlabBeamFactory::IsShearKey(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType)
{
   return false;
}

void CDeckedSlabBeamFactory::GetShearKeyAreas(const IBeamFactory::Dimensions& dimensions, pgsTypes::SupportedBeamSpacing spacingType,Float64* uniformArea, Float64* areaPerJoint)
{
   *uniformArea = 0.0;
   *areaPerJoint = 0.0;
}
