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

// AnalysisResultsView.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperDoc.h"
#include "PGSuperException.h"
#include "AnalysisResultsView.h"

#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Allowables.h>
#include <IFace\MomentCapacity.h>
#include <IFace\ShearCapacity.h>
#include <PgsExt\PointOfInterest.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>
#include <PgsExt\BridgeDescription.h>
#include "PGSuperCalculationSheet.h"
#include <MfcTools\Text.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


// some styles
static const COLORREF GRAPH_BACKGROUND = RGB(220,255,220);
static const COLORREF GRID_COLOR       = RGB(0,150,0);
static const COLORREF BEAM_COLOR       = RGB(200,200,200);

// create a dummy unit conversion tool to pacify the graph constructor
static unitmgtLengthData DUMMY(unitMeasure::Meter);
static LengthTool    DUMMY_TOOL(DUMMY);

// Pen styles for stresses at top and bottom of girder
#define PS_STRESS_TOP     PS_SOLID
#define PS_STRESS_BOTTOM  PS_DASH


/////////////////////////////////////////////////////////////////////////////
// CAnalysisResultsView

IMPLEMENT_DYNCREATE(CAnalysisResultsView, CView)

CAnalysisResultsView::CAnalysisResultsView():
CEAFAutoCalcViewMixin(this),
m_bValidGraph(false),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_pXFormat(0),
m_pYFormat(0),
m_pBroker(0),
m_IsPrinting(false)
{
   m_bUpdateError = false;
}

CAnalysisResultsView::~CAnalysisResultsView()
{
   delete m_pXFormat;
   delete m_pYFormat;

   if ( m_pBroker != 0 )
      m_pBroker->Release();
}


BEGIN_MESSAGE_MAP(CAnalysisResultsView, CView)
	//{{AFX_MSG_MAP(CAnalysisResultsView)
	ON_WM_CREATE()
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_DIRECT, OnUpdateFilePrint)
   ON_COMMAND(ID_DUMP_LBAM,DumpLBAM)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAnalysisResultsView drawing

void CAnalysisResultsView::OnDraw(CDC* pDC)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int save = pDC->SaveDC();

   // deal with printing and reentrant behavior
   if(m_IsPrinting)
      return;

   CString msg;
   if ((m_bValidGraph) && !m_bUpdateError)
   {
      // The graph is valided and there was not an error
      // updating data.... draw the graph
      CRect rect;
      if (pDC->IsPrinting())
      {
         rect = m_PrintRect;
         m_IsPrinting = true;
      }
      else
      {
         GetClientRect(&rect);
      }

      m_Graph.SetOutputRect(rect);
      m_Graph.UpdateGraphMetrics(pDC->GetSafeHdc());
      m_Graph.DrawBackground(pDC->GetSafeHdc());

      // superimpose beam on graph
      // do it before the graph so the graph draws on top of it
      DrawBeam(pDC);

      m_Graph.DrawDataSeries(pDC->GetSafeHdc());

      // window text for child frame
      SetWindowText(m_Graph.GetTitle().c_str());
   }
   else
   {
      // There was an error and data isn't available or data isn't
      // available because autocalc is turned off and the results
      // haven't been updated yet.... either way, there is nothing to draw.

      // Display a message to indicate to the user the state of the results

      if ( m_bUpdateError )
         AfxFormatString1(msg,IDS_E_UPDATE,m_ErrorMsg.c_str());
      else
         msg.LoadString(IDS_RESULTS_NOT_AVAILABLE);

      CFont font;
      CFont* pOldFont = NULL;
      if ( font.CreatePointFont(100,_T("Arial"),pDC) )
         pOldFont = pDC->SelectObject(&font);

      MultiLineTextOut(pDC,0,0,msg);

      if ( pOldFont )
         pDC->SelectObject(pOldFont);

      // give child frame a reasonable default
      SetWindowText(_T("Analysis Results"));
   }

   pDC->RestoreDC(save);

   if (m_IsPrinting)
   {
      m_IsPrinting = false;
      Invalidate();
   }
   else
   {
      // update the child frame title
      m_pFrame->OnUpdateFrameTitle(TRUE);
   }
}

void CAnalysisResultsView::DrawBeam(CDC* pDC)
{
   if ( m_pFrame->GetGraphCount() == 0 )
      return; // nothing to graph

   pgsTypes::Stage stage = m_pFrame->GetStage();

   GET_IFACE(IBridge,pBridge);
   GET_IFACE(IBridgeDescription,pIBridgeDesc);

   const CBridgeDescription* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   SpanIndexType span = m_pFrame->GetSpanIdx();
   // span < 0 = all spans
   SpanIndexType startSpan = (span == ALL_SPANS ? 0 : span);
   SpanIndexType nSpans    = (span == ALL_SPANS ? pBridge->GetSpanCount() : 1);

   GirderIndexType girder = m_pFrame->GetGirderIdx();

   const grlibPointMapper& mapper = m_Graph.GetClientAreaPointMapper(pDC->GetSafeHdc());

   CBrush beam_brush;
   beam_brush.CreateSolidBrush(BEAM_COLOR);
   CBrush* pold_brush = pDC->SelectObject(&beam_brush );

   gpSize2d  ext = mapper.GetWorldExt();

   if (m_IsPrinting)
      m_SupportSize = CSize(20,20);
   else
      m_SupportSize = CSize(5,5);

   double x,y;
   double x_start = 0;
   SpanIndexType spanIdx;
   for ( spanIdx = 0; spanIdx < startSpan; spanIdx++ )
   {
      // deal with girder index when there are different number of girders in each span
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(girder,nGirders-1);

      double span_length = (stage == pgsTypes::CastingYard ? pBridge->GetGirderLength(spanIdx,gdrIdx) : pBridge->GetSpanLength(spanIdx,gdrIdx));
      arvPhysicalConverter* pcx = dynamic_cast<arvPhysicalConverter*>(m_pXFormat);
      span_length = pcx->Convert(span_length);
      x_start += span_length;
   }

   for ( spanIdx = startSpan; spanIdx < startSpan+nSpans; spanIdx++ )
   {
      // deal with girder index when there are different number of girders in each span
      GirderIndexType nGirders = pBridge->GetGirderCount(spanIdx);
      GirderIndexType gdrIdx = min(girder,nGirders-1);

      // draw beam
      double span_length = (stage == pgsTypes::CastingYard ? pBridge->GetGirderLength(spanIdx,gdrIdx) : pBridge->GetSpanLength(spanIdx,gdrIdx));
      arvPhysicalConverter* pcx = dynamic_cast<arvPhysicalConverter*>(m_pXFormat);
      span_length = pcx->Convert(span_length);

      CPoint points[4];
      x = x_start;
      y = 0;
      mapper.WPtoDP(x,y,&points[0].x,&points[0].y);

      x = x_start + span_length;
      mapper.WPtoDP(x,y,&points[1].x,&points[1].y);

      points[2] = points[1];
      points[2].y += m_SupportSize.cy;

      points[3] = points[0];
      points[3].y += m_SupportSize.cy;

      pDC->Polygon(points, 4);

      // draw the left support
      PierIndexType pierIdx = spanIdx;
      const CPierData* pPier = pBridgeDesc->GetPier(pierIdx);
      pgsTypes::PierConnectionType connectionType = pPier->GetConnectionType();

      DrawSupport(stage,connectionType,points[3],pDC);

      x_start += span_length;
   }

   // draw last support
   PierIndexType pierIdx = pBridgeDesc->GetPierCount()-1;
   const CPierData* pLastPier = pBridgeDesc->GetPier(pierIdx);
   pgsTypes::PierConnectionType connectionType = pLastPier->GetConnectionType();

   CPoint p;
   mapper.WPtoDP(x_start,0.0,&p.x,&p.y);
   p.y += m_SupportSize.cy;

   DrawSupport(stage,connectionType,p,pDC);

   pDC->SelectObject(pold_brush);
}

