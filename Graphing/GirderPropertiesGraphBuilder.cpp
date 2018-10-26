///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

#include "stdafx.h"
#include "resource.h"
#include <Graphing\GirderPropertiesGraphBuilder.h>
#include <Graphing\DrawBeamTool.h>
#include "GirderPropertiesGraphController.h"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <UnitMgt\UnitValueNumericalFormatTools.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>
#include <IFace\DocumentType.h>

#include <EAF\EAFGraphView.h>
#include <EAF\EAFDocument.h>

#include <MFCTools\MFCTools.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

BEGIN_MESSAGE_MAP(CGirderPropertiesGraphBuilder, CGirderGraphBuilderBase)
END_MESSAGE_MAP()


CGirderPropertiesGraphBuilder::CGirderPropertiesGraphBuilder() :
CGirderGraphBuilderBase()
{
   SetName(_T("Girder Properties"));
   
   InitDocumentation(EAFGetDocument()->GetDocumentationSetName(),IDH_GIRDER_PROPERTIES);
}

CGirderPropertiesGraphBuilder::CGirderPropertiesGraphBuilder(const CGirderPropertiesGraphBuilder& other) :
CGirderGraphBuilderBase(other)
{
}

CGirderPropertiesGraphBuilder::~CGirderPropertiesGraphBuilder()
{
}

int CGirderPropertiesGraphBuilder::InitializeGraphController(CWnd* pParent,UINT nID)
{
   if ( !CGirderGraphBuilderBase::InitializeGraphController(pParent,nID) )
   {
      return FALSE;
   }

   m_Graph.SetPinYAxisAtZero(true);

   m_pGraphController->CheckRadioButton(IDC_TRANSFORMED,IDC_GROSS,IDC_TRANSFORMED);

   return 0;
}

BOOL CGirderPropertiesGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != NULL);
   return m_pGraphController->Create(pParent,IDD_GIRDER_PROPERTIES_GRAPH_CONTROLLER, CBRS_LEFT, nID);
}

CGraphBuilder* CGirderPropertiesGraphBuilder::Clone()
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CGirderPropertiesGraphBuilder(*this);
}

void CGirderPropertiesGraphBuilder::UpdateXAxis()
{
   CGirderGraphBuilderBase::UpdateXAxis();
   m_Graph.SetXAxisTitle(std::_tstring(_T("Distance From CL Bearing at Left End of Girder (")+m_pXFormat->UnitTag()+_T(")")).c_str());
}

CGirderGraphControllerBase* CGirderPropertiesGraphBuilder::CreateGraphController()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CGirderPropertiesGraphController;
}

bool CGirderPropertiesGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   CGirderGraphBuilderBase::UpdateNow();

   // Update graph properties
   CGirderKey girderKey = m_pGraphController->GetGirderKey();
   IntervalIndexType intervalIdx = ((CIntervalGirderGraphControllerBase*)m_pGraphController)->GetInterval();
   PropertyType propertyType = ((CGirderPropertiesGraphController*)m_pGraphController)->GetPropertyType();
   pgsTypes::SectionPropertyType sectionPropertyType = ((CGirderPropertiesGraphController*)m_pGraphController)->GetSectionPropertyType();

   UpdateYAxisUnits(propertyType);

   UpdateGraphTitle(girderKey,intervalIdx,propertyType);

   UpdateGraphData(girderKey,intervalIdx,propertyType,sectionPropertyType);

   return true;
}

