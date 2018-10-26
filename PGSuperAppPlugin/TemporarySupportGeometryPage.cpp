// TemporarySupportGeometryPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "PGSuperAppPlugin\Resource.h"
#include "PGSuperAppPlugin\TemporarySupportDlg.h"
#include "TemporarySupportGeometryPage.h"

#include <EAF\EAFDisplayUnits.h>
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>

// CTemporarySupportGeometryPage dialog

IMPLEMENT_DYNAMIC(CTemporarySupportGeometryPage, CPropertyPage)

CTemporarySupportGeometryPage::CTemporarySupportGeometryPage()
	: CPropertyPage(CTemporarySupportGeometryPage::IDD)
{
   m_WhiteBrush.CreateSolidBrush(RGB(255,255,255));
}

CTemporarySupportGeometryPage::~CTemporarySupportGeometryPage()
{
}

void CTemporarySupportGeometryPage::DoDataExchange(CDataExchange* pDX)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState()); // for access to metafile resources
   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();

   if ( !pDX->m_bSaveAndValidate )
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      pParent->m_TemporarySupport.GetGirderEndDistance(&m_GirderEndDistance,&m_EndDistanceMeasurementType);
      pParent->m_TemporarySupport.GetBearingOffset(&m_GirderBearingOffset,&m_BearingOffsetMeasurementType);
      m_SupportWidth = pParent->m_TemporarySupport.GetSupportWidth();
      m_ConnectionType = pParent->m_TemporarySupport.GetConnectionType();
   }

	CPropertyPage::DoDataExchange(pDX);

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CString image_name = GetImageName(m_ConnectionType,m_BearingOffsetMeasurementType,m_EndDistanceMeasurementType);
	DDX_MetaFileStatic(pDX, IDC_CONNECTION_MF, m_ConnectionPicture,image_name, _T("Metafile") );

   DDX_UnitValueAndTag(pDX, IDC_END_DISTANCE, IDC_END_DISTANCE_T, m_GirderEndDistance, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_END_DISTANCE, m_GirderEndDistance, pDisplayUnits->GetComponentDimUnit() );
   DDX_UnitValueAndTag(pDX, IDC_BEARING_OFFSET, IDC_BEARING_OFFSET_T, m_GirderBearingOffset, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_BEARING_OFFSET, m_GirderBearingOffset, pDisplayUnits->GetComponentDimUnit() );

   DDX_UnitValueAndTag(pDX, IDC_SUPPORT_WIDTH, IDC_SUPPORT_WIDTH_T, m_SupportWidth, pDisplayUnits->GetComponentDimUnit() );
   DDV_UnitValueZeroOrMore(pDX, IDC_SUPPORT_WIDTH, m_SupportWidth, pDisplayUnits->GetComponentDimUnit() );

   DDX_CBItemData(pDX,IDC_BEARING_OFFSET_MEASURE,m_BearingOffsetMeasurementType);
   DDX_CBItemData(pDX,IDC_END_DISTANCE_MEASURE,m_EndDistanceMeasurementType);

   DDX_CBItemData(pDX,IDC_CONNECTION_TYPE,m_ConnectionType);
   if ( m_ConnectionType != pgsTypes::sctContinuousSegment )
   {
      DDX_CBItemData(pDX,IDC_EVENT,pParent->m_ClosurePourEventIndex);
   }

   if ( pDX->m_bSaveAndValidate )
   {
      CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
      pParent->m_TemporarySupport.SetConnectionType(m_ConnectionType);
      pParent->m_TemporarySupport.SetGirderEndDistance(m_GirderEndDistance,m_EndDistanceMeasurementType);
      pParent->m_TemporarySupport.SetBearingOffset(m_GirderBearingOffset,m_BearingOffsetMeasurementType);
      pParent->m_TemporarySupport.SetSupportWidth(m_SupportWidth);
   }
}


BEGIN_MESSAGE_MAP(CTemporarySupportGeometryPage, CPropertyPage)
	ON_WM_CTLCOLOR()
	ON_CBN_SELCHANGE(IDC_END_DISTANCE_MEASURE, OnEndDistanceMeasureChanged)
	ON_CBN_SELCHANGE(IDC_BEARING_OFFSET_MEASURE, OnBearingOffsetMeasureChanged)
   ON_CBN_SELCHANGE(IDC_CONNECTION_TYPE,OnConnectionTypeChanged)
