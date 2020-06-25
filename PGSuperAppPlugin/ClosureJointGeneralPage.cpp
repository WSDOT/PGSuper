// ClosureJointGeneralPage.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperAppPlugin.h"
#include "ClosureJointGeneralPage.h"
#include "ClosureJointDlg.h"
#include "TimelineEventDlg.h"


#include "PGSuperUnits.h"

#include <IFace\Project.h>
#include <IFace\Bridge.h>

#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\SplicedGirderData.h>

#include <EAF\EAFUtilities.h>
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>

#include <System\Tokenizer.h>
#include <Material\Material.h>

#include <PgsExt\ConcreteDetailsDlg.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// CClosureJointGeneralPage dialog

IMPLEMENT_DYNAMIC(CClosureJointGeneralPage, CPropertyPage)

CClosureJointGeneralPage::CClosureJointGeneralPage()
	: CPropertyPage(CClosureJointGeneralPage::IDD)
{
   m_bWasEventCreated = false;
}

CClosureJointGeneralPage::~CClosureJointGeneralPage()
{
}

void CClosureJointGeneralPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_EC,      m_ctrlEc);
	DDX_Control(pDX, IDC_ECI,     m_ctrlEci);
	DDX_Control(pDX, IDC_USER_EC,  m_ctrlEcCheck);
	DDX_Control(pDX, IDC_USER_ECI, m_ctrlEciCheck);
   DDX_Control(pDX, IDC_FC, m_ctrlFc);
	DDX_Control(pDX, IDC_FCI, m_ctrlFci);

   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();

   ExchangeConcreteData(pDX);

   if ( pDX->m_bSaveAndValidate && m_ctrlEcCheck.GetCheck() == 1 )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }

   if ( pDX->m_bSaveAndValidate && m_ctrlEciCheck.GetCheck() == 1 )
   {
      m_ctrlEci.GetWindowText(m_strUserEci);
   }

   if (pDX->m_bSaveAndValidate)
   {
      int result = pParent->m_TimelineMgr.Validate();
      if (result != TLM_SUCCESS)
      {
         auto strError = pParent->m_TimelineMgr.GetErrorMessage(result);
         CString strMsg;
         strMsg.Format(_T("%s"), strError.c_str());
         AfxMessageBox(strMsg, MB_OK | MB_ICONERROR);
         pDX->PrepareCtrl(IDC_EVENT);
         pDX->Fail();
      }
   }
}


BEGIN_MESSAGE_MAP(CClosureJointGeneralPage, CPropertyPage)
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


// CClosureJointGeneralPage message handlers

