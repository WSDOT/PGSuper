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

// GirderModelChildFrame.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuper.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "GirderModelChildFrame.h"
#include "GirderModelSectionView.h"
#include "GirderModelElevationView.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DrawBridgeSettings.h>
#include <IFace\DisplayUnits.h>

#include <PgsExt\PointOfInterest.h>

#include "PGSuperTypes.h"
#include "SectionCutDlg.h"
#include "SectionCutDlgEx.h"
#include "GirderViewPrintJob.h"
#include "EditPointLoadDlg.h"
#include "EditDistributedLoadDlg.h"
#include "htmlhelp\HelpTopics.hh"

#include <WBFLDManip.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// convert combo box index to stage
inline UserLoads::Stage GetCBStage(int sel)
{
   switch(sel)
   {
   case 0:
      return UserLoads::BridgeSite1;
   case 1:
      return UserLoads::BridgeSite2;
   case 2:
      return UserLoads::BridgeSite3;
   default:
      ATLASSERT(0);
   }

   return UserLoads::BridgeSite1;
}


static CString Get_Unit_Tag(bool siMode);
static Float64 Convert_Length_To_Display(Float64 val,bool siMode);
static Float64 Convert_Length_From_Display(Float64 val,bool siMode);

/////////////////////////////////////////////////////////////////////////////
// CGirderModelChildFrame

IMPLEMENT_DYNCREATE(CGirderModelChildFrame, CSplitChildFrame)

CGirderModelChildFrame::CGirderModelChildFrame():
m_CurrentCutLocation(0),
m_CutLocation(Center),
m_LoadingStage(UserLoads::BridgeSite1)
{
   m_CurrentSpanIdx   = 0;
   m_CurrentGirderIdx = 0;
}

CGirderModelChildFrame::~CGirderModelChildFrame()
{
}

BEGIN_MESSAGE_MAP(CGirderModelChildFrame, CSplitChildFrame)
	//{{AFX_MSG_MAP(CGirderModelChildFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrintDirect)
	ON_CBN_SELCHANGE(IDC_SELSTAGE, OnSelectLoadingStage)
	ON_COMMAND(ID_GV_ADD_POINTLOAD, OnAddPointload)
	ON_COMMAND(ID_ADD_GV_DISTRIBUTED_LOAD, OnAddGvDistributedLoad)
   ON_CBN_SELCHANGE( IDC_GIRDER, OnGirderChanged )
   ON_CBN_SELCHANGE( IDC_SPAN, OnSpanChanged )
   ON_COMMAND(IDC_SECTION_CUT, OnSectionCut )
	ON_BN_CLICKED(IDC_SYNC, OnSync)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_HELP, OnCommandHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderModelChildFrame message handlers
void CGirderModelChildFrame::SelectSpanAndGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx)
{
   m_CurrentSpanIdx   = spanIdx;
   m_CurrentGirderIdx = gdrIdx;
   UpdateBar();
   UpdateViews();
}

void CGirderModelChildFrame::GetSpanAndGirderSelection(SpanIndexType* pSpanIdx,GirderIndexType* pGdrIdx)
{
   *pSpanIdx = m_CurrentSpanIdx;
   *pGdrIdx  = m_CurrentGirderIdx;
}

bool CGirderModelChildFrame::SyncWithBridgeModelView()
{
   CButton* pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_SYNC);
   return (pBtn->GetCheck() == 0 ? false : true);
}

void CGirderModelChildFrame::RefreshGirderLabeling()
{
   UpdateBar();
}

CRuntimeClass* CGirderModelChildFrame::GetLowerPaneClass() const
{
   return RUNTIME_CLASS(CGirderModelSectionView);
}

double CGirderModelChildFrame::GetTopFrameFraction() const
{
   return 0.4;
}

int CGirderModelChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSplitChildFrame::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if ( !m_SettingsBar.Create( this, IDD_GIRDER_ELEVATION_BAR, CBRS_TOP, IDD_GIRDER_ELEVATION_BAR) )
	{
		TRACE0("Failed to create control bar\n");
		return -1;      // fail to create
	}

   m_SettingsBar.SetBarStyle(m_SettingsBar.GetBarStyle() |
   CBRS_TOOLTIPS | CBRS_FLYBY);
	
   // point load tool
   CComPtr<iTool> point_load_tool;
   ::CoCreateInstance(CLSID_Tool,NULL,CLSCTX_ALL,IID_iTool,(void**)&point_load_tool);
   point_load_tool->SetID(IDC_POINT_LOAD_DRAG);
   point_load_tool->SetToolTipText("Drag me onto girder to create a point load");

   CComQIPtr<iToolIcon, &IID_iToolIcon> pti(point_load_tool);
   pti->SetIcon(::AfxGetInstanceHandle(), IDI_POINT_LOAD);

   m_SettingsBar.AddTool(point_load_tool);

   // distributed load tool
   CComPtr<iTool> distributed_load_tool;
   ::CoCreateInstance(CLSID_Tool,NULL,CLSCTX_ALL,IID_iTool,(void**)&distributed_load_tool);
   distributed_load_tool->SetID(IDC_DISTRIBUTED_LOAD_DRAG);
   distributed_load_tool->SetToolTipText("Drag me onto girder to create a distributed load");

   CComQIPtr<iToolIcon, &IID_iToolIcon> dti(distributed_load_tool);
   dti->SetIcon(::AfxGetInstanceHandle(), IDI_DISTRIBUTED_LOAD);

   m_SettingsBar.AddTool(distributed_load_tool);

   // moment load tool
   CComPtr<iTool> moment_load_tool;
   ::CoCreateInstance(CLSID_Tool,NULL,CLSCTX_ALL,IID_iTool,(void**)&moment_load_tool);
   moment_load_tool->SetID(IDC_MOMENT_LOAD_DRAG);
   moment_load_tool->SetToolTipText("Drag me onto girder to create a moment load");
   
   CComQIPtr<iToolIcon, &IID_iToolIcon> mti(moment_load_tool);
   mti->SetIcon(::AfxGetInstanceHandle(), IDI_MOMENT_LOAD);

   m_SettingsBar.AddTool(moment_load_tool);

   CPGSuperApp* pApp = (CPGSuperApp*)AfxGetApp();
   UINT settings = pApp->GetGirderEditorSettings();
   CButton* pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_SYNC);
   pBtn->SetCheck( settings & IDG_SV_SYNC_GIRDER ? TRUE : FALSE);

   if ( SyncWithBridgeModelView() ) 
   {
      CPGSuperDoc* pdoc = (CPGSuperDoc*) GetActiveDocument();
      m_CurrentSpanIdx = pdoc->GetSpanIdx();
      m_CurrentGirderIdx = pdoc->GetGirderIdx();
   }

   UpdateBar();

	return 0;
}

CGirderModelElevationView* CGirderModelChildFrame::GetGirderModelElevationView() const
{
   CWnd* pwnd = m_SplitterWnd.GetPane(0, 0);
   CGirderModelElevationView* pvw = dynamic_cast<CGirderModelElevationView*>(pwnd);
   ASSERT(pvw);
   return pvw;
}

CGirderModelSectionView* CGirderModelChildFrame::GetGirderModelSectionView() const
{
   CWnd* pwnd = m_SplitterWnd.GetPane(1, 0);
   CGirderModelSectionView* pvw = dynamic_cast<CGirderModelSectionView*>(pwnd);
   ASSERT(pvw);
   return pvw;
}

void CGirderModelChildFrame::UpdateViews()
{
   GetGirderModelSectionView()->OnUpdate(NULL,0,NULL);
   GetGirderModelElevationView()->OnUpdate(NULL,0,NULL);
}

void CGirderModelChildFrame::Update()
{
   UpdateBar();
   UpdateViews();
}

void CGirderModelChildFrame::UpdateCutLocation(CutLocation cutLoc,Float64 cut)
{
   m_CurrentCutLocation = cut;
   m_CutLocation = cutLoc;
   UpdateBar();
   GetGirderModelSectionView()->OnUpdate(NULL, HINT_GIRDERVIEWSECTIONCUTCHANGED, NULL);
   GetGirderModelElevationView()->OnUpdate(NULL, HINT_GIRDERVIEWSECTIONCUTCHANGED, NULL);
}

