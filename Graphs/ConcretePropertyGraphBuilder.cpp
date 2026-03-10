///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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
#include <Graphs/ConcretePropertyGraphBuilder.h>
#include <Graphs/ExportGraphXYTool.h>
#include "ConcretePropertyGraphController.h"
#include "ConcretePropertiesGraphViewControllerImp.h"
#include "..\Documentation\PGSuper.hh"

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF/AutoProgress.h>
#include <Units\UnitValueNumericalFormatTools.h>
#include <PgsExt\IntervalTool.h>

#include <PsgLib\ClosureJointData.h>

#include <IFace\Intervals.h>
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\PointOfInterest.h>

#include <EAF\EAFGraphView.h>
#include <EAF\EAFDocument.h>

#include <Graphing/AxisXY.h>

#include <MFCTools\MFCTools.h>

#include <Reporting\ReportNotes.h> // for IncrementValue

#include <algorithm>



// create a dummy unit conversion tool to pacify the graph constructor
static WBFL::Units::LengthData DUMMY(WBFL::Units::Measure::Meter);
static WBFL::Units::LengthTool DUMMY_TOOL(DUMMY);

BEGIN_MESSAGE_MAP(CConcretePropertyGraphBuilder, CEAFGraphBuilderBase)
END_MESSAGE_MAP()


CConcretePropertyGraphBuilder::CConcretePropertyGraphBuilder() :
CEAFAutoCalcGraphBuilder(),
m_Graph(&DUMMY_TOOL,&DUMMY_TOOL),
m_pTimeFormat(nullptr),
m_pIntervalFormat(nullptr),
m_pYFormat(nullptr)
{
   Init();
}

CConcretePropertyGraphBuilder::CConcretePropertyGraphBuilder(const CConcretePropertyGraphBuilder& other) :
CEAFAutoCalcGraphBuilder(other),
m_Graph(&DUMMY_TOOL,&DUMMY_TOOL),
m_pTimeFormat(nullptr),
m_pIntervalFormat(nullptr),
m_pYFormat(nullptr)
{
   Init();
}

void CConcretePropertyGraphBuilder::Init()
{
   m_XAxisType = X_AXIS_TIME_LOG;

   SetName(_T("Concrete Properties"));

   InitDocumentation(EAFGetDocument()->GetDocumentationSetName(),IDH_CONCRETE_PROPERTIES);

   m_pGraphController = new CConcretePropertyGraphController;

   m_Time.Width = 7;
   m_Time.Precision = 0;
   m_Time.Format = WBFL::System::NumericFormatTool::Format::Fixed;

   m_Interval.Width = 7;
   m_Interval.Precision = 1;
   m_Interval.Format = WBFL::System::NumericFormatTool::Format::Fixed;

   m_StrainScalar.Width = 5;
   m_StrainScalar.Precision = 0;
   m_StrainScalar.Format = WBFL::System::NumericFormatTool::Format::Fixed;

   m_CreepScalar.Width = 6;
   m_CreepScalar.Precision = 3;
   m_CreepScalar.Format = WBFL::System::NumericFormatTool::Format::Fixed;
}

CConcretePropertyGraphBuilder::~CConcretePropertyGraphBuilder()
{
   if ( m_pGraphController != nullptr )
   {
      delete m_pGraphController;
      m_pGraphController = nullptr;
   }

   if ( m_pTimeFormat != nullptr )
   {
      delete m_pTimeFormat;
      m_pTimeFormat = nullptr;
   }

   if ( m_pIntervalFormat != nullptr )
   {
      delete m_pIntervalFormat;
      m_pIntervalFormat = nullptr;
   }

   if ( m_pYFormat != nullptr )
   {
      delete m_pYFormat;
      m_pYFormat = nullptr;
   }

}

CEAFGraphControlWindow* CConcretePropertyGraphBuilder::GetGraphControlWindow()
{
   return m_pGraphController;
}

void CConcretePropertyGraphBuilder::CreateViewController(IEAFViewController** ppController)
{
   CComPtr<IEAFViewController> stdController;
   __super::CreateViewController(&stdController);

   CComObject<CConcretePropertiesGraphViewController>* pController;
   CComObject<CConcretePropertiesGraphViewController>::CreateInstance(&pController);
   pController->Init(m_pGraphController, stdController);

   (*ppController) = pController;
   (*ppController)->AddRef();
}

