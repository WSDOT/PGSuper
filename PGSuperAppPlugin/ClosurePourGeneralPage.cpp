// ClosurePourGeneralPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin.h"
#include "ClosurePourGeneralPage.h"
#include "ClosurePourDlg.h"
#include "TimelineEventDlg.h"


#include "PGSuperUnits.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\SplicedGirderData.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>

#include <System\Tokenizer.h>
#include <Material\Material.h>

#include "ConcreteDetailsDlg.h"

#include "HtmlHelp\HelpTopics.hh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CClosurePourGeneralPage dialog

IMPLEMENT_DYNAMIC(CClosurePourGeneralPage, CPropertyPage)

CClosurePourGeneralPage::CClosurePourGeneralPage()
	: CPropertyPage(CClosurePourGeneralPage::IDD)
{

}

CClosurePourGeneralPage::~CClosurePourGeneralPage()
{
}

void CClosurePourGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EC,      m_ctrlEc);
	DDX_Control(pDX, IDC_ECI,     m_ctrlEci);
	DDX_Control(pDX, IDC_USER_EC,  m_ctrlEcCheck);
	DDX_Control(pDX, IDC_USER_ECI, m_ctrlEciCheck);
   DDX_Control(pDX, IDC_FC, m_ctrlFc);
	DDX_Control(pDX, IDC_FCI, m_ctrlFci);

   CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();

   DDX_CBItemData(pDX,IDC_EVENT,pParent->m_EventIdx);

   ExchangeConcreteData(pDX);

   if ( pDX->m_bSaveAndValidate && m_ctrlEcCheck.GetCheck() == 1 )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }

   if ( pDX->m_bSaveAndValidate && m_ctrlEciCheck.GetCheck() == 1 )
   {
      m_ctrlEci.GetWindowText(m_strUserEci);
   }
}


BEGIN_MESSAGE_MAP(CClosurePourGeneralPage, CPropertyPage)
	ON_BN_CLICKED(IDC_MORE_PROPERTIES, OnMoreConcreteProperties)
   ON_BN_CLICKED(IDC_USER_ECI, OnUserEci)
   ON_BN_CLICKED(IDC_USER_EC, OnUserEc)
	ON_COMMAND(ID_HELP, OnHelp)
	ON_EN_CHANGE(IDC_FCI, OnChangeFci)
	ON_EN_CHANGE(IDC_FC, OnChangeFc)
	ON_EN_CHANGE(IDC_ECI, OnChangeEci)
	ON_EN_CHANGE(IDC_EC, OnChangeEc)
   ON_NOTIFY_EX(TTN_NEEDTEXT,0,OnToolTipNotify)
   ON_CBN_SELCHANGE(IDC_EVENT, OnEventChanged)
   ON_CBN_DROPDOWN(IDC_EVENT, OnEventChanging)
   ON_BN_CLICKED(IDC_FC1, OnConcreteStrength)
   ON_BN_CLICKED(IDC_FC2, OnConcreteStrength)
END_MESSAGE_MAP()


// CClosurePourGeneralPage message handlers