void CGirderModelChildFrame::UpdateBar()
{
   CComboBox* pspan_ctrl     = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SPAN);
   CComboBox* pgirder_ctrl   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_GIRDER);
   CComboBox* pstage_ctrl    = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SELSTAGE);
   CStatic* plocation_static = (CStatic*)m_SettingsBar.GetDlgItem(IDC_SECTION_CUT);
   ASSERT(pspan_ctrl);
   ASSERT(pgirder_ctrl);
   ASSERT(plocation_static);

   SpanIndexType spanIdx, gdrIdx;
   GetSpanAndGirderSelection(&spanIdx,&gdrIdx);

   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridge, pBridge);

   // make sure controls are in sync with actual data
   // spans
   SpanIndexType num_spans = pBridge->GetSpanCount();
   ASSERT(num_spans!=0);
   int nc_items = pspan_ctrl->GetCount();
   if (nc_items != num_spans)
   {
      // number of spans has changed, need to refill control
      int sel = pspan_ctrl->GetCurSel();
      
      if (sel == CB_ERR)
         sel = spanIdx;

      pspan_ctrl->ResetContent();
      CString csv;
      for (SpanIndexType i=0; i<num_spans; i++)
      {
         csv.Format("Span %i", i+1);
         pspan_ctrl->AddString(csv);
      }
   }
   pspan_ctrl->SetCurSel(spanIdx);

   if ( pspan_ctrl->GetCurSel() != CB_ERR )
   {
      // girders
      GirderIndexType num_girders = pBridge->GetGirderCount(spanIdx);
      pgirder_ctrl->ResetContent();
      CString csv;
      for (GirderIndexType i=0; i<num_girders; i++)
      {
         csv.Format("Girder %s", LABEL_GIRDER(i));
         pgirder_ctrl->AddString(csv);
      }
      pgirder_ctrl->SetCurSel(gdrIdx);
   }

   if ( spanIdx != ALL_SPANS && gdrIdx != ALL_GIRDERS )
   {
      // cut location
      Float64 gird_len = pBridge->GetGirderLength(spanIdx,gdrIdx);
      m_MaxCutLocation = gird_len;

      if (m_CutLocation==UserInput)
      {
         if (m_CurrentCutLocation > gird_len)
            m_CurrentCutLocation = gird_len;
      }
      else if (m_CutLocation == LeftEnd)
      {
         m_CurrentCutLocation = 0.0;
      }
      else if (m_CutLocation == RightEnd)
      {
         m_CurrentCutLocation = gird_len;
      }
      else if (m_CutLocation == Center)
      {
         m_CurrentCutLocation = gird_len/2.0;
      }
      else
      {
         // cut was taken at a harping point, must enlist poi interface
         GET_IFACE2(pBroker, IPointOfInterest, pPoi);
         std::vector<pgsPointOfInterest> poi;
         std::vector<pgsPointOfInterest>::iterator iter;
         poi = pPoi->GetPointsOfInterest(pgsTypes::CastingYard, spanIdx, gdrIdx, POI_HARPINGPOINT);
         int nPoi = poi.size();
         ASSERT(0 < nPoi && nPoi <3);
         iter = poi.begin();
         pgsPointOfInterest left_hp_poi = *iter++;
         pgsPointOfInterest right_hp_poi = left_hp_poi;
         if ( nPoi == 2 )
            right_hp_poi = *iter++;

         if (m_CutLocation == LeftHarp)
         {
            m_CurrentCutLocation = left_hp_poi.GetDistFromStart();
         }
         else if (m_CutLocation == RightHarp)
         {
            m_CurrentCutLocation = right_hp_poi.GetDistFromStart();
         }
         else
            ASSERT(0); // unknown cut location type
      }

      GET_IFACE2(pBroker,IDisplayUnits,pDisplayUnits);
      CString msg;
      msg.Format("Section Cut Offset: %s",FormatDimension(m_CurrentCutLocation,pDisplayUnits->GetXSectionDimUnit()));

      plocation_static->SetWindowText(msg);
      plocation_static->EnableWindow();
   }
   else
   {
      plocation_static->EnableWindow(FALSE);
   }

   // loading stage
   int sel = pstage_ctrl->GetCurSel();
   if (sel==CB_ERR) 
   {
      m_LoadingStage = UserLoads::BridgeSite1;
      pstage_ctrl->SetCurSel(0);
   }
   else
      m_LoadingStage = GetCBStage(sel);

   // frame title
   OnUpdateFrameTitle(TRUE);
}

void CGirderModelChildFrame::OnGirderChanged()
{
   CComboBox* pgirder_ctrl   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_GIRDER);
   m_CurrentGirderIdx = pgirder_ctrl->GetCurSel();

   if ( SyncWithBridgeModelView() ) 
   {
      CPGSuperDoc* pdoc = (CPGSuperDoc*) GetActiveDocument();
      pdoc->SelectGirder(m_CurrentSpanIdx,m_CurrentGirderIdx);
   }

   UpdateBar();
   UpdateViews();
}