BOOL CClosureJointGeneralPage::OnInitDialog()
{
   FillEventList();

   CPropertyPage::OnInitDialog();

   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   GET_IFACE2(pBroker,ISpecification,pSpec);
   std::_tstring strSpecName = pSpec->GetSpecification();

   GET_IFACE2(pBroker,ILibrary,pLib);
   const SpecLibraryEntry* pSpecEntry = pLib->GetSpecEntry( strSpecName.c_str() );
   m_LossMethod = pSpecEntry->GetLossMethod();
   m_TimeDependentModel = pSpecEntry->GetTimeDependentModel();

   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();

   GroupIndexType  grpIdx  = pParent->m_ClosureKey.groupIndex;
   GirderIndexType gdrIdx  = pParent->m_ClosureKey.girderIndex;
   SegmentIndexType segIdx = pParent->m_ClosureKey.segmentIndex;

   // initialize the event combobox
   EventIndexType eventIdx = pParent->m_TimelineMgr.GetCastClosureJointEventIndex(pParent->m_ClosureID);
   CDataExchange dx(this,FALSE);
   DDX_CBItemData(&dx,IDC_EVENT,eventIdx);

   m_AgeAtContinuity = pParent->m_TimelineMgr.GetEventByIndex(eventIdx)->GetCastClosureJointActivity().GetConcreteAgeAtContinuity();

   if ( m_strUserEc == _T("") )
   {
      m_ctrlEc.GetWindowText(m_strUserEc);
   }
	
   if ( m_strUserEci == _T("") )
   {
      m_ctrlEci.GetWindowText(m_strUserEci);
   }

   if ( pParent->m_ClosureJoint.GetConcrete().bBasePropertiesOnInitialValues )
   {
      OnChangeFci();
   }
   else
   {
      OnChangeFc();
   }
   OnConcreteStrength();

   EnableToolTips(TRUE);


   // get the real closure joint so we can access the temporary support
   const CClosureJointData* pClosure = pParent->m_pBridgeDesc->GetClosureJoint(pParent->m_ClosureKey);
   if ( pClosure->GetTemporarySupport() )
   {
      Float64 station = pClosure->GetTemporarySupport()->GetStation();
      CString strDescription;
      strDescription.Format(_T("Location: Closure Joint for Group %d Girder %s at Temporary Support %d (%s)"),
         LABEL_GROUP(grpIdx),
         LABEL_GIRDER(gdrIdx),
         LABEL_TEMPORARY_SUPPORT(pClosure->GetTemporarySupport()->GetIndex()),
         FormatStation(pDisplayUnits->GetStationFormat(),station)
         );
      GetDlgItem(IDC_LOCATION)->SetWindowText(strDescription);

      GetDlgItem(IDC_NOTE)->SetWindowText(_T("NOTE: Changes to the Installation Event apply to all closure joints at this temporary support."));
   }
   else
   {
      Float64 station = pClosure->GetPier()->GetStation();
      CString strDescription;
      strDescription.Format(_T("Location: Closure Joint for Group %d Girder %s at Pier %s (%s)"),
         LABEL_GROUP(grpIdx),
         LABEL_GIRDER(gdrIdx),
         LABEL_PIER(pClosure->GetPier()->GetIndex()),
         FormatStation(pDisplayUnits->GetStationFormat(),station)
         );
      GetDlgItem(IDC_LOCATION)->SetWindowText(strDescription);

      GetDlgItem(IDC_NOTE)->SetWindowText(_T("NOTE: Changes to the Installation Event apply to all closure joints at this pier."));
   }

   GET_IFACE2(pBroker,IBridge,pBridge);
   Float64 length = pBridge->GetClosureJointLength(pParent->m_ClosureKey);
   CString strLength;
   strLength.Format(_T("Length: %s"),FormatDimension(length,pDisplayUnits->GetXSectionDimUnit()));
   GetDlgItem(IDC_LENGTH)->SetWindowText(strLength);

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CClosureJointGeneralPage::ExchangeConcreteData(CDataExchange* pDX)
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);
   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();

   int value;
   if ( !pDX->m_bSaveAndValidate )
   {
      value = pParent->m_ClosureJoint.GetConcrete().bBasePropertiesOnInitialValues ? 0 : 1;
   }
   DDX_Radio(pDX,IDC_FC1,value);
   if ( pDX->m_bSaveAndValidate )
   {
      pParent->m_ClosureJoint.GetConcrete().bBasePropertiesOnInitialValues = (value == 0 ? true : false);
   }

   DDX_UnitValueAndTag(pDX,IDC_FCI,IDC_FCI_UNIT,pParent->m_ClosureJoint.GetConcrete().Fci,pDisplayUnits->GetStressUnit());
   DDX_UnitValueAndTag(pDX,IDC_FC,IDC_FC_UNIT,pParent->m_ClosureJoint.GetConcrete().Fc,pDisplayUnits->GetStressUnit());
   DDV_UnitValueLimitOrLess( pDX, IDC_FCI, pParent->m_ClosureJoint.GetConcrete().Fci,  pParent->m_ClosureJoint.GetConcrete().Fc, pDisplayUnits->GetStressUnit() );

   DDX_Check_Bool(pDX, IDC_USER_ECI, pParent->m_ClosureJoint.GetConcrete().bUserEci);
   DDX_UnitValueAndTag( pDX, IDC_ECI,  IDC_ECI_UNIT,   pParent->m_ClosureJoint.GetConcrete().Eci , pDisplayUnits->GetModEUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_ECI,pParent->m_ClosureJoint.GetConcrete().Eci, pDisplayUnits->GetModEUnit() );

   DDX_Check_Bool(pDX, IDC_USER_EC,  pParent->m_ClosureJoint.GetConcrete().bUserEc);
   DDX_UnitValueAndTag( pDX, IDC_EC,  IDC_EC_UNIT, pParent->m_ClosureJoint.GetConcrete().Ec , pDisplayUnits->GetModEUnit() );
   DDV_UnitValueGreaterThanZero( pDX, IDC_EC, pParent->m_ClosureJoint.GetConcrete().Ec, pDisplayUnits->GetModEUnit() );
}

