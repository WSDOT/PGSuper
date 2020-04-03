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

// GirderModelChildFrame.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "resource.h"
#include "PGSuperDoc.h"
#include "PGSuperUnits.h"
#include "GirderModelChildFrame.h"
#include "GirderModelSectionView.h"
#include "GirderModelElevationView.h"

#include <GirderModelViewController.h>
#include "GirderModelViewControllerImp.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\DrawBridgeSettings.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\EditByUI.h>

#include <PgsExt\ReportPointOfInterest.h>
#include <PgsExt\BridgeDescription2.h>

#include "PGSuperTypes.h"
#include "SectionCutDlg.h"
#include "SectionCutDlgEx.h"
#include "GirderViewPrintJob.h"
#include "EditPointLoadDlg.h"
#include "EditDistributedLoadDlg.h"
#include "EditMomentLoadDlg.h"

#include <PgsExt\InsertDeleteLoad.h>

#include <WBFLDManip.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// NOTES:
// The girder model elevation view uses one of two coordinate systems depending on what is being displayed.
// If the girder elevation view is for a single group (or span), the girder path coordinate system is used.
// If the girder elevation is for ALL GROUPS a coordinate system similar to the girder line coordinate system is used.
// Call this the pseudo girder line coordinate system. This coordinate system is like the girder line coordinate system
// in that it begins at its origin and continues for the entire lenght of the girder line. However, the origin
// is different then the girder line coordate system. In the girder line coordinate system, the origin is at the start
// face of the first segment. For the pseudo-girder line coordinate system, the origin is at the point where the
// first Abutment Reference Line intersects the girder line. This is origin is in the same location as the origin
// of the girder path coordinate system.
//
// Use the ConvertToGirderlineCoordinates and ConvertFromGirderlineCoordinates functions to convert girderline coordinates
// to/from the pseudo-girder line coordinate system.

// template function to directly set item data in a combo box
template <class T>
int SetCBCurSelItemData( CComboBox* pCB, T& itemdata )
{
   int succ=CB_ERR;
   int count = pCB->GetCount();
   for ( int i = 0; i < count; i++ )
   {
      if ( ((T)pCB->GetItemData(i)) == itemdata )
      {
         pCB->SetCurSel(i);
         succ=i;
         break;
      }
   }

   return succ;
}

/////////////////////////////////////////////////////////////////////////////
// CGirderModelChildFrame

IMPLEMENT_DYNCREATE(CGirderModelChildFrame, CSplitChildFrame)

CGirderModelChildFrame::CGirderModelChildFrame():
m_EventIndex(0),
m_GirderKey(ALL_GROUPS,0),
m_bIsAfterFirstUpdate(false)
{
   CEAFViewControllerFactory::Init(this);
}

CGirderModelChildFrame::~CGirderModelChildFrame()
{
}

BOOL CGirderModelChildFrame::Create(LPCTSTR lpszClassName,
				LPCTSTR lpszWindowName,
				DWORD dwStyle,
				const RECT& rect,
				CMDIFrameWnd* pParentWnd,
				CCreateContext* pContext)
{
   BOOL bResult = CSplitChildFrame::Create(lpszClassName,lpszWindowName,dwStyle,rect,pParentWnd,pContext);
   if ( bResult )
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
      HICON hIcon = AfxGetApp()->LoadIcon(IDR_GIRDERMODELEDITOR);
      SetIcon(hIcon,TRUE);
   }

   return bResult;
}

BOOL CGirderModelChildFrame::OnCmdMsg(UINT nID,int nCode,void* pExtra,AFX_CMDHANDLERINFO* pHandlerInfo)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();

   // capture the current selection
   CSelection selection = pDoc->GetSelection();

   bool bSync = SyncWithBridgeModelView();
   BOOL bIsQuickReportCommand = pDoc->IsReportCommand(nID,TRUE);
   if ( bIsQuickReportCommand && nCode == CN_COMMAND /*&& !bSync*/ )
   {
      // the command is for a "quick report" and we are not sync'ed with the bridge view... we want to use our selection
     
      // get the selection for this view
      const CGirderKey& girderKey = GetSelection();

      // create and set the main selection.... later, when the report is created, it will call GetSelection and our selection will be returned
      CSelection ourSelection;
      ourSelection.Type = CSelection::Girder;
      ourSelection.GroupIdx = girderKey.groupIndex;
      ourSelection.GirderIdx = girderKey.girderIndex;
      pDoc->SetSelection(ourSelection,FALSE/* don't broadcast a selection change notification */);
   }
   
   // continue with normal command processing
   BOOL bHandled = CSplitChildFrame::OnCmdMsg(nID,nCode,pExtra,pHandlerInfo);

   if ( bIsQuickReportCommand && nCode == CN_COMMAND /*&& !bSync*/ )
   {
      // we messed with the selection, so put it back the way it was
      pDoc->SetSelection(selection,FALSE/* don't broadcast a selection change notification */);
   }

   return bHandled;
}