BOOL CClosurePourGeneralPage::OnInitDialog()
{
   FillEventList();

   CPropertyPage::OnInitDialog();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();
   const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();

   GroupIndexType  grpIdx  = pParent->m_ClosureKey.groupIndex;
   GirderIndexType gdrIdx  = pParent->m_ClosureKey.girderIndex;
   SegmentIndexType segIdx = pParent->m_ClosureKey.segmentIndex;

   const CClosurePourData* pClosurePour = pBridgeDesc->GetGirderGroup(grpIdx)->GetGirder(gdrIdx)->GetClosurePour(segIdx);

   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();
   EventIndexType eventIdx = pTimelineMgr->GetCastClosurePourEventIndex(pClosurePour->GetID());
   m_AgeAtContinuity = pTimelineMgr->GetEventByIndex(eventIdx)->GetCastClosurePourActivity().GetConcreteAgeAtContinuity();

   if ( m_strUserEc == _T("") )
      m_ctrlEc.GetWindowText(m_strUserEc);
	
   if ( m_strUserEci == _T("") )
      m_ctrlEci.GetWindowText(m_strUserEci);

   OnUserEci();
   OnUserEc();
   OnConcreteStrength();

   EnableToolTips(TRUE);


   // get the real closure pour so we can access the temporary support
   if ( pClosurePour->GetTemporarySupport() )
   {
      Float64 station = pClosurePour->GetTemporarySupport()->GetStation();
      CString strDescription;
      strDescription.Format(_T("Closure pour for Girderline %s at Temporary Support %d (%s)"),
         LABEL_GIRDER(gdrIdx),
         LABEL_PIER(pClosurePour->GetTemporarySupport()->GetIndex()),
         FormatStation(pDisplayUnits->GetStationFormat(),station)
         );
      GetDlgItem(IDC_LOCATION)->SetWindowText(strDescription);
   }
   else
   {
      Float64 station = pClosurePour->GetPier()->GetStation();
      CString strDescription;
      strDescription.Format(_T("Closure pour for Girderline %s at Pier %d (%s)"),
         LABEL_GIRDER(gdrIdx),
         LABEL_PIER(pClosurePour->GetPier()->GetIndex()),
         FormatStation(pDisplayUnits->GetStationFormat(),station)
         );
      GetDlgItem(IDC_LOCATION)->SetWindowText(strDescription);
   }

   GET_IFACE2(pBroker,IClosurePour,pIClosurePour);
   CSegmentKey closureKey = pParent->m_ClosureKey;
   closureKey.segmentIndex = pClosurePour->GetIndex();
   Float64 length = pIClosurePour->GetClosurePourLength(closureKey);
   CString strLength;
   strLength.Format(_T("Length: %s"),FormatDimension(length,pDisplayUnits->GetXSectionDimUnit()));
   GetDlgItem(IDC_LENGTH)->SetWindowText(strLength);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CClosurePourGeneralPage::ExchangeConcreteData(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();

   int value;
   if ( !pDX->m_bSaveAndValidate )
      value = pParent->m_ClosurePour.GetConcrete().bBasePropertiesOnInitialValues ? 0 : 1;
   DDX_Radio(pDX,IDC_FC1,value);
   if ( pDX->m_bSaveAndValidate )
      pParent->m_ClosurePour.GetConcrete().bBasePropertiesOnInitialValues = (value == 0 ? true : false);

   DDX_UnitValueAndTag(pDX,IDC_FCI,IDC_FCI_UNIT,pParent->m_ClosurePour.GetConcrete().Fci,pDisplayUnits->GetStressUnit());
   DDX_UnitValueAndTag(pDX,IDC_FC,IDC_FC_UNIT,pParent->m_ClosurePour.GetConcrete().Fc,pDisplayUnits->GetStressUnit());
   DDV_UnitValueLimitOrLess( pDX, IDC_FCI, pParent->m_ClosurePour.GetConcrete().Fci,  pParent->m_ClosurePour.GetConcrete().Fc, pDisplayUnits->GetStressUnit() );

   DDX_Check_Bool(pDX, IDC_USER_ECI, pParent->m_ClosurePour.GetConcrete().bUserEci);
   DDX_UnitValueAndTag( pDX, IDC_ECI,  IDC_ECI_UNIT,   pParent->m_ClosurePour.GetConcrete().Eci , pDisplayUnits->GetModEUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_ECI,pParent->m_ClosurePour.GetConcrete().Eci, pDisplayUnits->GetModEUnit() );

   DDX_Check_Bool(pDX, IDC_USER_EC,  pParent->m_ClosurePour.GetConcrete().bUserEc);
   DDX_UnitValueAndTag( pDX, IDC_EC,  IDC_EC_UNIT, pParent->m_ClosurePour.GetConcrete().Ec , pDisplayUnits->GetModEUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_EC, pParent->m_ClosurePour.GetConcrete().Ec, pDisplayUnits->GetModEUnit() );

   if ( !pDX->m_bSaveAndValidate )
   {
      GetDlgItem(IDC_CONCRETE_TYPE_LABEL)->SetWindowText( matConcrete::GetTypeName((matConcrete::Type)pParent->m_ClosurePour.GetConcrete().Type,true).c_str() );
   }
}

void CClosurePourGeneralPage::OnMoreConcreteProperties() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CConcreteDetailsDlg dlg;

   CDataExchange dx(this,TRUE);
   ExchangeConcreteData(&dx);

   CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();

   dlg.m_General.m_Type        = pParent->m_ClosurePour.GetConcrete().Type;
   dlg.m_General.m_Fc          = pParent->m_ClosurePour.GetConcrete().Fc;
   dlg.m_General.m_AggSize     = pParent->m_ClosurePour.GetConcrete().MaxAggregateSize;
   dlg.m_General.m_bUserEc     = pParent->m_ClosurePour.GetConcrete().bUserEc;
   dlg.m_General.m_Ds          = pParent->m_ClosurePour.GetConcrete().StrengthDensity;
   dlg.m_General.m_Dw          = pParent->m_ClosurePour.GetConcrete().WeightDensity;
   dlg.m_General.m_Ec          = pParent->m_ClosurePour.GetConcrete().Ec;

   dlg.m_AASHTO.m_EccK1       = pParent->m_ClosurePour.GetConcrete().EcK1;
   dlg.m_AASHTO.m_EccK2       = pParent->m_ClosurePour.GetConcrete().EcK2;
   dlg.m_AASHTO.m_CreepK1     = pParent->m_ClosurePour.GetConcrete().CreepK1;
   dlg.m_AASHTO.m_CreepK2     = pParent->m_ClosurePour.GetConcrete().CreepK2;
   dlg.m_AASHTO.m_ShrinkageK1 = pParent->m_ClosurePour.GetConcrete().ShrinkageK1;
   dlg.m_AASHTO.m_ShrinkageK2 = pParent->m_ClosurePour.GetConcrete().ShrinkageK2;
   dlg.m_AASHTO.m_bHasFct     = pParent->m_ClosurePour.GetConcrete().bHasFct;
   dlg.m_AASHTO.m_Fct         = pParent->m_ClosurePour.GetConcrete().Fct;

   dlg.m_ACI.m_bUserParameters = pParent->m_ClosurePour.GetConcrete().bACIUserParameters;
   dlg.m_ACI.m_A               = pParent->m_ClosurePour.GetConcrete().A;
   dlg.m_ACI.m_B               = pParent->m_ClosurePour.GetConcrete().B;
   dlg.m_ACI.m_CureMethod      = pParent->m_ClosurePour.GetConcrete().CureMethod;
   dlg.m_ACI.m_CementType      = pParent->m_ClosurePour.GetConcrete().CementType;

#pragma Reminder("UPDATE: deal with CEB-FIP concrete models")

   dlg.m_General.m_strUserEc  = m_strUserEc;

   if ( dlg.DoModal() == IDOK )
   {
      pParent->m_ClosurePour.GetConcrete().Type             = dlg.m_General.m_Type;
      pParent->m_ClosurePour.GetConcrete().Fc               = dlg.m_General.m_Fc;
      pParent->m_ClosurePour.GetConcrete().MaxAggregateSize = dlg.m_General.m_AggSize;
      pParent->m_ClosurePour.GetConcrete().bUserEc          = dlg.m_General.m_bUserEc;
      pParent->m_ClosurePour.GetConcrete().StrengthDensity  = dlg.m_General.m_Ds;
      pParent->m_ClosurePour.GetConcrete().WeightDensity    = dlg.m_General.m_Dw;
      pParent->m_ClosurePour.GetConcrete().Ec               = dlg.m_General.m_Ec;

      pParent->m_ClosurePour.GetConcrete().EcK1             = dlg.m_AASHTO.m_EccK1;
      pParent->m_ClosurePour.GetConcrete().EcK2             = dlg.m_AASHTO.m_EccK2;
      pParent->m_ClosurePour.GetConcrete().CreepK1          = dlg.m_AASHTO.m_CreepK1;
      pParent->m_ClosurePour.GetConcrete().CreepK2          = dlg.m_AASHTO.m_CreepK2;
      pParent->m_ClosurePour.GetConcrete().ShrinkageK1      = dlg.m_AASHTO.m_ShrinkageK1;
      pParent->m_ClosurePour.GetConcrete().ShrinkageK2      = dlg.m_AASHTO.m_ShrinkageK2;
      pParent->m_ClosurePour.GetConcrete().bHasFct          = dlg.m_AASHTO.m_bHasFct;
      pParent->m_ClosurePour.GetConcrete().Fct              = dlg.m_AASHTO.m_Fct;

      pParent->m_ClosurePour.GetConcrete().bACIUserParameters = dlg.m_ACI.m_bUserParameters;
      pParent->m_ClosurePour.GetConcrete().A                  = dlg.m_ACI.m_A;
      pParent->m_ClosurePour.GetConcrete().B                  = dlg.m_ACI.m_B;
      pParent->m_ClosurePour.GetConcrete().CureMethod         = dlg.m_ACI.m_CureMethod;
      pParent->m_ClosurePour.GetConcrete().CementType         = dlg.m_ACI.m_CementType;

#pragma Reminder("UPDATE: deal with CEB-FIP concrete models")

      m_strUserEc  = dlg.m_General.m_strUserEc;
      m_ctrlEc.SetWindowText(m_strUserEc);

      dx.m_bSaveAndValidate = FALSE;
      ExchangeConcreteData(&dx);

      UpdateFci();
      UpdateFc();
      UpdateEci();
      UpdateEc();

      UpdateConcreteControls();
   }
	
}