std::unique_ptr<WBFL::Graphing::GraphBuilder> CConcretePropertyGraphBuilder::Clone() const
{
   // set the module state or the commands wont route to the
   // the graph control window
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   return std::make_unique<CConcretePropertyGraphBuilder>(*this);
}

int CConcretePropertyGraphBuilder::InitializeGraphController(CWnd* pParent,UINT nID)
{
   if ( CEAFAutoCalcGraphBuilder::InitializeGraphController(pParent,nID) < 0 )
   {
      return -1;
   }

   // create the graph definitions before creating the graph controller.
   // our graph controller will call GetLoadCaseNames to populate the 
   // list of load cases
   m_pBroker = EAFGetBroker();

   // setup the graph
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);
   m_Graph.SetGridPenStyle(GRAPH_GRID_PEN_STYLE, GRAPH_GRID_PEN_WEIGHT, GRAPH_GRID_COLOR);

   m_Graph.SetTitle(_T("Concrete Properties"));

   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);

   // x axis
   m_pTimeFormat = new WBFL::Units::ScalarTool(m_Time);
   m_pIntervalFormat = new IntervalTool(m_Interval);
   m_Graph.SetXAxisValueFormat(m_pTimeFormat);
   m_Graph.SetXAxisNumberOfMajorTics(11);
   m_pGraphController->CheckDlgButton(IDC_AGE_LOG,BST_CHECKED);
   m_XAxisType = X_AXIS_AGE_LOG;

   // y axis
   const WBFL::Units::StressData& stressUnit = pDisplayUnits->GetStressUnit();
   m_pYFormat = new WBFL::Units::StressTool(stressUnit);
   m_Graph.SetYAxisValueFormat(m_pYFormat);
   m_Graph.YAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);

   // Default Graph Data
   m_pGraphController->CheckDlgButton(IDC_PRECAST_SEGMENT,BST_CHECKED);
   m_pGraphController->CheckDlgButton(IDC_FC,BST_CHECKED);
   m_GraphElement = GRAPH_ELEMENT_SEGMENT;
   m_GraphType    = GRAPH_TYPE_FC;
   m_SegmentKey = CSegmentKey(0,0,0);
   m_ClosureKey = CClosureKey(0,0,0);
   m_DeckCastingRegionIdx = 0;

   // Show the grid by default... set the control to checked
   m_pGraphController->CheckDlgButton(IDC_GRID,BST_CHECKED);
   m_Graph.DrawGrid(); // show grid by default


   return 0;
}

BOOL CConcretePropertyGraphBuilder::CreateGraphController(CWnd* pParent,UINT nID)
{
   // create our controls
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   if ( !m_pGraphController->Create(pParent,IDD_CONCRETE_PROPERTY_GRAPH_CONTROLLER, CBRS_LEFT, nID) )
   {
      TRACE0("Failed to create control bar\n");
      return FALSE; // failed to create
   }

   return TRUE;
}

void CConcretePropertyGraphBuilder::ShowGrid(bool bShowGrid)
{
   m_Graph.DrawGrid(bShowGrid);
   GetView()->Invalidate();
}

bool CConcretePropertyGraphBuilder::UpdateNow()
{
   GET_IFACE(IEAFProgress,pProgress);
   WBFL::EAF::AutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   // Update the graph control data
   m_GraphElement = m_pGraphController->GetGraphElement();
   m_GraphType    = m_pGraphController->GetGraphType();
   m_SegmentKey   = m_pGraphController->GetSegment();
   m_ClosureKey   = m_pGraphController->GetClosureJoint();
   m_DeckCastingRegionIdx = m_pGraphController->GetDeckCastingRegion();
   m_XAxisType    = m_pGraphController->GetXAxisType();

   // Update graph properties
   UpdateXAxis();
   UpdateYAxis();
   UpdateGraphTitle();
   UpdateGraphData();
   return true;
}

