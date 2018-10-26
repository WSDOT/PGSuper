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

// BoxBeamFactory.cpp : Implementation of CBoxBeamFactory
#include "stdafx.h"
#include <Plugins\Beams.h>

#include "BeamFamilyCLSID.h"

#include "BoxBeamFactory.h"
#include "IBeamDistFactorEngineer.h"
#include "UBeamDistFactorEngineer.h"
#include "BoxBeamDistFactorEngineer.h"
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
// CBoxBeamFactory
HRESULT CBoxBeamFactory::FinalConstruct()
{
   // Initialize with default values... This are not necessarily valid dimensions
   m_DimNames.push_back("H1");
   m_DimNames.push_back("H2");
   m_DimNames.push_back("H3");
   m_DimNames.push_back("H4");
   m_DimNames.push_back("H5");
   m_DimNames.push_back("H6");
   m_DimNames.push_back("H7");
   m_DimNames.push_back("W1");
   m_DimNames.push_back("W2");
   m_DimNames.push_back("W3");
   m_DimNames.push_back("W4");
   m_DimNames.push_back("F1");
   m_DimNames.push_back("F2");
   m_DimNames.push_back("C1");
   m_DimNames.push_back("Jmax");

   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // H1
   m_DefaultDims.push_back(::ConvertToSysUnits(29.5,unitMeasure::Inch)); // H2
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // H3
   m_DefaultDims.push_back(::ConvertToSysUnits( 4.0,unitMeasure::Inch)); // H4
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // H5
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // H6
   m_DefaultDims.push_back(::ConvertToSysUnits(17.0,unitMeasure::Inch)); // H7
   m_DefaultDims.push_back(::ConvertToSysUnits( 3.0,unitMeasure::Inch)); // W1
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // W2
   m_DefaultDims.push_back(::ConvertToSysUnits(27.75,unitMeasure::Inch));// W3
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // W4
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // F1
   m_DefaultDims.push_back(::ConvertToSysUnits( 5.0,unitMeasure::Inch)); // F2
   m_DefaultDims.push_back(::ConvertToSysUnits( 0.75,unitMeasure::Inch)); // C1
   m_DefaultDims.push_back(::ConvertToSysUnits( 1.0,unitMeasure::Inch)); // Jmax

   // SI Units
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H5
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H6
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // H7
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W3
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // W4
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // F1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // F2
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // C1
   m_DimUnits[0].push_back(&unitMeasure::Millimeter); // Jmax

   // US Units
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H5
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H6
   m_DimUnits[1].push_back(&unitMeasure::Inch); // H7
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W3
   m_DimUnits[1].push_back(&unitMeasure::Inch); // W4
   m_DimUnits[1].push_back(&unitMeasure::Inch); // F1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // F2
   m_DimUnits[1].push_back(&unitMeasure::Inch); // C1
   m_DimUnits[1].push_back(&unitMeasure::Inch); // Jmax

   return S_OK;
}

void CBoxBeamFactory::CreateGirderSection(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IGirderSection** ppSection)
{
   CComPtr<IBoxBeamSection> gdrsection;
   gdrsection.CoCreateInstance(CLSID_BoxBeamSection);
   CComPtr<IBoxBeam> beam;
   gdrsection->get_Beam(&beam);

   double H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J;
   GetDimensions(dimensions,H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J);

   beam->put_H1(H1);
   beam->put_H2(H2);
   beam->put_H3(H3);
   beam->put_H4(H4);
   beam->put_H5(H5);
   beam->put_H6(H6);
   beam->put_H7(H7);
   beam->put_W1(W1);
   beam->put_W2(W2);
   beam->put_W3(W3);
   beam->put_W4(W4);
   beam->put_F1(F1);
   beam->put_F2(F2);
   beam->put_C1(C1);

   gdrsection.QueryInterface(ppSection);
}

void CBoxBeamFactory::CreateGirderProfile(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,const IBeamFactory::Dimensions& dimensions,IShape** ppShape)
{
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetGirderLength(spanIdx,gdrIdx);

   double H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J;
   GetDimensions(dimensions,H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J);

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

void CBoxBeamFactory::LayoutGirderLine(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,ISuperstructureMember* ssmbr)
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

void CBoxBeamFactory::LayoutSectionChangePointsOfInterest(IBroker* pBroker,SpanIndexType span,GirderIndexType gdr,pgsPoiMgr* pPoiMgr)
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
   poiEnd.RemoveStage(pgsTypes::CastingYard);

   poiStart.AddStages(stages);
   poiEnd.AddStages(stages);
   
   pPoiMgr->AddPointOfInterest(poiStart);
   pPoiMgr->AddPointOfInterest(poiEnd);
}

