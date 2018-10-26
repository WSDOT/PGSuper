///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

// BridgeDescFramingPage2.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "BridgeDescFramingPage.h"
#include "SpanLengthDlg.h"
#include "BridgeDescDlg.h"

#include <IFace\Project.h>
#include <IFace\DocumentType.h>
#include <EAF\EAFDisplayUnits.h>
#include <PgsExt\PierData.h>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void DDX_Orientation(CDataExchange* pDX,int nIDC,CPierData& pd);
void DDV_Orientation(CDataExchange* pDX,CPierData& pd,UINT id);

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingPage property page

IMPLEMENT_DYNCREATE(CBridgeDescFramingPage, CPropertyPage)

CBridgeDescFramingPage::CBridgeDescFramingPage() : 
CPropertyPage(CBridgeDescFramingPage::IDD)
{
	//{{AFX_DATA_INIT(CBridgeDescFramingPage)
	//}}AFX_DATA_INIT
}

CBridgeDescFramingPage::~CBridgeDescFramingPage()
{
}

void CBridgeDescFramingPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);


   //{{AFX_DATA_MAP(CBridgeDescFramingPage)
   DDX_Control(pDX, IDC_ORIENTATIONFMT, m_OrientationFormat);
	DDX_Control(pDX, IDC_STATIONFMT, m_StationFormat);
	//}}AFX_DATA_MAP

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   if ( pDX->m_bSaveAndValidate )
   {
      DDV_GXGridWnd(pDX, &m_Grid);
      // why validate grid and then not get the data out of it?
      // pParent->m_BridgeDesc is kept continuously up to date with grid
   }
   else
   {
      m_Grid.FillGrid(pParent->m_BridgeDesc);
   }
}


BEGIN_MESSAGE_MAP(CBridgeDescFramingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeDescFramingPage)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_COMMAND(IDC_ADD_PIER, OnAddPier)
   ON_COMMAND(IDC_REMOVE_PIER, OnRemovePier)
   ON_COMMAND(IDC_ADD_TEMP_SUPPORT, OnAddTemporarySupport)
   ON_COMMAND(IDC_REMOVE_TEMP_SUPPORT, OnRemoveTemporarySupport)
   ON_COMMAND(IDC_LAYOUT, OnLayoutBySpanLengths)
	//}}AFX_MSG_MAP
   ON_BN_CLICKED(IDC_ORIENT_PIERS, &CBridgeDescFramingPage::OnOrientPiers)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingPage message handlers

void CBridgeDescFramingPage::EnableRemovePierBtn(BOOL bEnable)
{
   GetDlgItem(IDC_REMOVE_PIER)->EnableWindow(bEnable);
   if ( bEnable )
   {
      PierIndexType pierIdx = m_Grid.GetSelectedPier();
      PierIndexType nPiers  = m_Grid.GetPierCount();

      if ( pierIdx == INVALID_INDEX )
      {
         GetDlgItem(IDC_REMOVE_PIER)->SetWindowText(_T("Remove Span"));
      }
      else
      {
         CString strLabel;
         if ( pierIdx == nPiers-1 )
         {
            // last pier is selected
            strLabel.Format(_T("Remove Span %d/Pier %d"),LABEL_SPAN(pierIdx-1),LABEL_PIER(pierIdx));
         }
         else
         {
            strLabel.Format(_T("Remove Pier %d/Span %d"),LABEL_PIER(pierIdx),LABEL_SPAN(pierIdx));
         }

         GetDlgItem(IDC_REMOVE_PIER)->SetWindowText(strLabel);
      }

   }
   else
   {
      GetDlgItem(IDC_REMOVE_PIER)->SetWindowText(_T("Remove Span"));
   }
}

void CBridgeDescFramingPage::EnableRemoveTemporarySupportBtn(BOOL bEnable)
{
   GetDlgItem(IDC_REMOVE_TEMP_SUPPORT)->EnableWindow(bEnable);
}

BOOL CBridgeDescFramingPage::OnInitDialog() 
{
   EnableToolTips(TRUE);

   m_Grid.SubclassDlgItem(IDC_PIER_GRID, this);
   m_Grid.CustomInit();

   CPropertyPage::OnInitDialog();
	
	// TODO: Add extra initialization here
   CString fmt;

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);
   
   fmt.LoadString( IS_SI_UNITS(pDisplayUnits) ? IDS_DLG_STATIONFMT_SI : IDS_DLG_STATIONFMT_US );
   m_StationFormat.SetWindowText( fmt );

   fmt.LoadString( IDS_DLG_ORIENTATIONFMT );
   m_OrientationFormat.SetWindowText( fmt );

   // causes the Remove Span button to be enabled/disabled
   EnableRemovePierBtn(m_Grid.EnableRemovePierBtn());
   EnableRemoveTemporarySupportBtn(m_Grid.EnableRemoveTemporarySupportBtn());

   // I don't like using this interface because we should be a generic as possible
   // I can't think of a different way to this this right now.
   // Also, in the future, PGSuper will support temporary supports in the form of shoring towers during erection
   GET_IFACE2(pBroker,IDocumentType,pDocType);
   if ( pDocType->IsPGSuperDocument() )
   {
      GetDlgItem(IDC_ADD_TEMP_SUPPORT)->EnableWindow(FALSE);
   }

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBridgeDescFramingPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_BRIDGEDESC_FRAMING );
}

void CBridgeDescFramingPage::OnRemovePier() 
{
   m_Grid.OnRemovePier();
   EnableRemovePierBtn(FALSE); // nothing selected
}

void CBridgeDescFramingPage::OnAddPier() 
{
   m_Grid.OnAddPier();
}

void CBridgeDescFramingPage::OnLayoutBySpanLengths()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   std::vector<Float64> spanLengths = m_Grid.GetSpanLengths();
   CSpanLengthDlg dlg;
   dlg.m_SpanLengths = spanLengths;
   if ( dlg.DoModal() == IDOK )
   {
      m_Grid.SetSpanLengths(dlg.m_SpanLengths,dlg.m_PierIdx);
   }
}

BOOL CBridgeDescFramingPage::OnNcActivate(BOOL bActive)
{
   // This is required when a CGXComboBox is used in the grid. It prevents
   // the dialog from losing focus and the title bar from flashing. See CGXComboBox documentation
   if ( GXDiscardNcActivate() )
   {
      return true;
   }

   return CPropertyPage::OnNcActivate(bActive);
}

void CBridgeDescFramingPage::OnAddTemporarySupport()
{
   m_Grid.OnAddTemporarySupport();
}

void CBridgeDescFramingPage::OnRemoveTemporarySupport()
{
   m_Grid.OnRemoveTemporarySupport();
   EnableRemoveTemporarySupportBtn(FALSE); // nothing selected
}

void CBridgeDescFramingPage::OnOrientPiers()
{
   CString strResult;
   if ( AfxQuestion(_T("Orient Piers"),_T("Enter orientation of all piers and temporary supports"),_T("NORMAL"),strResult) )
   {
      m_Grid.SetPierOrientation(strResult);
   }
}
