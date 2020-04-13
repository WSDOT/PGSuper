///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
#include "GirderPropertiesGraphController.h"
#include "GirderPropertiesGraphViewControllerImp.h"
#include "..\Documentation\PGSuper.hh"

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
   ATLASSERT(m_pGraphController != nullptr);
   return m_pGraphController->Create(pParent,IDD_GIRDER_PROPERTIES_GRAPH_CONTROLLER, CBRS_LEFT, nID);
}

void CGirderPropertiesGraphBuilder::CreateViewController(IEAFViewController** ppController)
{
   CComPtr<IEAFViewController> stdController;
   __super::CreateViewController(&stdController);

   CComObject<CGirderPropertiesGraphViewController>* pController;
   CComObject<CGirderPropertiesGraphViewController>::CreateInstance(&pController);
   pController->Init((CGirderPropertiesGraphController*)m_pGraphController, stdController);

   (*ppController) = pController;
   (*ppController)->AddRef();
}

CGraphBuilder* CGirderPropertiesGraphBuilder::Clone() const
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
   m_pYFormat = nullptr;

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
      std::_tstring strYAxisTitle = _T("Moment of Inertia (") + ((MomentOfInertiaTool*)m_pYFormat)->UnitTag() + _T(")");
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
      std::_tstring strYAxisTitle = _T("Elevation from top of non-composite girder (") + ((LengthTool*)m_pYFormat)->UnitTag() + _T(")");
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
      std::_tstring strYAxisTitle = _T("fc (") + ((StressTool*)m_pYFormat)->UnitTag() + _T(")");
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
      if (pDocType->IsPGSuperDocument())
      {
         strGraphTitle.Format(_T("Span %s Girder %s - %s - Interval %d: %s"), LABEL_SPAN(girderKey.groupIndex), LABEL_GIRDER(girderKey.girderIndex), GetPropertyLabel(propertyType), LABEL_INTERVAL(intervalIdx), strInterval);
      }
      else
      {
         strGraphTitle.Format(_T("Group %d Girder %s - %s - Interval %d: %s"), LABEL_GROUP(girderKey.groupIndex), LABEL_GIRDER(girderKey.girderIndex), GetPropertyLabel(propertyType), LABEL_INTERVAL(intervalIdx), strInterval);
      }
   }
   
   m_Graph.SetTitle(strGraphTitle);
}

