// TxDOTOptionalDesignPropertySheet.cpp : implementation file
//

#include "stdafx.h"
#include "TxDOTOptionalDesignPropertySheet.h"


// CTxDOTOptionalDesignPropertySheet

IMPLEMENT_DYNAMIC(CTxDOTOptionalDesignPropertySheet, CPropertySheet)

CTxDOTOptionalDesignPropertySheet::CTxDOTOptionalDesignPropertySheet(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage),
   m_TabHeight(0)
{
}

CTxDOTOptionalDesignPropertySheet::CTxDOTOptionalDesignPropertySheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage),
   m_TabHeight(0)
{
}

CTxDOTOptionalDesignPropertySheet::~CTxDOTOptionalDesignPropertySheet()
{
}


BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignPropertySheet, CPropertySheet)
   ON_WM_SIZE()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()


// CTxDOTOptionalDesignPropertySheet message handlers

void CTxDOTOptionalDesignPropertySheet::OnSize(UINT nType, int cx, int cy)
{
//   CPropertySheet::OnSize(nType, cx, cy);

   // resize the CTabCtrl
   CTabCtrl* pTab = GetTabControl ();
   if (pTab!=NULL && ::IsWindow(pTab->m_hWnd))
   {
      pTab->MoveWindow(0, 0, cx, cy);
   }

   CPropertyPage* pPage = GetActivePage();
   if (pPage!=NULL && ::IsWindow(pPage->m_hWnd))
   {
      CRect rc;
      pTab->GetWindowRect (&rc);
      ScreenToClient (&rc);

 	   pPage->MoveWindow(0, m_TabHeight*pTab->GetRowCount(), cx, cy);
   }
}

BOOL CTxDOTOptionalDesignPropertySheet::OnInitDialog()
{
   BOOL bResult = CPropertySheet::OnInitDialog();

   // Save size of tab control
   CTabCtrl* pTab = GetTabControl ();
   ASSERT (pTab);
   CRect rc;
   pTab->GetItemRect(0,&rc); // height of a tab
   m_TabHeight = rc.Height() + 6; // add a few pixels to make some space

   return bResult;
}