void CConcretePropertyGraphBuilder::UpdateXAxis()
{
   if ( m_XAxisType == X_AXIS_TIME_LINEAR )
   {
      m_Graph.SetXAxisScale(WBFL::Graphing::AxisXY::AxisScale::Linear);
      m_Graph.SetXAxisTitle(_T("Time (days)"));
      m_Graph.XAxisNiceRange(true);
      m_Graph.SetXAxisNumberOfMinorTics(10);
      m_Graph.SetXAxisValueFormat(m_pTimeFormat);
   }
   else if ( m_XAxisType == X_AXIS_TIME_LOG )
   {
      m_Graph.SetXAxisScale(WBFL::Graphing::AxisXY::AxisScale::Logarithmic);
      m_Graph.SetXAxisTitle(_T("Time (days)"));
      m_Graph.XAxisNiceRange(true);
      m_Graph.SetXAxisValueFormat(m_pTimeFormat);
   }
   else if ( m_XAxisType == X_AXIS_AGE_LINEAR )
   {
      m_Graph.SetXAxisScale(WBFL::Graphing::AxisXY::AxisScale::Linear);
      m_Graph.SetXAxisTitle(_T("Age (days)"));
      m_Graph.XAxisNiceRange(true);
      m_Graph.SetXAxisNumberOfMinorTics(10);
      m_Graph.SetXAxisValueFormat(m_pTimeFormat);
   }
   else if ( m_XAxisType == X_AXIS_AGE_LOG )
   {
      m_Graph.SetXAxisScale(WBFL::Graphing::AxisXY::AxisScale::Logarithmic);
      m_Graph.SetXAxisTitle(_T("Age (days)"));
      m_Graph.XAxisNiceRange(true);
      m_Graph.SetXAxisNumberOfMinorTics(10);
      m_Graph.SetXAxisValueFormat(m_pTimeFormat);
   }
   else
   {
      m_Graph.SetXAxisScale(WBFL::Graphing::AxisXY::AxisScale::Linear);
      m_Graph.SetXAxisTitle(_T("Interval"));
      m_Graph.XAxisNiceRange(false);
      m_Graph.SetXAxisNumberOfMinorTics(0);
      m_Graph.SetXAxisValueFormat(m_pIntervalFormat);
   }
}