void CClosurePourGeneralPage::OnUserEci()
{
   BOOL bEnable = ((CButton*)GetDlgItem(IDC_USER_ECI))->GetCheck();
   GetDlgItem(IDC_ECI)->EnableWindow(bEnable);
   GetDlgItem(IDC_ECI_UNIT)->EnableWindow(bEnable);

   if (bEnable==FALSE)
   {
      m_ctrlEci.GetWindowText(m_strUserEci);
      UpdateEci();
   }
   else
   {
      m_ctrlEci.SetWindowText(m_strUserEci);
   }

   UpdateEc();
}

void CClosurePourGeneralPage::OnUserEc()
{
   BOOL bEnable = ((CButton*)GetDlgItem(IDC_USER_EC))->GetCheck();
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);

   if (bEnable==FALSE)
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
      UpdateEc();
   }
   else
   {
      m_ctrlEc.SetWindowText(m_strUserEc); 
   }

   UpdateEci();
}

void CClosurePourGeneralPage::OnChangeFci() 
{
   UpdateEci();
   UpdateFc();
   UpdateEc();
}

void CClosurePourGeneralPage::OnChangeFc() 
{
   UpdateEc();
   UpdateFci();
   UpdateEci();
}

void CClosurePourGeneralPage::OnChangeEci()
{
   if (m_ctrlEciCheck.GetCheck() == TRUE) // checked
      UpdateEc();
}