BEGIN_MESSAGE_MAP(CGirderModelChildFrame, CSplitChildFrame)
	//{{AFX_MSG_MAP(CGirderModelChildFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_FILE_PRINT, OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, OnFilePrintDirect)
	ON_CBN_SELCHANGE(IDC_SELEVENT, OnSelectEvent)
	ON_COMMAND(ID_ADD_POINT_LOAD_CTX, OnAddPointload)
	ON_COMMAND(ID_ADD_DISTRIBUTED_LOAD_CTX, OnAddDistributedLoad)
	ON_COMMAND(ID_ADD_MOMENT_LOAD_CTX, OnAddMoment)
   ON_CBN_SELCHANGE( IDC_GIRDER, OnGirderChanged )
   ON_CBN_SELCHANGE( IDC_SPAN, OnGroupChanged )
   ON_COMMAND(IDC_SECTION_CUT, OnSectionCut )
	ON_BN_CLICKED(IDC_SYNC, OnSync)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_HELP, OnCommandHelp)
   ON_COMMAND(ID_GIRDERVIEW_DESIGNGIRDERDIRECT, OnDesignGirderDirect)
   ON_UPDATE_COMMAND_UI(ID_GIRDERVIEW_DESIGNGIRDERDIRECT, OnUpdateDesignGirderDirect)
   ON_COMMAND(ID_GIRDERVIEW_DESIGNGIRDERDIRECTHOLDSLABOFFSET, OnDesignGirderDirectHoldSlabOffset)
   ON_UPDATE_COMMAND_UI(ID_GIRDERVIEW_DESIGNGIRDERDIRECTHOLDSLABOFFSET, OnUpdateDesignGirderDirectHoldSlabOffset)
   ON_WM_SETFOCUS()
   ON_BN_CLICKED(IDC_STRANDS,OnStrandsButton)
   ON_UPDATE_COMMAND_UI(IDC_STRANDS,OnUpdateStrandsButton)
   ON_BN_CLICKED(IDC_STRANDS_CG, OnStrandsCGButton)
   ON_UPDATE_COMMAND_UI(IDC_STRANDS_CG, OnUpdateStrandsCGButton)
   ON_BN_CLICKED(IDC_DIMENSIONS, OnDimensionsButton)
   ON_UPDATE_COMMAND_UI(IDC_DIMENSIONS, OnUpdateDimensionsButton)
   ON_BN_CLICKED(IDC_PROPERTIES, OnPropertiesButton)
   ON_UPDATE_COMMAND_UI(IDC_PROPERTIES, OnUpdatePropertiesButton)
   ON_BN_CLICKED(IDC_LONGITUDINAL_REINFORCEMENT, OnLongitudinalReinforcementButton)
   ON_UPDATE_COMMAND_UI(IDC_LONGITUDINAL_REINFORCEMENT, OnUpdateLongitudinalReinforcementButton)
   ON_BN_CLICKED(IDC_STIRRUPS, OnStirrupsButton)
   ON_UPDATE_COMMAND_UI(IDC_STIRRUPS, OnUpdateStirrupsButton)
   ON_BN_CLICKED(IDC_USER_LOADS, OnUserLoadsButton)
   ON_UPDATE_COMMAND_UI(IDC_USER_LOADS, OnUpdateUserLoadsButton)
   ON_BN_CLICKED(IDC_SCHEMATIC, OnSchematicButton)
   ON_UPDATE_COMMAND_UI(IDC_SCHEMATIC, OnUpdateSchematicButton)
   ON_BN_CLICKED(IDC_SECTION_CG, OnSectionCGButton)
   ON_UPDATE_COMMAND_UI(IDC_SECTION_CG, OnUpdateSectionCGButton)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderModelChildFrame message handlers
bool CGirderModelChildFrame::SetEvent(EventIndexType eventIdx)
{
   CComboBox* pcbEvents = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SELEVENT);
   int nItems = pcbEvents->GetCount();
   for (int idx = 0; idx < nItems; idx++)
   {
      EventIndexType eIdx = (EventIndexType)(pcbEvents->GetItemData(idx));
      if (eIdx == eventIdx)
      {
         m_EventIndex = eventIdx;

         pcbEvents->SetCurSel(idx);
         UpdateBar();
         UpdateViews();

         return true;
      }
   }
   return false;
}

void CGirderModelChildFrame::SelectGirder(const CGirderKey& girderKey,bool bDoUpdate)
{
   if ( girderKey.groupIndex != INVALID_INDEX && girderKey.girderIndex != INVALID_INDEX )
   {
      m_GirderKey = girderKey;
      UpdateCutRange();
      UpdateBar();

      if ( bDoUpdate )
      {
   	   UpdateViews();
      }
   }
   else
   {
      ATLASSERT(false);
   }
}

const CGirderKey& CGirderModelChildFrame::GetSelection() const
{
   return m_GirderKey;
}

void CGirderModelChildFrame::DoSyncWithBridgeModelView(bool bSync)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
   UINT settings = pDoc->GetGirderEditorSettings();

   if (bSync)
   {
      settings |= IDG_SV_SYNC_GIRDER;
      pDoc->SelectGirder(m_GirderKey);
   }
   else
   {
      settings &= ~IDG_SV_SYNC_GIRDER;
   }

   pDoc->SetGirderEditorSettings(settings);
}

void CGirderModelChildFrame::SyncWithBridgeModelView(bool bSync)
{
   DoSyncWithBridgeModelView(bSync);
   CButton* pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_SYNC);
   pBtn->SetCheck(bSync ? BST_CHECKED : BST_UNCHECKED);
}

bool CGirderModelChildFrame::SyncWithBridgeModelView() const
{
   CButton* pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_SYNC);
   return (pBtn->GetCheck() == 0 ? false : true);
}

void CGirderModelChildFrame::ShowStrands(bool bShow)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   if (bShow)
   {
      sysFlags<UINT>::Set(&settings, IDG_EV_SHOW_STRANDS);
      sysFlags<UINT>::Set(&settings, IDG_SV_SHOW_STRANDS);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDG_EV_SHOW_STRANDS);
      sysFlags<UINT>::Clear(&settings, IDG_SV_SHOW_STRANDS);
   }
   pDoc->SetGirderEditorSettings(settings);
}

bool CGirderModelChildFrame::ShowStrands() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_STRANDS) ? true : false;
}

void CGirderModelChildFrame::ShowStrandCG(bool bShow)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   if (bShow)
   {
      sysFlags<UINT>::Set(&settings, IDG_EV_SHOW_PS_CG);
      sysFlags<UINT>::Set(&settings, IDG_SV_SHOW_PS_CG);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDG_EV_SHOW_PS_CG);
      sysFlags<UINT>::Clear(&settings, IDG_SV_SHOW_PS_CG);
   }
   pDoc->SetGirderEditorSettings(settings);
}

bool CGirderModelChildFrame::ShowStrandCG() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_STRANDS_CG) ? true : false;
}