void CClosureJointGeneralPage::OnMoreConcreteProperties() 
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
   bool bFinalProperties = (i == IDC_FC2 ? true : false);

   CConcreteDetailsDlg dlg(bFinalProperties);

   CDataExchange dx(this,TRUE);
   ExchangeConcreteData(&dx);

   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();

   dlg.m_fci  = pParent->m_ClosureJoint.GetConcrete().Fci;
   dlg.m_fc28 = pParent->m_ClosureJoint.GetConcrete().Fc;
   dlg.m_Eci  = pParent->m_ClosureJoint.GetConcrete().Eci;
   dlg.m_Ec28 = pParent->m_ClosureJoint.GetConcrete().Ec;
   dlg.m_bUserEci     = pParent->m_ClosureJoint.GetConcrete().bUserEci;
   dlg.m_bUserEc28    = pParent->m_ClosureJoint.GetConcrete().bUserEc;
   dlg.m_TimeAtInitialStrength = ::ConvertToSysUnits(m_AgeAtContinuity,unitMeasure::Day);

   dlg.m_General.m_Type        = pParent->m_ClosureJoint.GetConcrete().Type;
   dlg.m_General.m_AggSize     = pParent->m_ClosureJoint.GetConcrete().MaxAggregateSize;
   dlg.m_General.m_Ds          = pParent->m_ClosureJoint.GetConcrete().StrengthDensity;
   dlg.m_General.m_Dw          = pParent->m_ClosureJoint.GetConcrete().WeightDensity;

   dlg.m_AASHTO.m_EccK1       = pParent->m_ClosureJoint.GetConcrete().EcK1;
   dlg.m_AASHTO.m_EccK2       = pParent->m_ClosureJoint.GetConcrete().EcK2;
   dlg.m_AASHTO.m_CreepK1     = pParent->m_ClosureJoint.GetConcrete().CreepK1;
   dlg.m_AASHTO.m_CreepK2     = pParent->m_ClosureJoint.GetConcrete().CreepK2;
   dlg.m_AASHTO.m_ShrinkageK1 = pParent->m_ClosureJoint.GetConcrete().ShrinkageK1;
   dlg.m_AASHTO.m_ShrinkageK2 = pParent->m_ClosureJoint.GetConcrete().ShrinkageK2;
   dlg.m_AASHTO.m_bHasFct     = pParent->m_ClosureJoint.GetConcrete().bHasFct;
   dlg.m_AASHTO.m_Fct         = pParent->m_ClosureJoint.GetConcrete().Fct;

   dlg.m_ACI.m_bUserParameters = pParent->m_ClosureJoint.GetConcrete().bACIUserParameters;
   dlg.m_ACI.m_A               = pParent->m_ClosureJoint.GetConcrete().A;
   dlg.m_ACI.m_B               = pParent->m_ClosureJoint.GetConcrete().B;
   dlg.m_ACI.m_CureMethod      = pParent->m_ClosureJoint.GetConcrete().CureMethod;
   dlg.m_ACI.m_CementType      = pParent->m_ClosureJoint.GetConcrete().ACI209CementType;

   dlg.m_CEBFIP.m_bUserParameters = pParent->m_ClosureJoint.GetConcrete().bCEBFIPUserParameters;
   dlg.m_CEBFIP.m_S               = pParent->m_ClosureJoint.GetConcrete().S;
   dlg.m_CEBFIP.m_BetaSc          = pParent->m_ClosureJoint.GetConcrete().BetaSc;
   dlg.m_CEBFIP.m_CementType      = pParent->m_ClosureJoint.GetConcrete().CEBFIPCementType;

   dlg.m_General.m_strUserEc  = m_strUserEc;

   if ( dlg.DoModal() == IDOK )
   {
      pParent->m_ClosureJoint.GetConcrete().Type             = dlg.m_General.m_Type;

      pParent->m_ClosureJoint.GetConcrete().Fci              = dlg.m_fci;
      pParent->m_ClosureJoint.GetConcrete().Fc               = dlg.m_fc28;
      pParent->m_ClosureJoint.GetConcrete().Eci              = dlg.m_Eci;
      pParent->m_ClosureJoint.GetConcrete().Ec               = dlg.m_Ec28;
      pParent->m_ClosureJoint.GetConcrete().bUserEci         = dlg.m_bUserEci;
      pParent->m_ClosureJoint.GetConcrete().bUserEc          = dlg.m_bUserEc28;

      pParent->m_ClosureJoint.GetConcrete().MaxAggregateSize = dlg.m_General.m_AggSize;
      pParent->m_ClosureJoint.GetConcrete().StrengthDensity  = dlg.m_General.m_Ds;
      pParent->m_ClosureJoint.GetConcrete().WeightDensity    = dlg.m_General.m_Dw;

      pParent->m_ClosureJoint.GetConcrete().EcK1             = dlg.m_AASHTO.m_EccK1;
      pParent->m_ClosureJoint.GetConcrete().EcK2             = dlg.m_AASHTO.m_EccK2;
      pParent->m_ClosureJoint.GetConcrete().CreepK1          = dlg.m_AASHTO.m_CreepK1;
      pParent->m_ClosureJoint.GetConcrete().CreepK2          = dlg.m_AASHTO.m_CreepK2;
      pParent->m_ClosureJoint.GetConcrete().ShrinkageK1      = dlg.m_AASHTO.m_ShrinkageK1;
      pParent->m_ClosureJoint.GetConcrete().ShrinkageK2      = dlg.m_AASHTO.m_ShrinkageK2;
      pParent->m_ClosureJoint.GetConcrete().bHasFct          = dlg.m_AASHTO.m_bHasFct;
      pParent->m_ClosureJoint.GetConcrete().Fct              = dlg.m_AASHTO.m_Fct;

      pParent->m_ClosureJoint.GetConcrete().bACIUserParameters = dlg.m_ACI.m_bUserParameters;
      pParent->m_ClosureJoint.GetConcrete().A                  = dlg.m_ACI.m_A;
      pParent->m_ClosureJoint.GetConcrete().B                  = dlg.m_ACI.m_B;
      pParent->m_ClosureJoint.GetConcrete().CureMethod         = dlg.m_ACI.m_CureMethod;
      pParent->m_ClosureJoint.GetConcrete().ACI209CementType   = dlg.m_ACI.m_CementType;

      pParent->m_ClosureJoint.GetConcrete().bCEBFIPUserParameters = dlg.m_CEBFIP.m_bUserParameters;
      pParent->m_ClosureJoint.GetConcrete().S                     = dlg.m_CEBFIP.m_S;
      pParent->m_ClosureJoint.GetConcrete().BetaSc                = dlg.m_CEBFIP.m_BetaSc;
      pParent->m_ClosureJoint.GetConcrete().CEBFIPCementType      = dlg.m_CEBFIP.m_CementType;

      m_strUserEc  = dlg.m_General.m_strUserEc;
      m_ctrlEc.SetWindowText(m_strUserEc);

      dx.m_bSaveAndValidate = FALSE;
      ExchangeConcreteData(&dx);

      UpdateFci();
      UpdateFc();
      UpdateEci();
      UpdateEc();

      UpdateConcreteControls(true);
   }
	
}

