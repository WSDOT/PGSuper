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

// SelectGirderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperApp.h"
#include "SelectGirderDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include "PGSuperDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSelectGirderDlg dialog


CSelectGirderDlg::CSelectGirderDlg(IBroker* pBroker, CWnd* pParent /*=nullptr*/)
	: CDialog(CSelectGirderDlg::IDD, pParent),
   m_pBroker(pBroker)
{
	//{{AFX_DATA_INIT(CSelectGirderDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}


void CSelectGirderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectGirderDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP

	DDX_CBIndex(pDX, IDC_GROUP,  (int&)m_Group);
	DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_Girder);
}


BEGIN_MESSAGE_MAP(CSelectGirderDlg, CDialog)
	//{{AFX_MSG_MAP(CSelectGirderDlg)
	ON_CBN_SELCHANGE(IDC_GROUP, OnGroupChanged)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectGirderDlg message handlers

BOOL CSelectGirderDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	GET_IFACE( IBridgeDescription, pIBridgeDesc ); 
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

	CComboBox* pGroupBox   = (CComboBox*)GetDlgItem( IDC_GROUP );
   CComboBox* pGirderBox  = (CComboBox*)GetDlgItem( IDC_GIRDER );

   CEAFDocument* pDoc = EAFGetDocument();
   bool bPGSuper = (pDoc->IsKindOf(RUNTIME_CLASS(CPGSuperDoc)) ? true : false);
   GroupIndexType nGroups = pBridgeDesc->GetGirderGroupCount();
   for ( GroupIndexType grpIdx = 0; grpIdx < nGroups; grpIdx++ )
   {
	   CString strLabel;
      if (bPGSuper)
      {
         strLabel.Format(_T("Span %s"), LABEL_SPAN(grpIdx));
      }
      else
      {
         strLabel.Format(_T("Group %d"), LABEL_GROUP(grpIdx));
      }

	   pGroupBox->AddString(strLabel);
   }

   OnGroupChanged();

	/* Intialize each combo selections */
	if ( pGroupBox->SetCurSel((int)m_Group) == CB_ERR )
      pGroupBox->SetCurSel(0);
	
   if ( pGirderBox->SetCurSel((int)m_Girder) == CB_ERR )
      pGirderBox->SetCurSel(0);
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectGirderDlg::OnGroupChanged() 
{
	GET_IFACE( IBridgeDescription, pIBridgeDesc ); 
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

	CComboBox* pGroupBox   = (CComboBox*)GetDlgItem( IDC_GROUP );
   CComboBox* pGirderBox  = (CComboBox*)GetDlgItem( IDC_GIRDER );

   int group = pGroupBox->GetCurSel();
   if ( group == CB_ERR )
      group = 0;

   int girder = pGirderBox->GetCurSel();
   if (girder == CB_ERR )
      girder = 0;

   pGirderBox->ResetContent();

	/* Get count of girders (same number of girders in all spans) */
   GirderIndexType nGirders = pBridgeDesc->GetGirderGroup(group)->GetGirderCount();
	for ( GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++ )
	{
		/* Add current girder string to girder list */
		CString strLabel;
		strLabel.Format( _T("Girder %s"), LABEL_GIRDER(gdrIdx));
		pGirderBox->AddString( strLabel );
	}

   girder = (int)Min(GirderIndexType(girder), nGirders-1); // don't allow out of bounds if ng decreases between spans

   pGirderBox->SetCurSel(girder);
}