void CGirderPropertiesGraphBuilder::UpdateGraphData(const CGirderKey& girderKey,IntervalIndexType intervalIdx,PropertyType propertyType,pgsTypes::SectionPropertyType sectPropType)
{
   // clear graph
   m_Graph.ClearData();

   // Get the points of interest we need.
   GET_IFACE(IPointOfInterest,pPoi);
   PoiList vPoi;
   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IIntervals, pIntervals);
   std::vector<CGirderKey> vGirderKeys;
   pBridge->GetGirderline(girderKey, &vGirderKeys);
   for(const auto& thisGirderKey : vGirderKeys)
   {
      CSegmentKey segmentKey(thisGirderKey,ALL_SEGMENTS);
      PoiList vSegmentPoi;
      pPoi->GetPointsOfInterest(segmentKey, &vSegmentPoi);

      IntervalIndexType firstSegmentErectionintervalIdx = pIntervals->GetFirstSegmentErectionInterval(segmentKey);
      if (intervalIdx < firstSegmentErectionintervalIdx)
      {
         // these POI are between segments so they don't apply
         pPoi->RemovePointsOfInterest(vSegmentPoi, POI_CLOSURE);
         pPoi->RemovePointsOfInterest(vSegmentPoi, POI_BOUNDARY_PIER);
      }

      vPoi.insert(std::end(vPoi),std::begin(vSegmentPoi),std::end(vSegmentPoi));
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

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->GetDeckType();

   IndexType dataSeries1, dataSeries2, dataSeries3, dataSeries4;
   InitializeGraph(propertyType,girderKey,intervalIdx,&dataSeries1,&dataSeries2, &dataSeries3, &dataSeries4);

   Float64 nEffectiveStrands;

   GET_IFACE_NOCHECK(ISectionProperties, pSectProps);
   GET_IFACE_NOCHECK(IStrandGeometry, pStrandGeom);

   bool bIsAsymmetric = pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing();

   auto& iter(vPoi.cbegin());
   auto& end(vPoi.cend());
   auto& xIter(xVals.cbegin());
   for ( ; iter != end; iter++, xIter++ )
   {
      const pgsPointOfInterest& poi(*iter);
      Float64 value1,value2,value3,value4;

      switch(propertyType)
      {
      case Height:
         {
         value1 = pSectProps->GetHg(intervalIdx,poi);
         break;
         }
         
      case Area:
         {
         value1 = pSectProps->GetAg(sectPropType,intervalIdx,poi);
         break;
         }
         
      case MomentOfInertia:
         {
         value1 = pSectProps->GetIxx(sectPropType, intervalIdx, poi);
         value2 = pSectProps->GetIyy(sectPropType, intervalIdx, poi);
         value3 = pSectProps->GetIxy(sectPropType, intervalIdx, poi);
         break;
         }

      case Centroid:
         {
         IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
         if ( intervalIdx < compositeDeckIntervalIdx )
         {
            value1 = pSectProps->GetY(sectPropType,intervalIdx,poi,pgsTypes::TopGirder);
         }
         else
         {
            value1 = pSectProps->GetY(sectPropType,intervalIdx,poi,pgsTypes::TopDeck);
         }

         value2 = pSectProps->GetY(sectPropType,intervalIdx,poi,pgsTypes::BottomGirder);

         value3 = pSectProps->GetXleft(sectPropType, intervalIdx, poi);
         value4 = pSectProps->GetXright(sectPropType, intervalIdx, poi);

         break;
         }

      case SectionModulus:
         {
         IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
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
         value1 = pSectProps->GetKt(sectPropType,intervalIdx,poi);
         value2 = pSectProps->GetKb(sectPropType,intervalIdx,poi);
         break;
         }

      case StrandEccentricity:
         {
            if (bIsAsymmetric)
            {
               pStrandGeom->GetEccentricity(sectPropType, intervalIdx, poi, true/*include temp strands*/, &nEffectiveStrands, &value2, &value1);

            }
            else
            {
               value1 = pStrandGeom->GetEccentricity(sectPropType, intervalIdx, poi, true/*include temp strands*/, &nEffectiveStrands);
            }
         break;
         }

      case TendonEccentricity:
         ATLASSERT(false); // should never get here
         break;

      case EffectiveFlangeWidth:
         {
         IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
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

         IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
         ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
         if (deckType != pgsTypes::sdtNone && compositeDeckIntervalIdx <= intervalIdx)
         {
            value2 = pMaterials->GetDeckFc(deckCastingRegionIdx,intervalIdx);
         }
         else
         {
            value2 = 0;
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

         IndexType deckCastingRegionIdx = pPoi->GetDeckCastingRegion(poi);
         ATLASSERT(deckCastingRegionIdx != INVALID_INDEX);
         IntervalIndexType compositeDeckIntervalIdx = pIntervals->GetCompositeDeckInterval(deckCastingRegionIdx);
         if (deckType != pgsTypes::sdtNone && compositeDeckIntervalIdx <= intervalIdx )
         {
            value2 = pMaterials->GetDeckEc(deckCastingRegionIdx,intervalIdx);
         }
         else
         {
            value2 = 0;
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

      if (dataSeries2 != INVALID_INDEX)
      {
         AddGraphPoint(dataSeries2, X, value2);
      }

      if (dataSeries3 != INVALID_INDEX)
      {
         AddGraphPoint(dataSeries3, X, value3);
      }

      if (dataSeries4 != INVALID_INDEX)
      {
         AddGraphPoint(dataSeries4, X, value4);
      }
   }
}


void CGirderPropertiesGraphBuilder::UpdateTendonGraph(PropertyType propertyType,const CGirderKey& girderKey,IntervalIndexType intervalIdx,const PoiList& vPoi,const std::vector<Float64>& xVals)
{
   ATLASSERT(propertyType == TendonEccentricity || propertyType == TendonProfile);

   GET_IFACE(IBridge,pBridge);
   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   GroupIndexType startGroupIdx = (girderKey.groupIndex == ALL_GROUPS ? 0 : girderKey.groupIndex);
   GroupIndexType endGroupIdx   = (girderKey.groupIndex == ALL_GROUPS ? nGroups-1 : startGroupIdx);

   // Get max number of ducts per girder for the color range
   DuctIndexType nMaxDucts = 0;
   GET_IFACE(IGirderTendonGeometry, pGirderTendonGeometry);
   GET_IFACE(ISegmentTendonGeometry, pSegmentTendonGeometry);
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);
      DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(thisGirderKey);

      DuctIndexType nMaxSegmentDucts = pSegmentTendonGeometry->GetMaxDuctCount(thisGirderKey);;
      nGirderDucts += nMaxSegmentDucts; // max number of ducts this girder
      nMaxDucts = Max(nMaxDucts, nGirderDucts); // overall max number of ducts
   }
   
   grGraphColor graphColor(nMaxDucts);

   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      CGirderKey thisGirderKey(grpIdx,girderKey.girderIndex);
      DuctIndexType nGirderDucts = pGirderTendonGeometry->GetDuctCount(thisGirderKey);
      for ( DuctIndexType ductIdx = 0; ductIdx < nGirderDucts; ductIdx++ )
      {
         COLORREF color = graphColor.GetColor(ductIdx);

         CString strLabel;
         strLabel.Format(_T("Girder Tendon %d"),LABEL_DUCT(ductIdx));
         IndexType dataSeries = m_Graph.CreateDataSeries(strLabel,PS_SOLID,GRAPH_PEN_WEIGHT,color);

         auto iter(vPoi.begin());
         auto end(vPoi.end());
         auto xIter(xVals.begin());
         for ( ; iter != end; iter++, xIter++ )
         {
            const pgsPointOfInterest& poi = *iter;
            if (pGirderTendonGeometry->IsOnDuct(poi, ductIdx))
            {
               Float64 value;
               if (propertyType == TendonEccentricity)
               {
                  Float64 eccX, eccY;
                  pGirderTendonGeometry->GetGirderTendonEccentricity(intervalIdx, poi, ductIdx, &eccX, &eccY);
                  value = eccY;
               }
               else
               {
                  value = pGirderTendonGeometry->GetGirderDuctOffset(intervalIdx, poi, ductIdx);
               }

               Float64 X = *xIter;

               AddGraphPoint(dataSeries, X, value);
            } // point on duct
         } // next point
      } // next girder duct

      SegmentIndexType nSegments = pBridge->GetSegmentCount(thisGirderKey);
      for (SegmentIndexType segIdx = 0; segIdx < nSegments; segIdx++)
      {
         CSegmentKey thisSegmentKey(thisGirderKey, segIdx);
         DuctIndexType nSegmentDucts = pSegmentTendonGeometry->GetDuctCount(thisSegmentKey);
         for (DuctIndexType ductIdx = 0; ductIdx < nSegmentDucts; ductIdx++)
         {
            COLORREF color = graphColor.GetColor(nGirderDucts + ductIdx);

            CString strLabel;
            strLabel.Format(_T("Segment %d Tendon %d"), LABEL_SEGMENT(segIdx), LABEL_DUCT(ductIdx));
            IndexType dataSeries = m_Graph.CreateDataSeries(strLabel, PS_SOLID, GRAPH_PEN_WEIGHT, color);

            auto iter(vPoi.begin());
            auto end(vPoi.end());
            auto xIter(xVals.begin());
            for (; iter != end; iter++, xIter++)
            {
               const pgsPointOfInterest& poi = *iter;

               if (pSegmentTendonGeometry->IsOnDuct(poi))
               {
                  Float64 value;
                  if (propertyType == TendonEccentricity)
                  {
                     Float64 eccX, eccY;
                     pSegmentTendonGeometry->GetSegmentTendonEccentricity(intervalIdx, poi, ductIdx, &eccX, &eccY);
                     value = eccY;
                  }
                  else
                  {
                     value = pSegmentTendonGeometry->GetSegmentDuctOffset(intervalIdx, poi, ductIdx);
                  }

                  Float64 X = *xIter;

                  AddGraphPoint(dataSeries, X, value);
               } // if segment
            } // point on duct
         } // next segment duct
      } // next segment

   } // next group
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
      return _T("fc");
      break;

   case Ec:
      return _T("Ec");
      break;

   default:
      ATLASSERT(false);
   }

   return _T("");
}