END_MESSAGE_MAP()


// CTemporarySupportGeometryPage message handlers

BOOL CTemporarySupportGeometryPage::OnInitDialog()
{
   FillConnectionTypeComboBox();
   FillBearingOffsetComboBox();
   FillEndDistanceComboBox();

   FillEventList();

   CPropertyPage::OnInitDialog();

   OnConnectionTypeChanged();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CTemporarySupportGeometryPage::OnEndDistanceMeasureChanged() 
{
   UpdateConnectionPicture();
}

void CTemporarySupportGeometryPage::OnBearingOffsetMeasureChanged() 
{
   UpdateConnectionPicture();
}

void CTemporarySupportGeometryPage::OnConnectionTypeChanged()
{
   UpdateConnectionPicture();

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_CONNECTION_TYPE);
   int curSel = pcbConnectionType->GetCurSel();
   pgsTypes::SegmentConnectionType connectionType = (pgsTypes::SegmentConnectionType)pcbConnectionType->GetItemData(curSel);

   int showWindow = (connectionType == pgsTypes::sctClosurePour ? SW_SHOW : SW_HIDE);
   GetDlgItem(IDC_BEARING_OFFSET_LABEL)->ShowWindow(showWindow);
   GetDlgItem(IDC_BEARING_OFFSET)->ShowWindow(showWindow);
   GetDlgItem(IDC_BEARING_OFFSET_T)->ShowWindow(showWindow);
   GetDlgItem(IDC_BEARING_OFFSET_MEASURE)->ShowWindow(showWindow);

   GetDlgItem(IDC_END_DISTANCE_LABEL)->ShowWindow(showWindow);
   GetDlgItem(IDC_END_DISTANCE)->ShowWindow(showWindow);
   GetDlgItem(IDC_END_DISTANCE_T)->ShowWindow(showWindow);
   GetDlgItem(IDC_END_DISTANCE_MEASURE)->ShowWindow(showWindow);

   GetDlgItem(IDC_EVENT)->ShowWindow(showWindow);
   GetDlgItem(IDC_EVENT_LABEL)->ShowWindow(showWindow);
}

void CTemporarySupportGeometryPage::UpdateConnectionPicture()
{
   CComboBox* pcbEnd   = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   int curSel = pcbEnd->GetCurSel();
   ConnectionLibraryEntry::EndDistanceMeasurementType ems = (ConnectionLibraryEntry::EndDistanceMeasurementType)pcbEnd->GetItemData(curSel);

   CComboBox* pcbBrg   = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   curSel = pcbBrg->GetCurSel();
   ConnectionLibraryEntry::BearingOffsetMeasurementType bms = (ConnectionLibraryEntry::BearingOffsetMeasurementType)pcbBrg->GetItemData(curSel);

   CComboBox* pcbConnectionType = (CComboBox*)GetDlgItem(IDC_CONNECTION_TYPE);
   curSel = pcbConnectionType->GetCurSel();
   pgsTypes::SegmentConnectionType connectionType = (pgsTypes::SegmentConnectionType)pcbConnectionType->GetItemData(curSel);

   CString image_name = GetImageName(connectionType,bms,ems);

	m_ConnectionPicture.SetImage(image_name, _T("Metafile") );
}

void CTemporarySupportGeometryPage::FillConnectionTypeComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_CONNECTION_TYPE);
   int cursel = pCB->GetCurSel();

   pCB->ResetContent();

   pCB->SetItemData(pCB->AddString(_T("Closure Pour")),(DWORD_PTR)pgsTypes::sctClosurePour);

   CTemporarySupportDlg* pParent = (CTemporarySupportDlg*)GetParent();
   if ( pParent->m_TemporarySupport.GetSupportType() != pgsTypes::StrongBack )
      pCB->SetItemData(pCB->AddString(_T("Continuous Segment")),(DWORD_PTR)pgsTypes::sctContinuousSegment);

   if ( cursel != CB_ERR )
      cursel = pCB->SetCurSel(cursel);

   if ( cursel == CB_ERR )
      pCB->SetCurSel(0);
}