void CAnalysisResultsView::DrawSupport(pgsTypes::Stage stage,pgsTypes::PierConnectionType supportType,CPoint p,CDC* pDC)
{
   if ( stage == pgsTypes::CastingYard )
      supportType = pgsTypes::Hinged;

   if ( supportType == pgsTypes::Roller )
   {
      DrawRoller(p,pDC);
   }
   else if ( supportType == pgsTypes::Hinged )
   {
      DrawHinge(p,pDC);
   }
   else if ( supportType == pgsTypes::ContinuousBeforeDeck )
   {
      if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 )
         DrawContinuous(p,pDC);
      else
         DrawHinge(p,pDC);
   }
   else if ( supportType == pgsTypes::ContinuousAfterDeck )
   {
      if ( stage == pgsTypes::BridgeSite3 )
         DrawContinuous(p,pDC);
      else
         DrawHinge(p,pDC);
   }
   else if ( supportType == pgsTypes::IntegralBeforeDeck )
   {
      if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 )
         DrawIntegral(p,pDC);
      else
         DrawHinge(p,pDC);
   }
   else if ( supportType == pgsTypes::IntegralAfterDeck )
   {
      if ( stage == pgsTypes::BridgeSite3 )
         DrawIntegral(p,pDC);
      else
         DrawHinge(p,pDC);
   }
   else if ( supportType == pgsTypes::IntegralBeforeDeckHingeBack )
   {
      if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 )
         DrawIntegralHingeBack(p,pDC);
      else
         DrawHinge(p,pDC);
   }
   else if ( supportType == pgsTypes::IntegralAfterDeckHingeBack )
   {
      if ( stage == pgsTypes::BridgeSite3 )
         DrawIntegralHingeBack(p,pDC);
      else
         DrawHinge(p,pDC);
   }
   else if ( supportType == pgsTypes::IntegralBeforeDeckHingeAhead )
   {
      if ( stage == pgsTypes::BridgeSite2 || stage == pgsTypes::BridgeSite3 )
         DrawIntegralHingeAhead(p,pDC);
      else
         DrawHinge(p,pDC);
   }
   else if ( supportType == pgsTypes::IntegralAfterDeckHingeAhead )
   {
      if ( stage == pgsTypes::BridgeSite3 )
         DrawIntegralHingeAhead(p,pDC);
      else
         DrawHinge(p,pDC);
   }
}

void CAnalysisResultsView::DrawRoller(CPoint p,CDC* pDC)
{
   CRect circle;
   circle.left = p.x;
   circle.right = p.x;
   circle.top = p.y + m_SupportSize.cy;
   circle.bottom = p.y + m_SupportSize.cy;
   circle.InflateRect(m_SupportSize);
   pDC->Ellipse(circle);
}

void CAnalysisResultsView::DrawHinge(CPoint p,CDC* pDC)
{
   DrawContinuous(p,pDC);
   p.y -= m_SupportSize.cy;
   DrawRoller(p,pDC);
}

void CAnalysisResultsView::DrawContinuous(CPoint p,CDC* pDC)
{
   CPoint polyPnts[3];
   polyPnts[0] = p;

   polyPnts[1].x = polyPnts[0].x -   m_SupportSize.cx;
   polyPnts[1].y = polyPnts[0].y + 2*m_SupportSize.cy;

   polyPnts[2].x = polyPnts[0].x +   m_SupportSize.cx;
   polyPnts[2].y = polyPnts[0].y + 2*m_SupportSize.cy;

   pDC->Polygon(polyPnts,3);
}

void CAnalysisResultsView::DrawIntegral(CPoint p,CDC* pDC)
{
   CRect box;
   box.left = p.x;
   box.right = p.x;
   box.top = p.y + m_SupportSize.cy;
   box.bottom = p.y + m_SupportSize.cy;
   box.InflateRect(m_SupportSize);
   pDC->Rectangle(box);
}

void CAnalysisResultsView::DrawIntegralHingeBack(CPoint p,CDC* pDC)
{
   DrawIntegral(p,pDC);
   p.x -= m_SupportSize.cx;
   p.y -= m_SupportSize.cy;
   DrawRoller(p,pDC);
}

void CAnalysisResultsView::DrawIntegralHingeAhead(CPoint p,CDC* pDC)
{
   DrawIntegral(p,pDC);
   p.x += m_SupportSize.cx;
   p.y -= m_SupportSize.cy;
   DrawRoller(p,pDC);
}

void CAnalysisResultsView::DumpLBAM()
{
   // Alt + Ctrl + L
   GET_IFACE(IProductForces,pProductForces);
   pProductForces->DumpAnalysisModels(m_pFrame->GetGirderIdx());
   AfxMessageBox(_T("Analysis Model Dump Complete"),MB_OK);
}

/////////////////////////////////////////////////////////////////////////////
// CAnalysisResultsView diagnostics

#ifdef _DEBUG
void CAnalysisResultsView::AssertValid() const
{
	CView::AssertValid();
}

void CAnalysisResultsView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CAnalysisResultsView message handlers
bool CAnalysisResultsView::DoResultsExist() const
{
   return m_bValidGraph;
}

void CAnalysisResultsView::DoUpdateNow()
{
   PRECONDITION(m_pBroker);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   pProgress->UpdateMessage(_T("Building Graph"));

   CWaitCursor wait;

   // clear graph
   m_Graph.ClearData();
   m_bValidGraph = false;

   // get view data from child frame class
   pgsTypes::Stage stage  = m_pFrame->GetStage();
   GirderIndexType girder = m_pFrame->GetGirderIdx();
   SpanIndexType   span   = m_pFrame->GetSpanIdx();

   // Get the points of interest we need.
   GET_IFACE2(m_pBroker,IPointOfInterest,pIPoi);
   std::vector<pgsPointOfInterest> vPoi = pIPoi->GetPointsOfInterest( span, girder, stage, POI_GRAPHICAL );

   // response action - moment, shear, displacement, or stress
   ActionType action = m_pFrame->GetAction();

   // Set up the graph
   UpdateUnits(action);
   UpdateXAxisTitle(stage);
   UpdateGraphTitle(span,girder,stage,action);
   UpdateGrid();

   int nGraphs = m_pFrame->GetGraphCount();
   for ( int graphIdx = 0; graphIdx < nGraphs; graphIdx++ )
   {
      GraphType graph_type = m_pFrame->GetGraphType(graphIdx);
      switch( graph_type )
      {
      case graphCombined:
         CombinedLoadGraph(graphIdx,stage,action,vPoi);
         break;

      case graphLiveLoad:
         LiveLoadGraph(graphIdx,stage,action,vPoi);
         break;

      case graphVehicularLiveLoad:
         VehicularLiveLoadGraph(graphIdx,stage,action,vPoi);
         break;

      case graphProduct:
         ProductLoadGraph(graphIdx,stage,action,vPoi);
         break;

      case graphPrestress:
         // always use casting yard POI when plotting prestress deflection
         if ( action == actionDisplacement )
            vPoi = pIPoi->GetPointsOfInterest( span, girder, pgsTypes::CastingYard, POI_GRAPHICAL );

         PrestressLoadGraph(graphIdx,stage,action,vPoi);
         break;

      case graphLimitState:
      case graphDemand:
      case graphAllowable:
      case graphCapacity:
      case graphMinCapacity:
         LimitStateLoadGraph(graphIdx,stage,action,vPoi);
         break;

      default:
         ASSERT(false); // should never get here
      }
   }
 
   // set flags that graph is built and up to date
   m_bValidGraph       = true;
   m_bUpdateError      = false;

   // time to redraw
   Invalidate();
   UpdateWindow();
}