void CClosurePourGeneralPage::OnChangeEc()
{
   if (m_ctrlEcCheck.GetCheck() == TRUE) // checked
      UpdateEci();
}

void CClosurePourGeneralPage::UpdateEci()
{
   // update modulus
   int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
   int method = 0;

   if ( i == IDC_FC2 )
   {
      // concrete model is based on f'c
      if ( m_ctrlEcCheck.GetCheck() == TRUE )
      {
         // Ec box is checked... user has input a value for Ec
         // Eci is based on user value for Ec not f'ci
         method = 0;
      }
      else
      {
         // Ec box is not checked... Ec is computed from f'c, compute Eci from f'ci
         method = 1;
      }
   }
   else
   {
      if ( m_ctrlEciCheck.GetCheck() == FALSE )// not checked
      {
         method = 1;
      }
      else
      {
         method = -1; // don't compute... it has user input
      }
   }

   if ( method == 0 )
   {
      // Eci is based on the user input value of Ec and not f'ci
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CString strEc;
      m_ctrlEc.GetWindowText(strEc);
      Float64 Ec;
      sysTokenizer::ParseDouble(strEc,&Ec);
      Ec = ::ConvertToSysUnits(Ec,pDisplayUnits->GetModEUnit().UnitOfMeasure);

      CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();

      matACI209Concrete concrete;
      concrete.UserEc28(true);
      concrete.SetEc28(Ec);
      concrete.SetA(pParent->m_ClosurePour.GetConcrete().A);
      concrete.SetBeta(pParent->m_ClosurePour.GetConcrete().B);
      concrete.SetTimeAtCasting(0);
      concrete.SetFc28(pParent->m_ClosurePour.GetConcrete().Fc);
      concrete.SetStrengthDensity(pParent->m_ClosurePour.GetConcrete().StrengthDensity);
      Float64 Eci = concrete.GetEc(m_AgeAtContinuity);

      CString strEci;
      strEci.Format(_T("%s"),FormatDimension(Eci,pDisplayUnits->GetModEUnit(),false));
      m_ctrlEci.SetWindowText(strEci);
   }
   else if ( method == 1 )
   {
      CString strFci, strDensity, strK1, strK2;
      m_ctrlFci.GetWindowText(strFci);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();

      strDensity.Format(_T("%s"),FormatDimension(pParent->m_ClosurePour.GetConcrete().StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),pParent->m_ClosurePour.GetConcrete().EcK1);
      strK2.Format(_T("%f"),pParent->m_ClosurePour.GetConcrete().EcK2);

      CString strEci = CConcreteDetailsDlg::UpdateEc(strFci,strDensity,strK1,strK2);
      m_ctrlEci.SetWindowText(strEci);
   }
}

