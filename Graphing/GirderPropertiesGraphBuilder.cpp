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

#include "stdafx.h"
#include "resource.h"
#include <Graphing\GirderPropertiesGraphBuilder.h>
#include <Graphing\DrawBeamTool.h>
#include "GirderPropertiesGraphController.h"

#include <PGSuperColors.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\PrestressForce.h>

#include <EAF\EAFGraphView.h>

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
}

CGirderPropertiesGraphBuilder::CGirderPropertiesGraphBuilder(const CGirderPropertiesGraphBuilder& other) :
CGirderGraphBuilderBase(other)
{
}

CGirderPropertiesGraphBuilder::~CGirderPropertiesGraphBuilder()
{
}

CGraphBuilder* CGirderPropertiesGraphBuilder::Clone()
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CGirderPropertiesGraphBuilder(*this);
}

int CGirderPropertiesGraphBuilder::CreateControls(CWnd* pParent,UINT nID)
{
   CGirderGraphBuilderBase::CreateControls(pParent,nID);

   m_Graph.SetXAxisTitle(_T("Distance From CL Bearing at Left End of Girder (")+m_pXFormat->UnitTag()+_T(")"));
   m_Graph.SetYAxisTitle(_T("Stress (")+m_pYFormat->UnitTag()+_T(")"));
   m_Graph.SetPinYAxisAtZero(true);

   m_pGraphController->CheckRadioButton(IDC_TRANSFORMED,IDC_GROSS,IDC_TRANSFORMED);

   return 0;
}

CGirderGraphControllerBase* CGirderPropertiesGraphBuilder::CreateGraphController()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return new CGirderPropertiesGraphController;
}

int CGirderPropertiesGraphBuilder::InitGraphController(CWnd* pParent,UINT nID)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   ATLASSERT(m_pGraphController != NULL);
   return m_pGraphController->Create(pParent,IDD_GIRDER_PROPERTIES_GRAPH_CONTROLLER, CBRS_LEFT, nID);
}

bool CGirderPropertiesGraphBuilder::UpdateNow()
{
   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   CGirderGraphBuilderBase::UpdateNow();

   // Update graph properties
   GroupIndexType    grpIdx      = m_pGraphController->GetGirderGroup();
   GirderIndexType   gdrIdx      = m_pGraphController->GetGirder();
   IntervalIndexType intervalIdx = m_pGraphController->GetInterval();
   PropertyType propertyType = ((CGirderPropertiesGraphController*)m_pGraphController)->GetPropertyType();
   pgsTypes::SectionPropertyType sectionPropertyType = ((CGirderPropertiesGraphController*)m_pGraphController)->GetSectionPropertyType();

   UpdateYAxisUnits(propertyType);

   UpdateGraphTitle(grpIdx,gdrIdx,intervalIdx,propertyType);

   UpdateGraphData(grpIdx,gdrIdx,intervalIdx,propertyType,sectionPropertyType);

   return true;
}