void CGirderModelChildFrame::OnSpanChanged()
{
   CComboBox* pspan_ctrl     = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SPAN);
   m_CurrentSpanIdx = pspan_ctrl->GetCurSel();

   CComPtr<IBroker> pBroker;
   AfxGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);
   GirderIndexType nGirders = pBridge->GetGirderCount(m_CurrentSpanIdx);
   m_CurrentGirderIdx = min(m_CurrentGirderIdx,nGirders-1);

   if ( SyncWithBridgeModelView() ) 
   {
      CPGSuperDoc* pdoc = (CPGSuperDoc*) GetActiveDocument();
      pdoc->SelectGirder(m_CurrentSpanIdx,m_CurrentGirderIdx);
   }
   else
   {
      UpdateBar();
      UpdateViews();
   }
}

void CGirderModelChildFrame::OnSelectLoadingStage() 
{
   UpdateBar();
   UpdateViews();
}

void CGirderModelChildFrame::OnSectionCut()
{
   ShowCutDlg();
}

void CGirderModelChildFrame::ShowCutDlg()
{
   CPGSuperDoc* pdoc = (CPGSuperDoc*) GetActiveDocument();
   bool si_mode = pdoc->GetUnitsMode() == libUnitsMode::UNITS_SI;
   Float64 val  = m_CurrentCutLocation;
   Float64 high = m_MaxCutLocation;

   CComPtr<IBroker> pBroker;
   pdoc->GetBroker(&pBroker);
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   SpanIndexType span;
   GirderIndexType gdr;
   GetSpanAndGirderSelection(&span,&gdr);

   ATLASSERT( span != ALL_SPANS && gdr != ALL_GIRDERS  );
   Uint16 nHarpPoints = pStrandGeom->GetNumHarpPoints(span,gdr);

   CSectionCutDlgEx dlg(nHarpPoints,m_CurrentCutLocation,0.0,high,si_mode,m_CutLocation);

   int st = dlg.DoModal();
   if (st==IDOK)
   {
      m_CurrentCutLocation = dlg.GetValue();
      UpdateCutLocation(dlg.GetCutLocation(),m_CurrentCutLocation);
   }
}

void CGirderModelChildFrame::CutAt(Float64 cut)
{
   UpdateCutLocation(UserInput,cut);
}

void CGirderModelChildFrame::CutAtLeftEnd() 
{
   UpdateCutLocation(LeftEnd);
}

void CGirderModelChildFrame::CutAtLeftHp() 
{
   UpdateCutLocation(LeftHarp);
}

void CGirderModelChildFrame::CutAtCenter() 
{
   UpdateCutLocation(Center);
}

void CGirderModelChildFrame::CutAtRightHp() 
{
   UpdateCutLocation(RightHarp);
}

void CGirderModelChildFrame::CutAtRightEnd() 
{
   UpdateCutLocation(RightEnd);
}

void CGirderModelChildFrame::CutAtNext()
{
   double f = m_CurrentCutLocation/m_MaxCutLocation;
   f = ::RoundOff(f+0.1,0.1);
   if ( 1 < f )
      f = 1;

   CutAt(f*m_MaxCutLocation);
}

void CGirderModelChildFrame::CutAtPrev()
{
   double f = m_CurrentCutLocation/m_MaxCutLocation;
   f = ::RoundOff(f-0.1,0.1);
   if ( f < 0 )
      f = 0;

   CutAt(f*m_MaxCutLocation);
}

void CGirderModelChildFrame::CutAtLocation()
{
   CPGSuperDoc* pdoc = (CPGSuperDoc*) GetActiveDocument();
   bool si_mode = pdoc->GetUnitsMode() == libUnitsMode::UNITS_SI;
   Float64 val  = Convert_Length_To_Display(m_CurrentCutLocation,si_mode);
   Float64 high = Convert_Length_To_Display(m_MaxCutLocation,si_mode);

   CSectionCutDlg dlg(val,0.0,high,Get_Unit_Tag(si_mode));

   int st = dlg.DoModal();
   if (st==IDOK)
   {
      val = Convert_Length_From_Display(dlg.GetValue(),si_mode);
      CutAt(val);
   }

   // Because the dialog messes with the screen
   // force an update (this is a hack because of the selection tool).
   GetGirderModelElevationView()->Invalidate();
   GetGirderModelElevationView()->UpdateWindow();
}

/////// utility functions
CString Get_Unit_Tag(bool siMode)
{

   if (siMode)
      return unitMeasure::Meter.UnitTag().c_str();
   else
      return unitMeasure::Feet.UnitTag().c_str();
}

Float64 Convert_Length_To_Display(Float64 val,bool siMode)
{
   if (siMode)
      return ConvertFromSysUnits(val, unitMeasure::Meter);
   else
      return ConvertFromSysUnits(val, unitMeasure::Feet);
}

