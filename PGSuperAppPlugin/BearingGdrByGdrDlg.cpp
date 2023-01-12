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

//
#include "stdafx.h"
#include "resource.h"
#include "BearingGdrByGdrDlg.h"

#include <IFace\Bridge.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


IMPLEMENT_DYNAMIC(CBearingGdrByGdrDlg, CDialog)

CBearingGdrByGdrDlg::CBearingGdrByGdrDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CBearingGdrByGdrDlg::IDD, pParent)
{
}

CBearingGdrByGdrDlg::~CBearingGdrByGdrDlg()
{
}

void CBearingGdrByGdrDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	// validation routine for CGXTabWnd controls
	DDV_GXTabWnd(pDX, &m_GirderTabWnd);
}

BEGIN_MESSAGE_MAP(CBearingGdrByGdrDlg, CDialog)
END_MESSAGE_MAP()

BOOL CBearingGdrByGdrDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridge,pBridge);

   m_GirderTabWnd.SubclassDlgItem(IDC_BEARING_GRID, this);

   // Init Girder grids
   SpanIndexType nspans = pBridge->GetSpanCount();
   for (SpanIndexType ispan=0; ispan<nspans; ispan++)
   {
      // Each grid holds a reference to our data
      CBearingGdrGrid* pgrid = new CBearingGdrGrid(&m_BearingInputData);

	   pgrid->Create(0, CRect(0,0,1,1), &m_GirderTabWnd, m_GirderTabWnd.GetNextID());

      CString span_name;
      span_name.Format(_T("Span %s"), LABEL_SPAN(ispan));
   	m_GirderTabWnd.AttachWnd(pgrid, span_name);

      pgrid->CustomInit(ispan);

      m_GirderGrids.push_back( std::shared_ptr<CBearingGdrGrid>(pgrid) );
   }

   // Deal with scroll bars

   int growhgt(0);
   for (GirderGridIterator ig=m_GirderGrids.begin(); ig!=m_GirderGrids.end(); ig++)
   {
      ROWCOL nrows = ig->get()->GetRowCount();
      if (nrows>0)
         growhgt = Max(growhgt, ig->get()->CalcSumOfRowHeights(0,nrows-1)); 
   }

   CRect trect;
   m_GirderTabWnd.GetInsideRect(trect);

   if (growhgt < trect.Height())
   {
      m_GirderTabWnd.ShowScrollBar(SB_VERT, FALSE);
   }

	m_GirderTabWnd.SetFocus();


   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CBearingGdrByGdrDlg::UploadData(const BearingInputData& rData)
{
   m_BearingInputData = rData;

   for (auto& grid : m_GirderGrids)
   {
      grid->FillGrid();
   }
}

void CBearingGdrByGdrDlg::DownloadData(BearingInputData* pData, CDataExchange* pDX)
{
   for (auto& grid : m_GirderGrids)
   {
      // Each grid has a pointer to this' data
      grid->GetData(pDX);
   }

   *pData = m_BearingInputData;
}