void CClosureJointGeneralPage::OnUserEci()
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

void CClosureJointGeneralPage::OnUserEc()
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

void CClosureJointGeneralPage::OnChangeFci() 
{
   UpdateEci();
   UpdateFc();
   UpdateEc();
}

void CClosureJointGeneralPage::OnChangeFc() 
{
   UpdateEc();
   UpdateFci();
   UpdateEci();
}

void CClosureJointGeneralPage::OnChangeEci()
{
   if (m_ctrlEciCheck.GetCheck() == TRUE) // checked
   {
      UpdateEc();
   }
}

void CClosureJointGeneralPage::OnChangeEc()
{
   if (m_ctrlEcCheck.GetCheck() == TRUE) // checked
   {
      UpdateEci();
   }
}

void CClosureJointGeneralPage::UpdateEci()
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

      CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();

      Float64 Eci;
      if ( m_TimeDependentModel == TDM_AASHTO || m_TimeDependentModel == TDM_ACI209 )
      {
         matACI209Concrete concrete;
         concrete.UserEc28(true);
         concrete.SetEc28(Ec);
         concrete.SetA(pParent->m_ClosureJoint.GetConcrete().A);
         concrete.SetBeta(pParent->m_ClosureJoint.GetConcrete().B);
         concrete.SetTimeAtCasting(0);
         concrete.SetFc28(pParent->m_ClosureJoint.GetConcrete().Fc);
         concrete.SetStrengthDensity(pParent->m_ClosureJoint.GetConcrete().StrengthDensity);
         Eci = concrete.GetEc(m_AgeAtContinuity);
      }
      else
      {
         ATLASSERT(m_TimeDependentModel == TDM_CEBFIP);
         matCEBFIPConcrete concrete;
         concrete.UserEc28(true);
         concrete.SetEc28(Ec);
         concrete.SetTimeAtCasting(0);
         concrete.SetFc28(pParent->m_ClosureJoint.GetConcrete().Fc);
         concrete.SetStrengthDensity(pParent->m_ClosureJoint.GetConcrete().StrengthDensity);
         concrete.SetS(pParent->m_ClosureJoint.GetConcrete().S);
         concrete.SetBetaSc(pParent->m_ClosureJoint.GetConcrete().BetaSc);
         Eci = concrete.GetEc(m_AgeAtContinuity);
      }

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

      CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();

      strDensity.Format(_T("%s"),FormatDimension(pParent->m_ClosureJoint.GetConcrete().StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),pParent->m_ClosureJoint.GetConcrete().EcK1);
      strK2.Format(_T("%f"),pParent->m_ClosureJoint.GetConcrete().EcK2);

      CString strEci = CConcreteDetailsDlg::UpdateEc(pParent->m_ClosureJoint.GetConcrete().Type,strFci,strDensity,strK1,strK2);
      m_ctrlEci.SetWindowText(strEci);
   }
}