void CTemporarySupportGeometryPage::FillBearingOffsetComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_BEARING_OFFSET_MEASURE);
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Measured Normal to Temporary Support Line"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::NormalToPier));

   idx = pCB->AddString(_T("Measured Along Centerline Girder"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::AlongGirder));
}

void CTemporarySupportGeometryPage::FillEndDistanceComboBox()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_END_DISTANCE_MEASURE);
   pCB->ResetContent();

   int idx = pCB->AddString(_T("Measured from CL Bearing, Along Girder"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingAlongGirder));

   idx = pCB->AddString(_T("Measured from and normal to CL Bearing"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromBearingNormalToPier));

   idx = pCB->AddString(_T("Measured from Temporary Support Line, Along Girder"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierAlongGirder));

   idx = pCB->AddString(_T("Measured from and normal to Temporary Support Line"));
   pCB->SetItemData(idx,DWORD(ConnectionLibraryEntry::FromPierNormalToPier));
}

HBRUSH CTemporarySupportGeometryPage::OnCtlColor(CDC* pDC,CWnd* pWnd,UINT nCtlColor)
{
   HBRUSH hBrush = CDialog::OnCtlColor(pDC,pWnd,nCtlColor);
   if ( pWnd->GetDlgCtrlID() == IDC_CONNECTION_MF )
   {
      return m_WhiteBrush;
   }

   return hBrush;
}

CString CTemporarySupportGeometryPage::GetImageName(pgsTypes::SegmentConnectionType connectionType,ConnectionLibraryEntry::BearingOffsetMeasurementType brgOffsetType,ConnectionLibraryEntry::EndDistanceMeasurementType endType)
{
   CString strName;
   if ( connectionType == pgsTypes::sctClosurePour )
   {
      if ( brgOffsetType == ConnectionLibraryEntry::AlongGirder )
      {
         switch( endType )
         {
         case ConnectionLibraryEntry::FromBearingAlongGirder:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGGDRFROMBRG");
            break;

         case ConnectionLibraryEntry::FromBearingNormalToPier:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGNORMALFROMBRG");
            break;

         case ConnectionLibraryEntry::FromPierAlongGirder:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGGDRFROMPIER");
            break;

         case ConnectionLibraryEntry::FromPierNormalToPier:
            strName = _T("CLOSURE_BRGALONGGDR_ENDALONGNORMALFROMPIER");
            break;
         }
      }
      else if ( brgOffsetType == ConnectionLibraryEntry::NormalToPier )
      {
         switch( endType )
         {
         case ConnectionLibraryEntry::FromBearingAlongGirder:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGGDRFROMBRG");
            break;

         case ConnectionLibraryEntry::FromBearingNormalToPier:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGNORMALFROMBRG");
            break;

         case ConnectionLibraryEntry::FromPierAlongGirder:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGGDRFROMPIER");
            break;

         case ConnectionLibraryEntry::FromPierNormalToPier:
            strName = _T("CLOSURE_BRGALONGNORMAL_ENDALONGNORMALFROMPIER");
            break;
         }
      }
   }
   else
   {
      strName = _T("TS_CONTINUOUS_SEGMENT");
   }

   return strName;
}

BOOL CTemporarySupportGeometryPage::OnSetActive()
{
   FillConnectionTypeComboBox();

   return CPropertyPage::OnSetActive();
}

void CTemporarySupportGeometryPage::FillEventList()
{
   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

   int eventIdx = pcbEvent->GetCurSel();

   pcbEvent->ResetContent();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   EventIndexType nEvents = pTimelineMgr->GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pTimelineMgr->GetEventByIndex(eventIdx);

      CString label;
      label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

      pcbEvent->SetItemData(pcbEvent->AddString(label),eventIdx);
   }

   CString strNewEvent((LPCSTR)IDS_CREATE_NEW_EVENT);
   pcbEvent->SetItemData(pcbEvent->AddString(strNewEvent),CREATE_TIMELINE_EVENT);

   if ( eventIdx != CB_ERR )
      pcbEvent->SetCurSel(eventIdx);
}