void CGirderModelChildFrame::ShowCG(bool bShow)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   if (bShow)
   {
      sysFlags<UINT>::Set(&settings, IDG_SV_GIRDER_CG);
      sysFlags<UINT>::Set(&settings, IDG_EV_GIRDER_CG);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDG_SV_GIRDER_CG);
      sysFlags<UINT>::Clear(&settings, IDG_EV_GIRDER_CG);
   }
   pDoc->SetGirderEditorSettings(settings);
}

bool CGirderModelChildFrame::ShowCG() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_SECTION_CG) ? true : false;
}

void CGirderModelChildFrame::ShowSectionProperties(bool bShow)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   if (bShow)
   {
      sysFlags<UINT>::Set(&settings, IDG_SV_PROPERTIES);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDG_SV_PROPERTIES);
   }
   pDoc->SetGirderEditorSettings(settings);
}

bool CGirderModelChildFrame::ShowSectionProperties() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_PROPERTIES) ? true : false;
}

void CGirderModelChildFrame::ShowDimensions(bool bShow)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   if (bShow)
   {
      sysFlags<UINT>::Set(&settings, IDG_EV_SHOW_DIMENSIONS);
      sysFlags<UINT>::Set(&settings, IDG_SV_SHOW_DIMENSIONS);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDG_EV_SHOW_DIMENSIONS);
      sysFlags<UINT>::Clear(&settings, IDG_SV_SHOW_DIMENSIONS);
   }
   pDoc->SetGirderEditorSettings(settings);
}

bool CGirderModelChildFrame::ShowDimensions() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_DIMENSIONS) ? true : false;
}

void CGirderModelChildFrame::ShowLongitudinalReinforcement(bool bShow)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   if (bShow)
   {
      sysFlags<UINT>::Set(&settings, IDG_EV_SHOW_LONG_REINF);
      sysFlags<UINT>::Set(&settings, IDG_SV_SHOW_LONG_REINF);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDG_EV_SHOW_LONG_REINF);
      sysFlags<UINT>::Clear(&settings, IDG_SV_SHOW_LONG_REINF);
   }
   pDoc->SetGirderEditorSettings(settings);
}

bool CGirderModelChildFrame::ShowLongitudinalReinforcement() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_LONGITUDINAL_REINFORCEMENT) ? true : false;
}

void CGirderModelChildFrame::ShowTransverseReinforcement(bool bShow)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   if (bShow)
   {
      sysFlags<UINT>::Set(&settings, IDG_EV_SHOW_STIRRUPS);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDG_EV_SHOW_STIRRUPS);
   }
   pDoc->SetGirderEditorSettings(settings);
}

bool CGirderModelChildFrame::ShowTransverseReinforcement() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_STIRRUPS) ? true : false;
}

void CGirderModelChildFrame::ShowLoads(bool bShow)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   if (bShow)
   {
      sysFlags<UINT>::Set(&settings, IDG_EV_SHOW_LOADS);
      sysFlags<UINT>::Set(&settings, IDG_EV_SHOW_LEGEND);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDG_EV_SHOW_LOADS);
      sysFlags<UINT>::Clear(&settings, IDG_EV_SHOW_LEGEND);
   }
   pDoc->SetGirderEditorSettings(settings);
}

bool CGirderModelChildFrame::ShowLoads() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_USER_LOADS) ? true : false;
}

void CGirderModelChildFrame::Schematic(bool bSchematic)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   if (bSchematic)
   {
      sysFlags<UINT>::Set(&settings, IDG_EV_DRAW_ISOTROPIC);
   }
   else
   {
      sysFlags<UINT>::Clear(&settings, IDG_EV_DRAW_ISOTROPIC);
   }
   pDoc->SetGirderEditorSettings(settings);
}

bool CGirderModelChildFrame::Schematic() const
{
   return m_SettingsBar.IsDlgButtonChecked(IDC_SCHEMATIC) ? true : false;
}

void CGirderModelChildFrame::RefreshGirderLabeling()
{
   UpdateBar();
}

CRuntimeClass* CGirderModelChildFrame::GetLowerPaneClass() const
{
   return RUNTIME_CLASS(CGirderModelSectionView);
}

Float64 CGirderModelChildFrame::GetTopFrameFraction() const
{
   return 0.4;
}

BOOL CGirderModelChildFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) 
{
   if ( !__super::OnCreateClient(lpcs,pContext) )
   {
      return FALSE;
   }

   CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
   CDocTemplate* pDocTemplate = pDoc->GetDocTemplate();
   ASSERT( pDocTemplate->IsKindOf(RUNTIME_CLASS(CEAFDocTemplate)) );

   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)pDocTemplate;
   CGirderKey girderKey( *(CGirderKey*)pTemplate->GetViewCreationData() );

   if ( girderKey.groupIndex != ALL_GROUPS && girderKey.girderIndex != ALL_GIRDERS)
   {
      m_GirderKey = girderKey;
   }

   return TRUE;
}