void CClosureJointGeneralPage::UpdateEc()
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

      CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();

      Float64 Ec;
      if ( m_TimeDependentModel == TDM_AASHTO || m_TimeDependentModel == TDM_ACI209 )
      {
         Ec = matACI209Concrete::ComputeEc28(Eci,m_AgeAtContinuity,pParent->m_ClosureJoint.GetConcrete().A,pParent->m_ClosureJoint.GetConcrete().B);
      }
      else
      {
         ATLASSERT( m_TimeDependentModel == TDM_CEBFIP );
         Ec = matCEBFIPConcrete::ComputeEc28(Eci,m_AgeAtContinuity,pParent->m_ClosureJoint.GetConcrete().S);
      }

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

      CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();

      strDensity.Format(_T("%s"),FormatDimension(pParent->m_ClosureJoint.GetConcrete().StrengthDensity,pDisplayUnits->GetDensityUnit(),false));
      strK1.Format(_T("%f"),pParent->m_ClosureJoint.GetConcrete().EcK1);
      strK2.Format(_T("%f"),pParent->m_ClosureJoint.GetConcrete().EcK2);

      CString strEc = CConcreteDetailsDlg::UpdateEc(pParent->m_ClosureJoint.GetConcrete().Type,strFc,strDensity,strK1,strK2);
      m_ctrlEc.SetWindowText(strEc);
   }
}

