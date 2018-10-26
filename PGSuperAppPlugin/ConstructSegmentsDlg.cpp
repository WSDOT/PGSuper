// ConstructSegmentsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "ConstructSegmentsDlg.h"

#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>


#include <EAF\EAFDisplayUnits.h>

// CConstructSegmentsDlg dialog

IMPLEMENT_DYNAMIC(CConstructSegmentsDlg, CDialog)

CConstructSegmentsDlg::CConstructSegmentsDlg(const CTimelineManager* pTimelineMgr,CWnd* pParent /*=NULL*/)
	: CDialog(CConstructSegmentsDlg::IDD, pParent),
   m_pTimelineMgr(pTimelineMgr)
{
   m_pBridgeDesc = m_pTimelineMgr->GetBridgeDescription();
}

CConstructSegmentsDlg::~CConstructSegmentsDlg()
{
}

void CConstructSegmentsDlg::DoDataExchange(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

	CDialog::DoDataExchange(pDX);
   if ( pDX->m_bSaveAndValidate )
   {
      m_ConstructSegments.Enable(true);

      Float64 relax, age;
      DDX_Text(pDX,IDC_RELAXATION_TIME,relax);
      DDX_Text(pDX,IDC_AGE,age);

      if ( relax < age )
      {
         pDX->PrepareEditCtrl(IDC_AGE);
         AfxMessageBox(_T("Concrete age at release must be less than or equal to the time between strand stressing and release"),MB_ICONWARNING);
         pDX->Fail();
      }


      m_ConstructSegments.SetRelaxationTime(relax);
      m_ConstructSegments.SetAgeAtRelease(age);
   }
   else
   {
      Float64 relax = m_ConstructSegments.GetRelaxationTime();
      Float64 age   = m_ConstructSegments.GetAgeAtRelease();
      DDX_Text(pDX,IDC_RELAXATION_TIME,relax);
      DDX_Text(pDX,IDC_AGE,age);
   }
}


BEGIN_MESSAGE_MAP(CConstructSegmentsDlg, CDialog)
END_MESSAGE_MAP()


// CConstructSegmentsDlg message handlers

BOOL CConstructSegmentsDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}