int CGirderModelChildFrame::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CSplitChildFrame::OnCreate(lpCreateStruct) == -1)
   {
		return -1;
   }
	
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if ( !m_SettingsBar.Create( this, IDD_GIRDER_EDITOR_BAR, CBRS_TOP, IDD_GIRDER_EDITOR_BAR) )
	{
		TRACE0("Failed to create control bar\n");
		return -1;      // fail to create
	}

   m_SettingsBar.SetBarStyle(m_SettingsBar.GetBarStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);
	
   // point load tool
   CComPtr<iTool> point_load_tool;
   ::CoCreateInstance(CLSID_Tool,nullptr,CLSCTX_ALL,IID_iTool,(void**)&point_load_tool);
   point_load_tool->SetID(IDC_POINT_LOAD_DRAG);
   point_load_tool->SetToolTipText(_T("Drag me onto girder to create a point load"));

   CComQIPtr<iToolIcon, &IID_iToolIcon> pti(point_load_tool);
   HRESULT hr = pti->SetIcon(::AfxGetInstanceHandle(), IDI_POINT_LOAD);
   ATLASSERT(SUCCEEDED(hr));

   m_SettingsBar.AddTool(point_load_tool);

   // distributed load tool
   CComPtr<iTool> distributed_load_tool;
   ::CoCreateInstance(CLSID_Tool,nullptr,CLSCTX_ALL,IID_iTool,(void**)&distributed_load_tool);
   distributed_load_tool->SetID(IDC_DISTRIBUTED_LOAD_DRAG);
   distributed_load_tool->SetToolTipText(_T("Drag me onto girder to create a distributed load"));

   HINSTANCE hInstance = AfxGetInstanceHandle();

   CComQIPtr<iToolIcon, &IID_iToolIcon> dti(distributed_load_tool);
   hr = dti->SetIcon(hInstance, IDI_DISTRIBUTED_LOAD);
   ATLASSERT(SUCCEEDED(hr));

   m_SettingsBar.AddTool(distributed_load_tool);

   // moment load tool
   // Used only with PGSuper (not used for PGSplice)
   if ( EAFGetDocument()->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) )
   {
      CComPtr<iTool> moment_load_tool;
      ::CoCreateInstance(CLSID_Tool,nullptr,CLSCTX_ALL,IID_iTool,(void**)&moment_load_tool);
      moment_load_tool->SetID(IDC_MOMENT_LOAD_DRAG);
      moment_load_tool->SetToolTipText(_T("Drag me onto girder to create a moment load"));
      
      CComQIPtr<iToolIcon, &IID_iToolIcon> mti(moment_load_tool);
      hr = mti->SetIcon(hInstance, IDI_MOMENT_LOAD);
      ATLASSERT(SUCCEEDED(hr));

      m_SettingsBar.AddTool(moment_load_tool);
   }
   else
   {
      m_SettingsBar.GetDlgItem(IDC_MOMENT_LOAD_DRAG)->ShowWindow(SW_HIDE);
   }

   CButton* pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_STRANDS);
   pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_STRANDS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
   m_SettingsBar.AddTooltip(pBtn);

   pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_STRANDS_CG);
   pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_STRANDS_CG), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
   m_SettingsBar.AddTooltip(pBtn);

   pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_DIMENSIONS);
   pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_DIMENSIONS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
   m_SettingsBar.AddTooltip(pBtn);

   pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_PROPERTIES);
   pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_PROPERTIES), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
   m_SettingsBar.AddTooltip(pBtn);

   pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_LONGITUDINAL_REINFORCEMENT);
   pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_LONGITUDINAL_REINFORCEMENT), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
   m_SettingsBar.AddTooltip(pBtn);

   pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_STIRRUPS);
   pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_STIRRUPS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
   m_SettingsBar.AddTooltip(pBtn);

   pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_USER_LOADS);
   pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_USER_LOADS), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
   m_SettingsBar.AddTooltip(pBtn);

   pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_SCHEMATIC);
   pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_SCHEMATIC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
   m_SettingsBar.AddTooltip(pBtn);

   pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_SECTION_CG);
   pBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_SECTION_CG), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));
   m_SettingsBar.AddTooltip(pBtn);

   // sets the check state of the sync button
   CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   pBtn = (CButton*)m_SettingsBar.GetDlgItem(IDC_SYNC);
   pBtn->SetCheck( settings & IDG_SV_SYNC_GIRDER ? TRUE : FALSE);

   if ( SyncWithBridgeModelView() ) 
   {
      // Sync only if we can
      CSelection selection = pDoc->GetSelection();
      if ( selection.Type == CSelection::Girder && selection.GroupIdx != ALL_GROUPS && selection.GirderIdx != ALL_GIRDERS)
      {
	      m_GirderKey.groupIndex = selection.GroupIdx;
    	   m_GirderKey.girderIndex = selection.GirderIdx;
      }
   }

   UpdateCutRange();

   FillEventComboBox();
   UpdateBar();

	return 0;
}

void CGirderModelChildFrame::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint)
{
   if ( lHint == HINT_GIRDERLABELFORMATCHANGED )
   {
      RefreshGirderLabeling();
   }
   else if ( lHint == HINT_SELECTIONCHANGED )
   {
      CSelection* pSelection = (CSelection*)pHint;
      if( (pSelection->Type == CSelection::Girder || pSelection->Type == CSelection::Segment) && SyncWithBridgeModelView() )
      {
         if ( m_GirderKey.groupIndex != pSelection->GroupIdx || m_GirderKey.girderIndex != pSelection->GirderIdx )
         {
            if (0 <= pSelection->GroupIdx || 0 <= pSelection->GirderIdx )
            {
               CGirderKey girderKey(pSelection->GroupIdx,pSelection->GirderIdx);
               SelectGirder(girderKey,false);
            }
         }
      }
   }
   else if (lHint == 0 || lHint == HINT_BRIDGECHANGED || lHint == HINT_GIRDERCHANGED || lHint == HINT_UNITSCHANGED )
   {
      if (lHint == HINT_BRIDGECHANGED)
      {
         // If the bridge changed, make sure the girder key is still valid
         CComPtr<IBroker> pBroker;
         EAFGetBroker(&pBroker);
         GET_IFACE2(pBroker, IBridge, pBridge);
         GroupIndexType nGroups = pBridge->GetGirderGroupCount();
         if (nGroups <= m_GirderKey.groupIndex)
         {
            m_GirderKey.groupIndex = nGroups - 1;
         }

         GirderIndexType nGirders = pBridge->GetGirderCount(m_GirderKey.groupIndex);
         if (nGirders <= m_GirderKey.girderIndex)
         {
            m_GirderKey.girderIndex = nGirders - 1;
         }
      }

      UpdateCutRange();
      m_cutPoi = GetCutPointOfInterest(m_cutPoi.GetDistFromStart());
      FillEventComboBox();
      UpdateBar();
   }

   m_bIsAfterFirstUpdate = true;
}


CGirderModelElevationView* CGirderModelChildFrame::GetGirderModelElevationView() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState()); // GetPane does the "AssertValid" thing, the module state has to be correct
   CWnd* pwnd = m_SplitterWnd.GetPane(0, 0);
   CGirderModelElevationView* pvw = dynamic_cast<CGirderModelElevationView*>(pwnd);
   ASSERT(pvw);
   return pvw;
}

