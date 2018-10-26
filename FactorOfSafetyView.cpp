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

// FactorOfSafetyView.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "PGSuperDoc.h"
#include "FactorOfSafetyView.h"
#include "PGSuperException.h"

#include <IFace\Artifact.h>
#include <IFace\Bridge.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\AnalysisResults.h>
#include <IFace\GirderHandlingPointOfInterest.h>
#include <IFace\GirderHandlingSpecCriteria.h>
#include <PgsExt\PointOfInterest.h>
#include <EAF\EAFAutoProgress.h>
#include <PgsExt\PhysicalConverter.h>
#include <PgsExt\LiftingAnalysisArtifact.h>
#include <PgsExt\HaulingAnalysisArtifact.h>
#include "PGSuperCalculationSheet.h"
#include <MfcTools\Text.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// some styles
static const COLORREF GRAPH_BACKGROUND  = RGB(220,255,220);
static const COLORREF CURVE1_COLOR      = RGB(0,0,200);
static const COLORREF CURVE2_COLOR      = RGB(200,0,0);
static const COLORREF GRID_COLOR        = RGB(0,150,0);
static const int      CURVE_WIDTH      = 1;
static const int      CURVE_STYLE      = PS_SOLID;
static const int      LIMIT_STYLE      = PS_DASH;

// create a dummy unit conversion tool to pacify the graph constructor
static unitmgtLengthData DUMMY(unitMeasure::Meter);
static LengthTool    DUMMY_TOOL(DUMMY);

/////////////////////////////////////////////////////////////////////////////
// CFactorOfSafetyView

IMPLEMENT_DYNCREATE(CFactorOfSafetyView, CView)

CFactorOfSafetyView::CFactorOfSafetyView():
CEAFAutoCalcViewMixin(this),
m_bValidGraph(false),
m_pXFormat(0),
m_pYFormat(0),
m_pBroker(0),
m_Graph(DUMMY_TOOL,DUMMY_TOOL),
m_IsPrinting(false)
{
   m_bUpdateError = false;
}

CFactorOfSafetyView::~CFactorOfSafetyView()
{
   delete m_pXFormat;
   delete m_pYFormat;

   if ( m_pBroker != 0 )
      m_pBroker->Release();
}


BEGIN_MESSAGE_MAP(CFactorOfSafetyView, CView)
	//{{AFX_MSG_MAP(CFactorOfSafetyView)
	ON_WM_CREATE()
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT, OnUpdateFilePrint)
	ON_UPDATE_COMMAND_UI(ID_FILE_PRINT_DIRECT, OnUpdateFilePrint)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

int CFactorOfSafetyView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   m_pFrame = (CFactorOfSafetyChildFrame*)GetParent();
   ASSERT( m_pFrame != 0 );
   ASSERT( m_pFrame->IsKindOf( RUNTIME_CLASS( CFactorOfSafetyChildFrame ) ) );

   CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   ASSERT(pDoc->IsKindOf( RUNTIME_CLASS( CPGSuperDoc )));

   pDoc->GetBroker(&m_pBroker);
   ASSERT(m_pBroker);
	
   m_Graph.SetXAxisNumberOfMinorTics(0);
   m_Graph.SetYAxisNumberOfMinorTics(0);

   // draw to real length of beam and label 1/4 points
   m_Graph.SetXAxisNiceRange(false);
   m_Graph.SetXAxisNumberOfMajorTics(11);
   m_Graph.SetClientAreaColor(GRAPH_BACKGROUND);
   m_Graph.SetGridPenStyle(PS_DOT, 1, GRID_COLOR);
   m_Graph.SetYAxisTitle(_T("Factor of Safety"));

   // initial title for child frame
   this->SetWindowText(_T("Girder Stability"));

   return 0;
}

