///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2023  Washington State Department of Transportation
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

// DesignGirderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "DesignGirderDlg.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <IFace\GirderHandlingSpecCriteria.h>

#include <PsgLib\GirderLibraryEntry.h>
#include <EAF\EAFDocument.h>

#include <MFCTools\AutoRegistry.h>
#include "PGSuperBaseAppPlugin.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDesignGirderDlg dialog


CDesignGirderDlg::CDesignGirderDlg(const CGirderKey& girderKey, IBroker* pBroker, arSlabOffsetDesignType haunchDesignType, CWnd* pParent /*=nullptr*/)
	: CDialog(CDesignGirderDlg::IDD, pParent),
   m_GirderKey(girderKey),
   m_DesignRadioNum(0)
{
   m_pBroker = pBroker;

	//{{AFX_DATA_INIT(CDesignGirderDlg)
   //}}AFX_DATA_INIT

   m_HaunchDesignType = haunchDesignType;

   m_strToolTip = "Eugène Freyssinet\r\nFather of Prestressed Concrete";
}


void CDesignGirderDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CDesignGirderDlg)
   DDX_CBIndex(pDX, IDC_SPAN, m_GirderKey.groupIndex);
   DDX_CBIndex(pDX, IDC_GIRDER, m_GirderKey.girderIndex);
   DDX_Radio(pDX, IDC_RADIO_SINGLE, m_DesignRadioNum);
   //}}AFX_DATA_MAP

   if (pDX->m_bSaveAndValidate)
   {
      if (DesignForFlexure()==FALSE && DesignForShear()==FALSE)
      {
         ::AfxMessageBox(_T("No design requested. Please select Flexural and/or Shear design."),MB_OK | MB_ICONWARNING);
         pDX->Fail();
      }

      // build girder list based on input type
      if (m_DesignRadioNum==0)
      {
         // Single girder
         std::vector<CGirderKey> list_of_one;
         list_of_one.push_back(m_GirderKey);
         m_GirderKeys = list_of_one;
      }
      else
      {
         // Girder list was stored/passed from grid
         if (m_GirderKeys.empty())
         {
            ::AfxMessageBox(_T("No girders selected. Please select at least one girder"),MB_OK | MB_ICONWARNING);
            pDX->Fail();
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CDesignGirderDlg, CDialog)
	//{{AFX_MSG_MAP(CDesignGirderDlg)
	ON_BN_CLICKED(IDC_HELPME, OnHelp)
	ON_CBN_SELCHANGE(IDC_SPAN, OnSpanChanged)
	ON_BN_CLICKED(IDC_DESIGN_FLEXURE, OnDesignFlexure)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
   ON_WM_DESTROY()
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_SELECT_GIRDERS, &CDesignGirderDlg::OnBnClickedSelectGirders)
   ON_BN_CLICKED(IDC_RADIO_SINGLE, &CDesignGirderDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_RADIO_MULTIPLE, &CDesignGirderDlg::OnBnClickedRadio)
   ON_BN_CLICKED(IDC_DESIGN_SHEAR, &CDesignGirderDlg::OnBnClickedDesignShear)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDesignGirderDlg message handlers

BOOL CDesignGirderDlg::OnInitDialog() 
{
   // set up default design options
   GET_IFACE(IBridge, pBridge);
   GET_IFACE(ISpecification, pSpec);
   m_bEnableHaunchDesign = pSpec->IsSlabOffsetDesignEnabled() && 
                           pBridge->GetHaunchInputDepthType() == pgsTypes::hidACamber && // We don't design if direct haunch input
                           IsStructuralDeck(pBridge->GetDeckType()) ? TRUE : FALSE;

   // Load up the combo boxes with span and girder information

   CComboBox* pSpanBox = (CComboBox*)GetDlgItem( IDC_SPAN );
   CComboBox* pGdrBox  = (CComboBox*)GetDlgItem( IDC_GIRDER );

   GroupIndexType nGroups = pBridge->GetGirderGroupCount();
   for (GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
      CString strSpan;
      strSpan.Format(_T("Span %s"),LABEL_SPAN(grpIdx));
      pSpanBox->AddString(strSpan);
   }

   pSpanBox->SetCurSel((int)m_GirderKey.groupIndex);
   UpdateGirderComboBox(m_GirderKey.groupIndex);

   CComboBox* pcbDesignHaunch = (CComboBox*)GetDlgItem( IDC_DESIGN_HAUNCH );
   pcbDesignHaunch->AddString(_T("Preserve haunch geometry"));
   pcbDesignHaunch->AddString(_T("Design haunch geometry"));
   pcbDesignHaunch->EnableWindow(m_bEnableHaunchDesign);

   CComboBox* pcbDesignConcrete = (CComboBox*)GetDlgItem(IDC_DESIGN_CONCRETE_STRENGTH);
   pcbDesignConcrete->AddString(_T("Design for minimum concrete strength"));
   pcbDesignConcrete->AddString(_T("Preserve concrete strength during design"));

	CDialog::OnInitDialog();

   EnableToolTips(TRUE);

   // Initialize controls
   bool bDesignFlexure;
   arSlabOffsetDesignType haunchDesignType;
   arConcreteDesignType concreteDesignType;
   arShearDesignType shearDesignType;
   LoadSettings(bDesignFlexure, haunchDesignType, concreteDesignType, shearDesignType);
   
   CheckDlgButton(IDC_DESIGN_FLEXURE, bDesignFlexure ? BST_CHECKED : BST_UNCHECKED);
   
   CComboBox* pcbHaunch = (CComboBox*)GetDlgItem(IDC_DESIGN_HAUNCH);
   pcbHaunch->SetCurSel(m_HaunchDesignType == sodDefault ? (haunchDesignType == sodPreserveHaunch ? 0 : 1) : m_HaunchDesignType);

   CComboBox* pcbConcrete = (CComboBox*)GetDlgItem(IDC_DESIGN_CONCRETE_STRENGTH);
   pcbConcrete->SetCurSel(concreteDesignType == cdDesignForMinStrength ? 0 : 1);
   if (shearDesignType == sdtNoDesign)
   {
      CheckDlgButton(IDC_DESIGN_SHEAR, BST_UNCHECKED);
      CheckDlgButton(IDC_START_WITH_LAYOUT, BST_CHECKED);
   }
   else
   {
      CheckDlgButton(IDC_DESIGN_SHEAR, BST_CHECKED);
      CheckDlgButton(IDC_START_WITH_LAYOUT, shearDesignType == sdtRetainExistingLayout ? BST_CHECKED : BST_UNCHECKED);
   }

   // Update UI elements
   OnDesignFlexure();
   OnBnClickedRadio();
   OnBnClickedDesignShear();

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CDesignGirderDlg::OnDestroy()
{
   SaveSettings();
}

void CDesignGirderDlg::LoadSettings(bool& bDesignFlexure, arSlabOffsetDesignType& haunchDesignType, arConcreteDesignType& concreteDesignType, arShearDesignType& shearDesignType)
{
   // loads last settings from the registry
   CEAFDocument* pDoc = EAFGetDocument();
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(pDoc->GetDocTemplate());
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSBase = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   CEAFApp* pApp = EAFGetApp();
   CAutoRegistry autoReg(pPGSBase->GetAppName(), pApp);

   // Flexure and stirrup design defaults for design dialog.
   // Default is to design flexure and not shear.
   // If the string is not Off, then assume it is on.
   CString strDefaultDesignFlexure = pApp->GetLocalMachineString(_T("Settings"), _T("DesignFlexure"), _T("On"));
   CString strDesignFlexure = pApp->GetProfileString(_T("Settings"), _T("DesignFlexure"), strDefaultDesignFlexure);
   bDesignFlexure = (strDesignFlexure.CompareNoCase(_T("On")) == 0) ? true : false;

   CString strDefaultHaunchDesign = pApp->GetLocalMachineString(_T("Settings"), _T("HaunchDesign"), _T("On"));
   CString strHaunchDesign = pApp->GetProfileStringW(_T("Settings"), _T("HaunchDesign"), strDefaultHaunchDesign);
   haunchDesignType = (strHaunchDesign.CompareNoCase(_T("On")) == 0) ? sodDesignHaunch : sodPreserveHaunch;

   CString strDefaultDesignWithFixedConcreteStrength = pApp->GetLocalMachineString(_T("Settings"), _T("PreserveConcreteStrength"), _T("Off"));
   CString strDesignWithFixedConcreteStrength = pApp->GetProfileString(_T("Settings"), _T("PreserveConcreteStrength"), strDefaultDesignWithFixedConcreteStrength);
   concreteDesignType = (strDesignWithFixedConcreteStrength.CompareNoCase(_T("Off")) == 0) ? cdDesignForMinStrength : cdPreserveStrength;

   CString strDefaultDesignShear = pApp->GetLocalMachineString(_T("Settings"), _T("DesignShear"), _T("Off"));
   CString strDesignShear = pApp->GetProfileString(_T("Settings"), _T("DesignShear"), strDefaultDesignShear);
   bool bDesignShear = (strDesignShear.CompareNoCase(_T("Off")) == 0) ? false : true;

   CString strDefaultDesignStirrupsFromScratch = pApp->GetLocalMachineString(_T("Settings"), _T("DesignStirrupsFromScratch"), _T("On"));
   CString strDesignStirrupsFromScratch = pApp->GetProfileString(_T("Settings"), _T("DesignStirrupsFromScratch"), strDefaultDesignStirrupsFromScratch);
   bool bStirrupsFromScratch = (strDesignStirrupsFromScratch.CompareNoCase(_T("Off")) == 0) ? false : true;
   if (bDesignShear)
   {
      shearDesignType = (bStirrupsFromScratch ? sdtLayoutStirrups : sdtRetainExistingLayout);
   }
   else
   {
      shearDesignType = sdtNoDesign;
   }
}

void CDesignGirderDlg::SaveSettings()
{
   // saves current settings to the registry
   CEAFDocument* pDoc = EAFGetDocument();
   CEAFDocTemplate* pTemplate = (CEAFDocTemplate*)(pDoc->GetDocTemplate());
   CComPtr<IEAFAppPlugin> pAppPlugin;
   pTemplate->GetPlugin(&pAppPlugin);
   CPGSAppPluginBase* pPGSBase = dynamic_cast<CPGSAppPluginBase*>(pAppPlugin.p);

   CEAFApp* pApp = EAFGetApp();
   CAutoRegistry autoReg(pPGSBase->GetAppName(), pApp);

   VERIFY(pApp->WriteProfileString(_T("Settings"), _T("DesignFlexure"), DesignForFlexure() ? _T("On") : _T("Off")));
   VERIFY(pApp->WriteProfileString(_T("Settings"), _T("HaunchDesign"), DesignHaunch() ? _T("On") : _T("Off")));
   VERIFY(pApp->WriteProfileString(_T("Settings"), _T("PreserveConcreteStrength"), PreserveConcreteStrength() ? _T("On") : _T("Off")));
   VERIFY(pApp->WriteProfileString(_T("Settings"), _T("DesignShear"), DesignForShear() ? _T("On") : _T("Off")));
   VERIFY(pApp->WriteProfileString(_T("Settings"), _T("DesignStirrupsFromScratch"), DesignWithCurrentStirrups() ? _T("Off") : _T("On")));
}

BOOL CDesignGirderDlg::DesignForFlexure()
{
   return IsDlgButtonChecked(IDC_DESIGN_FLEXURE) == BST_CHECKED;
}

BOOL CDesignGirderDlg::DesignHaunch()
{
   CComboBox* pcbDesignHaunch = (CComboBox*)GetDlgItem(IDC_DESIGN_HAUNCH);
   if (m_bEnableHaunchDesign)
   {
      return pcbDesignHaunch->GetCurSel() == 0 ? FALSE : TRUE;
   }
   else
   {
      return FALSE;
   }
}

BOOL CDesignGirderDlg::PreserveConcreteStrength()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_DESIGN_CONCRETE_STRENGTH);
   return pCB->GetCurSel() == 0 ? FALSE : TRUE;
}

