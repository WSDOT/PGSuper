// TxDOTOptionalDesignStandardFillDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TxDOTOptionalDesignStandardFillDlg.h"
#include <System\Tokenizer.h>
#include <MfcTools\CustomDDX.h>
#include <EAF\EAFDisplayUnits.h>
#include "EccentricityDlg.h"

// CTxDOTOptionalDesignStandardFillDlg dialog

IMPLEMENT_DYNAMIC(CTxDOTOptionalDesignStandardFillDlg, CDialog)

CTxDOTOptionalDesignStandardFillDlg::CTxDOTOptionalDesignStandardFillDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTxDOTOptionalDesignStandardFillDlg::IDD, pParent)
   , m_strNumStrands(_T(""))
   , m_bFirstActive(true)
{
   m_pGirderData = NULL;
   m_pBrokerRetriever = NULL;
}

CTxDOTOptionalDesignStandardFillDlg::~CTxDOTOptionalDesignStandardFillDlg()
{
}

BOOL CTxDOTOptionalDesignStandardFillDlg::OnInitDialog()
{
   // Load data into local members
   LoadDialogData();


   CDialog::OnInitDialog();


   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignStandardFillDlg, CDialog)
   ON_CBN_SELCHANGE(IDC_OPT_NUM_STRANDS, &CTxDOTOptionalDesignStandardFillDlg::OnCbnSelchangeOptNumStrands)
   ON_BN_CLICKED(IDC_OPT_COMPUTE, &CTxDOTOptionalDesignStandardFillDlg::OnBnClickedOptCompute)
   ON_WM_ERASEBKGND()
   ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

void CTxDOTOptionalDesignStandardFillDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);

   if (!pDX->m_bSaveAndValidate)
   {
      UpdateLibraryData();
   }

   CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetClassicBroker();
   if (pBroker==NULL)
      return;

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   DDX_CBString(pDX, IDC_OPT_NUM_STRANDS, m_strNumStrands);

   bool st = sysTokenizer::ParseLong(m_strNumStrands, &m_NumStrands);  // save num strands as integral value as well
   ASSERT(st);

   if (m_NumStrands>0) // only parse To value if we have strands
   {
      DDX_UnitValueAndTag(pDX,IDC_OPT_TO, IDC_OPT_TO_UNITS, m_To, pDisplayUnits->GetComponentDimUnit() );

      if (pDX->m_bSaveAndValidate)
      {
         // some effort to respect To range
         GirderLibrary* pLib = m_pBrokerRetriever->GetGirderLibrary();

         Float64 toLower, toUpper;
         m_pGirderData->ComputeToRange(pLib, m_NumStrands, &toLower, &toUpper);

         DDV_UnitValueRange( pDX,IDC_OPT_TO,m_To,toLower, toUpper, pDisplayUnits->GetComponentDimUnit() );
      }
   }

   if (pDX->m_bSaveAndValidate)
   {
      SaveDialogData();
   }
   else
   {
      UpdateControls();
   }
}

void CTxDOTOptionalDesignStandardFillDlg::LoadDialogData()
{
   int ns = m_pGirderData->GetNumStrands();
   m_strNumStrands.Format("%d",ns);

   m_To = m_pGirderData->GetStrandTo();
}

void CTxDOTOptionalDesignStandardFillDlg::SaveDialogData()
{
   m_pGirderData->SetNumStrands(m_NumStrands);
   m_pGirderData->SetStrandTo(m_To);
}

void CTxDOTOptionalDesignStandardFillDlg::Init(CTxDOTOptionalDesignGirderData* pGirderData, ITxDOTBrokerRetriever* pBrokerRetriever, const char* entryName)
{
   m_pGirderData = pGirderData;
   m_pBrokerRetriever = pBrokerRetriever;
   m_GirderEntryName = entryName;
}

void CTxDOTOptionalDesignStandardFillDlg::SetGirderEntryName(const char* entryName)
{
   if (m_GirderEntryName != entryName)
   {
      m_GirderEntryName = entryName;

      UpdateLibraryData();
   }
}

// CTxDOTOptionalDesignStandardFillDlg message handlers

void CTxDOTOptionalDesignStandardFillDlg::OnCbnSelchangeOptNumStrands()
{
   CComboBox* pBox = (CComboBox*)GetDlgItem(IDC_OPT_NUM_STRANDS);
   int sel = pBox->GetCurSel();
   if (sel!=CB_ERR)
   {
      pBox->GetLBText(sel,m_strNumStrands);
      bool st = sysTokenizer::ParseLong(m_strNumStrands, &m_NumStrands);  // save num strands as integral value as well
      ASSERT(st);
   }
   else
   {
      m_strNumStrands="0";
      m_NumStrands=0;
   }

   UpdateControls();
}