void CConcretePropertyGraphBuilder::UpdateYAxis()
{
   delete m_pYFormat;

   switch(m_GraphType)
   {
   case GRAPH_TYPE_FC:
      {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      const WBFL::Units::StressData& stressUnit = pDisplayUnits->GetStressUnit();
      m_pYFormat = new WBFL::Units::StressTool(stressUnit);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      std::_tstring strYAxisTitle = _T("f'c (") + ((WBFL::Units::StressTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case GRAPH_TYPE_EC:
      {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      const WBFL::Units::StressData& stressUnit = pDisplayUnits->GetModEUnit();
      m_pYFormat = new WBFL::Units::StressTool(stressUnit);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      std::_tstring strYAxisTitle = _T("Ec (") + ((WBFL::Units::StressTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case GRAPH_TYPE_SH:
      {
      m_pYFormat = new WBFL::Units::ScalarTool(m_StrainScalar);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      std::_tstring strYAxisTitle = _T("Unrestrained Shrinkage Strain (x10^6)");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   case GRAPH_TYPE_CR:
      {
      m_pYFormat = new WBFL::Units::ScalarTool(m_CreepScalar);
      m_Graph.SetYAxisValueFormat(m_pYFormat);
      std::_tstring strYAxisTitle = _T("Creep Coefficient");
      m_Graph.SetYAxisTitle(strYAxisTitle.c_str());
      break;
      }
   default:
      ASSERT(0); 
   }
}

void CConcretePropertyGraphBuilder::UpdateGraphTitle()
{
   CString strType;
   if ( m_GraphType == GRAPH_TYPE_FC )
   {
      strType = _T("Concrete Strength");
   }
   else if ( m_GraphType == GRAPH_TYPE_EC )
   {
      strType = _T("Elastic Modulus");
   }
   else if ( m_GraphType == GRAPH_TYPE_SH )
   {
      strType = _T("Unrestrained Shrinkage Strain");
   }
   else if ( m_GraphType == GRAPH_TYPE_CR )
   {
      strType = _T("Creep Coefficients for Loads Applied in Various Intervals");
   }

   CString strElement;
   if ( m_GraphElement == GRAPH_ELEMENT_SEGMENT )
   {
      strElement.Format(_T("Group %d, Girder %s, Segment %d"),LABEL_GROUP(m_SegmentKey.groupIndex),LABEL_GIRDER(m_SegmentKey.girderIndex),LABEL_SEGMENT(m_SegmentKey.segmentIndex));
   }
   else if ( m_GraphElement == GRAPH_ELEMENT_CLOSURE )
   {
      GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
      GET_IFACE(IBridgeDescription,pIBridgeDesc);
      const CClosureJointData* pClosure = pIBridgeDesc->GetClosureJointData(m_ClosureKey);
      CString strLabel;
      if ( pClosure->GetTemporarySupport() )
      {
         strLabel = GetLabel(pClosure->GetTemporarySupport(),pDisplayUnits);
      }
      else
      {
         strLabel = GetLabel(pClosure->GetPier(),pDisplayUnits);
      }


      strElement.Format(_T("Closure Joint at %s"),strLabel);
   }
   else
   {
      ATLASSERT(m_GraphElement == GRAPH_ELEMENT_DECK);
      strElement.Format(_T("Bridge Deck, Region %d"), LABEL_INDEX(m_DeckCastingRegionIdx));
   }

   m_Graph.SetTitle(strType);
   m_Graph.SetSubtitle(strElement);
}

void CConcretePropertyGraphBuilder::UpdateGraphData()
{
   // clear graph
   m_Graph.ClearData();
   m_Graph.SetMinimumSize(m_XAxisType == X_AXIS_INTERVAL ? 1 : 0,1,0,1);

   int penWeight = GRAPH_PEN_WEIGHT;

   CString strLabel;
   if ( m_GraphType == GRAPH_TYPE_FC )
   {
      strLabel = _T("fc");
   }
   else if ( m_GraphType == GRAPH_TYPE_EC )
   {
      strLabel = _T("Ec");
   }
   else if ( m_GraphType == GRAPH_TYPE_SH )
   {
      strLabel = _T("e");
      m_Graph.SetMinimumSize(m_XAxisType == X_AXIS_INTERVAL ? 1 : 0,1,0,0);
   }
   else if ( m_GraphType == GRAPH_TYPE_CR )
   {
      strLabel = _T("Y");
   }

   GET_IFACE(IMaterials,pMaterials);
   GET_IFACE(IIntervals,pIntervals);

   IntervalIndexType releaseIntervalIdx = pIntervals->GetPrestressReleaseInterval(m_SegmentKey);
   Float64 releaseTime = pIntervals->GetTime(releaseIntervalIdx,pgsTypes::Start);
   GET_IFACE(ILossParameters,pLossParams);
   bool bIsTimeStep = pLossParams->GetLossMethod() == PrestressLossCriteria::LossMethodType::TIME_STEP ? true : false;

   IntervalIndexType nIntervals = pIntervals->GetIntervalCount();
   IntervalIndexType startIntervalIdx = 0;
   if ( m_GraphElement == GRAPH_ELEMENT_SEGMENT )
   {
      startIntervalIdx = releaseIntervalIdx;
   }
   else if ( m_GraphElement == GRAPH_ELEMENT_CLOSURE )
   {
      startIntervalIdx = pIntervals->GetCompositeClosureJointInterval(m_ClosureKey);
   }
   else
   {
      startIntervalIdx = pIntervals->GetCompositeDeckInterval(m_DeckCastingRegionIdx);
   }

   std::vector<IntervalIndexType> vIntervals;
   if ( m_GraphType == GRAPH_TYPE_CR )
   {
      vIntervals.resize(nIntervals-startIntervalIdx);
      std::generate(vIntervals.begin(),vIntervals.end(),IncrementValue<IntervalIndexType>(startIntervalIdx));
   }
   else
   {
      vIntervals.push_back(startIntervalIdx);
   }

   WBFL::Graphing::GraphColor graphColor;

   if ( m_XAxisType == X_AXIS_INTERVAL )
   {
      m_Graph.SetXAxisForcedRange((Float64)LABEL_INTERVAL(startIntervalIdx),(Float64)LABEL_INTERVAL(nIntervals),0.5);
      IntervalTool* pIntervalTool = dynamic_cast<IntervalTool*>(m_pIntervalFormat);
      pIntervalTool->SetLastValue((Float64)LABEL_INTERVAL(nIntervals));
   }

   for (const auto& iIdx : vIntervals)
   {
      IndexType dataSeries;
      if ( m_GraphType == GRAPH_TYPE_CR )
      {
         CString strGraphLabel;
         strGraphLabel.Format(_T("Interval %d"),LABEL_INTERVAL(iIdx));
         dataSeries = m_Graph.CreateDataSeries(strGraphLabel, PS_SOLID, penWeight, graphColor.GetColor(iIdx-vIntervals.front()));
      }
      else
      {
         dataSeries = m_Graph.CreateDataSeries(strLabel, PS_SOLID, penWeight, graphColor.GetColor(iIdx-vIntervals.front()));
      }

      for ( IntervalIndexType intervalIdx = iIdx; intervalIdx < nIntervals; intervalIdx++ )
      {
         bool bConcreteStep = false;
         Float64 xStart;
         Float64 xStep;
         Float64 xMiddle;
         Float64 xEnd;
         if ( m_XAxisType == X_AXIS_TIME_LINEAR || m_XAxisType == X_AXIS_TIME_LOG )
         {
            xStart  = pIntervals->GetTime(intervalIdx,pgsTypes::Start);
            xMiddle = pIntervals->GetTime(intervalIdx,pgsTypes::Middle);
            xEnd    = pIntervals->GetTime(intervalIdx,pgsTypes::End);
         }
         else if ( m_XAxisType == X_AXIS_AGE_LINEAR || m_XAxisType == X_AXIS_AGE_LOG )
         {
            if ( m_GraphElement == GRAPH_ELEMENT_SEGMENT )
            {
               xStart  = pMaterials->GetSegmentConcreteAge(m_SegmentKey,intervalIdx,pgsTypes::Start);
               xMiddle = pMaterials->GetSegmentConcreteAge(m_SegmentKey,intervalIdx,pgsTypes::Middle);
               xEnd    = pMaterials->GetSegmentConcreteAge(m_SegmentKey,intervalIdx,pgsTypes::End);
            }
            else if ( m_GraphElement == GRAPH_ELEMENT_CLOSURE )
            {
               xStart  = pMaterials->GetClosureJointConcreteAge(m_ClosureKey,intervalIdx,pgsTypes::Start);
               xMiddle = pMaterials->GetClosureJointConcreteAge(m_ClosureKey,intervalIdx,pgsTypes::Middle);
               xEnd    = pMaterials->GetClosureJointConcreteAge(m_ClosureKey,intervalIdx,pgsTypes::End);
            }
            else
            {
               xStart  = pMaterials->GetDeckConcreteAge(m_DeckCastingRegionIdx,intervalIdx,pgsTypes::Start);
               xMiddle = pMaterials->GetDeckConcreteAge(m_DeckCastingRegionIdx, intervalIdx,pgsTypes::Middle);
               xEnd    = pMaterials->GetDeckConcreteAge(m_DeckCastingRegionIdx, intervalIdx,pgsTypes::End);
            }
         }
         else
         {
            xStart  = (Float64)LABEL_INTERVAL(intervalIdx);
            xMiddle = (Float64)LABEL_INTERVAL(intervalIdx) + 0.5;
            xEnd    = (Float64)LABEL_INTERVAL(intervalIdx+1);
         }

         if ( m_GraphElement == GRAPH_ELEMENT_SEGMENT && !bIsTimeStep && (m_GraphType == GRAPH_TYPE_FC || m_GraphType == GRAPH_TYPE_EC) && xStart <= releaseTime && releaseTime + 28.0 <= xEnd )
         {
            xStep = releaseTime + 28.0;
            xMiddle = xStep;
            bConcreteStep = true;
         }


         // this is value at middle of interval...
         Float64 startValue;
         Float64 middleValue,stepValue;
         Float64 endValue;
         if ( m_GraphElement == GRAPH_ELEMENT_SEGMENT )
         {
            if ( m_GraphType == GRAPH_TYPE_FC )
            {
               startValue  = pMaterials->GetSegmentFc(m_SegmentKey,intervalIdx,pgsTypes::Start);
               if ( bConcreteStep )
               {
                  stepValue = startValue;
               }
               middleValue = pMaterials->GetSegmentFc(m_SegmentKey,intervalIdx,pgsTypes::Middle);
               endValue    = pMaterials->GetSegmentFc(m_SegmentKey,intervalIdx,pgsTypes::End);
            }
            else if ( m_GraphType == GRAPH_TYPE_EC )
            {
               startValue  = pMaterials->GetSegmentEc(m_SegmentKey,intervalIdx,pgsTypes::Start);
               if ( bConcreteStep )
               {
                  stepValue = startValue;
               }
               middleValue = pMaterials->GetSegmentEc(m_SegmentKey,intervalIdx,pgsTypes::Middle);
               endValue    = pMaterials->GetSegmentEc(m_SegmentKey,intervalIdx,pgsTypes::End);
            }
            else if ( m_GraphType == GRAPH_TYPE_SH )
            {
               // shrinkage strain is a "compressive" strain so it's value is less than zero
               // for plotting, we like it to be > 0 so multiple by -1e6 (this makes it micro strains)
               startValue  = -1e6*pMaterials->GetTotalSegmentFreeShrinkageStrain(m_SegmentKey,intervalIdx,pgsTypes::Start);
               middleValue = -1e6*pMaterials->GetTotalSegmentFreeShrinkageStrain(m_SegmentKey,intervalIdx,pgsTypes::Middle);
               endValue    = -1e6*pMaterials->GetTotalSegmentFreeShrinkageStrain(m_SegmentKey,intervalIdx,pgsTypes::End);
            }
            else if ( m_GraphType == GRAPH_TYPE_CR )
            {
               if ( iIdx == intervalIdx )
               {
                  startValue  = pMaterials->GetSegmentCreepCoefficient(m_SegmentKey,iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Middle);
                  xStart = xMiddle;
               }
               else
               {
                  startValue  = pMaterials->GetSegmentCreepCoefficient(m_SegmentKey,iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
               }
               middleValue = pMaterials->GetSegmentCreepCoefficient(m_SegmentKey,iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Middle);
               endValue    = pMaterials->GetSegmentCreepCoefficient(m_SegmentKey,iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
            }
         }
         else if ( m_GraphElement == GRAPH_ELEMENT_CLOSURE )
         {
            if ( m_GraphType == GRAPH_TYPE_FC )
            {
               startValue  = pMaterials->GetClosureJointFc(m_ClosureKey,intervalIdx,pgsTypes::Start);
               middleValue = pMaterials->GetClosureJointFc(m_ClosureKey,intervalIdx,pgsTypes::Middle);
               endValue    = pMaterials->GetClosureJointFc(m_ClosureKey,intervalIdx,pgsTypes::End);
            }
            else if ( m_GraphType == GRAPH_TYPE_EC )
            {
               startValue  = pMaterials->GetClosureJointEc(m_ClosureKey,intervalIdx,pgsTypes::Start);
               middleValue = pMaterials->GetClosureJointEc(m_ClosureKey,intervalIdx,pgsTypes::Middle);
               endValue    = pMaterials->GetClosureJointEc(m_ClosureKey,intervalIdx,pgsTypes::End);
            }
            else if ( m_GraphType == GRAPH_TYPE_SH )
            {
               startValue  = -1e6*pMaterials->GetTotalClosureJointFreeShrinkageStrain(m_ClosureKey,intervalIdx,pgsTypes::Start);
               middleValue = -1e6*pMaterials->GetTotalClosureJointFreeShrinkageStrain(m_ClosureKey,intervalIdx,pgsTypes::Middle);
               endValue    = -1e6*pMaterials->GetTotalClosureJointFreeShrinkageStrain(m_ClosureKey,intervalIdx,pgsTypes::End);
            }
            else if ( m_GraphType == GRAPH_TYPE_CR )
            {
               if ( iIdx == intervalIdx )
               {
                  startValue  = pMaterials->GetClosureJointCreepCoefficient(m_ClosureKey,iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Middle);
                  xStart = xMiddle;
               }
               else
               {
                  startValue  = pMaterials->GetClosureJointCreepCoefficient(m_ClosureKey,iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
               }
               middleValue = pMaterials->GetClosureJointCreepCoefficient(m_ClosureKey,iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Middle);
               endValue    = pMaterials->GetClosureJointCreepCoefficient(m_ClosureKey,iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
            }
         }
         else if ( m_GraphElement = GRAPH_ELEMENT_DECK )
         {
            if ( m_GraphType == GRAPH_TYPE_FC )
            {
               startValue  = pMaterials->GetDeckFc(m_DeckCastingRegionIdx, intervalIdx,pgsTypes::Start);
               middleValue = pMaterials->GetDeckFc(m_DeckCastingRegionIdx, intervalIdx,pgsTypes::Middle);
               endValue    = pMaterials->GetDeckFc(m_DeckCastingRegionIdx, intervalIdx,pgsTypes::End);
            }
            else if ( m_GraphType == GRAPH_TYPE_EC )
            {
               startValue  = pMaterials->GetDeckEc(m_DeckCastingRegionIdx, intervalIdx,pgsTypes::Start);
               middleValue = pMaterials->GetDeckEc(m_DeckCastingRegionIdx, intervalIdx,pgsTypes::Middle);
               endValue    = pMaterials->GetDeckEc(m_DeckCastingRegionIdx, intervalIdx,pgsTypes::End);
            }
            else if ( m_GraphType == GRAPH_TYPE_SH )
            {
               startValue  = -1e6*pMaterials->GetTotalDeckFreeShrinkageStrain(m_DeckCastingRegionIdx, intervalIdx,pgsTypes::Start);
               middleValue = -1e6*pMaterials->GetTotalDeckFreeShrinkageStrain(m_DeckCastingRegionIdx, intervalIdx,pgsTypes::Middle);
               endValue    = -1e6*pMaterials->GetTotalDeckFreeShrinkageStrain(m_DeckCastingRegionIdx, intervalIdx,pgsTypes::End);
            }
            else if ( m_GraphType == GRAPH_TYPE_CR )
            {
               if ( iIdx == intervalIdx )
               {
                  startValue  = pMaterials->GetDeckCreepCoefficient(m_DeckCastingRegionIdx, iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Middle);
                  xStart = xMiddle;
               }
               else
               {
                  startValue  = pMaterials->GetDeckCreepCoefficient(m_DeckCastingRegionIdx, iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Start);
               }
               middleValue = pMaterials->GetDeckCreepCoefficient(m_DeckCastingRegionIdx, iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::Middle);
               endValue    = pMaterials->GetDeckCreepCoefficient(m_DeckCastingRegionIdx, iIdx,pgsTypes::Middle,intervalIdx,pgsTypes::End);
            }
         }
         AddGraphPoint(dataSeries,xStart, startValue);
         if ( bConcreteStep )
         {
            AddGraphPoint(dataSeries,xStep, stepValue);
         }
         AddGraphPoint(dataSeries,xMiddle,middleValue);
         AddGraphPoint(dataSeries,xEnd,   endValue);
      }
   }
}

void CConcretePropertyGraphBuilder::AddGraphPoint(IndexType series, Float64 xval, Float64 yval)
{
   // deal with unit conversion
   const WBFL::Units::PhysicalConverter* pcx = dynamic_cast<const WBFL::Units::PhysicalConverter*>(m_Graph.GetXAxisValueFormat());
   ASSERT(pcx);
   const WBFL::Units::PhysicalConverter* pcy = dynamic_cast<const WBFL::Units::PhysicalConverter*>(m_Graph.GetYAxisValueFormat());
   ASSERT(pcy);
   Float64 x = pcx->Convert(xval);
   Float64 y = pcy->Convert(yval);
   m_Graph.AddPoint(series, WBFL::Graphing::Point(x,y));
}

void CConcretePropertyGraphBuilder::DrawGraphNow(CWnd* pGraphWnd,CDC* pDC)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int save = pDC->SaveDC();

   // The graph is valided and there was not an error
   // updating data.... draw the graph
   CRect rect = GetView()->GetDrawingRect();

   m_Graph.SetOutputRect(rect);
   m_Graph.UpdateGraphMetrics(pDC->GetSafeHdc());
   m_Graph.Draw(pDC->GetSafeHdc());

   pDC->RestoreDC(save);
}

void CConcretePropertyGraphBuilder::ExportGraphData(LPCTSTR rstrDefaultFileName)
{
   CExportGraphXYTool::ExportGraphData(m_Graph,rstrDefaultFileName);
}