void CClosurePourGeneralPage::UpdateEc()
{
   // update modulus
   int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
   int method = 0;

   if ( i == IDC_FC1 )
   {
      // concrete model is based on f'ci
      if ( m_ctrlEciCheck.GetCheck() == TRUE )
      {
         // Eci box is checked... user has input a value for Eci
         // Ec is based on the user input value of Eci and not f'c
         method = 0;
      }
      else
      {
         // Eci box is not checked... Eci is computed from f'ci, compute Ec from f'c
         method = 1;
      }
   }
   else
   {
      if (m_ctrlEcCheck.GetCheck() == FALSE) // not checked
      {
         method = 1;
      }
      else
      {
         method = -1; // don't compute.... it is user input
      }
   }

   if ( method == 0 )
   {
      // Ec is based on the user input value of Eci and not f'c
      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CString strEci;
      m_ctrlEci.GetWindowText(strEci);
      Float64 Eci;
      sysTokenizer::ParseDouble(strEci,&Eci);
      Eci = ::ConvertToSysUnits(Eci,pDisplayUnits->GetModEUnit().UnitOfMeasure);

      CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();

      Float64 Ec = matACI209Concrete::ComputeEc28(Eci,m_AgeAtContinuity,pParent->m_ClosurePour.GetConcrete().A,pParent->m_ClosurePour.GetConcrete().B);

      CString strEc;
      strEc.Format(_T("%s"),FormatDimension(Ec,pDisplayUnits->GetModEUnit(),false));
      m_ctrlEc.SetWindowText(strEc);
   }
   else if ( method == 1 )
   {
      // Compute Ec based on f'c
      CString strFc, strDensity, strK1, strK2;
      m_ctrlFc.GetWindowText(strFc);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();

      strDensity.Format(_T("%s"),FormatDimension(pParent->m_ClosurePour.GetConcrete().StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),pParent->m_ClosurePour.GetConcrete().EcK1);
      strK2.Format(_T("%f"),pParent->m_ClosurePour.GetConcrete().EcK2);

      CString strEc = CConcreteDetailsDlg::UpdateEc(strFc,strDensity,strK1,strK2);
      m_ctrlEc.SetWindowText(strEc);
   }
}

void CClosurePourGeneralPage::UpdateFc()
{
   int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
   if ( i == IDC_FC1 )
   {
      // concrete model is based on f'ci... compute f'c
#pragma Reminder("UPDATE: assuming ACI209 concrete model")
      // Get f'ci from edit control
      CString strFci;
      m_ctrlFci.GetWindowText(strFci);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      Float64 fci;
      sysTokenizer::ParseDouble(strFci, &fci);
      fci = ::ConvertToSysUnits(fci,pDisplayUnits->GetStressUnit().UnitOfMeasure);

      CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();
      Float64 fc = matACI209Concrete::ComputeFc28(fci,m_AgeAtContinuity,pParent->m_ClosurePour.GetConcrete().A,pParent->m_ClosurePour.GetConcrete().B);

      CString strFc;
      strFc.Format(_T("%s"),FormatDimension(fc,pDisplayUnits->GetStressUnit(),false));
      m_ctrlFc.SetWindowText(strFc);
   }
}