void CFactorOfSafetyView::UpdateUnits()
{
   delete m_pXFormat;
   delete m_pYFormat;

   // Set up the unit formatters for the X and Y axes
   GET_IFACE2(m_pBroker,IEAFDisplayUnits,pUnits);

   // first x axis
   const unitmgtLengthData& rlen = pUnits->GetSpanLengthUnit();
   m_pXFormat = new LengthTool(rlen);
   m_Graph.SetXAxisValueFormat(*m_pXFormat);

   // now the Y axis
   const unitmgtScalar& rScalar = pUnits->GetScalarFormat();
   m_pYFormat = new ScalarTool(rScalar);
   m_Graph.SetYAxisValueFormat(*m_pYFormat);
}

bool CFactorOfSafetyView::DoResultsExist() const
{
   return m_bValidGraph;
}

/////////////////////////////////////////////////////////////////////////////
// CFactorOfSafetyView drawing

void CFactorOfSafetyView::OnDraw(CDC* pDC)
{
   int save = pDC->SaveDC();

   // deal with printing and reentrant behavior
   if(m_IsPrinting)
      return;

   CString msg;
   if ((m_bValidGraph) && !m_bUpdateError)
   {
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

      // Set the Y axis subtitle
      TCHAR buffer[45];
      if ( m_pFrame->GetStage() == CFactorOfSafetyChildFrame::Lifting )
      {
         GET_IFACE(IGirderLiftingSpecCriteria,pCriteria);
         _stprintf_s(buffer,sizeof(buffer)/sizeof(TCHAR),_T("Min. FScr = %3.1f, Min. FSf = %3.1f"),
            pCriteria->GetLiftingCrackingFs(),
            pCriteria->GetLiftingFailureFs() );
      }
      else
      {
         GET_IFACE(IGirderHaulingSpecCriteria,pCriteria);
         _stprintf_s(buffer,sizeof(buffer)/sizeof(TCHAR),_T("Min. FScr = %3.1f, Min. FSr = %3.1f"),
            pCriteria->GetHaulingCrackingFs(),
            pCriteria->GetHaulingRolloverFs() );
      }
      m_Graph.SetSubtitle(std::_tstring(buffer));

      // Draw the graph
      m_Graph.SetOutputRect(rect);
      m_Graph.Draw(pDC->GetSafeHdc());
      DrawLegend(pDC); // Graph doesn't support legends...

      // window text for child frame
      this->SetWindowText(m_Graph.GetTitle().c_str());
   }
   else
   {
      GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
      GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
      if (m_pFrame->GetStage() == CFactorOfSafetyChildFrame::Lifting && 
          !pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
      {
         msg = _T("Lifting Analysis is Disabled in the Project Criteria Library");
      }
      else if (m_pFrame->GetStage() == CFactorOfSafetyChildFrame::Hauling && 
          !pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
      {
         msg = _T("Hauling Analysis is Disabled in the Project Criteria Library");
      }
      else
      {
         if ( m_bUpdateError )
            AfxFormatString1(msg,IDS_E_UPDATE,m_ErrorMsg.c_str());
         else
            msg.LoadString(IDS_RESULTS_NOT_AVAILABLE);
      }

      MultiLineTextOut(pDC,0,0,msg);

      // give child frame a reasonable default
      this->SetWindowText(_T("Girder Stability"));
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

void CFactorOfSafetyView::DrawLegend(CDC* pDC)
{
   // Graph doesn't support legends... Draw a legend in the top
   // left corner of the graph's client area

   CPen pen1(CURVE_STYLE,CURVE_WIDTH,CURVE1_COLOR);
   CPen pen2(CURVE_STYLE,CURVE_WIDTH,CURVE2_COLOR);

   CFont font;
   font.CreatePointFont(80,_T("Arial"),pDC);
   CFont* oldFont = pDC->SelectObject(&font);

   CBrush brush;
   brush.CreateSolidBrush(GRAPH_BACKGROUND);
   CBrush* oldBrush = pDC->SelectObject(&brush );

   COLORREF oldBkColor = pDC->SetBkColor(GRAPH_BACKGROUND);

   const grlibPointMapper& mapper = m_Graph.GetClientAreaPointMapper(pDC->GetSafeHdc());
   gpPoint2d org = mapper.GetWorldOrg();
   gpSize2d  ext = mapper.GetWorldExt();

   CPoint topLeft;
   mapper.WPtoDP(org.X()-ext.Dx()/2.,org.Y()+ext.Dy()/2.,&topLeft.x,&topLeft.y);
   topLeft.x += 5;
   topLeft.y += 5;

   UINT oldAlign = pDC->SetTextAlign(TA_LEFT | TA_TOP);

   CString legend1, legend2;
   CSize size1, size2;

   if ( m_pFrame->GetStage() == CFactorOfSafetyChildFrame::Lifting )
   {
      legend1 = _T("Min. F.S. Against Cracking (FScr)");
      legend2 = _T("F.S. Against Failure (FSf)");
   }
   else
   {
      legend1 = _T("Min. F.S. Against Cracking (FScr)");
      legend2 = _T("F.S. Against Rollover (FSr)");
   }
   
   size1 = pDC->GetTextExtent(legend1);
   size2 = pDC->GetTextExtent(legend2);

   int logPixelsX = pDC->GetDeviceCaps(LOGPIXELSX); // Pixels per inch in the x direction
   // we want a 1/2" line for the legend
   int legendLength = logPixelsX/2;

   CPoint bottomRight;
   bottomRight.x = 5 + topLeft.x + max(size1.cx,size2.cx) + 5 + legendLength + 5;
   bottomRight.y = 5 + topLeft.y + size1.cy + 5 + size2.cy + 5;

   pDC->Rectangle(CRect(topLeft,bottomRight));
   pDC->TextOut(5 + topLeft.x,5 + topLeft.y,legend1);
   CPen* oldPen = pDC->SelectObject(&pen1);
   pDC->MoveTo(5 + topLeft.x + max(size1.cx,size1.cx) + 5,5 + topLeft.y + size1.cy/2);
   pDC->LineTo(5 + topLeft.x + max(size1.cx,size1.cx) + 5 + legendLength,5 + topLeft.y + size1.cy/2);

   pDC->SelectObject(&pen2);
   pDC->TextOut(5 + topLeft.x,5 + topLeft.y + size1.cy + 5, legend2);
   pDC->MoveTo(5 + topLeft.x + max(size1.cx,size1.cx) + 5,5 + topLeft.y + size1.cy + 5 + size2.cy/2);
   pDC->LineTo(5 + topLeft.x + max(size1.cx,size1.cx) + 5 + legendLength,5 + topLeft.y + size1.cy + 5 + size2.cy/2);


   pDC->SelectObject(oldPen);
   pDC->SelectObject(oldFont);
   pDC->SelectObject(oldBrush);
   pDC->SetTextAlign(oldAlign);
   pDC->SetBkColor(oldBkColor);
}

void CFactorOfSafetyView::OnInitialUpdate()
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

void CFactorOfSafetyView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
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

   // may be a hack to put this here, but library change causes problems
   GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
   GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
   if (m_pFrame->GetStage() == CFactorOfSafetyChildFrame::Lifting && 
       !pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
   {
       m_bValidGraph = false;
   }
   else if (m_pFrame->GetStage() == CFactorOfSafetyChildFrame::Hauling && 
       !pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
   {
       m_bValidGraph = false;
   }


   // deal with license plate stuff
   CView::OnUpdate(pSender,lHint,pHint);
   CEAFAutoCalcViewMixin::OnUpdate( pSender, lHint, pHint);

   // update model is controlled by frame window. This is definitely a hack, but I
   // don't think there is a clean way to do it without adding complexity.
   m_pFrame->Update();

   // update graph data
   Update();
}

/////////////////////////////////////////////////////////////////////////////
// CFactorOfSafetyView diagnostics

#ifdef _DEBUG
void CFactorOfSafetyView::AssertValid() const
{
	CView::AssertValid();
}

void CFactorOfSafetyView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFactorOfSafetyView message handlers
void CFactorOfSafetyView::UpdateFromBar()
{
//   m_bUpdateError      = false;
   m_bValidGraph       = false;
   Update();
}

void CFactorOfSafetyView::Update()
{
 	CPGSuperDoc* pDoc = (CPGSuperDoc*)GetDocument();
   if ( pDoc->IsAutoCalcEnabled() )
   {
      UpdateNow();
   }

   // time to redraw
   Invalidate();
   UpdateWindow();
}

void CFactorOfSafetyView::UpdateNow()
{
   // catch any exceptions coming out of analysis and set to safe mode if a problem occurs
   try
   {
      DoUpdateNow();
   }
   catch(...)
   {
      m_bValidGraph       = false;
      Invalidate();
      throw;
   }
}

void CFactorOfSafetyView::UpdateGrid()
{
   m_Graph.SetDoDrawGrid(m_pFrame->GetGrid());
   Invalidate();
}

void CFactorOfSafetyView::DoUpdateNow()
{
   PRECONDITION(m_pBroker);

   GET_IFACE(IProgress,pProgress);
   CEAFAutoProgress ap(pProgress);

   m_bValidGraph = false;
   m_Graph.ClearData();

   IndexType seriesFS1 = m_Graph.CreateDataSeries();
   m_Graph.SetPenStyle(seriesFS1, CURVE_STYLE, CURVE_WIDTH, CURVE1_COLOR);
   IndexType seriesFS2 = m_Graph.CreateDataSeries();
   m_Graph.SetPenStyle(seriesFS2, CURVE_STYLE, CURVE_WIDTH, CURVE2_COLOR);

   IndexType limitFS1 = m_Graph.CreateDataSeries();
   m_Graph.SetPenStyle(limitFS1, LIMIT_STYLE, CURVE_WIDTH, CURVE1_COLOR);
   IndexType limitFS2 = m_Graph.CreateDataSeries();
   m_Graph.SetPenStyle(limitFS2, LIMIT_STYLE, CURVE_WIDTH, CURVE2_COLOR);

   UpdateUnits();

   GET_IFACE(IArtifact,pArtifact);
   GET_IFACE(IStrandGeometry,pStrandGeom);
   
   SpanIndexType   span = m_pFrame->GetSpanIdx();
   GirderIndexType gdr  = m_pFrame->GetGirderIdx();

   CString subtitle;
   subtitle.Format(_T("Span %d Girder %s"), LABEL_SPAN(span), LABEL_GIRDER(gdr));
   m_PrintSubtitle = std::_tstring(subtitle);

   Float64 hp1,hp2;
   Float64 stepSize = ::ConvertToSysUnits(1.0,unitMeasure::Feet);
   pStrandGeom->GetHarpingPointLocations(span,gdr,&hp1,&hp2);

   if ( m_pFrame->GetStage() == CFactorOfSafetyChildFrame::Lifting )
   {
      GET_IFACE(IGirderLiftingSpecCriteria,pGirderLiftingSpecCriteria);
      if (pGirderLiftingSpecCriteria->IsLiftingCheckEnabled())
      {
         CString strTitle;
         strTitle.Format(_T("Effect of support location - Lifting Stage - %s"),m_PrintSubtitle.c_str());
         m_Graph.SetTitle(std::_tstring(strTitle));
         m_Graph.SetXAxisTitle(_T("Lift Point Location from End of Girder (") + m_pXFormat->UnitTag() + _T(")"));

         Float64 FS1 = pGirderLiftingSpecCriteria->GetLiftingCrackingFs();
         Float64 FS2 = pGirderLiftingSpecCriteria->GetLiftingFailureFs();

         Float64 loc = 0.0;
         while ( loc <= hp1 )
         {
            pProgress->UpdateMessage(_T("Working..."));
            pgsLiftingAnalysisArtifact artifact;

            pArtifact->CreateLiftingAnalysisArtifact(span,gdr,loc,&artifact);

            AddGraphPoint(seriesFS1,loc,artifact.GetMinFsForCracking());
            AddGraphPoint(seriesFS2,loc,artifact.GetFsFailure());
            AddGraphPoint(limitFS1,loc,FS1);
            AddGraphPoint(limitFS2,loc,FS2);

            loc += stepSize;
         }

         // set flags that graph is built and up to date
         m_bValidGraph       = true;
         m_bUpdateError      = false;
      }
      else
      {
         m_bValidGraph = false;
      }
   }
   else
   {
      GET_IFACE(IGirderHaulingSpecCriteria,pGirderHaulingSpecCriteria);
      if (pGirderHaulingSpecCriteria->IsHaulingCheckEnabled())
      {
         CString strTitle;
         strTitle.Format(_T("Effect of support location - Transportation Stage - %s"),m_PrintSubtitle.c_str());
         m_Graph.SetTitle(std::_tstring(strTitle));
         m_Graph.SetXAxisTitle(_T("Truck Support Location from End of Girder (") + m_pXFormat->UnitTag() + _T(")"));

         GET_IFACE(IGirderHaulingPointsOfInterest,pHaulingPoi);

         Float64 FS1 = pGirderHaulingSpecCriteria->GetHaulingCrackingFs();
         Float64 FS2 = pGirderHaulingSpecCriteria->GetHaulingRolloverFs();

         Float64 loc = pHaulingPoi->GetMinimumOverhang(span,gdr);
         while ( loc <= hp1 )
         {
            pProgress->UpdateMessage(_T("Working..."));
            pgsHaulingAnalysisArtifact artifact;
   #pragma Reminder("REVIEW: Equal overhangs")
            // this is probably the best thing to do with this view... but... give it some thought
            pArtifact->CreateHaulingAnalysisArtifact(span,gdr,loc,loc,&artifact);

            AddGraphPoint(seriesFS1,loc,artifact.GetMinFsForCracking());
            AddGraphPoint(seriesFS2,loc,artifact.GetFsRollover());

            AddGraphPoint(limitFS1,loc,FS1);
            AddGraphPoint(limitFS2,loc,FS2);

            loc += stepSize;
         }

         // set flags that graph is built and up to date
         m_bValidGraph       = true;
         m_bUpdateError      = false;

      }
      else
      {
         m_bValidGraph=false;
      }
   }


   // time to redraw
   Invalidate();
   UpdateWindow();
}

void CFactorOfSafetyView::AddGraphPoint(IndexType series, Float64 xval, Float64 yval)
{
   // deal with unit conversion
   arvPhysicalConverter* pcx = dynamic_cast<arvPhysicalConverter*>(m_pXFormat);
   ASSERT(pcx);
   arvPhysicalConverter* pcy = dynamic_cast<arvPhysicalConverter*>(m_pYFormat);
   ASSERT(pcy);
   m_Graph.AddPoint(series, gpPoint2d(pcx->Convert(xval),pcy->Convert(yval)));
}


void CFactorOfSafetyView::OnUpdateFilePrint(CCmdUI* pCmdUI) 
{
   BOOL flag = (m_bValidGraph && !m_bUpdateError) ? TRUE:FALSE;
	pCmdUI->Enable(flag); 
}

void CFactorOfSafetyView::OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo) 
{
	CView::OnBeginPrinting(pDC, pInfo);
}

void CFactorOfSafetyView::OnEndPrinting(CDC* pDC, CPrintInfo* pInfo) 
{
	CView::OnEndPrinting(pDC, pInfo);
}

BOOL CFactorOfSafetyView::OnPreparePrinting(CPrintInfo* pInfo) 
{
	if (DoPreparePrinting(pInfo))
	   return CView::OnPreparePrinting(pInfo);
   else
      return FALSE;
}

void CFactorOfSafetyView::OnPrint(CDC* pDC, CPrintInfo* pInfo) 
{
   // get paper size
   PGSuperCalculationSheet border(m_pBroker);
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

