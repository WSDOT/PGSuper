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

// TxDotDoubleTFactory.cpp : Implementation of CTxDotDoubleTFactory
#include "stdafx.h"
#include <Plugins\Beams.h>
#include "BeamFamilyCLSID.h"
#include "TxDotDoubleTFactory.h"
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
#include <PgsExt\BridgeDescription.h>

#include <IFace\StatusCenter.h>
#include <PgsExt\StatusItem.h>

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
   m_DimNames.push_back("C1");
   m_DimNames.push_back("C2");
   m_DimNames.push_back("H1");
   m_DimNames.push_back("H2");
   m_DimNames.push_back("H3");
   m_DimNames.push_back("T1");
   m_DimNames.push_back("T2");
   m_DimNames.push_back("T3");
   m_DimNames.push_back("F1");
   m_DimNames.push_back("W1");
   m_DimNames.push_back("W2");
   m_DimNames.push_back("J");

   m_DefaultDims.push_back(::ConvertToSysUnits( 0.0,unitMeasure::Inch)); // C1
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.0,unitMeasure::Inch)); // C2
   m_DefaultDims.push_back(::ConvertToSysUnits(27.0,unitMeasure::Inch)); // H1
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // H2
   m_DefaultDims.push_back(::ConvertToSysUnits( 6.0,unitMeasure::Inch)); // H3
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.0,unitMeasure::Inch)); // T1
   m_DefaultDims.push_back(::ConvertToSysUnits( 6.5,unitMeasure::Inch)); // T2
   m_DefaultDims.push_back(::ConvertToSysUnits( 1.5,unitMeasure::Inch)); // T3
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // F1
   m_DefaultDims.push_back(::ConvertToSysUnits(24.0,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits(48.0,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits( 1.0,unitMeasure::Inch)); // J

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // T3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // F1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // J

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // T3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // F1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // J
   

   return S_OK;
}

void CTxDotDoubleTFactory::CreateGirderSection(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IGirderSection** ppSection)
{
   CComPtr<IMultiWebSection2> gdrsection;
   gdrsection.CoCreateInstance(CLSID_MultiWebSection2);
   CComPtr<IMultiWeb2> beam;
   gdrsection->get_Beam(&beam);

   double c1,c2;
   double h1,h2,h3;
   double w1,w2,j;
   double t1,t2,t3;
   double f1;
   GetDimensions(dimensions,h1,h2,h3,t1,t2,t3,f1,c1,c2,w1,w2,j);

   beam->put_C1(c1);
   beam->put_C2(c2);
   beam->put_H1(h1);
   beam->put_H2(h2);
   beam->put_H3(h3);
   beam->put_T1(t1);
   beam->put_T2(t2);
   beam->put_T3(t3);
   beam->put_T4(0);
   beam->put_T5(0);
   beam->put_F1(f1);
   beam->put_F2(0);
   beam->put_W1(w1);
   beam->put_W2(w2);
   beam->put_WebCount(2);

   gdrsection.QueryInterface(ppSection);
}

void CTxDotDoubleTFactory::CreateGirderProfile(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetGirderLength(spanIdx,gdrIdx);

   double c1,c2;
   double h1,h2,h3;
   double w1,w2,j;
   double t1,t2,t3;
   double f1;
   GetDimensions(dimensions,h1,h2,h3,t1,t2,t3,f1,c1,c2,w1,w2,j);

   Float64 height = h1 + h2 + h3;

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

void CTxDotDoubleTFactory::LayoutGirderLine(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* ssmbr)
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
   CreateGirderSection(pBroker,agentID,spanIdx,gdrIdx,dimensions,&gdrsection);
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

void CTxDotDoubleTFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsPoiMgr* pPoiMgr)
{
   // This is a prismatic beam so only add section change POI at the start and end of the beam
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
}

void CTxDotDoubleTFactory::CreateDistFactorEngineer(IBroker* pBroker,long agentID,const pgsTypes::SupportedDeckType* pDeckType, const pgsTypes::AdjacentTransverseConnectivity* pConnect,IDistFactorEngineer** ppEng)
{
   CComObject<CMultiWebDistFactorEngineer>* pEngineer;
   CComObject<CMultiWebDistFactorEngineer>::CreateInstance(&pEngineer);
   pEngineer->SetBroker(pBroker,agentID);
   (*ppEng) = pEngineer;
   (*ppEng)->AddRef();
}

void CTxDotDoubleTFactory::CreatePsLossEngineer(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,IPsLossEngineer** ppEng)
{
    CComObject<CPsBeamLossEngineer>* pEngineer;
    CComObject<CPsBeamLossEngineer>::CreateInstance(&pEngineer);
    pEngineer->Init(CPsLossEngineer::SingleT);
    pEngineer->SetBroker(pBroker,agentID);
    (*ppEng) = pEngineer;
    (*ppEng)->AddRef();
}

void CTxDotDoubleTFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, double endTopLimit, IBeamFactory::BeamFace endBottomFace, double endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, double hpTopLimit, IBeamFactory::BeamFace hpBottomFace, double hpBottomLimit, 
                                  double endIncrement, double hpIncrement, IStrandMover** strandMover)
{
   double h1,h2,h3;
   double c1,c2;
   double w1,w2,j;
   double t1,t2,t3;
   double f1;
   GetDimensions(dimensions,h1,h2,h3,t1,t2,t3,f1,c1,c2,w1,w2,j);

   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   double width = t2;
   double depth = h1 + h2 + h3;

   CComPtr<IRectangle> lft_harp_rect, rgt_harp_rect;
   hr = lft_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));
   hr = rgt_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   lft_harp_rect->put_Width(width);
   lft_harp_rect->put_Height(depth);
   rgt_harp_rect->put_Width(width);
   rgt_harp_rect->put_Height(depth);

   double hook_offset = w2/2.0 + t2/2.0 + t3;

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
   ATLASSERT (SUCCEEDED(hr));
}