void CAnalysisResultsView::LimitStateLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPoi)
{
   pgsTypes::LimitState limit_state = m_pFrame->GetLimitState(graphIdx);
   GraphType graph_type = m_pFrame->GetGraphType(graphIdx);

   GET_IFACE(IPointOfInterest,pIPOI);
   GET_IFACE(IBridge,pBridge);

   // Combined forces
   GET_IFACE(ILimitStateForces2,pForces);

   CString strDataLabel(m_pFrame->GetGraphDataLabel(graphIdx));

   pgsTypes::AnalysisType analysis_type = m_pFrame->GetAnalysisType();

   COLORREF c = m_pFrame->GetGraphColor(graphIdx);
   int pen_size = (graph_type == graphAllowable || graph_type == graphCapacity ? 3 : 1);

   // data series for moment, shears and deflections
   Uint32 max_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,pen_size,c);
   Uint32 min_data_series = m_Graph.CreateDataSeries(_T(""),          PS_SOLID,pen_size,c);

   // data series for stresses
   Uint32 stress_top_max = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),    PS_STRESS_TOP,   1,c);
   Uint32 stress_top_min = m_Graph.CreateDataSeries(_T(""),                       PS_STRESS_TOP,   1,c);
   Uint32 stress_bot_max = m_Graph.CreateDataSeries(strDataLabel+_T(" -  Bottom"),PS_STRESS_BOTTOM,1,c);
   Uint32 stress_bot_min = m_Graph.CreateDataSeries(_T(""),                       PS_STRESS_BOTTOM,1,c);

   std::vector<Float64> xVals;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 loc;
      if ( stage == pgsTypes::CastingYard )
         loc = poi.GetDistFromStart();
      else
         loc = pIPOI->GetDistanceFromFirstPier(poi,stage);

      xVals.push_back(loc);
   }

   switch(action)
   {
   case actionMoment:
      {
         if ( graph_type == graphCapacity )
         {
            GET_IFACE(IMomentCapacity,pCapacity);
            std::vector<Float64> pMn = pCapacity->GetMomentCapacity(stage,vPoi,true);
            AddGraphPoints(max_data_series, xVals, pMn);

            std::vector<Float64> nMn = pCapacity->GetMomentCapacity(stage,vPoi,false);
            AddGraphPoints(min_data_series, xVals, nMn);
         }
         else if ( graph_type == graphMinCapacity )
         {
            GET_IFACE(IMomentCapacity,pCapacity);
            std::vector<Float64> pMrMin = pCapacity->GetMinMomentCapacity(stage,vPoi,true);
            AddGraphPoints(max_data_series, xVals, pMrMin);

            std::vector<Float64> nMrMin = pCapacity->GetMinMomentCapacity(stage,vPoi,false);
            AddGraphPoints(min_data_series, xVals, nMrMin);
         }
         else
         {
            if ( analysis_type == pgsTypes::Envelope )
            {
               std::vector<Float64> mmax, mmin;
               pForces->GetMoment( limit_state, stage, vPoi, MaxSimpleContinuousEnvelope, &mmin, &mmax );
               AddGraphPoints(max_data_series, xVals, mmax);
               
               if ( stage == pgsTypes::BridgeSite3 && IsStrengthLimitState(limit_state) )
                  mmin = pForces->GetSlabDesignMoment(limit_state,vPoi, MinSimpleContinuousEnvelope );
               else
                  pForces->GetMoment( limit_state, stage, vPoi, MinSimpleContinuousEnvelope, &mmin, &mmax );

               AddGraphPoints(min_data_series, xVals, mmin);
            }
            else
            {
               std::vector<Float64> mmax, mmin;
               pForces->GetMoment( limit_state, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &mmin, &mmax );
               AddGraphPoints(max_data_series, xVals, mmax);

               if ( stage == pgsTypes::BridgeSite3 && IsStrengthLimitState(limit_state) )
                  mmin = pForces->GetSlabDesignMoment(limit_state,vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan );

               AddGraphPoints(min_data_series, xVals, mmin);
            }
         }
      break;
      }
   case actionShear:
      {
         if ( graph_type == graphCapacity )
         {
            GET_IFACE(IShearCapacity,pCapacity);
            std::vector<Float64> pVn = pCapacity->GetShearCapacity(limit_state, stage, vPoi);
            AddGraphPoints(max_data_series, xVals,  pVn);

            std::vector<Float64>::iterator iter;
            std::vector<Float64> nVn;
            for ( iter = pVn.begin(); iter != pVn.end(); iter++ )
            {
               nVn.push_back(-1*(*iter));
            }
            AddGraphPoints(min_data_series, xVals, nVn);
         }
         else
         {
            if ( analysis_type == pgsTypes::Envelope )
            {
               std::vector<sysSectionValue> shearmn, shearmx;
               pForces->GetShear( limit_state, stage, vPoi, MinSimpleContinuousEnvelope, &shearmn, &shearmx );
               AddGraphPoints(min_data_series, xVals, shearmn);

               pForces->GetShear( limit_state, stage, vPoi, MaxSimpleContinuousEnvelope, &shearmn, &shearmx );
               AddGraphPoints(max_data_series, xVals, shearmx);
            }
            else
            {
               std::vector<sysSectionValue> shearmn, shearmx;
               pForces->GetShear( limit_state, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &shearmn, &shearmx );
               AddGraphPoints(min_data_series, xVals, shearmn);
               AddGraphPoints(max_data_series, xVals, shearmx);
            }
         }
      break;
      }
   case actionDisplacement:
      {
         if ( analysis_type == pgsTypes::Envelope )
         {
            std::vector<Float64> dispmn, dispmx;
            pForces->GetDisplacement( limit_state, stage, vPoi, MinSimpleContinuousEnvelope, &dispmn, &dispmx);
            AddGraphPoints(min_data_series, xVals, dispmn);

            pForces->GetDisplacement( limit_state, stage, vPoi, MaxSimpleContinuousEnvelope, &dispmn, &dispmx);
            AddGraphPoints(max_data_series, xVals, dispmx);
         }
         else
         {
            std::vector<Float64> dispmn, dispmx;
            pForces->GetDisplacement( limit_state, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &dispmn, &dispmx);
            AddGraphPoints(min_data_series, xVals, dispmn);
            AddGraphPoints(max_data_series, xVals, dispmx);
         }
      break;
      }
   case actionStress:
      {
         GET_IFACE(IAllowableConcreteStress,pAllowable);
         pgsTypes::LimitState ls_type = m_pFrame->GetLimitState(graphIdx);

         if ( graph_type == graphAllowable )
         {
            std::vector<Float64> c,t;

            if ( stage == pgsTypes::CastingYard            || 
                 stage == pgsTypes::GirderPlacement        || 
                 stage == pgsTypes::TemporaryStrandRemoval || 
                 stage == pgsTypes::BridgeSite1 )
            {
               c = pAllowable->GetAllowableStress(vPoi,stage,ls_type,pgsTypes::Compression);
               AddGraphPoints(max_data_series, xVals, c);

               t = pAllowable->GetAllowableStress(vPoi,stage,ls_type,pgsTypes::Tension);
               AddGraphPoints(min_data_series, xVals, t);
            }
            else if ( stage == pgsTypes::BridgeSite2 )
            {
               c = pAllowable->GetAllowableStress(vPoi,stage,ls_type,pgsTypes::Compression);
               AddGraphPoints(max_data_series, xVals, c);
            }
            else if ( stage == pgsTypes::BridgeSite3 )
            {
               if ( ls_type == pgsTypes::ServiceI || ls_type == pgsTypes::ServiceIA || ls_type == pgsTypes::FatigueI )
               {
                  c = pAllowable->GetAllowableStress(vPoi,stage,ls_type,pgsTypes::Compression);
                  AddGraphPoints(max_data_series, xVals, c);
               }
               else
               {
                  t = pAllowable->GetAllowableStress(vPoi,stage,ls_type,pgsTypes::Tension);
                  AddGraphPoints(max_data_series, xVals, t);
               }
            }
            else
            {
               ATLASSERT(false); // who added a stage and didn't tell me?
            }
         }
         else
         {
            bool bIncPrestress = (graph_type == graphDemand ? true : false);
            std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;

            if ( analysis_type == pgsTypes::Envelope )
            {
               pForces->GetStress( limit_state, stage, vPoi, pgsTypes::TopGirder,    bIncPrestress, MaxSimpleContinuousEnvelope, &fTopMin, &fTopMax);
               pForces->GetStress( limit_state, stage, vPoi, pgsTypes::BottomGirder, bIncPrestress, MaxSimpleContinuousEnvelope, &fBotMin, &fBotMax);
            }
            else
            {
               pForces->GetStress( limit_state, stage, vPoi, pgsTypes::TopGirder,    bIncPrestress, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMin, &fTopMax);
               pForces->GetStress( limit_state, stage, vPoi, pgsTypes::BottomGirder, bIncPrestress, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fBotMin, &fBotMax);
            }

            if ( stage == pgsTypes::CastingYard            || 
                 stage == pgsTypes::GirderPlacement        ||
                 stage == pgsTypes::TemporaryStrandRemoval ||
                 stage == pgsTypes::BridgeSite1 )
            {
               AddGraphPoints(stress_top_max, xVals, fTopMax);
               AddGraphPoints(stress_bot_max, xVals, fBotMax);
            }
            else if ( stage == pgsTypes::BridgeSite2 )
            {
               AddGraphPoints(stress_top_max, xVals, fTopMax);
               AddGraphPoints(stress_bot_min, xVals, fBotMin);
            }
            else if ( stage == pgsTypes::BridgeSite3 )
            {
               if ( ls_type == pgsTypes::ServiceI || ls_type == pgsTypes::ServiceIA || ls_type == pgsTypes::FatigueI )
               {
                  AddGraphPoints(stress_top_min, xVals, fTopMin);
                  AddGraphPoints(stress_bot_min, xVals, fBotMin);

                  m_Graph.SetDataLabel(stress_top_min,strDataLabel+_T(" - Top"));
                  m_Graph.SetDataLabel(stress_bot_min,strDataLabel+_T(" - Bottom"));
               }
               else
               {
                  if ( ls_type != pgsTypes::ServiceIII)
                     AddGraphPoints(stress_top_max, xVals, fTopMax);

                  AddGraphPoints(stress_bot_max, xVals, fBotMax);
               }
            }
            else
            {
               ATLASSERT(false); // who added a stage and didn't tell me?
            }
         }
      break;
      }
   }
}

