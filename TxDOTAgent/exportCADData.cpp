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
//
// exportCADData.cpp : implementation file
//

#include "stdafx.h"

#include <ATLBase.h>

#include "exportCADData.h"

#include <IFace\Bridge.h>
//#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// exportCADData dialog

/*--------------------------------------------------------------------*/
exportCADData::exportCADData(IBroker* pBroker,CWnd* pParent /*=NULL*/)
	: CDialog(exportCADData::IDD, pParent)
{
   m_pBroker  = pBroker;
	//{{AFX_DATA_INIT(exportCADData)
	m_Girder = -1;
	m_Span = -1;
	m_IsExtended = FALSE;
	//}}AFX_DATA_INIT
}

/*--------------------------------------------------------------------*/
exportCADData::~exportCADData()
{
	/* Do nothing for now */
}


/*--------------------------------------------------------------------*/
void exportCADData::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(exportCADData)
	DDX_CBIndex(pDX, IDC_GIRDER, m_Girder);
	DDX_CBIndex(pDX, IDC_SPAN, m_Span);
	DDX_Check(pDX, IDC_EXTENDED, m_IsExtended);
	//}}AFX_DATA_MAP
}

/*--------------------------------------------------------------------*/
BEGIN_MESSAGE_MAP(exportCADData, CDialog)
	//{{AFX_MSG_MAP(exportCADData)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_CBN_SELCHANGE(IDC_SPAN, OnSelchangeSpan)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// exportCADData message handlers


/*--------------------------------------------------------------------*/
BOOL exportCADData::OnInitDialog() 
{
	/* Perform the root dialog initialization */
	CDialog::OnInitDialog();

	/* Get interface pointer to Bridge Agent */
	GET_IFACE( IBridge, pBridge ); 

	/* Get combo box pointers */
   CComboBox* pSpanBox = (CComboBox*)GetDlgItem( IDC_SPAN );
   CComboBox* pGdrBox  = (CComboBox*)GetDlgItem( IDC_GIRDER );
	  
	  /* Get count of spans from bridge agent */
	  /* Get count of spans from bridge agent */
	Uint32 cSpan = pBridge->GetSpanCount();

	/* Extract data from spans */
	for ( Uint32 i = 0; i < cSpan; i++ )
	{
		/* Add current span string to span list */
		CString strSpan;
		strSpan.Format(_T("Span %d"),LABEL_SPAN(i));
		pSpanBox->AddString(strSpan);
	}

   OnSelchangeSpan();

	pSpanBox->SetCurSel(m_Span);
	pGdrBox->SetCurSel(m_Girder);

   return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}


/*--------------------------------------------------------------------*/
void exportCADData::OnHelp() 
{
//   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_DIALOG_EXPORTTXDOTCADDATA);
}

void exportCADData::OnSelchangeSpan() 
{
	GET_IFACE( IBridge, pBridge ); 
	CComboBox* pSpanBox = (CComboBox*)GetDlgItem( IDC_SPAN );
   CComboBox* pGdrBox  = (CComboBox*)GetDlgItem( IDC_GIRDER );

   SpanIndexType span = pSpanBox->GetCurSel();
   if ( span == CB_ERR )
      span = 0;

   GirderIndexType girder = pGdrBox->GetCurSel();
   if (girder == CB_ERR )
      girder = 0;

   pGdrBox->ResetContent();

	/* Get count of girders (same number of girders in all spans) */
   GirderIndexType cGirder = pBridge->GetGirderCount(span);
	for ( GirderIndexType j = 0; j < cGirder; j++ )
	{
		/* Add current girder string to girder list */
		CString strGdr;
		strGdr.Format( _T("Girder %s"), LABEL_GIRDER(j));
		pGdrBox->AddString( strGdr );
	}

   girder = min(girder, cGirder-1); // don't allow out of bounds if ng decreases between spans

   pGdrBox->SetCurSel(girder);
	
}
