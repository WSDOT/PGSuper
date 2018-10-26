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

// BridgeDescFramingPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperDoc.h"
#include "BridgeDescFramingPage.h"
#include <MFCTools\CogoDDX.h>
#include <MFCTools\CustomDDX.h>
#include "SpanLengthDlg.h"
#include "BridgeDescDlg.h"

#include "HtmlHelp\HelpTopics.hh"

#include <IFace\Project.h>
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
   m_strRemoveSpanTip = "Select a span row in the grid.\nRemoves the selected span and the next pier from the bridge model";
   m_strAddSpanTip = "Adds a pier and a span at the end of the selected span\nAdds a span and a pier after the selected pier";
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
   DDX_Control(pDX, IDC_ALIGNMENTOFFSET_FMT, m_AlignmentOffsetFormat);
	//}}AFX_DATA_MAP

   CBridgeDescDlg* pParent = (CBridgeDescDlg*)GetParent();
   ASSERT( pParent->IsKindOf(RUNTIME_CLASS(CBridgeDescDlg)) );

   if ( pDX->m_bSaveAndValidate )
   {
      DDV_GXGridWnd(pDX, &m_Grid);

      m_Grid.GetGridData();
   }
   else
   {
      m_Grid.FillGrid(pParent->m_BridgeDesc);
   }

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   if ( !pDX->m_bSaveAndValidate )
   {
      m_AlignmentOffset = pParent->m_BridgeDesc.GetAlignmentOffset();
   }

   DDX_OffsetAndTag(pDX, IDC_ALIGNMENTOFFSET, IDC_ALIGNMENTOFFSET_UNIT, m_AlignmentOffset, pDisplayUnits->GetAlignmentLengthUnit() );
   
   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_BridgeDesc.SetAlignmentOffset(m_AlignmentOffset);
   }

}


BEGIN_MESSAGE_MAP(CBridgeDescFramingPage, CPropertyPage)
	//{{AFX_MSG_MAP(CBridgeDescFramingPage)
	ON_COMMAND(ID_HELP, OnHelp)
   ON_COMMAND(IDC_ADD_SPAN, OnAddSpan)
   ON_COMMAND(IDC_REMOVE_SPAN, OnRemoveSpan)
   ON_COMMAND(IDC_LAYOUT, OnLayoutBySpanLengths)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CBridgeDescFramingPage message handlers

void CBridgeDescFramingPage::EnableRemove(BOOL bEnable)
{
   GetDlgItem(IDC_REMOVE_SPAN)->EnableWindow(bEnable);
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

   fmt.LoadString( IDS_ALIGNMENTOFFSET_FMT);
   m_AlignmentOffsetFormat.SetWindowText(fmt);

   // causes the Remove Span button to be enabled/disabled
   EnableRemove(m_Grid.EnableItemDelete());

	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CBridgeDescFramingPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_BRIDGEDESC_FRAMING );
}

void CBridgeDescFramingPage::OnRemoveSpan() 
{
   m_Grid.OnRemoveSpan();
   EnableRemove(FALSE); // nothing selected
}

void CBridgeDescFramingPage::OnAddSpan() 
{
   m_Grid.OnAddSpan();
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

BOOL CBridgeDescFramingPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_ADD_SPAN:
         pTTT->lpszText = m_strAddSpanTip.LockBuffer();
         pTTT->hinst = NULL;
         break;

      case IDC_REMOVE_SPAN:
         pTTT->lpszText = m_strRemoveSpanTip.LockBuffer();
         pTTT->hinst = NULL;
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

BOOL CBridgeDescFramingPage::OnNcActivate(BOOL bActive)
{
   // This is required when a CGXComboBox is used in the grid. It prevents
   // the dialog from losing focus and the title bar from flashing. See CGXComboBox documentation
   if ( GXDiscardNcActivate() )
      return true;

   return CPropertyPage::OnNcActivate(bActive);
}