void CBoxBeamFactory::CreateDistFactorEngineer(IBroker* pBroker,long agentID,
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
      pEngineer->SetBroker(pBroker,agentID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
   else
   {
      // this is a type b section... type b's are the same as type c's which are U-beams
      ATLASSERT( deckType == pgsTypes::sdtCompositeCIP || deckType == pgsTypes::sdtCompositeSIP );

      CComObject<CUBeamDistFactorEngineer>* pEngineer;
      CComObject<CUBeamDistFactorEngineer>::CreateInstance(&pEngineer);
      pEngineer->Init(true); // this is a type b cross section
      pEngineer->SetBroker(pBroker,agentID);
      (*ppEng) = pEngineer;
      (*ppEng)->AddRef();
   }
}

void CBoxBeamFactory::CreatePsLossEngineer(IBroker* pBroker,long agentID,SpanIndexType spanIdx,GirderIndexType gdrIdx,IPsLossEngineer** ppEng)
{
    CComObject<CPsBeamLossEngineer>* pEngineer;
    CComObject<CPsBeamLossEngineer>::CreateInstance(&pEngineer);
    pEngineer->Init(CPsLossEngineer::BoxBeam);
    pEngineer->SetBroker(pBroker,agentID);
    (*ppEng) = pEngineer;
    (*ppEng)->AddRef();
}

void CBoxBeamFactory::CreateStrandMover(const IBeamFactory::Dimensions& dimensions, 
                                  IBeamFactory::BeamFace endTopFace, double endTopLimit, IBeamFactory::BeamFace endBottomFace, double endBottomLimit, 
                                  IBeamFactory::BeamFace hpTopFace, double hpTopLimit, IBeamFactory::BeamFace hpBottomFace, double hpBottomLimit, 
                                  double endIncrement, double hpIncrement, IStrandMover** strandMover)
{
   HRESULT hr = S_OK;

   CComObject<CStrandMoverImpl>* pStrandMover;
   CComObject<CStrandMoverImpl>::CreateInstance(&pStrandMover);

   CComPtr<IStrandMover> sm = pStrandMover;

   // set the shapes for harped strand bounds - only in the thinest part of the webs
   double H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J;
   GetDimensions(dimensions,H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J);

   double width = W2;
   double depth = H1+H2+H3;

   CComPtr<IRectangle> lft_harp_rect, rgt_harp_rect;
   hr = lft_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));
   hr = rgt_harp_rect.CoCreateInstance(CLSID_Rect);
   ATLASSERT (SUCCEEDED(hr));

   lft_harp_rect->put_Width(width);
   lft_harp_rect->put_Height(depth);
   rgt_harp_rect->put_Width(width);
   rgt_harp_rect->put_Height(depth);

   double hook_offset = (W3 + width)/2.0;

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

std::vector<std::string> CBoxBeamFactory::GetDimensionNames()
{
   return m_DimNames;
}

std::vector<double> CBoxBeamFactory::GetDefaultDimensions()
{
   return m_DefaultDims;
}

std::vector<const unitLength*> CBoxBeamFactory::GetDimensionUnits(bool bSIUnits)
{
   return m_DimUnits[ bSIUnits ? 0 : 1 ];
}