void CGirderPropertiesGraphBuilder::UpdateYAxisUnits(PropertyType propertyType)
{
   delete m_pYFormat;

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   switch(propertyType)
   {
   case Height:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Height (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }

   case Area:
      {
      const unitmgtLength2Data& areaUnit = pDisplayUnits->GetAreaUnit();
      m_pYFormat = new AreaTool(areaUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Area (") + ((AreaTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }

   case MomentOfInertia:
      {
      const unitmgtLength4Data& momentOfInertiaUnit = pDisplayUnits->GetMomentOfInertiaUnit();
      m_pYFormat = new MomentOfInertiaTool(momentOfInertiaUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Moment of Insertia (") + ((MomentOfInertiaTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }

   case YTop:
   case YBottom:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Centroid (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }

   case TopSectionModulus:
   case BottomSectionModulus:
      {
      const unitmgtLength3Data& sectionModulusUnit = pDisplayUnits->GetSectModulusUnit();
      m_pYFormat = new SectionModulusTool(sectionModulusUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Section Modulus (") + ((SectionModulusTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }

   case TopKernPoint:
   case BottomKernPoint:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Kern Point (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }

   case StrandEccentricity:
   case TendonEccentricity:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Eccentricty (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }

   case TendonProfile:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Elevation (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }

   case EffectiveFlangeWidth:
      {
      const unitmgtLengthData& heightUnit = pDisplayUnits->GetComponentDimUnit();
      m_pYFormat = new LengthTool(heightUnit);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring strYAxisTitle = _T("Effective Flange Width (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle);
      break;
      }

   default:
      ASSERT(false); 
   }
}

void CGirderPropertiesGraphBuilder::UpdateGraphTitle(GroupIndexType grpIdx,GirderIndexType gdrIdx,IntervalIndexType intervalIdx,PropertyType propertyType)
{
   GET_IFACE(IIntervals,pIntervals);
   CString strInterval( pIntervals->GetDescription(intervalIdx) );

   CString strGraphTitle;
   strGraphTitle.Format(_T("Group %d Girder %s - %s - %s"),LABEL_GROUP(grpIdx),LABEL_GIRDER(gdrIdx),GetPropertyLabel(propertyType),strInterval);
   
   m_Graph.SetTitle(std::_tstring(strGraphTitle));
}

void CGirderPropertiesGraphBuilder::UpdateGraphData(GroupIndexType grpIdx,GirderIndexType gdrIdx,IntervalIndexType intervalIdx,PropertyType propertyType,pgsTypes::SectionPropertyType sectPropType)
{
   // clear graph
   m_Graph.ClearData();

   // Get the points of interest we need.
   GET_IFACE(IPointOfInterest,pIPoi);
   CSegmentKey segmentKey(grpIdx,gdrIdx,ALL_SEGMENTS);
   std::vector<pgsPointOfInterest> vPoi( pIPoi->GetPointsOfInterest( segmentKey ) );

   // Map POI coordinates to X-values for the graph
   std::vector<Float64> xVals;
   GetXValues(vPoi,xVals);

   // The tendon graph is different than all the rest...
   if ( propertyType == TendonEccentricity || propertyType == TendonProfile)
   {
      // ... deal with it and return
      UpdateTendonGraph(propertyType,CGirderKey(grpIdx,gdrIdx),intervalIdx,vPoi,xVals);
      return;
   }

   IndexType dataSeries = InitializeGraph(propertyType);

   GET_IFACE(ISectionProperties,pSectProps);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   GET_IFACE(ITendonGeometry,pTendonGeom);
   GET_IFACE(IIntervals,pIntervals);

   Float64 nEffectiveStrands;

   std::vector<pgsPointOfInterest>::iterator iter(vPoi.begin());
   std::vector<pgsPointOfInterest>::iterator end(vPoi.end());
   std::vector<Float64>::iterator xIter(xVals.begin());
   for ( ; iter != end; iter++, xIter++ )
   {
      pgsPointOfInterest& poi = *iter;
      Float64 value;
      switch(propertyType)
      {
      case Height:
         value = pSectProps->GetHg(intervalIdx,poi);
         break;
         
      case Area:
         value = pSectProps->GetAg(sectPropType,intervalIdx,poi);
         break;
         
      case MomentOfInertia:
         value = pSectProps->GetIx(sectPropType,intervalIdx,poi);
         break;

      case YTop:
         value = pSectProps->GetYt(sectPropType,intervalIdx,poi);
         break;

      case YBottom:
         value = pSectProps->GetYb(sectPropType,intervalIdx,poi);
         break;

      case TopSectionModulus:
         value = pSectProps->GetSt(sectPropType,intervalIdx,poi);
         break;

      case BottomSectionModulus:
         value = pSectProps->GetSb(sectPropType,intervalIdx,poi);
         break;

      case TopKernPoint:
         value = pSectProps->GetKt(sectPropType,intervalIdx,poi);
         break;

      case BottomKernPoint:
         value = pSectProps->GetKb(sectPropType,intervalIdx,poi);
         break;

      case StrandEccentricity:
         value = pStrandGeom->GetEccentricity(sectPropType,intervalIdx,poi,true,&nEffectiveStrands);
         break;

      case TendonEccentricity:
         ATLASSERT(false); // should never get here
         break;

      case EffectiveFlangeWidth:
         if ( intervalIdx < pIntervals->GetCompositeDeckInterval() )
            value = 0;
         else
            value = pSectProps->GetEffectiveFlangeWidth(poi);
         break;

      default:
         ATLASSERT(false);
      }

      Float64 X = *xIter;

      AddGraphPoint(dataSeries,X,value);
   }
}


void CGirderPropertiesGraphBuilder::UpdateTendonGraph(PropertyType propertyType,const CGirderKey& girderKey,IntervalIndexType intervalIdx,const std::vector<pgsPointOfInterest>& vPoi,const std::vector<Float64>& xVals)
{
   ATLASSERT(propertyType == TendonEccentricity || propertyType == TendonProfile);
#pragma Reminder("UPDATE: this is a kludgy way to handle colors")
   // Need a color generation algorithm so that we don't repeat colors
   // Should be generated and made part of the graphing sub-system

   COLORREF colors[4] = {RED,ORANGE,PINK,BLUE};
   int colorIdx = 0;
   int nColors = 4;

   GET_IFACE(ITendonGeometry,pTendonGeom);
   DuctIndexType nDucts = pTendonGeom->GetDuctCount(girderKey);
   for ( DuctIndexType ductIdx = 0; ductIdx < nDucts; ductIdx++,colorIdx++ )
   {
      if (nColors <= colorIdx )
         colorIdx = 0;

      CString strLabel;
      strLabel.Format(_T("Tendon %d"),LABEL_DUCT(ductIdx));
      IndexType dataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,1,colors[colorIdx]);

      std::vector<pgsPointOfInterest>::const_iterator iter(vPoi.begin());
      std::vector<pgsPointOfInterest>::const_iterator end(vPoi.end());
      std::vector<Float64>::const_iterator xIter(xVals.begin());
      for ( ; iter != end; iter++, xIter++ )
      {
         const pgsPointOfInterest& poi = *iter;
         Float64 value;
         if ( propertyType == TendonEccentricity )
            value = pTendonGeom->GetEccentricity(intervalIdx,poi,ductIdx);
         else
            value = pTendonGeom->GetDuctOffset(intervalIdx,poi,ductIdx);

         Float64 X = *xIter;

         AddGraphPoint(dataSeries,X,value);
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

   case YTop:
      return _T("Centroid from Top");
      break;

   case YBottom:
      return _T("Centroid from Bottom");
      break;

   case TopSectionModulus:
      return _T("Top Section Modulus");
      break;

   case BottomSectionModulus:
      return _T("Bottom Section Modulus");
      break;

   case TopKernPoint:
      return _T("Top Kern Point");
      break;

   case BottomKernPoint:
      return _T("Bottom Kern Point");
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

   default:
      ATLASSERT(false);
   }

   return _T("");
}

IndexType CGirderPropertiesGraphBuilder::InitializeGraph(PropertyType propertyType)
{
   return m_Graph.CreateDataSeries(GetPropertyLabel(propertyType),PS_SOLID,1,ORANGE);
}