void CTxDOTOptionalDesignStandardFillDlg::UpdateControls()
{
   // Note - this function assumes that dialog data is updated

   // update number of depressed strands
   GirderLibrary* pLib = m_pBrokerRetriever->GetGirderLibrary();
   const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pLib->GetEntry(m_GirderEntryName));
   ASSERT(pGdrEntry!=NULL);

   CWnd* pout = (CWnd*)GetDlgItem(IDC_OPT_NO_DEPRESSED);

   StrandIndexType numStraight(0), numHarped(0);
   if (!pGdrEntry->ComputeGlobalStrands(m_NumStrands, &numStraight, &numHarped))
   {
      ASSERT(0);
      pout->SetWindowTextA("Error computing harped strands");
   }
   else
   {
      CString msg;
      msg.Format("(No. Depressed strands = %d)", numHarped);
      pout->SetWindowTextA(msg);
   }

   // To Value range
   BOOL benable = m_NumStrands>0 ? TRUE:FALSE;

   CWnd* pToEdit = GetDlgItem(IDC_OPT_TO);
   CWnd* pToUnit = GetDlgItem(IDC_OPT_TO_UNITS);
   CWnd* pToTag  = GetDlgItem(IDC_OPT_TO_TAG);
   CWnd* pToRange = GetDlgItem(IDC_OPT_TO_VALID_RANGE);
   CWnd* pCompute  = GetDlgItem(IDC_OPT_COMPUTE);

   pToEdit->EnableWindow(benable);
   pToUnit->EnableWindow(benable);
   pToTag->EnableWindow(benable);
   pCompute->EnableWindow(benable);
   pToRange->ShowWindow(benable?SW_SHOW:SW_HIDE);

   if (benable)
   {
      CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetClassicBroker();
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      Float64 toLower, toUpper;
      m_pGirderData->ComputeToRange(pLib, m_NumStrands, &toLower, &toUpper);
      toLower = ::ConvertFromSysUnits(toLower, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
      toUpper = ::ConvertFromSysUnits(toUpper, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

      CString umsg;
      umsg.Format("(Valid Range: %.3f to %.3f)",toLower, toUpper);

      pToRange->SetWindowTextA(umsg);
   }
}

void  CTxDOTOptionalDesignStandardFillDlg::UpdateLibraryData()
{
   InitStrandNoCtrl();
}

void CTxDOTOptionalDesignStandardFillDlg::InitStrandNoCtrl()
{
   CComboBox* pList = (CComboBox*)GetDlgItem( IDC_OPT_NUM_STRANDS );
   pList->ResetContent();

   GirderLibrary* pLib = m_pBrokerRetriever->GetGirderLibrary();
   CString str;
   std::vector<StrandIndexType> strands = m_pGirderData->ComputeAvailableNumStrands(pLib);
   for (std::vector<StrandIndexType>::iterator iter = strands.begin(); iter != strands.end(); iter++)
   {
      str.Format("%d", *iter);
      pList->AddString(str);
   }
}

void CTxDOTOptionalDesignStandardFillDlg::OnBnClickedOptCompute()
{
   if (!UpdateData(TRUE))
      return;

   GirderLibrary* pLib = m_pBrokerRetriever->GetGirderLibrary();

   Float64 eccEnds, eccCL;
   m_pGirderData->ComputeEccentricities(pLib,m_NumStrands,m_To,&eccEnds,&eccCL);

   eccEnds = ::ConvertFromSysUnits(eccEnds,unitMeasure::Inch);
   eccCL = ::ConvertFromSysUnits(eccCL,unitMeasure::Inch);

   CEccentricityDlg dlg;
   dlg.m_Message.Format("ecc, cl = %.3f in\n ecc, ends = %.3f in", eccCL, eccEnds);
   dlg.DoModal();
}

// We want to play like a property page, but we aren't one
BOOL CTxDOTOptionalDesignStandardFillDlg::OnFillSetActive()
{
   if (m_bFirstActive)
   {
      m_bFirstActive = false;
      return TRUE;
   }
   else
   {
      LoadDialogData();

      return UpdateData(FALSE);
   }
}

BOOL CTxDOTOptionalDesignStandardFillDlg::OnFillKillActive()
{
   return UpdateData(TRUE);
}

BOOL CTxDOTOptionalDesignStandardFillDlg::OnEraseBkgnd(CDC* pDC)
{
    CRect r;
    GetClientRect(&r);

    CBrush brush;
    brush.CreateSolidBrush(TXDOT_BACK_COLOR);

    pDC->FillRect(&r, &brush);
    return TRUE;
}

HBRUSH CTxDOTOptionalDesignStandardFillDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   HBRUSH hbr = __super::OnCtlColor(pDC, pWnd, nCtlColor);

   TCHAR classname[MAX_PATH];
   if(::GetClassName(pWnd->m_hWnd, classname, MAX_PATH) == 0)
     return hbr;
   if(_tcsicmp(classname, _T("EDIT")) == 0)
     return hbr;
   if(_tcsicmp(classname, _T("COMBOBOX")) == 0)
     return hbr;
   if(_tcsicmp(classname, _T("COMBOLBOX")) == 0)
     return hbr;
   if(_tcsicmp(classname, _T("LISTBOX")) == 0)
     return hbr;

   pDC->SetBkColor(TXDOT_BACK_COLOR);

   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   return (HBRUSH)backBrush;
}
