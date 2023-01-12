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

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include "SelectGirderDlg.h"
#include <IFace\Project.h>
#include <IFace\DocumentType.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CSelectGirderDlg dialog

IMPLEMENT_DYNAMIC(CSelectGirderDlg, CDialog)

CSelectGirderDlg::CSelectGirderDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CSelectGirderDlg::IDD, pParent)
{

}

CSelectGirderDlg::~CSelectGirderDlg()
{
}

void CSelectGirderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_CBIndex(pDX, IDC_GROUP,  (int&)m_GirderKey.groupIndex);
	DDX_CBIndex(pDX, IDC_GIRDER, (int&)m_GirderKey.girderIndex);
}


BEGIN_MESSAGE_MAP(CSelectGirderDlg, CDialog)
	ON_CBN_SELCHANGE(IDC_GROUP, OnGroupChanged)
END_MESSAGE_MAP()


// CSelectGirderDlg message handlers

BOOL CSelectGirderDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

	GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc ); 
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

	CComboBox* pGroupBox   = (CComboBox*)GetDlgItem( IDC_GROUP );
   CComboBox* pGirderBox  = (CComboBox*)GetDlgItem( IDC_GIRDER );

   GET_IFACE2(pBroker,IDocumentType,pDocType);
   bool bPGSuper = pDocType->IsPGSuperDocument();
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
	if ( pGroupBox->SetCurSel((int)m_GirderKey.groupIndex) == CB_ERR )
   {
      pGroupBox->SetCurSel(0);
   }
	
   if ( pGirderBox->SetCurSel((int)m_GirderKey.girderIndex) == CB_ERR )
   {
      pGirderBox->SetCurSel(0);
   }
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CSelectGirderDlg::OnGroupChanged() 
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

	GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc ); 
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

	CComboBox* pGroupBox   = (CComboBox*)GetDlgItem( IDC_GROUP );
   CComboBox* pGirderBox  = (CComboBox*)GetDlgItem( IDC_GIRDER );

   int group = pGroupBox->GetCurSel();
   if ( group == CB_ERR )
   {
      group = 0;
   }

   int girder = pGirderBox->GetCurSel();
   if (girder == CB_ERR )
   {
      girder = 0;
   }

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