BOOL CDesignGirderDlg::DesignForShear()
{
   return IsDlgButtonChecked(IDC_DESIGN_SHEAR) == BST_CHECKED;
}

BOOL CDesignGirderDlg::DesignWithCurrentStirrups()
{
   return IsDlgButtonChecked(IDC_START_WITH_LAYOUT) == BST_CHECKED;
}

void CDesignGirderDlg::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_DIALOG_DESIGNGIRDER );
}

void CDesignGirderDlg::OnSpanChanged() 
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_SPAN);
   SpanIndexType spanIdx = (SpanIndexType)pCB->GetCurSel();

   UpdateGirderComboBox(spanIdx);
}

void CDesignGirderDlg::UpdateGirderComboBox(SpanIndexType spanIdx)
{
   GET_IFACE( IBridge, pBridge );

   CComboBox* pGdrBox = (CComboBox*)GetDlgItem(IDC_GIRDER);
   Uint16 curSel = pGdrBox->GetCurSel();
   pGdrBox->ResetContent();

   GirderIndexType cGirder = pBridge->GetGirderCount( spanIdx );
   for ( GirderIndexType j = 0; j < cGirder; j++ )
   {
      CString strGdr;
      strGdr.Format( _T("Girder %s"), LABEL_GIRDER(j));
      pGdrBox->AddString( strGdr );
   }

   if (pGdrBox->SetCurSel(curSel == CB_ERR ? 0 : curSel) == CB_ERR)
   {
      pGdrBox->SetCurSel(0);
   }
}