void CAnalysisResultsView::CombinedLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPoi)
{
   // Combined forces
   GET_IFACE(IPointOfInterest,pIPOI);
   GET_IFACE(ICombinedForces2,pForces);
   LoadingCombination combination_type = m_pFrame->GetCombinedLoadCase(graphIdx);

   pgsTypes::AnalysisType analysis_type = m_pFrame->GetAnalysisType();

   Uint32 data_series_id[4];
   BridgeAnalysisType bat[4];
   Uint16 nAnalysisTypes;
   InitializeGraph(graphIdx,action,data_series_id,bat,&nAnalysisTypes);

   std::vector<Float64> xVals;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 loc;
      if ( stage == pgsTypes::CastingYard )
         loc = poi.GetDistFromStart();
      else
         loc = pIPOI->GetDistanceFromFirstPier(poi,stage);
      xVals.push_back(loc);
   }

   for ( Uint16 analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      switch(action)
      {
      case actionMoment:
         {
         std::vector<Float64> moments = pForces->GetMoment( combination_type, stage, vPoi, ctCummulative, bat[analysisIdx] );
         AddGraphPoints(data_series_id[analysisIdx], xVals, moments);
         break;
         }
      case actionShear:
         {
         std::vector<sysSectionValue> shear = pForces->GetShear( combination_type, stage, vPoi, ctCummulative, bat[analysisIdx] );
         AddGraphPoints(data_series_id[analysisIdx], xVals, shear);
         break;
         }
      case actionDisplacement:
         {
         std::vector<Float64> displ = pForces->GetDisplacement( combination_type, stage, vPoi, ctCummulative, bat[analysisIdx] );
         AddGraphPoints(data_series_id[analysisIdx], xVals, displ);
         break;
         }
      case actionStress:
         {
         std::vector<Float64> fTop, fBot;
         pForces->GetStress( combination_type, stage, vPoi, ctCummulative, bat[analysisIdx], &fTop, &fBot );
         AddGraphPoints(data_series_id[2*analysisIdx],   xVals, fTop);
         AddGraphPoints(data_series_id[2*analysisIdx+1], xVals, fBot);
         break;
         }
      }
   }
}