void CGirderPropertiesGraphBuilder::UpdateYAxisUnits(PropertyType propertyType)
{
   delete m_pYFormat;
   m_pYFormat = NULL;

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   switch(propertyType)
   {
   case Height:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Height (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }

   case Area:
      {
      const unitmgtLength2Data& areaUnit = pDisplayUnits->GetAreaUnit();
      m_pYFormat = new AreaTool(areaUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Area (") + ((AreaTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }

   case MomentOfInertia:
      {
      const unitmgtLength4Data& momentOfInertiaUnit = pDisplayUnits->GetMomentOfInertiaUnit();
      m_pYFormat = new MomentOfInertiaTool(momentOfInertiaUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Moment of Insertia (") + ((MomentOfInertiaTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }

   case Centroid:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Centroid (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }

   case SectionModulus:
      {
      const unitmgtLength3Data& sectionModulusUnit = pDisplayUnits->GetSectModulusUnit();
      m_pYFormat = new SectionModulusTool(sectionModulusUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Section Modulus (") + ((SectionModulusTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }

   case KernPoint:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Kern Point (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }

   case StrandEccentricity:
   case TendonEccentricity:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Eccentricty (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }

   case TendonProfile:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Elevation (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }

   case EffectiveFlangeWidth:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Effective Flange Width (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }

   case Fc:
      {
      const unitmgtStressData& stressUnit = pDisplayUnits->GetStressUnit();
      m_pYFormat = new StressTool(stressUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("f'c (") + ((StressTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }

   case Ec:
      {
      const unitmgtStressData& stressUnit = pDisplayUnits->GetModEUnit();
      m_pYFormat = new StressTool(stressUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Ec (") + ((StressTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }

   default:
      ASSERT(false); 
   }
}

void CGirderPropertiesGraphBuilder::UpdateGraphTitle(const CGirderKey& girderKey,IntervalIndexType intervalIdx,PropertyType propertyType)
{
   GET_IFACE(IIntervals,pIntervals);
   CString strInterval( pIntervals->GetDescription(intervalIdx) );

   CString strGraphTitle;
   if ( girderKey.groupIndex == ALL_GROUPS )
   {
      strGraphTitle.Format(_T("Girder Line %s - %s - Interval %d: %s"),LABEL_GIRDER(girderKey.girderIndex),GetPropertyLabel(propertyType),LABEL_INTERVAL(intervalIdx),strInterval);
   }
   else
   {
      GET_IFACE(IDocumentType,pDocType);
      CString strGroupLabel(pDocType->IsPGSuperDocument() ? _T("Span") : _T("Group"));
      strGraphTitle.Format(_T("%s %d Girder %s - %s - Interval %d: %s"),strGroupLabel,LABEL_GROUP(girderKey.groupIndex),LABEL_GIRDER(girderKey.girderIndex),GetPropertyLabel(propertyType),LABEL_INTERVAL(intervalIdx),strInterval);
   }
   
   m_Graph.SetTitle(strGraphTitle);
}

void CGirderPropertiesGraphBuilder::UpdateGraphData(const CGirderKey& girderKey,IntervalIndexType intervalIdx,PropertyType propertyType,pgsTypes::SectionPropertyType sectPropType)
{
   // clear graph
   m_Graph.ClearData();

   // Get the points of interest we need.
   GET_IFACE(IPointOfInterest,pPoi);
   std::vector<pgsPointOfInterest> vPoi;
   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType firstGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType lastGroupIdx = (girderKey.groupIndex== ALL_GROUPS ? nGroups-1 : firstGroupIdx);
   for ( GroupIndexType grpIdx = firstGroupIdx; grpIdx <= lastGroupIdx; grpIdx++ )
   {
      GirderIndexType nGirders = pBridge->GetGirderCount(grpIdx);
      GirderIndexType gdrIdx = Min(girderKey.girderIndex,nGirders-1);
      CSegmentKey segmentKey(grpIdx,gdrIdx,ALL_SEGMENTS);
      std::vector<pgsPointOfInterest> vSegmentPoi(pPoi->GetPointsOfInterest(segmentKey));
      vPoi.insert(vPoi.end(),vSegmentPoi.begin(),vSegmentPoi.end());
   }

   // Map POI coordinates to X-values for the graph
   std::vector<Float64> xVals;
   GetXValues(vPoi,&xVals);

   // The tendon graph is different than all the rest...
   if ( propertyType == TendonEccentricity || propertyType == TendonProfile)
   {
      // ... deal with it and return
      UpdateTendonGraph(propertyType,girderKey,intervalIdx,vPoi,xVals);
      return;
   }

   IndexType dataSeries1, dataSeries2;
   InitializeGraph(propertyType,girderKey,intervalIdx,&dataSeries1,&dataSeries2);

   Float64 nEffectiveStrands;

   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   std::vector<Float64>::iterator xIter(xVals.begin());
   for ( ; iter != end; iter++, xIter++ )
   {
      pgsPointOfInterest& poi = *iter;
      Float64 value1,value2;
      switch(propertyType)
      {
      case Height:
         {
         GET_IFACE(ISectionProperties,pSectProps);
         value1 = pSectProps->GetHg(intervalIdx,poi);
         break;
         }
         
      case Area:
         {
         GET_IFACE(ISectionProperties,pSectProps);
         value1 = pSectProps->GetAg(sectPropType,intervalIdx,poi);
         break;
         }
         
      case MomentOfInertia:
         {
         GET_IFACE(ISectionProperties,pSectProps);
         value1 = pSectProps->GetIx(sectPropType,intervalIdx,poi);
         break;
         }

      case Centroid:
         {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
         GET_IFACE(ISectionProperties,pSectProps);
         if ( intervalIdx < compositeDeckIntervalIdx )
         {
            value1 = pSectProps->GetY(sectPropType,intervalIdx,poi,pgsTypes::TopGirder);
         }
         else
         {
            value1 = pSectProps->GetY(sectPropType,intervalIdx,poi,pgsTypes::TopDeck);
         }

         value2 = pSectProps->GetY(sectPropType,intervalIdx,poi,pgsTypes::BottomGirder);
         break;
         }

      case SectionModulus:
         {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
         GET_IFACE(ISectionProperties,pSectProps);
         if ( intervalIdx < compositeDeckIntervalIdx )
         {
            value1 = pSectProps->GetS(sectPropType,intervalIdx,poi,pgsTypes::TopGirder);
         }
         else
         {
            value1 = pSectProps->GetS(sectPropType,intervalIdx,poi,pgsTypes::TopDeck);
         }

         value2 = pSectProps->GetS(sectPropType,intervalIdx,poi,pgsTypes::BottomGirder);
         break;
         }

      case KernPoint:
         {
         GET_IFACE(ISectionProperties,pSectProps);
         value1 = pSectProps->GetKt(sectPropType,intervalIdx,poi);
         value2 = pSectProps->GetKb(sectPropType,intervalIdx,poi);
         break;
         }

      case StrandEccentricity:
         {
         GET_IFACE(IStrandGeometry,pStrandGeom);
         value1 = pStrandGeom->GetEccentricity(sectPropType,intervalIdx,poi,true/*include temp strands*/,&nEffectiveStrands);
         break;
         }

      case TendonEccentricity:
         ATLASSERT(false); // should never get here
         break;

      case EffectiveFlangeWidth:
         {
         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
         if ( intervalIdx < compositeDeckIntervalIdx )
         {
            value1 = 0;
         }
         else
         {
            GET_IFACE(ISectionProperties,pSectProps);
            value1 = pSectProps->GetEffectiveFlangeWidth(poi);
         }
         break;
         }

      case Fc:
         {
         GET_IFACE(IMaterials,pMaterials);
         GET_IFACE(IPointOfInterest,pPoi);
         CClosureKey closureKey;
         if ( pPoi->IsInClosureJoint(poi,&closureKey) )
         {
            value1 = pMaterials->GetClosureJointFc(closureKey,intervalIdx);
         }
         else
         {
            value1 = pMaterials->GetSegmentFc(poi.GetSegmentKey(),intervalIdx);
         }

         GET_IFACE(IIntervals,pIntervals);
         if ( pIntervals->GetCompositeDeckInterval() )
         {
            value2 = pMaterials->GetDeckFc(intervalIdx);
         }
         break;
         }

      case Ec:
         {
         GET_IFACE(IMaterials,pMaterials);
         GET_IFACE(IPointOfInterest,pPoi);
         CClosureKey closureKey;
         if ( pPoi->IsInClosureJoint(poi,&closureKey) )
         {
            value1 = pMaterials->GetClosureJointEc(closureKey,intervalIdx);
         }
         else
         {
            value1 = pMaterials->GetSegmentEc(poi.GetSegmentKey(),intervalIdx);
         }

         GET_IFACE(IIntervals,pIntervals);
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval();
         if ( compositeDeckIntervalIdx <= intervalIdx )
         {
            value2 = pMaterials->GetDeckEc(intervalIdx);
         }
         break;
         }

      default:
         ATLASSERT(false);
      }

      Float64 X = *xIter;

      if ( dataSeries1 != INVALID_INDEX )
      {
         AddGraphPoint(dataSeries1,X,value1);
      }

      if ( dataSeries2 != INVALID_INDEX )
      {
         AddGraphPoint(dataSeries2,X,value2);
      }
   }
}


void CGirderPropertiesGraphBuilder::UpdateTendonGraph(PropertyType propertyType,const CGirderKey& girderKey,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals)
{
   ATLASSERT(propertyType == TendonEccentricity || propertyType == TendonProfile);

   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   DuctIndexType nMaxDucts = 0;
   GET_IFACE(ITendonGeometry,pTendonGeom);
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);
      DuctIndexType nDucts = pTendonGeom->GetDuctCount(thisGirderKey);
      nMaxDucts = Max(nMaxDucts,nDucts);
   }
   
   grGraphColor graphColor(nMaxDucts);

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);
      DuctIndexType nDucts = pTendonGeom->GetDuctCount(thisGirderKey);
      for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++ )
      {
         COLORREF color = graphColor.GetColor(ductIdx);

         CString strLabel;
         strLabel.Format(_T("Tendon %d"),LABEL_DUCT(ductIdx));
         IndexType dataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,GRAPH_PEN_WEIGHT,color);

         std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
         std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
         std::vector<Float64>::const_iterator xIter(xVals.begin());
         for ( ; iter != end; iter++, xIter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            Float64 value;
            if ( propertyType == TendonEccentricity )
            {
               value = pTendonGeom->GetEccentricity(intervalIdx,poi,ductIdx);
            }
            else
            {
               value = pTendonGeom->GetDuctOffset(intervalIdx,poi,ductIdx);
            }

            Float64 X = *xIter;

            AddGraphPoint(dataSeries,X,value);
         }
      }
   }
}

LPCTSTR CGirderPropertiesGraphBuilder::GetPropertyLabel(PropertyType propertyType)
{
   switch(propertyType)
   {
   case Height:
      return _T("Height");
      break;

   case Area:
      return _T("Area");
      break;

   case MomentOfInertia:
      return _T("Moment of Inertia");
      break;

   case Centroid:
      return _T("Centroid");
      break;

   case SectionModulus:
      return _T("Section Modulus");
      break;

   case KernPoint:
      return _T("Kern Point");
      break;

   case StrandEccentricity:
      return _T("Strand Eccentricity");
      break;

   case TendonEccentricity:
      return _T("Tendon Eccentricity");
      break;

   case TendonProfile:
      return _T("Tendon Profile");
      break;

   case EffectiveFlangeWidth:
      return _T("Effective Flange Width");
      break;

   case Fc:
      return _T("f'c");
      break;

   case Ec:
      return _T("Ec");
      break;

   default:
      ATLASSERT(false);
   }

   return _T("");
}

void CGirderPropertiesGraphBuilder::InitializeGraph(PropertyType propertyType,const CGirderKey& girderKey,IntervalIndexType intervalIdx,IndexType* pGraph1,IndexType* pGraph2)
{
   *pGraph1 = INVALID_INDEX;
   *pGraph2 = INVALID_INDEX;

   GET_IFACE_NOCHECK(IIntervals,pIntervals); // not always used

   std::_tstring strPropertyLabel1( GetPropertyLabel(propertyType) );
   std::_tstring strPropertyLabel2(strPropertyLabel1);
   switch(propertyType)
   {
   case Height:
   case Area:
   case MomentOfInertia:
   case StrandEccentricity:
   case TendonEccentricity:
   case TendonProfile:
   case EffectiveFlangeWidth:
      *pGraph1 = m_Graph.CreateDataSeries(strPropertyLabel1.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,ORANGE);
      break;

   case Fc:
      strPropertyLabel1 += _T(" Girder");
      *pGraph1 = m_Graph.CreateDataSeries(strPropertyLabel1.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,ORANGE);
      if ( pIntervals->GetCompositeDeckInterval() <= intervalIdx )
      {
         strPropertyLabel2 += _T(" Deck");
         *pGraph2 = m_Graph.CreateDataSeries(strPropertyLabel2.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,BLUE);
      }
      break;

   case Ec:
      strPropertyLabel1 += _T(" Girder");
      *pGraph1 = m_Graph.CreateDataSeries(strPropertyLabel1.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,ORANGE);
      if ( pIntervals->GetCompositeDeckInterval() <= intervalIdx )
      {
         strPropertyLabel2 += _T(" Deck");
         *pGraph2 = m_Graph.CreateDataSeries(strPropertyLabel2.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,BLUE);
      }
      break;

   case Centroid:
      strPropertyLabel1 += _T(" from Top");
      strPropertyLabel2 += _T(" from Bottom");
      *pGraph1 = m_Graph.CreateDataSeries(strPropertyLabel1.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,ORANGE);
      *pGraph2 = m_Graph.CreateDataSeries(strPropertyLabel2.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,BLUE);
      break;

   case SectionModulus:
   case KernPoint:
      strPropertyLabel1 = _T("Top ") + strPropertyLabel1;
      strPropertyLabel2 = _T("Bottom ") + strPropertyLabel2;
      *pGraph1 = m_Graph.CreateDataSeries(strPropertyLabel1.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,ORANGE);
      *pGraph2 = m_Graph.CreateDataSeries(strPropertyLabel2.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,BLUE);
      break;

   default:
      ATLASSERT(false);
   }
}

IntervalIndexType CGirderPropertiesGraphBuilder::GetBeamDrawInterval()
{
   return ((CIntervalGirderGraphControllerBase*)m_pGraphController)->GetInterval();
}