void CDesignGirderDlg::OnDesignFlexure() 
{
   UpdateDesignHaunchCtrl();
   UpdateConcreteDesignCtrl();
}

BOOL CDesignGirderDlg::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_PICTURE:
         pTTT->lpszText = m_strToolTip.LockBuffer();
         pTTT->hinst = nullptr;
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip

      return TRUE;
   }
   return FALSE;
}

void CDesignGirderDlg::OnBnClickedSelectGirders()
{
   CMultiGirderSelectDlg dlg;
   dlg.m_GirderKeys = m_GirderKeys;

   if (dlg.DoModal()==IDOK)
   {
      m_GirderKeys = dlg.m_GirderKeys;

      // update button text
      CString msg;
      msg.Format(_T("Select Girders\n(%d Selected)"), m_GirderKeys.size());
      GetDlgItem(IDC_SELECT_GIRDERS)->SetWindowText(msg);
   }
}

void CDesignGirderDlg::OnBnClickedRadio()
{
   BOOL enab_sgl = IsDlgButtonChecked(IDC_RADIO_SINGLE) == BST_CHECKED ? TRUE : FALSE;
   BOOL enab_mpl = enab_sgl ? FALSE : TRUE;

   GetDlgItem(IDC_SPAN)->EnableWindow(enab_sgl);
   GetDlgItem(IDC_GIRDER)->EnableWindow(enab_sgl);

   GetDlgItem(IDC_SELECT_GIRDERS)->EnableWindow(enab_mpl);

   if ( enab_mpl && m_GirderKeys.size() == 0 )
   {
      OnBnClickedSelectGirders();
   }
}

void CDesignGirderDlg::UpdateDesignHaunchCtrl()
{
   if (m_bEnableHaunchDesign)
   {
      CWnd* pWnd = GetDlgItem( IDC_DESIGN_HAUNCH );
      pWnd->EnableWindow(IsDlgButtonChecked(IDC_DESIGN_FLEXURE) == BST_CHECKED);
   }
}

void CDesignGirderDlg::UpdateConcreteDesignCtrl()
{
   CWnd* pWnd = GetDlgItem(IDC_DESIGN_CONCRETE_STRENGTH);
   pWnd->EnableWindow(IsDlgButtonChecked(IDC_DESIGN_FLEXURE) == BST_CHECKED);
}

void CDesignGirderDlg::OnBnClickedDesignShear()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_DESIGN_SHEAR) == BST_CHECKED;
   GetDlgItem( IDC_START_WITH_LAYOUT )->EnableWindow(bEnable);
}