void CAnalysisResultsView::LiveLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPoi)
{
   // Live Load
   GET_IFACE(ICombinedForces2,pForces);
   GET_IFACE(IPointOfInterest,pIPOI);

   ASSERT(stage==pgsTypes::BridgeSite3);

   pgsTypes::LiveLoadType llType = m_pFrame->GetLiveLoadType(graphIdx);

   CString strDataLabel(m_pFrame->GetGraphDataLabel(graphIdx));
   strDataLabel += _T(" (per girder)");

   COLORREF c = m_pFrame->GetGraphColor(graphIdx);

   pgsTypes::AnalysisType analysis_type = m_pFrame->GetAnalysisType();

   VehicleIndexType vehicleIndex = m_pFrame->GetVehicleIndex(graphIdx);
   ATLASSERT(vehicleIndex == INVALID_INDEX);

   // data series for moment, shears and deflections
   Uint32 min_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,1,c);
   Uint32 max_data_series = m_Graph.CreateDataSeries(_T(""),PS_SOLID,1,c);

   // data series for stresses
   Uint32 stress_top_max = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),   PS_STRESS_TOP,   1,c);
   Uint32 stress_top_min = m_Graph.CreateDataSeries(_T(""),                      PS_STRESS_TOP,   1,c);
   Uint32 stress_bot_max = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom"),PS_STRESS_BOTTOM,1,c);
   Uint32 stress_bot_min = m_Graph.CreateDataSeries(_T(""),                      PS_STRESS_BOTTOM,1,c);

   std::vector<Float64> xVals;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 loc;
      if ( stage == pgsTypes::CastingYard )
         loc = poi.GetDistFromStart();
      else
         loc = pIPOI->GetDistanceFromFirstPier(poi,stage);
      xVals.push_back(loc);
   }

   switch(action)
   {
   case actionMoment:
      {
         std::vector<Float64> Mmin, Mmax;
         if ( analysis_type == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadMoment(llType, stage, vPoi, MinSimpleContinuousEnvelope, &Mmin, &Mmax);
            AddGraphPoints(min_data_series, xVals, Mmin);

            pForces->GetCombinedLiveLoadMoment(llType, stage, vPoi, MaxSimpleContinuousEnvelope, &Mmin, &Mmax);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadMoment(llType, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &Mmin, &Mmax);
            AddGraphPoints(min_data_series, xVals, Mmin);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
      break;

      }
   case actionShear:
      {
         std::vector<sysSectionValue> Vmin, Vmax;
         if ( analysis_type == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadShear(llType, stage, vPoi, MinSimpleContinuousEnvelope, &Vmin, &Vmax);
            AddGraphPoints(min_data_series, xVals, Vmin);

            pForces->GetCombinedLiveLoadShear(llType, stage, vPoi, MaxSimpleContinuousEnvelope, &Vmin, &Vmax);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadShear(llType, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &Vmin, &Vmax);
            AddGraphPoints(min_data_series, xVals, Vmin);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
      break;
      }
   case actionDisplacement:
      {
         std::vector<Float64> Dmin, Dmax;
         if ( analysis_type == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadDisplacement(llType, stage, vPoi, MinSimpleContinuousEnvelope, &Dmin, &Dmax);
            AddGraphPoints(min_data_series, xVals, Dmin);

            pForces->GetCombinedLiveLoadDisplacement(llType, stage, vPoi, MaxSimpleContinuousEnvelope, &Dmin, &Dmax);
            AddGraphPoints(max_data_series, xVals, Dmax);
         }
         else
         {
            pForces->GetCombinedLiveLoadDisplacement(llType, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &Dmin, &Dmax);
            AddGraphPoints(min_data_series, xVals, Dmin);
            AddGraphPoints(max_data_series, xVals, Dmax);
         }
      break;
      }
   case actionStress:
      {
         std::vector<Float64> fTopMin,fTopMax,fBotMin,fBotMax;
         if ( analysis_type == pgsTypes::Envelope )
         {
            pForces->GetCombinedLiveLoadStress(llType, stage, vPoi, MaxSimpleContinuousEnvelope, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
            AddGraphPoints(stress_top_min, xVals, fTopMin);
            AddGraphPoints(stress_bot_min, xVals, fBotMin);
            AddGraphPoints(stress_top_max, xVals, fTopMax);
            AddGraphPoints(stress_bot_max, xVals, fBotMax);
         }
         else
         {
            pForces->GetCombinedLiveLoadStress(llType, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
            AddGraphPoints(stress_top_min, xVals, fTopMin);
            AddGraphPoints(stress_bot_min, xVals, fBotMin);
            AddGraphPoints(stress_top_max, xVals, fTopMax);
            AddGraphPoints(stress_bot_max, xVals, fBotMax);
         }
      break;
      }
   }
}

void CAnalysisResultsView::VehicularLiveLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPoi)
{
   // Live Load
   GET_IFACE(IProductForces2,pForces);
   GET_IFACE(IPointOfInterest,pIPOI);

   ASSERT(stage==pgsTypes::BridgeSite3);

   CString strDataLabel(m_pFrame->GetGraphDataLabel(graphIdx));
   strDataLabel += _T(" (per lane)");

   COLORREF c = m_pFrame->GetGraphColor(graphIdx);

   pgsTypes::AnalysisType analysis_type = m_pFrame->GetAnalysisType();

   pgsTypes::LiveLoadType llType = m_pFrame->GetLiveLoadType(graphIdx);
   VehicleIndexType vehicleIndex = m_pFrame->GetVehicleIndex(graphIdx);

   // data series for moment, shears and deflections
   Uint32 min_data_series = m_Graph.CreateDataSeries(_T(""),          PS_SOLID,1,c);
   Uint32 max_data_series = m_Graph.CreateDataSeries(strDataLabel,PS_SOLID,1,c);

   // data series for stresses
   Uint32 stress_top_max = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),   PS_STRESS_TOP,   1,c);
   Uint32 stress_top_min = m_Graph.CreateDataSeries(_T(""),                      PS_STRESS_TOP,   1,c);
   Uint32 stress_bot_max = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom"),PS_STRESS_BOTTOM,1,c);
   Uint32 stress_bot_min = m_Graph.CreateDataSeries(_T(""),                      PS_STRESS_BOTTOM,1,c);

   std::vector<Float64> xVals;
   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 loc;
      if ( stage == pgsTypes::CastingYard )
         loc = poi.GetDistFromStart();
      else
         loc = pIPOI->GetDistanceFromFirstPier(poi,stage);
      xVals.push_back(loc);
   }

   switch(action)
   {
   case actionMoment:
      {
         std::vector<Float64> Mmin, Mmax;
         if ( analysis_type == pgsTypes::Envelope )
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadMoment(llType, vehicleIndex, stage, vPoi, MinSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            else
               pForces->GetLiveLoadMoment(llType, stage, vPoi, MinSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);

            AddGraphPoints(min_data_series, xVals, Mmin);

            if ( vehicleIndex != Uint32_Max )
               pForces->GetVehicularLiveLoadMoment(llType, vehicleIndex, stage, vPoi, MaxSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);
            else
               pForces->GetLiveLoadMoment(llType, stage, vPoi, MaxSimpleContinuousEnvelope, true, false, &Mmin, &Mmax);

            AddGraphPoints(max_data_series, xVals, Mmax);
         }
         else
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadMoment(llType, vehicleIndex, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &Mmin, &Mmax, NULL, NULL);
            else
               pForces->GetLiveLoadMoment(llType, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &Mmin, &Mmax, NULL, NULL);

            AddGraphPoints(min_data_series, xVals, Mmin);
            AddGraphPoints(max_data_series, xVals, Mmax);
         }
      break;

      }
   case actionShear:
      {
         std::vector<sysSectionValue> Vmin, Vmax;
         if ( analysis_type == pgsTypes::Envelope )
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadShear(llType, vehicleIndex, stage, vPoi, MinSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            else
               pForces->GetLiveLoadShear(llType, stage, vPoi, MinSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);

            AddGraphPoints(min_data_series, xVals, Vmin);

            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadShear(llType, vehicleIndex, stage, vPoi, MaxSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);
            else
               pForces->GetLiveLoadShear(llType, stage, vPoi, MaxSimpleContinuousEnvelope, true, false, &Vmin, &Vmax);

            AddGraphPoints(max_data_series, xVals, Vmax);
         }
         else
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadShear(llType, vehicleIndex, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &Vmin, &Vmax);
            else
               pForces->GetLiveLoadShear(llType, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &Vmin, &Vmax);

            AddGraphPoints(min_data_series, xVals, Vmin);
            AddGraphPoints(max_data_series, xVals, Vmax);
         }
      break;
      }
   case actionDisplacement:
      {
         std::vector<Float64> Dmin, Dmax;
         if ( analysis_type == pgsTypes::Envelope )
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadDisplacement(llType, vehicleIndex, stage, vPoi, MinSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            else
               pForces->GetLiveLoadDisplacement(llType, stage, vPoi, MinSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);

            AddGraphPoints(min_data_series, xVals, Dmin);

            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadDisplacement(llType, vehicleIndex, stage, vPoi, MaxSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);
            else
               pForces->GetLiveLoadDisplacement(llType, stage, vPoi, MaxSimpleContinuousEnvelope, true, false, &Dmin, &Dmax);

            AddGraphPoints(max_data_series, xVals, Dmax);
         }
         else
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadDisplacement(llType, vehicleIndex, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &Dmin, &Dmax);
            else
               pForces->GetLiveLoadDisplacement(llType, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &Dmin, &Dmax);

            AddGraphPoints(min_data_series, xVals, Dmin);
            AddGraphPoints(max_data_series, xVals, Dmax);
         }
      break;
      }
   case actionStress:
      {
         std::vector<Float64> fTopMin, fTopMax, fBotMin, fBotMax;
         if ( analysis_type == pgsTypes::Envelope )
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadStress(llType, vehicleIndex, stage, vPoi, MaxSimpleContinuousEnvelope, true, false, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
            else
               pForces->GetLiveLoadStress(llType, stage, vPoi, MaxSimpleContinuousEnvelope, true, false, &fTopMin, &fTopMax, &fBotMin, &fBotMax );

            AddGraphPoints(stress_top_min, xVals, fTopMin);
            AddGraphPoints(stress_bot_min, xVals, fBotMin);
            AddGraphPoints(stress_top_max, xVals, fTopMax);
            AddGraphPoints(stress_bot_max, xVals, fBotMax);
         }
         else
         {
            if ( vehicleIndex != INVALID_INDEX )
               pForces->GetVehicularLiveLoadStress(llType, vehicleIndex, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMin, &fTopMax, &fBotMin, &fBotMax );
            else
               pForces->GetLiveLoadStress(llType, stage, vPoi, analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan, true, false, &fTopMin, &fTopMax, &fBotMin, &fBotMax );

            AddGraphPoints(stress_top_min, xVals, fTopMin);
            AddGraphPoints(stress_bot_min, xVals, fBotMin);
            AddGraphPoints(stress_top_max, xVals, fTopMax);
            AddGraphPoints(stress_bot_max, xVals, fBotMax);
         }
      break;
      }
   }
}