void CClosurePourGeneralPage::UpdateFci()
{
   int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
   if ( i == IDC_FC2 )
   {
      // concrete model is based on f'ci... compute f'c
#pragma Reminder("UPDATE: assuming ACI209 concrete model")
      // Get f'c from edit control
      CString strFc;
      m_ctrlFc.GetWindowText(strFc);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      Float64 fc;
      sysTokenizer::ParseDouble(strFc, &fc);
      fc = ::ConvertToSysUnits(fc,pDisplayUnits->GetStressUnit().UnitOfMeasure);

      CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();

      matACI209Concrete concrete;
      concrete.SetTimeAtCasting(0);
      concrete.SetFc28(fc);
      concrete.SetA(pParent->m_ClosurePour.GetConcrete().A);
      concrete.SetBeta(pParent->m_ClosurePour.GetConcrete().B);
      concrete.SetStrengthDensity(pParent->m_ClosurePour.GetConcrete().StrengthDensity);
      Float64 fci = concrete.GetFc(m_AgeAtContinuity);

      CString strFci;
      strFci.Format(_T("%s"),FormatDimension(fci,pDisplayUnits->GetStressUnit(),false));
      m_ctrlFci.SetWindowText(strFci);
   }
}

void CClosurePourGeneralPage::UpdateConcreteControls()
{
   int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
   INT idFci[5] = {IDC_FCI, IDC_FCI_UNIT, IDC_USER_ECI, IDC_ECI, IDC_ECI_UNIT};
   INT idFc[5]  = {IDC_FC,  IDC_FC_UNIT,  IDC_USER_EC,  IDC_EC,  IDC_EC_UNIT };

   BOOL bEnableFci = (i == IDC_FC1);

   for ( int j = 0; j < 5; j++ )
   {
      GetDlgItem(idFci[j])->EnableWindow(  bEnableFci );
      GetDlgItem(idFc[j] )->EnableWindow( !bEnableFci );
   }

   if ( i == IDC_FC1 ) // input based on f'ci
   {
      m_ctrlEciCheck.SetCheck(m_ctrlEcCheck.GetCheck());
      m_ctrlEcCheck.SetCheck(FALSE); // can't check Ec
   }

   if ( i == IDC_FC2 ) // input is based on f'ci
   {
      m_ctrlEcCheck.SetCheck(m_ctrlEciCheck.GetCheck());
      m_ctrlEciCheck.SetCheck(FALSE); // can't check Eci
   }

   BOOL bEnable = m_ctrlEcCheck.GetCheck();
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);

   bEnable = m_ctrlEciCheck.GetCheck();
   GetDlgItem(IDC_ECI)->EnableWindow(bEnable);
   GetDlgItem(IDC_ECI_UNIT)->EnableWindow(bEnable);
}


BOOL CClosurePourGeneralPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
{
   TOOLTIPTEXT* pTTT = (TOOLTIPTEXT*)pNMHDR;
   HWND hwndTool = (HWND)pNMHDR->idFrom;
   if ( pTTT->uFlags & TTF_IDISHWND )
   {
      // idFrom is actually HWND of tool
      UINT nID = ::GetDlgCtrlID(hwndTool);
      switch(nID)
      {
      case IDC_MORE_PROPERTIES:
         UpdateConcreteParametersToolTip();
         break;

      default:
         return FALSE;
      }

      ::SendMessage(pNMHDR->hwndFrom,TTM_SETDELAYTIME,TTDT_AUTOPOP,TOOLTIP_DURATION); // sets the display time to 10 seconds
      ::SendMessage(pNMHDR->hwndFrom,TTM_SETMAXTIPWIDTH,0,TOOLTIP_WIDTH); // makes it a multi-line tooltip
      pTTT->lpszText = m_strTip.GetBuffer();
      pTTT->hinst = NULL;
      return TRUE;
   }
   return FALSE;
}