CGirderModelSectionView* CGirderModelChildFrame::GetGirderModelSectionView() const
{
   AFX_MANAGE_STATE(AfxGetAppModuleState()); // GetPane does the "AssertValid" thing, the module state has to be correct
   CWnd* pwnd = m_SplitterWnd.GetPane(1, 0);
   CGirderModelSectionView* pvw = dynamic_cast<CGirderModelSectionView*>(pwnd);
   ASSERT(pvw);
   return pvw;
}

void CGirderModelChildFrame::UpdateViews()
{
   CWaitCursor wait;
   GetGirderModelElevationView()->OnUpdate(nullptr,0,nullptr);
   GetGirderModelSectionView()->OnUpdate(nullptr,0,nullptr);
}

void CGirderModelChildFrame::UpdateCutLocation(const pgsPointOfInterest& poi)
{
   m_cutPoi = poi;
   UpdateBar();
   GetGirderModelSectionView()->OnUpdate(nullptr, HINT_GIRDERVIEWSECTIONCUTCHANGED, nullptr);
   GetGirderModelElevationView()->OnUpdate(nullptr, HINT_GIRDERVIEWSECTIONCUTCHANGED, nullptr);
}

Float64 CGirderModelChildFrame::ConvertFromGirderlineCoordinate(Float64 Xgl) const
{
   // we aren't actually working in girder line coordinates
   // we need X as a distance along the girder line measured relative to the first support
   // this method makes the conversion
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GirderIndexType nGirders = pBridge->GetGirderCount(0);
   CSegmentKey segmentKey(0, Min(m_GirderKey.girderIndex, nGirders - 1), 0);
   Float64 brgOffset = pBridge->GetSegmentStartBearingOffset(segmentKey);
   Float64 endDist = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 offset = brgOffset - endDist;
   Float64 X = Xgl + offset;
   return X;
}

Float64 CGirderModelChildFrame::ConvertToGirderlineCoordinate(Float64 Xgl) const
{
   // we aren't actually working in girder line coordinates
   // we need X as a distance along the girder line measured relative to the first support
   // this method makes the conversion
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridge, pBridge);
   GirderIndexType nGirders = pBridge->GetGirderCount(0);
   CSegmentKey segmentKey(0, Min(m_GirderKey.girderIndex, nGirders - 1), 0);
   Float64 brgOffset = pBridge->GetSegmentStartBearingOffset(segmentKey);
   Float64 endDist = pBridge->GetSegmentStartEndDistance(segmentKey);
   Float64 offset = brgOffset - endDist;
   Float64 X = Xgl - offset;
   return X;
}

void CGirderModelChildFrame::UpdateCutRange()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   PoiList vPoi;
   pPoi->GetPointsOfInterest(CSegmentKey(m_GirderKey, ALL_SEGMENTS), &vPoi);

   m_minPoi = vPoi.front();
   m_maxPoi = vPoi.back();

   if (m_bFirstCut)
   {
      IndexType pos = vPoi.size() / 2; // default is mid-span
      m_cutPoi = vPoi.at(pos);
      m_bFirstCut = false;
   }
   else
   {
      CGirderKey girderKey = m_cutPoi.GetSegmentKey();
      if (m_GirderKey.groupIndex != ALL_GROUPS && !m_GirderKey.IsEqual(girderKey))
      {
         // if the cut poi is no longer on the selected girder, update it
         m_cutPoi = pPoi->GetNearestPointOfInterest(CSegmentKey(m_GirderKey,0), m_cutPoi.GetDistFromStart());
      }

      Float64 Xmin, Xmax, X;
      if (m_GirderKey.groupIndex == ALL_GROUPS)
      {
         Xmin = pPoi->ConvertPoiToGirderlineCoordinate(m_minPoi);
         Xmax = pPoi->ConvertPoiToGirderlineCoordinate(m_maxPoi);
         X = pPoi->ConvertPoiToGirderlineCoordinate(m_cutPoi);

         Xmin = ConvertFromGirderlineCoordinate(Xmin);
         Xmax = ConvertFromGirderlineCoordinate(Xmax);
         X    = ConvertFromGirderlineCoordinate(X);
      }
      else
      {
         Xmin = pPoi->ConvertPoiToGirderPathCoordinate(m_minPoi);
         Xmax = pPoi->ConvertPoiToGirderPathCoordinate(m_maxPoi);
         X = pPoi->ConvertPoiToGirderPathCoordinate(m_cutPoi);
      }

      if (!InRange(Xmin, X, Xmax))
      {
         IndexType pos = vPoi.size() / 2; // default is mid-span
         m_cutPoi = vPoi.at(pos);
      }
   }
}