bool CBoxBeamFactory::ValidateDimensions(const IBeamFactory::Dimensions& dimensions,bool bSI,std::string* strErrMsg)
{
   double H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J;
   GetDimensions(dimensions,H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J);

   if ( H1 <= 0.0 )
   {
      std::ostringstream os;
      os << "H1 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H2 < 0.0 )
   {
      std::ostringstream os;
      os << "H2 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H3 < 0.0 )
   {
      std::ostringstream os;
      os << "H3 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H4 < 0.0 )
   {
      std::ostringstream os;
      os << "H4 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H5 < 0.0 )
   {
      std::ostringstream os;
      os << "H5 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H6 <= 0.0 )
   {
      std::ostringstream os;
      os << "H6 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H7 <= 0.0 )
   {
      std::ostringstream os;
      os << "H7 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W1 <= 0.0 )
   {
      std::ostringstream os;
      os << "W1 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W2 <= 0.0 )
   {
      std::ostringstream os;
      os << "W2 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W3 < 0.0 )
   {
      std::ostringstream os;
      os << "W3 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( W4 < 0.0 )
   {
      std::ostringstream os;
      os << "W4 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F1 < 0.0 )
   {
      std::ostringstream os;
      os << "F1 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F2 < 0.0 )
   {
      std::ostringstream os;
      os << "F2 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( C1 < 0.0 )
   {
      std::ostringstream os;
      os << "C1 must be a positive value" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( C1 >= H7 )
   {
      std::ostringstream os;
      os << "C1 must be less than H7" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( H1+H2+H3+TOLERANCE <= H4+H5+H6+H7 )
   {
      std::ostringstream os;
      os << "H1+H2+H3 must be greater than or equal to H4+H5+H6+H7" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F1 > W3/2 )
   {
      std::ostringstream os;
      os << "F1 must be less than W3/2" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F1 > H2/2 )
   {
      std::ostringstream os;
      os << "F1 must be less than H2/2" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F2 > W3/2 )
   {
      std::ostringstream os;
      os << "F2 must be less than W3/2" << std::ends;
      *strErrMsg = os.str();
      return false;
   }

   if ( F2 > H2/2 )
   {
      std::ostringstream os;
      os << "F2 must be less than H2/2" << std::ends;
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

void CBoxBeamFactory::SaveSectionDimensions(sysIStructuredSave* pSave,const IBeamFactory::Dimensions& dimensions)
{
   std::vector<std::string>::iterator iter;
   pSave->BeginUnit("BoxBeamDimensions",2.0);
   for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
   {
      std::string name = *iter;
      Float64 value = GetDimension(dimensions,name);
      pSave->Property(name.c_str(),value);
   }
   pSave->EndUnit();
}

IBeamFactory::Dimensions CBoxBeamFactory::LoadSectionDimensions(sysIStructuredLoad* pLoad)
{
   IBeamFactory::Dimensions dimensions;
   Float64 parent_version = pLoad->GetVersion();

   // In Feb,2008 we created version 10.0 which updated the flawed dimensioning for box beams. This update completely
   // rearranged all box dimensions. 
   if ( parent_version < 10.0 )
   {
      // ordering of old dimensions
      const long NDMS=17;
      //                            0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16
      const char* box_dims[NDMS]={"H1","H2","H3","H4","H5","H6","H7","H8","W1","W2","W3","W4","W5","F1","F2","C1","Jmax"};
      double dim_vals[NDMS];
      for (long id=0; id<NDMS; id++)
      {
         if ( !pLoad->Property(box_dims[id],&(dim_vals[id])) )
         {
            std::string name(box_dims[id]);
            if ( parent_version < 9.0 && name == "Jmax")
            {
               dim_vals[id] = 0.0;
            }
            else if ( parent_version < 3.0 && (name == "C1" || name == "C2" ) )
            {
               dim_vals[id] = 0.0;
            }
            else
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }
      } // end for loop

      // check that redundant dimensions matched up in old section
      // H1 == H6+H7+H8
      ATLASSERT( ::IsEqual(dim_vals[0], dim_vals[5]+dim_vals[6]+dim_vals[7]) );
      // W1-2*W4 == W2-2*W3
      ATLASSERT( ::IsEqual(dim_vals[8]-2*dim_vals[11], dim_vals[9]-2*dim_vals[10]) );

      // Now we have values in old format, convert to new. 
      // Refer to BoxBeam.vsd visio file in documentation for mapping
      double H1, H2, H3, H4, H5, H6, H7, W1, W2, W3, W4, F1, F2, C1, J;

      // H1 = H8
      H1 = dim_vals[7];

      // H2 = H7
      H2 = dim_vals[6];

      // H3 = H6
      H3 = dim_vals[5];

      // H4 = H5
      H4 = dim_vals[4];

      // H5 = H4
      H5 = dim_vals[3];

      // H6 = H3
      H6 = dim_vals[2];

      // H7 = H2
      H7 = dim_vals[1];

      // W1 = W4
      W1 = dim_vals[11];

      // W2 = (W1-2*W4-W5)/2
      W2 = (dim_vals[8] - 2*dim_vals[11] - dim_vals[12])/2.0;

      // W3 = W5
      W3 = dim_vals[12];

      // W4 = W3
      W4 = dim_vals[10];

      // the rest map 1-1
      F1 = dim_vals[13];
      F2 = dim_vals[14];
      C1 = dim_vals[15];
       J = dim_vals[16];

      // now we can set our local dimensions
      dimensions.push_back(Dimension("H1",H1));
      dimensions.push_back(Dimension("H2",H2));
      dimensions.push_back(Dimension("H3",H3));
      dimensions.push_back(Dimension("H4",H4));
      dimensions.push_back(Dimension("H5",H5));
      dimensions.push_back(Dimension("H6",H6));
      dimensions.push_back(Dimension("H7",H7));
      dimensions.push_back(Dimension("W1",W1));
      dimensions.push_back(Dimension("W2",W2));
      dimensions.push_back(Dimension("W3",W3));
      dimensions.push_back(Dimension("W4",W4));
      dimensions.push_back(Dimension("F1",F1));
      dimensions.push_back(Dimension("F2",F2));
      dimensions.push_back(Dimension("C1",C1));
      dimensions.push_back(Dimension("Jmax",J));
   }
   else
   {
      Float64 dimVersion = 1.0;
      if ( 14 <= parent_version )
      {
         if ( pLoad->BeginUnit("BoxBeamDimensions") )
            dimVersion = pLoad->GetVersion();
         else
            THROW_LOAD(InvalidFileFormat,pLoad);
      }

      std::vector<std::string>::iterator iter;
      for ( iter = m_DimNames.begin(); iter != m_DimNames.end(); iter++ )
      {
         std::string name = *iter;
         Float64 value;
         if ( !pLoad->Property(name.c_str(),&value) )
         {
            // failed to read dimension value...
            
            // if this is before dimension data block versio 2 and the
            // dimension is Jmax, the fail to read is expected
            if ( dimVersion < 2 && parent_version < 9.0 && name == "Jmax" )
            {
               value = 0.0; // set the default value
            }
            else if ( dimVersion < 2 && parent_version < 3.0 && name == "C1" )
            {
               value = 0.0; // set the default value
            }
            else
            {
               THROW_LOAD(InvalidFileFormat,pLoad);
            }
         }

         dimensions.push_back( Dimension(name,value) );
      }

      if ( 14 <= parent_version && !pLoad->EndUnit() )
         THROW_LOAD(InvalidFileFormat,pLoad);
   }
   return dimensions;
}

bool CBoxBeamFactory::IsPrismatic(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   return true;
}

Float64 CBoxBeamFactory::GetVolume(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   Float64 area = pSectProp2->GetAg(pgsTypes::CastingYard,pgsPointOfInterest(spanIdx,gdrIdx,0.00));
   
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetGirderLength(spanIdx,gdrIdx);

   Float64 volume = area*Lg;
   return volume;
}

Float64 CBoxBeamFactory::GetSurfaceArea(IBroker* pBroker,SpanIndexType spanIdx,GirderIndexType gdrIdx,bool bReduceForPoorlyVentilatedVoids)
{
   GET_IFACE2(pBroker,ISectProp2,pSectProp2);
   Float64 perimeter = pSectProp2->GetPerimeter(pgsPointOfInterest(spanIdx,gdrIdx,0.00));
   
   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 Lg = pBridge->GetGirderLength(spanIdx,gdrIdx);

   Float64 solid_box_surface_area = perimeter*Lg;

   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   const GirderLibraryEntry* pGdrEntry = pBridgeDesc->GetSpan(spanIdx)->GetGirderTypes()->GetGirderLibraryEntry(gdrIdx);
   const GirderLibraryEntry::Dimensions& dimensions = pGdrEntry->GetDimensions();
   Float64 W3 = GetDimension(dimensions,"W3");
   Float64 H2 = GetDimension(dimensions,"H2");
   Float64 F1 = GetDimension(dimensions,"F1");

   Float64 void_surface_area = Lg*( 2*(H2 - 2*F1) + 2*(W3 - 2*F1) + 4*sqrt(2*F1*F1) );

   if ( bReduceForPoorlyVentilatedVoids )
      void_surface_area *= 0.50;

   Float64 surface_area = solid_box_surface_area + void_surface_area;

   return surface_area;
}

std::string CBoxBeamFactory::GetImage()
{
   return std::string("BoxBeam.jpg");
}

std::string CBoxBeamFactory::GetSlabDimensionsImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
      strImage = "BoxBeam_Composite_CIP.gif";
      break;

   case pgsTypes::sdtCompositeSIP:
      strImage = "BoxBeam_Composite_SIP.gif";
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage = "BoxBeam_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage = "BoxBeam_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CBoxBeamFactory::GetPositiveMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  "+Mn_SpreadBoxBeam_Composite.gif";
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage =  "+Mn_BoxBeam_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "+Mn_BoxBeam_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CBoxBeamFactory::GetNegativeMomentCapacitySchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  "-Mn_SpreadBoxBeam_Composite.gif";
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage =  "-Mn_BoxBeam_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "-Mn_BoxBeam_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CBoxBeamFactory::GetShearDimensionsSchematicImage(pgsTypes::SupportedDeckType deckType)
{
   std::string strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      strImage =  "Vn_SpreadBoxBeam_Composite.gif";
      break;

   case pgsTypes::sdtCompositeOverlay:
      strImage =  "Vn_BoxBeam_Composite.gif";
      break;

   case pgsTypes::sdtNone:
      strImage =  "Vn_BoxBeam_Noncomposite.gif";
      break;

   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CBoxBeamFactory::GetInteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   std::string strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
      {
         strImage =  "SpreadBoxBeam_Effective_Flange_Width_Interior_Girder_2008.gif";
      }
      else
      {
         strImage =  "SpreadBoxBeam_Effective_Flange_Width_Interior_Girder.gif";
      }
      break;

   case pgsTypes::sdtCompositeOverlay:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
      {
         strImage =  "BoxBeam_Effective_Flange_Width_Interior_Girder_2008.gif";
      }
      else
      {
         strImage =  "BoxBeam_Effective_Flange_Width_Interior_Girder.gif";
      }
      break;

   case pgsTypes::sdtNone:
   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

std::string CBoxBeamFactory::GetExteriorGirderEffectiveFlangeWidthImage(IBroker* pBroker,pgsTypes::SupportedDeckType deckType)
{
   GET_IFACE2(pBroker, ILibrary,       pLib);
   GET_IFACE2(pBroker, ISpecification, pSpec);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( pSpec->GetSpecification().c_str() );

   std::string strImage;
   switch(deckType)
   {
   case pgsTypes::sdtCompositeCIP:
   case pgsTypes::sdtCompositeSIP:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
      {
         strImage =  "SpreadBoxBeam_Effective_Flange_Width_Exterior_Girder_2008.gif";
      }
      else
      {
         strImage =  "SpreadBoxBeam_Effective_Flange_Width_Exterior_Girder.gif";
      }
      break;

   case pgsTypes::sdtCompositeOverlay:
      if ( pSpecEntry->GetEffectiveFlangeWidthMethod() == pgsTypes::efwmTribWidth || lrfdVersionMgr::FourthEditionWith2008Interims <= pSpecEntry->GetSpecificationType() )
      {
         strImage =  "BoxBeam_Effective_Flange_Width_Exterior_Girder_2008.gif";
      }
      else
      {
         strImage =  "BoxBeam_Effective_Flange_Width_Exterior_Girder.gif";
      }
      break;

   case pgsTypes::sdtNone:
   default:
      ATLASSERT(false); // shouldn't get here
      break;
   };

   return strImage;
}

CLSID CBoxBeamFactory::GetCLSID()
{
   return CLSID_BoxBeamFactory;
}

CLSID CBoxBeamFactory::GetFamilyCLSID()
{
   return CLSID_BoxBeamFamily;
}

std::string CBoxBeamFactory::GetGirderFamilyName()
{
   USES_CONVERSION;
   LPOLESTR pszUserType;
   OleRegGetUserType(GetFamilyCLSID(),USERCLASSTYPE_SHORT,&pszUserType);
   return std::string( OLE2A(pszUserType) );
}

std::string CBoxBeamFactory::GetPublisher()
{
   return std::string("WSDOT");
}

HINSTANCE CBoxBeamFactory::GetResourceInstance()
{
   return _Module.GetResourceInstance();
}

LPCTSTR CBoxBeamFactory::GetImageResourceName()
{
   return _T("BoxBeam");
}

HICON  CBoxBeamFactory::GetIcon() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   return ::LoadIcon(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDI_BOXBEAM) );
}

void CBoxBeamFactory::GetDimensions(const IBeamFactory::Dimensions& dimensions,
                                    double& H1, 
                                    double& H2, 
                                    double& H3, 
                                    double& H4, 
                                    double& H5,
                                    double& H6, 
                                    double& H7, 
                                    double& W1, 
                                    double& W2, 
                                    double& W3, 
                                    double& W4, 
                                    double& F1, 
                                    double& F2, 
                                    double& C1,
                                    double& J)
{
   H1 = GetDimension(dimensions,"H1");
   H2 = GetDimension(dimensions,"H2");
   H3 = GetDimension(dimensions,"H3");
   H4 = GetDimension(dimensions,"H4");
   H5 = GetDimension(dimensions,"H5");
   H6 = GetDimension(dimensions,"H6");
   H7 = GetDimension(dimensions,"H7");
   W1 = GetDimension(dimensions,"W1");
   W2 = GetDimension(dimensions,"W2");
   W3 = GetDimension(dimensions,"W3");
   W4 = GetDimension(dimensions,"W4");
   F1 = GetDimension(dimensions,"F1");
   F2 = GetDimension(dimensions,"F2");
   C1 = GetDimension(dimensions,"C1");
   J  = GetDimension(dimensions,"Jmax");
}

double CBoxBeamFactory::GetDimension(const IBeamFactory::Dimensions& dimensions,
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

pgsTypes::SupportedDeckTypes CBoxBeamFactory::GetSupportedDeckTypes(pgsTypes::SupportedBeamSpacing sbs)
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

pgsTypes::SupportedBeamSpacings CBoxBeamFactory::GetSupportedBeamSpacings()
{
   pgsTypes::SupportedBeamSpacings sbs;
   sbs.push_back(pgsTypes::sbsUniform);
   sbs.push_back(pgsTypes::sbsGeneral);
   sbs.push_back(pgsTypes::sbsUniformAdjacent);
   sbs.push_back(pgsTypes::sbsGeneralAdjacent);

   return sbs;
}


long CBoxBeamFactory::GetNumberOfWebs(const IBeamFactory::Dimensions& dimensions)
{
   return 2;
}

void CBoxBeamFactory::GetAllowableSpacingRange(const IBeamFactory::Dimensions& dimensions,pgsTypes::SupportedDeckType sdt, 
                                               pgsTypes::SupportedBeamSpacing sbs, double* minSpacing, double* maxSpacing)
{
   *minSpacing = 0.0;
   *maxSpacing = 0.0;

   double W1 = GetDimension(dimensions,"W1");
   double W2 = GetDimension(dimensions,"W2");
   double W3 = GetDimension(dimensions,"W3");
   double W4 = GetDimension(dimensions,"W4");
   double J  = GetDimension(dimensions,"Jmax");

   // girder width is max of top and bottom width
   double gwt = 2*(W1+W2) + W3;
   double gwb = 2*(W4+W2) + W3;

   double gw = max(gwt,gwb);

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
         *minSpacing = gw;
         *maxSpacing = gw+J;
      }
      else
      {
         ATLASSERT(false); // shouldn't get here
      }
   }
}

Float64 CBoxBeamFactory::GetBeamHeight(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   double H1 = GetDimension(dimensions,"H1");
   double H2 = GetDimension(dimensions,"H2");
   double H3 = GetDimension(dimensions,"H3");

   return H1 + H2 + H3;
}

Float64 CBoxBeamFactory::GetBeamWidth(const IBeamFactory::Dimensions& dimensions,pgsTypes::MemberEndType endType)
{
   double W1 = GetDimension(dimensions,"W1");
   double W2 = GetDimension(dimensions,"W2");
   double W3 = GetDimension(dimensions,"W3");
   double W4 = GetDimension(dimensions,"W4");

   double top = 2*(W1+W2) + W3;

   double bot = 2*(W4+W2) + W3;

   return max(top,bot);
}