void CClosurePourGeneralPage::UpdateConcreteParametersToolTip()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();

   const unitmgtDensityData& density = pDisplayUnits->GetDensityUnit();
   const unitmgtLengthData&  aggsize = pDisplayUnits->GetComponentDimUnit();
   const unitmgtStressData&  stress  = pDisplayUnits->GetStressUnit();
   const unitmgtScalar&      scalar  = pDisplayUnits->GetScalarFormat();

#pragma Reminder("UPDATE: add ACI and CEB-FIP parameters to the tooltip text")

   CString strTip;
   strTip.Format(_T("%-20s %s\r\n%-20s %s\r\n%-20s %s\r\n%-20s %s"),
      _T("Type"), matConcrete::GetTypeName((matConcrete::Type)pParent->m_ClosurePour.GetConcrete().Type,true).c_str(),
      _T("Unit Weight"),FormatDimension(pParent->m_ClosurePour.GetConcrete().StrengthDensity,density),
      _T("Unit Weight (w/ reinforcement)"),  FormatDimension(pParent->m_ClosurePour.GetConcrete().WeightDensity,density),
      _T("Max Aggregate Size"),  FormatDimension(pParent->m_ClosurePour.GetConcrete().MaxAggregateSize,aggsize)
      );

   //if ( lrfdVersionMgr::ThirdEditionWith2005Interims <= lrfdVersionMgr::GetVersion() )
   //{
   //   // add K1 parameter
   //   CString strK1;
   //   strK1.Format(_T("\r\n%-20s %s"),
   //      _T("K1"),FormatScalar(pParent->m_ClosurePour.K1,scalar));

   //   strTip += strK1;
   //}

   if ( pParent->m_ClosurePour.GetConcrete().Type != pgsTypes::Normal && pParent->m_ClosurePour.GetConcrete().bHasFct )
   {
      CString strLWC;
      strLWC.Format(_T("\r\n%-20s %s"),
         _T("fct"),FormatDimension(pParent->m_ClosurePour.GetConcrete().Fct,stress));

      strTip += strLWC;
   }

   CString strPress(_T("\r\n\r\nPress button to edit"));
   strTip += strPress;

   m_strTip = strTip;
}

void CClosurePourGeneralPage::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_SHEARDESC );
}

void CClosurePourGeneralPage::FillEventList()
{
   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

   int selEventIdx = pcbEvent->GetCurSel();

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

   if ( selEventIdx != CB_ERR )
      pcbEvent->SetCurSel(selEventIdx);
}

void CClosurePourGeneralPage::OnEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   m_PrevEventIdx = pCB->GetCurSel();
}

void CClosurePourGeneralPage::OnEventChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   int curSel = pCB->GetCurSel();
   EventIndexType idx = (EventIndexType)pCB->GetItemData(curSel);
   if ( idx == CREATE_TIMELINE_EVENT )
   {
      CClosurePourDlg* pParent = (CClosurePourDlg*)GetParent();

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);

      idx = CreateEvent();
      if ( idx != INVALID_INDEX )
      {
         pIBridgeDesc->SetCastClosurePourEventByIndex(pParent->m_ClosureKey.groupIndex,pParent->m_ClosureKey.segmentIndex,idx);
#pragma Reminder("UPDATE") // need to update concrete time parameter based on settings for new event
         FillEventList();

         pCB->SetCurSel((int)idx);
      }
      else
      {
         pCB->SetCurSel(m_PrevEventIdx);
      }
   }
}

EventIndexType CClosurePourGeneralPage::CreateEvent()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IBridgeDescription,pIBridgeDesc);
   const CTimelineManager* pTimelineMgr = pIBridgeDesc->GetTimelineManager();

   CTimelineEventDlg dlg(pTimelineMgr,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      return pIBridgeDesc->AddTimelineEvent(dlg.m_TimelineEvent);
  }

   return INVALID_INDEX;
}

void CClosurePourGeneralPage::OnConcreteStrength()
{
   UpdateConcreteControls();
}