void CAnalysisResultsView::ProductLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPoi)
{
   ProductForceType prod_type = m_pFrame->GetProductLoadCase(graphIdx);
   
   // Product forces
   GET_IFACE(IPointOfInterest,pIPOI);   
   GET_IFACE(IProductForces2,pForces);

   Uint32 data_series_id[4];
   BridgeAnalysisType bat[4];
   Uint16 nAnalysisTypes;
   InitializeGraph(graphIdx,action,data_series_id,bat,&nAnalysisTypes);

   CString strDataLabel = m_pFrame->GetGraphDataLabel(graphIdx);

   COLORREF c = m_pFrame->GetGraphColor(graphIdx);

   pgsTypes::AnalysisType analysis_type = m_pFrame->GetAnalysisType();

   std::vector<pgsPointOfInterest>::const_iterator i;
   std::vector<Float64> xVals;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 loc;
      if ( stage == pgsTypes::CastingYard )
         loc = poi.GetDistFromStart();
      else
         loc = pIPOI->GetDistanceFromFirstPier(poi,stage);
      xVals.push_back(loc);
   }

   for ( Uint16 analysisIdx = 0; analysisIdx < nAnalysisTypes; analysisIdx++ )
   {
      switch(action)
      {
      case actionMoment:
         {
            std::vector<Float64> moments = pForces->GetMoment( stage, prod_type, vPoi, bat[analysisIdx]);
            AddGraphPoints(data_series_id[analysisIdx], xVals, moments);
            break;
         }
      case actionShear:
         {
            std::vector<sysSectionValue> shears = pForces->GetShear( stage, prod_type, vPoi, bat[analysisIdx]);
            AddGraphPoints(data_series_id[analysisIdx], xVals, shears);
            break;
         }
      case actionDisplacement:
         {
            std::vector<Float64> displacements = pForces->GetDisplacement( stage, prod_type, vPoi, bat[analysisIdx]);
            AddGraphPoints(data_series_id[analysisIdx], xVals, displacements);
         break;
         }
      case actionStress:
         {
            std::vector<Float64> fTop, fBot;
            pForces->GetStress( stage, prod_type, vPoi, bat[analysisIdx], &fTop, &fBot);
            AddGraphPoints(data_series_id[2*analysisIdx], xVals, fTop);
            AddGraphPoints(data_series_id[2*analysisIdx+1], xVals, fBot);
            break;
         }
      }
   }
}

void CAnalysisResultsView::PrestressLoadGraph(int graphIdx,pgsTypes::Stage stage,ActionType action,const std::vector<pgsPointOfInterest>& vPoi)
{
   // Prestress
   GET_IFACE(IPointOfInterest,pIPOI);
   GET_IFACE(IPrestressStresses,pPrestress);

   CString strDataLabel(m_pFrame->GetGraphDataLabel(graphIdx));

   COLORREF c = m_pFrame->GetGraphColor(graphIdx);

   Uint32 deflection = m_Graph.CreateDataSeries(strDataLabel,   PS_SOLID,   1,c);
   Uint32 stress_top = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),   PS_STRESS_TOP,   1,c);
   Uint32 stress_bot = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom"),PS_STRESS_BOTTOM,1,c);

   std::vector<pgsPointOfInterest>::const_iterator i;
   for ( i = vPoi.begin(); i != vPoi.end(); i++ )
   {
      const pgsPointOfInterest& poi = *i;
      Float64 loc;
      if ( stage == pgsTypes::CastingYard )
         loc = poi.GetDistFromStart();
      else
         loc = pIPOI->GetDistanceFromFirstPier(poi,stage);

      switch(action)
      {
      case actionMoment:
      case actionShear:
         ATLASSERT(false); // should never get here
         break;

      case actionDisplacement:
         {
            GET_IFACE(ICamber,pCamber);
            bool bRelativeToBearings = (stage == pgsTypes::CastingYard ? false : true);
            double dy = pCamber->GetPrestressDeflection(poi,bRelativeToBearings);
            AddGraphPoint(deflection, loc, dy);
         }
         break;

      case actionStress:
         {
            Float64 fTop,fBot;
            fTop = pPrestress->GetStress(stage,poi,pgsTypes::TopGirder);
            fBot = pPrestress->GetStress(stage,poi,pgsTypes::BottomGirder);
            AddGraphPoint(stress_top, loc, fTop);
            AddGraphPoint(stress_bot, loc, fBot);
         break;
         }
      }
   }
}