void CGirderPropertiesGraphBuilder::InitializeGraph(PropertyType propertyType,const CGirderKey& girderKey,IntervalIndexType intervalIdx,IndexType* pGraph1,IndexType* pGraph2, IndexType* pGraph3, IndexType* pGraph4)
{
   *pGraph1 = INVALID_INDEX;
   *pGraph2 = INVALID_INDEX;
   *pGraph3 = INVALID_INDEX;
   *pGraph4 = INVALID_INDEX;

   GET_IFACE_NOCHECK(IIntervals,pIntervals); // not always used

   GET_IFACE(IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
   pgsTypes::SupportedDeckType deckType = pBridgeDesc->GetDeckDescription()->GetDeckType();

   GET_IFACE(IBridge, pBridge);
   bool bIsAsymmetric = pBridge->HasAsymmetricGirders() || pBridge->HasAsymmetricPrestressing();

   std::_tstring strPropertyLabel1( GetPropertyLabel(propertyType) );
   std::_tstring strPropertyLabel2(strPropertyLabel1);
   std::_tstring strPropertyLabel3(strPropertyLabel1);
   std::_tstring strPropertyLabel4(strPropertyLabel1);

   switch(propertyType)
   {
   case Height:
   case Area:
   case TendonEccentricity:
   case TendonProfile:
   case EffectiveFlangeWidth:
      *pGraph1 = m_Graph.CreateDataSeries(strPropertyLabel1.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,ORANGE);
      break;

   case StrandEccentricity:
      if (bIsAsymmetric)
      {
         CString strLabel1;
         strLabel1.Format(_T("%s, ey"), strPropertyLabel1.c_str());
         *pGraph1 = m_Graph.CreateDataSeries(strLabel1, PS_SOLID, GRAPH_PEN_WEIGHT, ORANGE);

         CString strLabel2;
         strLabel2.Format(_T("%s, ex"), strPropertyLabel1.c_str());
         *pGraph2 = m_Graph.CreateDataSeries(strLabel2, PS_SOLID, GRAPH_PEN_WEIGHT, BLUE);
      }
      else
      {
         *pGraph1 = m_Graph.CreateDataSeries(strPropertyLabel1.c_str(), PS_SOLID, GRAPH_PEN_WEIGHT, ORANGE);
      }
      break;

   case Fc:
      strPropertyLabel1 += _T(" Girder");
      *pGraph1 = m_Graph.CreateDataSeries(strPropertyLabel1.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,ORANGE);
      if (deckType != pgsTypes::sdtNone && pIntervals->GetFirstCompositeDeckInterval() <= intervalIdx )
      {
         strPropertyLabel2 += _T(" Deck");
         *pGraph2 = m_Graph.CreateDataSeries(strPropertyLabel2.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,BLUE);
      }
      break;

   case Ec:
      strPropertyLabel1 += _T(" Girder");
      *pGraph1 = m_Graph.CreateDataSeries(strPropertyLabel1.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,ORANGE);
      if (deckType != pgsTypes::sdtNone &&  pIntervals->GetFirstCompositeDeckInterval() <= intervalIdx )
      {
         strPropertyLabel2 += _T(" Deck");
         *pGraph2 = m_Graph.CreateDataSeries(strPropertyLabel2.c_str(),PS_SOLID,GRAPH_PEN_WEIGHT,BLUE);
      }
      break;

   case MomentOfInertia:
      strPropertyLabel1 = _T("Ix");
      strPropertyLabel2 = _T("Iy");
      strPropertyLabel3 = _T("Ixy");
      *pGraph1 = m_Graph.CreateDataSeries(strPropertyLabel1.c_str(), PS_SOLID, GRAPH_PEN_WEIGHT, ORANGE);
      *pGraph2 = m_Graph.CreateDataSeries(strPropertyLabel2.c_str(), PS_SOLID, GRAPH_PEN_WEIGHT, BLUE);
      *pGraph3 = m_Graph.CreateDataSeries(strPropertyLabel3.c_str(), PS_SOLID, GRAPH_PEN_WEIGHT, GREEN);
      break;

   case Centroid:
      strPropertyLabel1 += _T(" from Top");
      strPropertyLabel2 += _T(" from Bottom");
      strPropertyLabel3 += _T(" from Left");
      strPropertyLabel4 += _T(" from Right");
      *pGraph1 = m_Graph.CreateDataSeries(strPropertyLabel1.c_str(), PS_SOLID, GRAPH_PEN_WEIGHT, ORANGE);
      *pGraph2 = m_Graph.CreateDataSeries(strPropertyLabel2.c_str(), PS_SOLID, GRAPH_PEN_WEIGHT, BLUE);
      *pGraph3 = m_Graph.CreateDataSeries(strPropertyLabel3.c_str(), PS_SOLID, GRAPH_PEN_WEIGHT, GREEN);
      *pGraph4 = m_Graph.CreateDataSeries(strPropertyLabel4.c_str(), PS_SOLID, GRAPH_PEN_WEIGHT, RED);
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

void CGirderPropertiesGraphBuilder::GetBeamDrawIntervals(IntervalIndexType* pFirstIntervalIdx, IntervalIndexType* pLastIntervalIdx)
{
   *pFirstIntervalIdx = ((CIntervalGirderGraphControllerBase*)m_pGraphController)->GetInterval();
   *pLastIntervalIdx = *pFirstIntervalIdx;
}