void CGirderModelChildFrame::UpdateBar()
{
   CComboBox* pcbGroup    = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SPAN);
   CComboBox* pcbGirder   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_GIRDER);
   CStatic* pwndCutLocation = (CStatic*)m_SettingsBar.GetDlgItem(IDC_SECTION_CUT);
   ASSERT(pcbGroup);
   ASSERT(pcbGirder);
   ASSERT(pwndCutLocation);
   
   CEAFDocument* pDoc = EAFGetDocument();
   bool isPGSuper = pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc));
   CString strGroupLabel( isPGSuper ? _T("Span") : _T("Group") );

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   // make sure controls are in sync with actual data groups
   int curSel = pcbGroup->GetCurSel();
   pcbGroup->ResetContent();

   // refill group control
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   CString strLabel;
   strLabel.Format(_T("All %ss"), strGroupLabel);
   int idx = pcbGroup->AddString(strLabel);
   pcbGroup->SetItemData(idx,(DWORD_PTR)ALL_GROUPS);


   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++)
   {
      if (isPGSuper)
      {
         strLabel.Format(_T("%s %s"), strGroupLabel, LABEL_SPAN(grpIdx));
      }
      else
      {
         strLabel.Format(_T("%s %d"), strGroupLabel, LABEL_GROUP(grpIdx));
      }

      idx = pcbGroup->AddString(strLabel);
      pcbGroup->SetItemData(idx,(DWORD_PTR)grpIdx);
   }

   curSel = SetCBCurSelItemData(pcbGroup, m_GirderKey.groupIndex);
   
   if ( curSel == CB_ERR )
   {
      // Default to group 0 if not set
      curSel = pcbGroup->SetCurSel(0);
      ATLASSERT(curSel != CB_ERR);
      m_GirderKey.groupIndex = (GroupIndexType)(pcbGroup->GetItemData(curSel));
   }

   // girders
   GroupIndexType startGroupIdx = (m_GirderKey.groupIndex == ALL_GROUPS ? 0 : m_GirderKey.groupIndex );
   GroupIndexType endGroupIdx   = (m_GirderKey.groupIndex == ALL_GROUPS ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);
   GirderIndexType nGirders = 0;
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      nGirders = Max(nGirders,pGroup->GetGirderCount());
   }

   curSel = pcbGirder->GetCurSel();
   pcbGirder->ResetContent();
   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      CString strLabel;
      strLabel.Format(_T("Girder %s"), LABEL_GIRDER(gdrIdx));
      idx = pcbGirder->AddString(strLabel);
      pcbGirder->SetItemData(idx,(DWORD_PTR)gdrIdx);
   }
   
   curSel = SetCBCurSelItemData(pcbGirder, m_GirderKey.girderIndex);

   if ( curSel == CB_ERR )
   {
      // Default to girder 0 if not set
      curSel = pcbGirder->SetCurSel(0);
      ATLASSERT(curSel != CB_ERR);
      m_GirderKey.girderIndex = 0;
   }

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   Float64 cut;
   if (m_GirderKey.groupIndex == ALL_GROUPS)
   {
      cut = pPoi->ConvertPoiToGirderlineCoordinate(m_cutPoi);
      cut = ConvertFromGirderlineCoordinate(cut);
   }
   else
   {
      cut = pPoi->ConvertPoiToGirderPathCoordinate(m_cutPoi);
   }

   CString msg;
   msg.Format(_T("Section Cut: %s"),FormatDimension(cut,pDisplayUnits->GetSpanLengthUnit()));

   pwndCutLocation->SetWindowText(msg);
   pwndCutLocation->EnableWindow();

   // frame title
   OnUpdateFrameTitle(TRUE);
}

void CGirderModelChildFrame::OnGirderChanged()
{
   CComboBox* pcbGirder   = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_GIRDER);
   m_GirderKey.girderIndex = pcbGirder->GetCurSel();

   UpdateCutRange();
   UpdateBar();
   UpdateViews();

   if ( SyncWithBridgeModelView() )
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
      pDoc->SelectGirder(m_GirderKey);
   }
}

void CGirderModelChildFrame::OnGroupChanged()
{
   CComboBox* pcbGroup = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SPAN);
   m_GirderKey.groupIndex = (GroupIndexType)pcbGroup->GetItemData(pcbGroup->GetCurSel());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType startGroupIdx = (m_GirderKey.groupIndex == INVALID_INDEX ? 0 : m_GirderKey.groupIndex);
   GroupIndexType endGroupIdx   = (m_GirderKey.groupIndex == INVALID_INDEX ? pBridgeDesc->GetGirderGroupCount()-1 : startGroupIdx);
   for ( GroupIndexType grpIdx = startGroupIdx; grpIdx <= endGroupIdx; grpIdx++ )
   {
      const CGirderGroupData* pGroup = pBridgeDesc->GetGirderGroup(grpIdx);
      GirderIndexType nGirders = pGroup->GetGirderCount();
      m_GirderKey.girderIndex = Min(m_GirderKey.girderIndex,nGirders-1);
   }

   UpdateCutRange();
   UpdateBar();
   UpdateViews();

   if ( SyncWithBridgeModelView() ) 
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
      pDoc->SelectGirder(m_GirderKey);
   }
}

void CGirderModelChildFrame::OnSelectEvent() 
{
   CComboBox* pcbEvents = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SELEVENT);
   int curSel = pcbEvents->GetCurSel();
   m_EventIndex = (EventIndexType)pcbEvents->GetItemData(curSel);

   UpdateBar();
   UpdateViews();
}

void CGirderModelChildFrame::OnSectionCut()
{
   ShowCutDlg();
}

void CGirderModelChildFrame::ShowCutDlg()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   CSectionCutDlgEx dlg(pBroker,m_GirderKey,GetCutLocation());
   if ( dlg.DoModal() == IDOK )
   {
      UpdateCutLocation(dlg.GetPOI());
   }
}

void CGirderModelChildFrame::GetCutRange(Float64* pMin, Float64* pMax)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   if (m_GirderKey.groupIndex == ALL_GROUPS)
   {
      *pMin = pPoi->ConvertPoiToGirderlineCoordinate(m_minPoi);
      *pMax = pPoi->ConvertPoiToGirderlineCoordinate(m_maxPoi);

      *pMin = ConvertFromGirderlineCoordinate(*pMin);
      *pMax = ConvertFromGirderlineCoordinate(*pMax);
   }
   else
   {
      *pMin = pPoi->ConvertPoiToGirderPathCoordinate(m_minPoi);
      *pMax = pPoi->ConvertPoiToGirderPathCoordinate(m_maxPoi);
   }
}

Float64 CGirderModelChildFrame::GetCurrentCutLocation() 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);

   Float64 cut;
   if (m_GirderKey.groupIndex == ALL_GROUPS)
   {
      cut = pPoi->ConvertPoiToGirderlineCoordinate(m_cutPoi);
      cut = ConvertFromGirderlineCoordinate(cut);
   }
   else
   {
      cut = pPoi->ConvertPoiToGirderPathCoordinate(m_cutPoi);
   }

   return cut;
}

