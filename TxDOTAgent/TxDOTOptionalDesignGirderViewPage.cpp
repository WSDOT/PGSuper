// TxDOTOptionalDesignGirderViewPage.cpp : implementation file
//

#include "stdafx.h"
#include "TxDOTOptionalDesignGirderViewPage.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <EAF\EAFDisplayUnits.h>

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

void CTxDOTOptionalDesignGirderViewPage::GetSpanAndGirderSelection(SpanIndexType* pSpan,GirderIndexType* pGirder)
{
   *pSpan = TOGA_SPAN;
   *pGirder = TOGA_FABR_GDR;
}



void CTxDOTOptionalDesignGirderViewPage::ShowCutDlg()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   Float64 val  = m_CurrentCutLocation;
   Float64 high = m_MaxCutLocation;

   CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetUpdatedBroker();
   GET_IFACE2(pBroker,IStrandGeometry,pStrandGeom);

   SpanIndexType span;
   GirderIndexType gdr;
   GetSpanAndGirderSelection(&span,&gdr);

   ATLASSERT( span != ALL_SPANS && gdr != ALL_GIRDERS  );
   Uint16 nHarpPoints = pStrandGeom->GetNumHarpPoints(span,gdr);

   ASSERT(0);
/*
   CSectionCutDlgEx dlg(nHarpPoints,m_CurrentCutLocation,0.0,high,m_CutLocation);

   int st = dlg.DoModal();
   if (st==IDOK)
   {
      m_CurrentCutLocation = dlg.GetValue();
      UpdateCutLocation(dlg.GetCutLocation(),m_CurrentCutLocation);
   }
*/
}

void CTxDOTOptionalDesignGirderViewPage::CutAt(Float64 cut)
{
   UpdateCutLocation(UserInput,cut);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtLeftEnd() 
{
   UpdateCutLocation(LeftEnd);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtLeftHp() 
{
   UpdateCutLocation(LeftHarp);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtCenter() 
{
   UpdateCutLocation(Center);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtRightHp() 
{
   UpdateCutLocation(RightHarp);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtRightEnd() 
{
   UpdateCutLocation(RightEnd);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtNext()
{
   double f = m_CurrentCutLocation/m_MaxCutLocation;
   f = ::RoundOff(f+0.1,0.1);
   if ( 1 < f )
      f = 1;

   CutAt(f*m_MaxCutLocation);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtPrev()
{
   double f = m_CurrentCutLocation/m_MaxCutLocation;
   f = ::RoundOff(f-0.1,0.1);
   if ( f < 0 )
      f = 0;

   CutAt(f*m_MaxCutLocation);
}

void CTxDOTOptionalDesignGirderViewPage::CutAtLocation()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetUpdatedBroker();

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   Float64 val  = ::ConvertFromSysUnits(m_CurrentCutLocation,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
   Float64 high = ::ConvertFromSysUnits(m_MaxCutLocation,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);

   ASSERT(0);
/*
   CSectionCutDlg dlg(val,0.0,high,pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure.UnitTag().c_str());

   int st = dlg.DoModal();
   if (st==IDOK)
   {
      val = ::ConvertToSysUnits(dlg.GetValue(),pDisplayUnits->GetSpanLengthUnit().UnitOfMeasure);
      CutAt(val);
   }

   // Because the dialog messes with the screen
   // force an update (this is a hack because of the selection tool).
   GetGirderModelElevationView()->Invalidate();
   GetGirderModelElevationView()->UpdateWindow();
*/
}


void CTxDOTOptionalDesignGirderViewPage::UpdateCutLocation(CutLocation cutLoc,Float64 cut)
{
   m_CurrentCutLocation = cut;
   m_CutLocation = cutLoc;

   ASSERT(0);
/*
   UpdateBar();
   GetGirderModelSectionView()->OnUpdate(NULL, HINT_GIRDERVIEWSECTIONCUTCHANGED, NULL);
   GetGirderModelElevationView()->OnUpdate(NULL, HINT_GIRDERVIEWSECTIONCUTCHANGED, NULL);
*/
}