void CAnalysisResultsView::UpdateFromBar()
{
   m_bUpdateError      = false;
   m_bValidGraph       = false;
   Update();
}

void CAnalysisResultsView::Update()
{
 	CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   if ( pDoc->IsAutoCalcEnabled() )
   {
      // AutoCalc is on so update the contents of the view now
      UpdateNow();
   }
   else
   {
      // AutoCalc is off. Invalid the view and force it to redraw.
      // OnDraw will display the hint message for updating the view.
      Invalidate();
      UpdateWindow();
   }
}

void CAnalysisResultsView::UpdateNow()
{
   // catch any exceptions coming out of analysis and set to safe mode if a problem occurs
   try{
      DoUpdateNow();
   }
   catch(...)
   {
      m_bUpdateError = true;
      m_bValidGraph       = false;
      Invalidate();
      UpdateWindow();
      throw;
   }
}


int CAnalysisResultsView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   m_pFrame = (CAnalysisResultsChildFrame*)GetParent();
   ASSERT( m_pFrame != 0 );
   ASSERT( m_pFrame->IsKindOf( RUNTIME_CLASS( CAnalysisResultsChildFrame ) ) );

   CPGSuperDoc* pdoc = (CPGSuperDoc*)GetDocument();
   ASSERT(pdoc->IsKindOf( RUNTIME_CLASS( CPGSuperDoc )));

   pdoc->GetBroker(&m_pBroker);
	

   m_Graph.SetYAxisNiceRange(true);
   m_Graph.SetYAxisNumberOfMinorTics(5);
   m_Graph.SetYAxisNumberOfMajorTics(21);

   // draw to real length of beam and label 1/4 points
   m_Graph.SetXAxisNumberOfMinorTics(0);
   m_Graph.SetXAxisNiceRange(false);
   m_Graph.SetXAxisNumberOfMajorTics(11);
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);
   m_Graph.SetGridPenStyle(PS_DOT, 1, GRID_COLOR);

   // initial title for child frame
   this->SetWindowText(_T("Analysis Results"));

	return 0;
}

void CAnalysisResultsView::OnInitialUpdate()
{
   CView::OnInitialUpdate();
   CEAFAutoCalcViewMixin::Initialize();

   CDocument* pDoc = GetDocument();
   CDocTemplate* pDocTemplate = pDoc->GetDocTemplate();
   ASSERT( pDocTemplate->IsKindOf(RUNTIME_CLASS(CEAFDocTemplate)) );

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)pDocTemplate;
   SpanGirderHashType* pHash = (SpanGirderHashType*)pTemplate->GetViewCreationData();
   SpanIndexType spanIdx;
   GirderIndexType gdrIdx;
   UnhashSpanGirder(*pHash,&spanIdx,&gdrIdx);

   m_pFrame->SelectSpan(spanIdx,gdrIdx);
}

void CAnalysisResultsView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
   if ( 0 < lHint && lHint <= MAX_DISPLAY_HINT && lHint != HINT_GIRDERLABELFORMATCHANGED )
      return; // some display feature changed... not data... nothing to do here

   if ( lHint == EAF_HINT_UPDATEERROR )
   {
      CString* pmsg = (CString*)pHint;
      m_ErrorMsg = *pmsg;
      m_bUpdateError = true;
      m_bValidGraph       = false;
      CEAFAutoCalcViewMixin::OnUpdate( pSender, lHint, pHint );
      Invalidate();
      return;
   }

   m_bUpdateError = false;

   // update model is controlled by frame window. This is definitely a hack, but I
   // don't think there is a clean way to do it without adding complexity.
   m_pFrame->Update(lHint);

   // deal with license plate stuff
   CView::OnUpdate(pSender,lHint,pHint);
   CEAFAutoCalcViewMixin::OnUpdate( pSender, lHint, pHint);

   // update graph data
   Update();
}