Float64 Convert_Length_From_Display(Float64 val,bool siMode)
{
   if (siMode)
      return ConvertToSysUnits(val, unitMeasure::Meter);
   else
      return ConvertToSysUnits(val, unitMeasure::Feet);
}

void CGirderModelChildFrame::OnFilePrintDirect() 
{
   DoFilePrint(true);
}

void CGirderModelChildFrame::OnFilePrint() 
{
   DoFilePrint(false);
}

void CGirderModelChildFrame::DoFilePrint(bool direct) 
{
   CGirderModelElevationView* pev = GetGirderModelElevationView();
   CGirderModelSectionView*   psv = GetGirderModelSectionView();

   CPGSuperDoc* pDoc = (CPGSuperDoc*) GetActiveDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);
   
   // create a print job and do it
   CGirderViewPrintJob pj(pev, psv, this, pBroker);
   pj.OnFilePrint(direct);
}

void CGirderModelChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if (bAddToTitle)
   {
      SpanIndexType spanIdx;
      GirderIndexType gdrIdx;
      GetSpanAndGirderSelection(&spanIdx,&gdrIdx);
      CString msg;
      if (  spanIdx != ALL_SPANS && gdrIdx != ALL_GIRDERS  )
         msg.Format("Girder Model View - Span %d, Girder %s", LABEL_SPAN(spanIdx), LABEL_GIRDER(gdrIdx));
      else
         msg.Format("%s","Girder Model View");

      // set our title
		AfxSetWindowText(m_hWnd, msg);
   }
}

void CGirderModelChildFrame::OnAddPointload()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*) GetActiveDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   SpanIndexType spanIdx, gdrIdx;
   GetSpanAndGirderSelection(&spanIdx,&gdrIdx);

   ATLASSERT(  spanIdx != ALL_SPANS && gdrIdx != ALL_GIRDERS  ); // if we are adding a point load, a girder better be selected

   // set data to that of view
   CPointLoadData data;
   data.m_Span   = spanIdx;
   data.m_Girder = gdrIdx;

   if (this->m_LoadingStage != UserLoads::BridgeSite3)
      data.m_Stage = this->m_LoadingStage;
   else
      data.m_LoadCase = UserLoads::LL_IM;

	CEditPointLoadDlg dlg(data,pBroker);
   if (dlg.DoModal() == IDOK)
   {
      GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
      pUdl->AddPointLoad(dlg.m_Load);
   }
}

void CGirderModelChildFrame::OnAddGvDistributedLoad() 
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*) GetActiveDocument();
   CComPtr<IBroker> pBroker;
   pDoc->GetBroker(&pBroker);

   SpanIndexType spanIdx, gdrIdx;
   GetSpanAndGirderSelection(&spanIdx,&gdrIdx);

   ATLASSERT(  spanIdx != ALL_SPANS && gdrIdx != ALL_GIRDERS  ); // if we are adding a point load, a girder better be selected

   // set data to that of view
   CDistributedLoadData data;
   data.m_Span   = spanIdx;
   data.m_Girder = gdrIdx;

   if (this->m_LoadingStage != UserLoads::BridgeSite3)
      data.m_Stage = this->m_LoadingStage;
   else
      data.m_LoadCase = UserLoads::LL_IM;

	CEditDistributedLoadDlg dlg(data,pBroker);
   if (dlg.DoModal() == IDOK)
   {
      GET_IFACE2(pBroker,IUserDefinedLoadData, pUdl);
      pUdl->AddDistributedLoad(dlg.m_Load);
   }
}

LRESULT CGirderModelChildFrame::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDER_VIEW );
   return TRUE;
}

void CGirderModelChildFrame::OnSync() 
{
   CPGSuperApp* pApp = (CPGSuperApp*)AfxGetApp();
   UINT settings = pApp->GetGirderEditorSettings();

   if ( SyncWithBridgeModelView() )
   {
      settings |= IDG_SV_SYNC_GIRDER;

      SpanIndexType spanIdx, gdrIdx;
      GetSpanAndGirderSelection(&spanIdx,&gdrIdx);

      CPGSuperDoc* pDoc = (CPGSuperDoc*) GetActiveDocument();
      pDoc->SelectGirder(spanIdx,gdrIdx);
   }
   else
   {
      settings &= ~IDG_SV_SYNC_GIRDER;

      GetGirderModelElevationView()->Invalidate();
      GetGirderModelSectionView()->Invalidate();
   }

   pApp->SetGirderEditorSettings(settings);
}