pgsPointOfInterest CGirderModelChildFrame::GetCutPointOfInterest(Float64 X)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IPointOfInterest, pPoi);
   pgsPointOfInterest poi;
   if (m_GirderKey.groupIndex == ALL_GROUPS)
   {
      X = ConvertToGirderlineCoordinate(X);
      poi = pPoi->ConvertGirderlineCoordinateToPoi(m_GirderKey.girderIndex, X);
   }
   else
   {
      poi = pPoi->ConvertGirderPathCoordinateToPoi(m_GirderKey, X);
   }

   if (poi.GetID() == INVALID_ID)
   {
      // make sure we are at an actual poi
      poi = pPoi->GetNearestPointOfInterest(poi.GetSegmentKey(), poi.GetDistFromStart());
   }

   return poi;
}

void CGirderModelChildFrame::CutAt(Float64 X)
{
   pgsPointOfInterest poi = GetCutPointOfInterest(X);
   UpdateCutLocation(poi);
}

void CGirderModelChildFrame::CutAtNext()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   pgsPointOfInterest currentPoi = GetCutLocation();
   pgsPointOfInterest poi = pPoi->GetNextPointOfInterest(currentPoi.GetID(),POI_ERECTED_SEGMENT);
   if ( poi.GetID() != INVALID_ID )
   {
      UpdateCutLocation(poi);
   }
}

void CGirderModelChildFrame::CutAtPrev()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IPointOfInterest,pPoi);
   pgsPointOfInterest currentPoi = GetCutLocation();
   pgsPointOfInterest poi = pPoi->GetPrevPointOfInterest(currentPoi.GetID(),POI_ERECTED_SEGMENT);
   if ( poi.GetID() != INVALID_ID )
   {
      UpdateCutLocation(poi);
   }
}

pgsPointOfInterest CGirderModelChildFrame::GetCutLocation()
{
   return m_cutPoi;
}

void CGirderModelChildFrame::CreateViewController(IEAFViewController** ppController)
{
   CComPtr<IEAFViewController> stdController;
   CEAFViewControllerFactory::CreateViewController(&stdController);

   CComObject<CGirderModelViewController>* pController;
   CComObject<CGirderModelViewController>::CreateInstance(&pController);
   pController->Init(this, stdController);

   (*ppController) = pController;
   (*ppController)->AddRef();
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
   CGirderModelElevationView* pElevationView = GetGirderModelElevationView();
   CGirderModelSectionView*   pSectionView   = GetGirderModelSectionView();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   
   // create a print job and do it
   CGirderViewPrintJob pj(pElevationView, pSectionView, this, pBroker);
   pj.OnFilePrint(direct);
}

void CGirderModelChildFrame::OnUpdateFrameTitle(BOOL bAddToTitle)
{
	if (bAddToTitle)
   {
      CString msg;
      if ( m_GirderKey.groupIndex != ALL_GROUPS && m_GirderKey.girderIndex != ALL_GIRDERS  )
      {
         msg.Format(_T("Girder View - %s"), GIRDER_LABEL(m_GirderKey));
      }
      else
      {
         msg.Format(_T("%s"),_T("Girder View"));
      }

      // set our title
		AfxSetWindowText(m_hWnd, msg);
   }
}

void CGirderModelChildFrame::OnAddPointload()
{
   CPointLoadData load;
   InitLoad(load);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEditByUI, pEdit);
   pEdit->AddPointLoad(load);
}

void CGirderModelChildFrame::OnAddDistributedLoad() 
{
   CDistributedLoadData load;
   InitLoad(load);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEditByUI, pEdit);
   pEdit->AddDistributedLoad(load);
}

void CGirderModelChildFrame::OnAddMoment() 
{
   CMomentLoadData load;
   InitLoad(load);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IEditByUI, pEdit);
   pEdit->AddMomentLoad(load);
}

LRESULT CGirderModelChildFrame::OnCommandHelp(WPARAM, LPARAM lParam)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDER_VIEW );
   return TRUE;
}

void CGirderModelChildFrame::OnSync() 
{
   DoSyncWithBridgeModelView(SyncWithBridgeModelView());
}

void CGirderModelChildFrame::OnUpdateDesignGirderDirect(CCmdUI* pCmdUI)
{
   pCmdUI->Enable( m_GirderKey.groupIndex != ALL_GROUPS && m_GirderKey.girderIndex != ALL_GIRDERS );
}

void CGirderModelChildFrame::OnUpdateDesignGirderDirectHoldSlabOffset(CCmdUI* pCmdUI)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,ISpecification,pSpecification);
   GET_IFACE2_NOCHECK(pBroker,IBridge,pBridge);
   bool bDesignSlabOffset = pSpecification->IsSlabOffsetDesignEnabled() && pBridge->GetDeckType() != pgsTypes::sdtNone;
   pCmdUI->Enable( m_GirderKey.groupIndex != ALL_GROUPS && m_GirderKey.girderIndex != ALL_GIRDERS && bDesignSlabOffset );
}

void CGirderModelChildFrame::OnDesignGirderDirect()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)EAFGetDocument();
   pDoc->DesignGirder(false,sodDesignHaunch,m_GirderKey);
}

void CGirderModelChildFrame::OnDesignGirderDirectHoldSlabOffset()
{
   CPGSuperDoc* pDoc = (CPGSuperDoc*)EAFGetDocument();
   pDoc->DesignGirder(false,sodPreserveHaunch,m_GirderKey);
}


void CGirderModelChildFrame::OnSetFocus(CWnd* pOldWnd)
{
   __super::OnSetFocus(pOldWnd);

   if ( m_bIsAfterFirstUpdate && SyncWithBridgeModelView() ) 
   {
      CPGSDocBase* pDoc = (CPGSDocBase*)GetActiveDocument();
      pDoc->SelectGirder(m_GirderKey);
   }
}