void CClosureJointGeneralPage::UpdateFc()
{
   int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
   if ( i == IDC_FC1 )
   {
      // concrete model is based on f'ci... compute f'c
      // Get f'ci from edit control
      CString strFci;
      m_ctrlFci.GetWindowText(strFci);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      Float64 fci;
      sysTokenizer::ParseDouble(strFci, &fci);
      fci = ::ConvertToSysUnits(fci,pDisplayUnits->GetStressUnit().UnitOfMeasure);

      CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();
      Float64 fc;

      if ( m_TimeDependentModel == TDM_AASHTO || m_TimeDependentModel == TDM_ACI209 )
      {
         fc = matACI209Concrete::ComputeFc28(fci,m_AgeAtContinuity,pParent->m_ClosureJoint.GetConcrete().A,pParent->m_ClosureJoint.GetConcrete().B);
      }
      else
      {
         ATLASSERT(m_TimeDependentModel == TDM_CEBFIP);
         fc = matCEBFIPConcrete::ComputeFc28(fci,m_AgeAtContinuity,pParent->m_ClosureJoint.GetConcrete().S);
      }

      CString strFc;
      strFc.Format(_T("%s"),FormatDimension(fc,pDisplayUnits->GetStressUnit(),false));
      m_ctrlFc.SetWindowText(strFc);
   }
}

void CClosureJointGeneralPage::UpdateFci()
{
   int i = GetCheckedRadioButton(IDC_FC1,IDC_FC2);
   if ( i == IDC_FC2 )
   {
      // concrete model is based on f'ci... compute f'c
      // Get f'c from edit control
      CString strFc;
      m_ctrlFc.GetWindowText(strFc);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

      Float64 fc;
      sysTokenizer::ParseDouble(strFc, &fc);
      fc = ::ConvertToSysUnits(fc,pDisplayUnits->GetStressUnit().UnitOfMeasure);

      CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();

      Float64 fci;
      if ( m_TimeDependentModel == TDM_AASHTO || m_TimeDependentModel == TDM_ACI209 )
      {
         matACI209Concrete concrete;
         concrete.SetTimeAtCasting(0);
         concrete.SetFc28(fc);
         concrete.SetA(pParent->m_ClosureJoint.GetConcrete().A);
         concrete.SetBeta(pParent->m_ClosureJoint.GetConcrete().B);
         fci = concrete.GetFc(m_AgeAtContinuity);
      }
      else
      {
         ATLASSERT(m_TimeDependentModel == TDM_CEBFIP);
         matCEBFIPConcrete concrete;
         concrete.SetTimeAtCasting(0);
         concrete.SetFc28(fc);
         concrete.SetS(pParent->m_ClosureJoint.GetConcrete().S);
         concrete.SetBetaSc(pParent->m_ClosureJoint.GetConcrete().BetaSc);
         fci = concrete.GetFc(m_AgeAtContinuity);
      }

      CString strFci;
      strFci.Format(_T("%s"),FormatDimension(fci,pDisplayUnits->GetStressUnit(),false));
      m_ctrlFci.SetWindowText(strFci);
   }
}

void CClosureJointGeneralPage::UpdateConcreteControls(bool bSkipEcCheckBoxes)
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

   if ( !bSkipEcCheckBoxes )
   {
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
   }

   BOOL bEnable = m_ctrlEcCheck.GetCheck();
   GetDlgItem(IDC_EC)->EnableWindow(bEnable);
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(bEnable);

   bEnable = m_ctrlEciCheck.GetCheck();
   GetDlgItem(IDC_ECI)->EnableWindow(bEnable);
   GetDlgItem(IDC_ECI_UNIT)->EnableWindow(bEnable);

   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();
   CString strLabel(ConcreteDescription(pParent->m_ClosureJoint.GetConcrete()));
   GetDlgItem(IDC_CONCRETE_TYPE_LABEL)->SetWindowText( strLabel );
}


BOOL CClosureJointGeneralPage::OnToolTipNotify(UINT id,NMHDR* pNMHDR, LRESULT* pResult)
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
      pTTT->hinst = nullptr;
      return TRUE;
   }
   return FALSE;
}

