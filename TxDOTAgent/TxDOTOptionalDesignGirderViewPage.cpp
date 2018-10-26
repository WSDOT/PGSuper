// TxDOTOptionalDesignGirderViewPage.cpp : implementation file
//

#include "stdafx.h"
#include "TxDOTOptionalDesignGirderViewPage.h"


// CTxDOTOptionalDesignGirderViewPage dialog

IMPLEMENT_DYNAMIC(CTxDOTOptionalDesignGirderViewPage, CPropertyPage)

CTxDOTOptionalDesignGirderViewPage::CTxDOTOptionalDesignGirderViewPage()
	: CPropertyPage(CTxDOTOptionalDesignGirderViewPage::IDD),
   m_pData(NULL),
   m_pBrokerRetriever(NULL),
   m_ChangeStatus(0)
{
}

CTxDOTOptionalDesignGirderViewPage::~CTxDOTOptionalDesignGirderViewPage()
{
}

void CTxDOTOptionalDesignGirderViewPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignGirderViewPage, CPropertyPage)
   ON_WM_ERASEBKGND()
   ON_WM_CTLCOLOR()
   ON_COMMAND(ID_FILE_PRINT, &CTxDOTOptionalDesignGirderViewPage::OnFilePrint)
END_MESSAGE_MAP()


// CTxDOTOptionalDesignGirderViewPage message handlers

BOOL CTxDOTOptionalDesignGirderViewPage::OnSetActive()
{
   if (m_ChangeStatus!=0)
   {
      try
      {
         // We need an updated broker to work for us
         CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetUpdatedBroker();

         // our data is updated
         m_ChangeStatus = 0;

      }
      catch(TxDOTBrokerRetrieverException exc)
      {
         ASSERT(0);
      }
      catch(...)
      {
         ASSERT(0);
      }
   }


   return CPropertyPage::OnSetActive();
}

BOOL CTxDOTOptionalDesignGirderViewPage::OnInitDialog()
{
   __super::OnInitDialog();

   // At this point our document is alive. 
   // We aren't a view, so we subvert doc view and listen directly to data source
   ASSERT(m_pData);
   m_pData->Attach(this);

   // This is our first update - we know changes have happened
   m_ChangeStatus = ITxDataObserver::ctPGSuper;

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CTxDOTOptionalDesignGirderViewPage::OnTxDotDataChanged(int change)
{
   // save change information
   m_ChangeStatus |= change;
}

BOOL CTxDOTOptionalDesignGirderViewPage::OnEraseBkgnd(CDC* pDC)
{
   // Set brush to dialog background color
   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   // Save old brush
   CBrush* pOldBrush = pDC->SelectObject(&backBrush);

   CRect rect;
   pDC->GetClipBox(&rect);     // Erase the area needed

   pDC->PatBlt(rect.left, rect.top, rect.Width(), rect.Height(),
       PATCOPY);
   pDC->SelectObject(pOldBrush);

   return true;
}

HBRUSH CTxDOTOptionalDesignGirderViewPage::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   pDC->SetBkColor(TXDOT_BACK_COLOR);

   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   return (HBRUSH)backBrush;
}

void CTxDOTOptionalDesignGirderViewPage::OnFilePrint()
{
   // TODO: Add your command handler code here
}

void CTxDOTOptionalDesignGirderViewPage::AssertValid() const
{
   // Asserts will fire if not in static module state
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   __super::AssertValid();
}