std::vector<std::string> CTxDotDoubleTFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<double> CTxDotDoubleTFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CTxDotDoubleTFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CTxDotDoubleTFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSIUnits,std::string* strErrMsg)
{
   double h1,h2,h3;
   double c1,c2;
   double w1,w2,j;
   double t1,t2,t3;
   double f1;
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
      std::ostringstream os;
      os << "H1 must be greater than 0.0"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( h3 <= 0.0 )
   {
      std::ostringstream os;
      os << "H3 must be greater than 0.0"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( t2 <= 0.0 )
   {
      std::ostringstream os;
      os << "T2 must be greater than 0.0"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w1 <= 0.0 )
   {
      std::ostringstream os;
      os << "W1 must be greater than 0.0"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   // values that cant be negative
   if ( f1 < 0.0 )
   {
      std::ostringstream os;
      os << "F1 must be zero or greater"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( h2 < 0.0 )
   {
      std::ostringstream os;
      os << "H2 must be zero or greater"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( c1 < 0.0 )
   {
      std::ostringstream os;
      os << "C1 must be zero or greater"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( c2 < 0.0 )
   {
      std::ostringstream os;
      os << "C2 must be zero or greater"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( t1 < 0.0 )
   {
      std::ostringstream os;
      os << "T1 must be zero or greater"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( t3 < 0.0 )
   {
      std::ostringstream os;
      os << "T3 must be zero or greater"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( j < 0.0 )
   {
      std::ostringstream os;
      os << "J must be zero or greater"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   // relations
   if ( w1 < f1 )
   {
      std::ostringstream os;
      os << "W1 must be greater than F1"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( w2 < 2*f1 )
   {
      std::ostringstream os;
      os << "W2 must be greater than 2*F1"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( c1 > t2/2.0 )
   {
      std::ostringstream os;
      os << "T2 must be greater than 2 * C1"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( c2 > h3 )
   {
      std::ostringstream os;
      os << "C2 must be less than H3"<< std::ends;
      *strErrMsg = os.str();
      return false;
   }

   return true;
}

void CTxDotDoubleTFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::string>::iterator iter;
   pSave->BeginUnit("TxDOTDoubleTeeDimensions",1.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::string name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CTxDotDoubleTFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   Float64 parent_version = pLoad->GetVersion();

   IBeamFactory::Dimensions dimensions;
   std::vector<std::string>::iterator iter;

   Float64 dimVersion = 1.0;
   if ( 14 <= parent_version )
   {
      if ( pLoad->BeginUnit("TxDOTDoubleTeeDimensions") )
         dimVersion = pLoad->GetVersion();
      else
         THROW_LOAD(InvalidFileFormat,pLoad);
   }

   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::string name = *iter;
      Float64 value;
      if ( !pLoad->Property(name.c_str(),&value) )
      {
         // failed to read dimension value...
         
         if ( dimVersion < 2 && parent_version < 3.0 && (name == "C1" || name == "C2") )
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

bool CTxDotDoubleTFactory::IsPrismatic(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   return true;
}

Float64 CTxDotDoubleTFactory::GetVolume(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   Float64 area = pSectProp2->GetAg(pgsTypes::CastingYard,pgsPointOfInterest(spanIdx,gdrIdx,0.00));
   
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetGirderLength(spanIdx,gdrIdx);

   Float64 volume = area*Lg;

   return volume;
}

Float64 CTxDotDoubleTFactory::GetSurfaceArea(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bReduceForPoorlyVentilatedVoids)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   Float64 perimeter = pSectProp2->GetPerimeter(pgsPointOfInterest(spanIdx,gdrIdx,0.00));
   
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetGirderLength(spanIdx,gdrIdx);

   Float64 surface_area = perimeter*Lg;

   return surface_area;
}

std::string CTxDotDoubleTFactory::GetImage()
{
   return std::string("TxDotDoubleT.gif");
}

std::string CTxDotDoubleTFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  "DoubleTee_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "DoubleTee_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CTxDotDoubleTFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  "+Mn_DoubleTee_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "+Mn_DoubleTee_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CTxDotDoubleTFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  "-Mn_DoubleTee_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "-Mn_DoubleTee_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CTxDotDoubleTFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;

   switch(deckType)
   {
   case pgsTypes::sdtCompositeOverlay:
      strImage =  "Vn_DoubleTee_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "Vn_DoubleTee_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CTxDotDoubleTFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return "DoubleTee_Effective_Flange_Width_Interior_Girder_2008.gif";
   }
   else
   {
      return "DoubleTee_Effective_Flange_Width_Interior_Girder.gif";
   }
}

std::string CTxDotDoubleTFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );
   if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
   {
      return "DoubleTee_Effective_Flange_Width_Exterior_Girder_2008.gif";
   }
   else
   {
      return "DoubleTee_Effective_Flange_Width_Exterior_Girder.gif";
   }
}

CLSID CTxDotDoubleTFactory::GetCLSID()
{
   return CLSID_TxDotDoubleTFactory;
}

CLSID CTxDotDoubleTFactory::GetFamilyCLSID()
{
   return CLSID_DoubleTeeBeamFamily;
}

std::string CTxDotDoubleTFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::string( OLE2A(pszUserType) );
}

std::string CTxDotDoubleTFactory::GetPublisher()
{
   return std::string("WSDOT");
}

HINSTANCE CTxDotDoubleTFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CTxDotDoubleTFactory::GetImageResourceName()
{
   return _T("TxDotDoubleT");
}

HICON  CTxDotDoubleTFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_TXDOTDOUBLET) );
}

void CTxDotDoubleTFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                      double& h1,double& h2,double& h3,
                                      double& t1,double& t2,double& t3,
                                      double& f1,
                                      double& c1,double& c2,
                                      double& w1,double& w2,double& j)
{
   c1 = GetDimension(dimensions,"C1");
   c2 = GetDimension(dimensions,"C2");
   h1 = GetDimension(dimensions,"H1");
   h2 = GetDimension(dimensions,"H2");
   h3 = GetDimension(dimensions,"H3");
   t1 = GetDimension(dimensions,"T1");
   t2 = GetDimension(dimensions,"T2");
   t3 = GetDimension(dimensions,"T3");
   f1 = GetDimension(dimensions,"F1");
   w1 = GetDimension(dimensions,"W1");
   w2 = GetDimension(dimensions,"W2");
   j = GetDimension(dimensions,"J");
}

double CTxDotDoubleTFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,const std::string& name)
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

pgsTypes::SupportedDeckTypes CTxDotDoubleTFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CTxDotDoubleTFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);

   return sbs;
}

void CTxDotDoubleTFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, double* minSpacing, double* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   double T1 = GetDimension(dimensions,"T1");
   double T2 = GetDimension(dimensions,"T2");
   double T3 = GetDimension(dimensions,"T3");
   double W1 = GetDimension(dimensions,"W1");
   double W2 = GetDimension(dimensions,"W2");
   double  J = GetDimension(dimensions,"J");

   double gw_min = W2 + 2.0*(W1 + T1 + T2 + T3);

   double gw_max = gw_min + J;

   if ( sdt == pgsTypes::sdtCompositeOverlay ||  sdt == pgsTypes::sdtNone )
   {
      if(sbs == pgsTypes::sbsUniformAdjacent || sbs == pgsTypes::sbsGeneralAdjacent )
      {
         *minSpacing = gw_min;
         *maxSpacing = gw_max;
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

long CTxDotDoubleTFactory::GetNumberOfWebs(const IBeamFactory::Dimensions& dimensions)
{
   return 2;
}

Float64 CTxDotDoubleTFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   double H1 = GetDimension(dimensions,"H1");
   double H2 = GetDimension(dimensions,"H2");
   double H3 = GetDimension(dimensions,"H3");

   return H1 + H2 + H3;
}

Float64 CTxDotDoubleTFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   double T1 = GetDimension(dimensions,"T1");
   double T2 = GetDimension(dimensions,"T2");
   double T3 = GetDimension(dimensions,"T3");

   double W1 = GetDimension(dimensions,"W1");
   double W2 = GetDimension(dimensions,"W2");

   return 2*(T1+T2+T3+W1) + W2;
}