void CClosureJointGeneralPage::UpdateConcreteParametersToolTip()
{
   CComPtr<IBroker> pBroker;
   EAFGetBroker(&pBroker);

   GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();

   const unitmgtDensityData& density = pDisplayUnits->GetDensityUnit();
   const unitmgtLengthData&  aggsize = pDisplayUnits->GetComponentDimUnit();
   const unitmgtStressData&  stress  = pDisplayUnits->GetStressUnit();
   const unitmgtScalar&      scalar  = pDisplayUnits->GetScalarFormat();

   CString strTip;
   strTip.Format(_T("%-20s %s\r\n%-20s %s\r\n%-20s %s\r\n%-20s %s"),
      _T("Type"), lrfdConcreteUtil::GetTypeName((matConcrete::Type)pParent->m_ClosureJoint.GetConcrete().Type,true).c_str(),
      _T("Unit Weight"),FormatDimension(pParent->m_ClosureJoint.GetConcrete().StrengthDensity,density),
      _T("Unit Weight (w/ reinforcement)"),  FormatDimension(pParent->m_ClosureJoint.GetConcrete().WeightDensity,density),
      _T("Max Aggregate Size"),  FormatDimension(pParent->m_ClosureJoint.GetConcrete().MaxAggregateSize,aggsize)
      );

   if ( pParent->m_ClosureJoint.GetConcrete().Type != pgsTypes::Normal && pParent->m_ClosureJoint.GetConcrete().bHasFct )
   {
      CString strLWC;
      strLWC.Format(_T("\r\n%-20s %s"),
         _T("fct"),FormatDimension(pParent->m_ClosureJoint.GetConcrete().Fct,stress));

      strTip += strLWC;
   }

   CString strPress(_T("\r\n\r\nPress button to edit"));
   strTip += strPress;

   m_strTip = strTip;
}

void CClosureJointGeneralPage::OnHelp() 
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_CLOSUREJOINT_DETAILS_GENERAL );
}

void CClosureJointGeneralPage::FillEventList()
{
   CComboBox* pcbEvent = (CComboBox*)GetDlgItem(IDC_EVENT);

   int selEventIdx = pcbEvent->GetCurSel();

   pcbEvent->ResetContent();

   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();
   EventIndexType nEvents = pParent->m_TimelineMgr.GetEventCount();
   for ( EventIndexType eventIdx = 0; eventIdx < nEvents; eventIdx++ )
   {
      const CTimelineEvent* pTimelineEvent = pParent->m_TimelineMgr.GetEventByIndex(eventIdx);

      CString label;
      label.Format(_T("Event %d: %s"),LABEL_EVENT(eventIdx),pTimelineEvent->GetDescription());

      pcbEvent->SetItemData(pcbEvent->AddString(label),eventIdx);
   }

   CString strNewEvent((LPCSTR)IDS_CREATE_NEW_EVENT);
   pcbEvent->SetItemData(pcbEvent->AddString(strNewEvent),CREATE_TIMELINE_EVENT);

   if ( selEventIdx != CB_ERR )
   {
      pcbEvent->SetCurSel(selEventIdx);
   }
   else
   {
      pcbEvent->SetCurSel(0);
   }
}

void CClosureJointGeneralPage::OnEventChanging()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   m_PrevEventIdx = pCB->GetCurSel();
}

void CClosureJointGeneralPage::OnEventChanged()
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_EVENT);
   int curSel = pCB->GetCurSel();
   EventIndexType eventIdx = (EventIndexType)pCB->GetItemData(curSel);
   if ( eventIdx == CREATE_TIMELINE_EVENT )
   {
      eventIdx = CreateEvent();
      if ( eventIdx == INVALID_INDEX )
      {
         pCB->SetCurSel(m_PrevEventIdx);
         return;
      }
      else
      {
         FillEventList();
      }
   }

   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();
   pParent->m_TimelineMgr.SetCastClosureJointEventByIndex(pParent->m_ClosureID,eventIdx);

   pCB->SetCurSel((int)eventIdx);
}

EventIndexType CClosureJointGeneralPage::CreateEvent()
{
   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();
   CTimelineEventDlg dlg(pParent->m_TimelineMgr,INVALID_INDEX,FALSE);
   if ( dlg.DoModal() == IDOK )
   {
      EventIndexType eventIdx;
      pParent->m_TimelineMgr.AddTimelineEvent(*dlg.m_pTimelineEvent,true,&eventIdx);
      m_bWasEventCreated = true;
      return eventIdx;
  }

   return INVALID_INDEX;
}

void CClosureJointGeneralPage::OnConcreteStrength()
{
   UpdateConcreteControls();
}
