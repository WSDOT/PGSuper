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

// GirderDimensionsPage.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psgLib.h>
#include "GirderMainSheet.h"
#include "GirderDimensionsPage.h"
#include "..\htmlhelp\HelpTopics.hh"
#include "ComCat.h"
#include "PGSuperCatCom.h"
#include "SectionViewDialog.h"
#include <PsgLib\BeamFamilyManager.h>
#include <IFace\BeamFactory.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CGirderDimensionsPage property page

IMPLEMENT_DYNCREATE(CGirderDimensionsPage, CPropertyPage)

CGirderDimensionsPage::CGirderDimensionsPage() : CPropertyPage(CGirderDimensionsPage::IDD,IDS_GIRDER_DIMENSIONS)
{
	//{{AFX_DATA_INIT(CGirderDimensionsPage)
	//}}AFX_DATA_INIT
   m_LastBeamType = CB_ERR;
}

CGirderDimensionsPage::~CGirderDimensionsPage()
{
}

void CGirderDimensionsPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDimensionsPage)
	//}}AFX_DATA_MAP

   DDV_GXGridWnd(pDX, &m_Grid);

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   // dad is a friend of the entry. use him to transfer data.
   pDad->ExchangeDimensionData(pDX);
   CComPtr<IBeamFactory> pFactory;
   pDad->m_Entry.GetBeamFactory(&pFactory);

   // girder picture metafile
	DDX_MetaFileStatic(pDX, IDC_GIRDER_MF, m_GirderPicture, pFactory->GetResourceInstance(), pFactory->GetImageResourceName(), _T("Metafile") );
}


BEGIN_MESSAGE_MAP(CGirderDimensionsPage, CPropertyPage)
	//{{AFX_MSG_MAP(CGirderDimensionsPage)
	ON_MESSAGE(WM_COMMANDHELP, OnCommandHelp)
   ON_CBN_DROPDOWN(IDC_BEAMTYPES, OnBeamTypesChanging)
	ON_CBN_SELCHANGE(IDC_BEAMTYPES, OnBeamTypeChanged)
   ON_WM_DESTROY()
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_VIEWSECTION,OnViewSection)
   ON_BN_CLICKED(IDC_VIEWSECTION_MID,OnViewSectionMid)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGirderDimensionsPage message handlers
LRESULT CGirderDimensionsPage::OnCommandHelp(WPARAM, LPARAM lParam)
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDER_DIMENSIONS_TAB );
   return TRUE;
}

BOOL CGirderDimensionsPage::OnInitDialog() 
{
	CWaitCursor cursor;

	m_Grid.SubclassDlgItem(IDC_DIMENSIONS, this);
   m_Grid.CustomInit();

   CPropertyPage::OnInitDialog();

   CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_BEAMTYPES);

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   CComPtr<IBeamFactory> pFactory;
   pDad->m_Entry.GetBeamFactory(&pFactory);

   // Fill the beam family combo box
   std::vector<CString> familyNames = CBeamFamilyManager::GetBeamFamilyNames();
   std::vector<CString>::iterator familyIter(familyNames.begin());
   std::vector<CString>::iterator familyIterEnd(familyNames.end());
   for ( ; familyIter != familyIterEnd; familyIter++ )
   {
      CString familyName = *familyIter;
      CComPtr<IBeamFamily> beamFamily;
      HRESULT hr = CBeamFamilyManager::GetBeamFamily(familyName,&beamFamily);
      ATLASSERT(SUCCEEDED(hr));
      if ( FAILED(hr) )
         continue;

      const std::vector<CString>& factoryNames( beamFamily->GetFactoryNames() );
      std::vector<CString>::const_iterator factoryIter(factoryNames.begin());
      std::vector<CString>::const_iterator factoryIterEnd(factoryNames.end());
      for ( ; factoryIter != factoryIterEnd; factoryIter++ )
      {
         const CString& factoryName = *factoryIter;   
         int idx = pComboBox->AddString(factoryName);
         CLSID* clsid = new CLSID;
         *clsid = beamFamily->GetFactoryCLSID(factoryName);
         pComboBox->SetItemDataPtr(idx,(void*)clsid);
      }
   }

   // Set the beam family combo box to the correct value
   for ( int i = 0; i < pComboBox->GetCount(); i++ )
   {
      CLSID* cid = (CLSID*)pComboBox->GetItemData(i);
      if ( IsEqualCLSID(*cid,pFactory->GetCLSID()) )
      {
         pComboBox->SetCurSel(i);
         break;
      }
   }

   // Disable the beam family combo box if the
   // dialog is opened as read only
   if ( !pDad->m_AllowEditing )
   {
      pComboBox->EnableWindow(FALSE);
   }


   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CGirderDimensionsPage::OnBeamTypesChanging()
{
   CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_BEAMTYPES);
   m_LastBeamType = pComboBox->GetCurSel();
}

void CGirderDimensionsPage::OnBeamTypeChanged() 
{
   // Change the factory object, empty the grid, and reload it with
   // the dimensions for the new beam type
   CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_BEAMTYPES);
   int selIdx = pComboBox->GetCurSel();
   CLSID* pCLSID = (CLSID*)pComboBox->GetItemDataPtr(selIdx);
   CComPtr<IBeamFactory> pFactory;
   HRESULT hr = ::CoCreateInstance(*pCLSID,NULL,CLSCTX_ALL,IID_IBeamFactory,(void**)&pFactory);
   if ( FAILED(hr) )
   {
      CString strGirderName;
      pComboBox->GetLBText(selIdx,strGirderName);
      CString strMsg;
      strMsg.Format(_T("Unable to create \"%s\"."),strGirderName);
      AfxMessageBox(strMsg);
      if ( m_LastBeamType != CB_ERR )
         pComboBox->SetCurSel(m_LastBeamType);

      return;
   }

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();
   pDad->m_Entry.SetBeamFactory(pFactory);

   m_Grid.ResetGrid();
   UpdateData(FALSE);
	m_Grid.ResizeRowHeightsToFit(CGXRange(0,0,0,m_Grid.GetColCount()));
}

void CGirderDimensionsPage::OnViewSection()
{
   ViewSection(true);
}

void CGirderDimensionsPage::OnViewSectionMid()
{
   ViewSection(false);
}

void CGirderDimensionsPage::ViewSection(bool isEnd)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   // Validate data before proceeding
   if ( !UpdateData(TRUE) )
      return;

   CGirderMainSheet* pDad = (CGirderMainSheet*)GetParent();

   CSectionViewDialog dlg(&(pDad->m_Entry),isEnd);
   dlg.DoModal();

}

void CGirderDimensionsPage::OnDestroy()
{
   CComboBox* pComboBox = (CComboBox*)GetDlgItem(IDC_BEAMTYPES);
   int count = pComboBox->GetCount();
   for ( int idx = 0; idx < count; idx++ )
   {
      CLSID* pItemDataPtr = (CLSID*)pComboBox->GetItemDataPtr(idx);
      delete pItemDataPtr;

      pComboBox->SetItemDataPtr(idx,NULL);
   }

   CPropertyPage::OnDestroy();
}