void CAnalysisResultsView::UpdateUnits(ActionType action)
{
   delete m_pXFormat;
   delete m_pYFormat;

   GET_IFACE2(m_pBroker,IEAFDisplayUnits,pUnits);

   // first x axis
   const unitmgtLengthData& rlen = pUnits->GetSpanLengthUnit();
   m_pXFormat = new LengthTool(rlen);
   m_Graph.SetXAxisValueFormat(*m_pXFormat);

   switch(action)
   {
   case actionMoment:
      {
      const unitmgtMomentData& rlen = pUnits->GetMomentUnit();
      m_pYFormat = new MomentTool(rlen);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring ytit = _T("Moment (") + ((MomentTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(ytit);
      break;
      }
   case actionShear:
      {
      const unitmgtForceData& rlen = pUnits->GetShearUnit();
      m_pYFormat = new ShearTool(rlen);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring ytit = _T("Shear (") + ((ShearTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(ytit);
      break;
      }
   case actionDisplacement:
      {
      const unitmgtLengthData& rlen = pUnits->GetDisplacementUnit();
      m_pYFormat = new DisplacementTool(rlen);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring ytit = _T("Displacement (") + ((DisplacementTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(ytit);
      break;
      }
   case actionStress:
      {
      const unitmgtStressData& rstress = pUnits->GetStressUnit();
      m_pYFormat = new StressTool(rstress);
      m_Graph.SetYAxisValueFormat(*m_pYFormat);
      std::_tstring ytit = _T("Stress (") + ((StressTool*)m_pYFormat)->UnitTag() + _T(")");
      m_Graph.SetYAxisTitle(ytit);
      break;
      }
   default:
      ASSERT(0); 
   }
}

void CAnalysisResultsView::UpdateGrid()
{
   m_Graph.SetDoDrawGrid(m_pFrame->GetGrid());
   Invalidate();
}

void CAnalysisResultsView::AddGraphPoints(Uint32 series, const std::vector<Float64>& xvals,const std::vector<Float64>& yvals)
{
   std::vector<Float64>::const_iterator xIter, yIter;
   for ( xIter = xvals.begin(), yIter = yvals.begin(); xIter != xvals.end() && yIter != yvals.end(); xIter++, yIter++ )
   {
      AddGraphPoint(series,*xIter,*yIter);
   }
}

void CAnalysisResultsView::AddGraphPoints(Uint32 series, const std::vector<Float64>& xvals,const std::vector<sysSectionValue>& yvals)
{
   std::vector<Float64>::const_iterator xIter;
   std::vector<sysSectionValue>::const_iterator yIter;
   for ( xIter = xvals.begin(), yIter = yvals.begin(); xIter != xvals.end() && yIter != yvals.end(); xIter++, yIter++ )
   {
      AddGraphPoint(series,*xIter,(*yIter).Left());
      AddGraphPoint(series,*xIter,(*yIter).Right());
   }
}

void CAnalysisResultsView::AddGraphPoint(Uint32 series, Float64 xval, Float64 yval)
{
   // deal with unit conversion
   arvPhysicalConverter* pcx = dynamic_cast<arvPhysicalConverter*>(m_pXFormat);
   ASSERT(pcx);
   arvPhysicalConverter* pcy = dynamic_cast<arvPhysicalConverter*>(m_pYFormat);
   ASSERT(pcy);
   m_Graph.AddPoint(series, gpPoint2d(pcx->Convert(xval),pcy->Convert(yval)));
}

void CAnalysisResultsView::InitializeGraph(int graphIdx,ActionType action,Uint32* pDataSeriesID,BridgeAnalysisType* pBAT,Uint16* pAnalysisTypeCount)
{
   CString strDataLabel = m_pFrame->GetGraphDataLabel(graphIdx);

   COLORREF c = m_pFrame->GetGraphColor(graphIdx);

   pgsTypes::AnalysisType analysis_type = m_pFrame->GetAnalysisType();

   if ( action == actionMoment ||
        action == actionShear  ||
        action == actionDisplacement )
   {
      // For moments, shears, and deflections
      if ( analysis_type == pgsTypes::Envelope )
      {
         *pAnalysisTypeCount = 2;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, 1, c);
         pDataSeriesID[1] = m_Graph.CreateDataSeries(_T(""), PS_SOLID, 1, c);

         pBAT[0] = MinSimpleContinuousEnvelope;
         pBAT[1] = MaxSimpleContinuousEnvelope;
      }
      else
      {
         *pAnalysisTypeCount = 1;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel, PS_SOLID, 1, c);

         pBAT[0] = (analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
      }
   }
   else if (action == actionStress)
   {
      ATLASSERT(action == actionStress);
      // for stresses

      if ( analysis_type == pgsTypes::Envelope )
      {
         *pAnalysisTypeCount = 1;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),    PS_STRESS_TOP,    1, c);
         pDataSeriesID[1] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom"), PS_STRESS_BOTTOM, 1, c);

         pBAT[0] = MaxSimpleContinuousEnvelope;
      }
      else
      {
         *pAnalysisTypeCount = 2;
         pDataSeriesID[0] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Top"),    PS_STRESS_TOP,    1, c);
         pDataSeriesID[1] = m_Graph.CreateDataSeries(strDataLabel+_T(" - Bottom"), PS_STRESS_BOTTOM, 1, c);

         pBAT[0] = (analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
         pBAT[1] = (analysis_type == pgsTypes::Simple ? SimpleSpan : ContinuousSpan);
      }
   }
}

/////////////////////////////////////////////////////////////////////////////
// CAnalysisResultsView printing

void CAnalysisResultsView::OnUpdateFilePrint(CCmdUI* pCmdUI) 
{
   BOOL flag = (m_bValidGraph && !m_bUpdateError) ? TRUE:FALSE;
	pCmdUI->Enable(flag); 
}

void CAnalysisResultsView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo) 
{
	CView::OnBeginPrinting(pDC, pInfo);
}

void CAnalysisResultsView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo) 
{
	CView::OnEndPrinting(pDC, pInfo);
}

BOOL CAnalysisResultsView::OnPreparePrinting(CPrintInfo* pInfo) 
{
	if (DoPreparePrinting(pInfo))
	   return CView::OnPreparePrinting(pInfo);
   else
      return FALSE;
}

void CAnalysisResultsView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
   // get paper size
   PGSuperCalculationSheet border(m_pBroker);
   CString strBottomTitle;
   strBottomTitle.Format(_T("PGSuper™, Copyright © %4d, WSDOT, All rights reserved"),sysDate().Year());
   border.SetTitle(strBottomTitle);
   CDocument* pdoc = GetDocument();
   CString path = pdoc->GetPathName();
   border.SetFileName(path);
   CRect rcPrint = border.Print(pDC, 1);

   if (rcPrint.IsRectEmpty())
   {
      CHECKX(0,_T("Can't print border - page too small?"));
      rcPrint = pInfo->m_rectDraw;
   }

   m_PrintRect = rcPrint;
	CView::OnPrint(pDC, pInfo);
}

void CAnalysisResultsView::UpdateXAxisTitle(pgsTypes::Stage stage)
{
   if ( stage == pgsTypes::CastingYard )
   {
      m_Graph.SetXAxisTitle(_T("Distance From Left End of Girder (")+m_pXFormat->UnitTag()+_T(")"));
   }
   else
   {
      m_Graph.SetXAxisTitle(_T("Location (")+m_pXFormat->UnitTag()+_T(")"));
   }
}

void CAnalysisResultsView::UpdateGraphTitle(SpanIndexType span,GirderIndexType girder,pgsTypes::Stage stage,ActionType action)
{
   CString strAction;
   switch(action)
   {
   case actionMoment:
      strAction = _T("Moments");
      break;
   case actionShear:
      strAction = _T("Shears");
      break;
   case actionDisplacement:
      strAction = _T("Deflections");
      break;
   case actionStress:
      strAction = _T("Stresses");
      break;
   default:
      ASSERT(0);
   }

   CString strStage;
   switch(stage)
   {
   case pgsTypes::CastingYard:
      strStage = _T("Casting Yard");
      break;

   case pgsTypes::GirderPlacement:
      strStage = _T("Girder Placement");
      break;

   case pgsTypes::TemporaryStrandRemoval:
      strStage = _T("Temporary Strand Removal");
      break;

   case pgsTypes::BridgeSite1:
      strStage = _T("Deck and Diaphragm Placement (Bridge Site 1)");
      break;

   case pgsTypes::BridgeSite2:
      strStage = _T("Superimposed Dead Loads (Bridge Site 2)");
      break;

   case pgsTypes::BridgeSite3:
      strStage = _T("Final with Live Load (Bridge Site 3)");
      break;

   default:
      ASSERT(0);
   }


   CString graph_title;
   if ( span == ALL_SPANS )
   {
      graph_title.Format(_T("Girder Line %s - %s - %s"),LABEL_GIRDER(girder),strStage,strAction);
   }
   else
   {
      graph_title.Format(_T("Span %d Girder %s - %s - %s"),LABEL_SPAN(span),LABEL_GIRDER(girder),strStage,strAction);
   }
   
   m_Graph.SetTitle(std::_tstring(graph_title));

   CString strSubtitle;
   switch ( m_pFrame->GetAnalysisType() )
   {
   case pgsTypes::Simple:
      strSubtitle = _T("Simple span");
      break;

   case pgsTypes::Continuous:
      strSubtitle = _T("Simple made continuous");
      break;

   case pgsTypes::Envelope:
      strSubtitle = _T("Envelope of Simple span and simple made continuous");
      break;
   }

   m_Graph.SetSubtitle(std::_tstring(strSubtitle));
}