void CGirderModelChildFrame::FillEventComboBox()
{
   CComboBox* pcbEvents = (CComboBox*)m_SettingsBar.GetDlgItem(IDC_SELEVENT);

   int sel = pcbEvents->GetCurSel();
   EventIndexType currentEvent = (sel == CB_ERR ? INVALID_INDEX : (EventIndexType)(pcbEvents->GetItemData(sel)));
   pcbEvents->ResetContent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   const CTimelineManager* pTimelineMgr = pBridgeDesc->GetTimelineManager();
   EventIndexType firstEventIdx = pTimelineMgr->GetFirstSegmentErectionEventIndex();
   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   currentEvent = (nEvents <= currentEvent && currentEvent != INVALID_INDEX ? nEvents-1 : currentEvent);
   for ( EventIndexType eventIdx = firstEventIdx; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);
      CString strLabel;
      strLabel.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());
      int idx = pcbEvents->AddString(strLabel);
      pcbEvents->SetItemData(idx,(DWORD_PTR)eventIdx);
      if ( eventIdx == currentEvent )
      {
         pcbEvents->SetCurSel(idx);
      }
   }

   sel = pcbEvents->GetCurSel();
   if ( sel == CB_ERR )
   {
      pcbEvents->SetCurSel(0);
   }
   m_EventIndex = (EventIndexType)pcbEvents->GetItemData(pcbEvents->GetCurSel());
}

void CGirderModelChildFrame::OnUpdateStrandsButton(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDG_EV_SHOW_STRANDS) || sysFlags<UINT>::IsSet(settings, IDG_SV_SHOW_STRANDS) ? BST_CHECKED : BST_UNCHECKED);
}

void CGirderModelChildFrame::OnStrandsButton()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   sysFlags<UINT>::Toggle(&settings, IDG_EV_SHOW_STRANDS);
   sysFlags<UINT>::Toggle(&settings, IDG_SV_SHOW_STRANDS);
   pDoc->SetGirderEditorSettings(settings);
}

void CGirderModelChildFrame::OnUpdateStrandsCGButton(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDG_EV_SHOW_PS_CG) || sysFlags<UINT>::IsSet(settings, IDG_SV_SHOW_PS_CG) ? BST_CHECKED : BST_UNCHECKED);
}

void CGirderModelChildFrame::OnStrandsCGButton()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   sysFlags<UINT>::Toggle(&settings, IDG_EV_SHOW_PS_CG);
   sysFlags<UINT>::Toggle(&settings, IDG_SV_SHOW_PS_CG);
   pDoc->SetGirderEditorSettings(settings);
}

void CGirderModelChildFrame::OnUpdateDimensionsButton(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDG_EV_SHOW_DIMENSIONS) || sysFlags<UINT>::IsSet(settings, IDG_SV_SHOW_DIMENSIONS)  ? BST_CHECKED : BST_UNCHECKED);
}

void CGirderModelChildFrame::OnDimensionsButton()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   sysFlags<UINT>::Toggle(&settings, IDG_EV_SHOW_DIMENSIONS);
   sysFlags<UINT>::Toggle(&settings, IDG_SV_SHOW_DIMENSIONS);
   pDoc->SetGirderEditorSettings(settings);
}

void CGirderModelChildFrame::OnUpdatePropertiesButton(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDG_SV_PROPERTIES) ? BST_CHECKED : BST_UNCHECKED);
}

void CGirderModelChildFrame::OnPropertiesButton()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   sysFlags<UINT>::Toggle(&settings, IDG_SV_PROPERTIES);
   pDoc->SetGirderEditorSettings(settings);
}

void CGirderModelChildFrame::OnUpdateLongitudinalReinforcementButton(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDG_EV_SHOW_LONG_REINF) || sysFlags<UINT>::IsSet(settings, IDG_SV_SHOW_LONG_REINF) ? BST_CHECKED : BST_UNCHECKED);
}

void CGirderModelChildFrame::OnLongitudinalReinforcementButton()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   sysFlags<UINT>::Toggle(&settings, IDG_EV_SHOW_LONG_REINF);
   sysFlags<UINT>::Toggle(&settings, IDG_SV_SHOW_LONG_REINF);
   pDoc->SetGirderEditorSettings(settings);
}

void CGirderModelChildFrame::OnUpdateStirrupsButton(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDG_EV_SHOW_STIRRUPS) ? BST_CHECKED : BST_UNCHECKED);
}

void CGirderModelChildFrame::OnStirrupsButton()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   sysFlags<UINT>::Toggle(&settings, IDG_EV_SHOW_STIRRUPS);
   pDoc->SetGirderEditorSettings(settings);
}

void CGirderModelChildFrame::OnUpdateUserLoadsButton(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDG_EV_SHOW_LOADS) || sysFlags<UINT>::IsSet(settings, IDG_EV_SHOW_LEGEND) ? BST_CHECKED : BST_UNCHECKED);
}

void CGirderModelChildFrame::OnUserLoadsButton()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   sysFlags<UINT>::Toggle(&settings, IDG_EV_SHOW_LOADS);
   sysFlags<UINT>::Toggle(&settings, IDG_EV_SHOW_LEGEND);
   pDoc->SetGirderEditorSettings(settings);
}

void CGirderModelChildFrame::OnUpdateSchematicButton(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDG_EV_DRAW_ISOTROPIC) ? BST_UNCHECKED : BST_CHECKED);
}

void CGirderModelChildFrame::OnSchematicButton()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   sysFlags<UINT>::Toggle(&settings, IDG_EV_DRAW_ISOTROPIC);
   pDoc->SetGirderEditorSettings(settings);
}

void CGirderModelChildFrame::OnUpdateSectionCGButton(CCmdUI* pCmdUI)
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   pCmdUI->SetCheck(sysFlags<UINT>::IsSet(settings, IDG_SV_GIRDER_CG) || sysFlags<UINT>::IsSet(settings, IDG_EV_GIRDER_CG) ? BST_CHECKED : BST_UNCHECKED);
}

void CGirderModelChildFrame::OnSectionCGButton()
{
   CPGSDocBase* pDoc = (CPGSDocBase*)EAFGetDocument();
   UINT settings = pDoc->GetGirderEditorSettings();
   sysFlags<UINT>::Toggle(&settings, IDG_SV_GIRDER_CG);
   sysFlags<UINT>::Toggle(&settings, IDG_EV_GIRDER_CG);
   pDoc->SetGirderEditorSettings(settings);
}
