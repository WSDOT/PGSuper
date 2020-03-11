///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// TrafficBarrierDlg.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\psglib.h>
#include "TrafficBarrierDlg.h"
#include "TrafficBarrierViewDialog.h"
#include <MfcTools\CustomDDX.h>
#include <WBFLGenericBridge.h>

#include <LRFD\ConcreteUtil.h>

#include <EAF\EAFApp.h>
#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierDlg dialog

IMPLEMENT_DYNAMIC(CTrafficBarrierDlg, CDialog)

CTrafficBarrierDlg::CTrafficBarrierDlg(bool allowEditing,
                                       CWnd* pParent /*=nullptr*/)
	: CDialog(CTrafficBarrierDlg::IDD, pParent),
   m_bAllowEditing(allowEditing)
{
	//{{AFX_DATA_INIT(CTrafficBarrierDlg)
	m_Name = _T("");
	//}}AFX_DATA_INIT

   m_BarrierPoints.CoCreateInstance(CLSID_Point2dCollection);

   m_Ec = ::ConvertToSysUnits(lrfdConcreteUtil::ModE(matConcrete::Normal, 4.0,155.0,false),unitMeasure::KSI);

   m_bStructurallyContinuous = false;
}


void CTrafficBarrierDlg::DoDataExchange(CDataExchange* pDX)
{
   CEAFApp* pApp = EAFGetApp();
   const unitmgtIndirectMeasure* pDisplayUnits = pApp->GetDisplayUnits();


   CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTrafficBarrierDlg)
	DDX_Text(pDX, IDC_NAME, m_Name);
	//}}AFX_DATA_MAP

   if (pDX->m_bSaveAndValidate)
   {
	   DDX_Text(pDX, IDC_NAME, m_Name);
      if (m_Name.IsEmpty())
      {
         AfxMessageBox(_T("Specification Name cannot be blank"));
         pDX->Fail();
      }
   }

   if ( pDX->m_bSaveAndValidate )
   {
      m_PointsGrid.DownloadData(pDX,m_BarrierPoints);
   }
   else
   {
      m_PointsGrid.UploadData(pDX,m_BarrierPoints);
   }

   DDX_Control(pDX,IDC_TRAFFIC_BARRIER, m_Picture);

   DDX_Check_Bool(pDX,IDC_CONTINOUOUS_BARRIER,m_bStructurallyContinuous);

   DDX_CBEnum(pDX,IDC_WEIGHT_METHOD,m_WeightMethod);
   DDX_UnitValueAndTag(pDX, IDC_WEIGHT, IDC_WEIGHT_UNIT, m_Weight, pDisplayUnits->ForcePerLength  );
   if ( !pDX->m_bSaveAndValidate )
   {
      CString strTag;
      GetDlgItem(IDC_WEIGHT_UNIT)->GetWindowText(strTag);
      strTag += _T("/barrier");
      GetDlgItem(IDC_WEIGHT_UNIT)->SetWindowText(strTag);
   }

   DDX_UnitValueAndTag(pDX, IDC_EC, IDC_EC_UNIT, m_Ec, pDisplayUnits->ModE );

   DDX_UnitValueAndTag(pDX, IDC_CURBOFFSET, IDC_CURBOFFSET_UNIT, m_CurbOffset, pDisplayUnits->ComponentDim );
}


BEGIN_MESSAGE_MAP(CTrafficBarrierDlg, CDialog)
	//{{AFX_MSG_MAP(CTrafficBarrierDlg)
	ON_BN_CLICKED(ID_HELP,OnHelp)
	ON_CBN_SELCHANGE(IDC_WEIGHT_METHOD, OnWeightMethodChanged)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_VIEW, OnView)
	ON_BN_CLICKED(IDC_UP, OnMoveUp)
	ON_BN_CLICKED(IDC_DOWN, OnMoveDown)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTrafficBarrierDlg message handlers
void CTrafficBarrierDlg::OnHelp()
{
   EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_TRAFFIC_BARRIER_DIALOG );
}

BOOL CTrafficBarrierDlg::OnInitDialog() 
{
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_WEIGHT_METHOD);
   int idx = pCB->AddString(TrafficBarrierEntry::GetWeightMethodType(TrafficBarrierEntry::Compute));
   pCB->SetItemData(idx,(DWORD)TrafficBarrierEntry::Compute);

   idx = pCB->AddString(TrafficBarrierEntry::GetWeightMethodType(TrafficBarrierEntry::Input));
   pCB->SetItemData(idx,(DWORD)TrafficBarrierEntry::Input);

	m_PointsGrid.SubclassDlgItem(IDC_TB_POINTS, this);
   m_PointsGrid.CustomInit();

   
   CDialog::OnInitDialog();

   m_Picture.SetImage(_T("TrafficBarrier"),_T("Metafile"),EMF_FIT);

   // disable OK button if editing not allowed
   CString head;
   GetWindowText(head);
   head += _T(" - ");
   head += m_Name;
	if (!m_bAllowEditing)
   {
      CWnd* pOK = GetDlgItem(IDOK);
      pOK->ShowWindow(SW_HIDE);

      CWnd* pCancel = GetDlgItem(IDCANCEL);
      pCancel->SetWindowText(_T("Close"));

      head += _T(" (Read Only)");
   }
   SetWindowText(head);

   OnWeightMethodChanged();

   EnableMoveUp(m_PointsGrid.CanEnableMoveUp());
   EnableMoveDown(m_PointsGrid.CanEnableMoveDown());
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CTrafficBarrierDlg::OnWeightMethodChanged() 
{
	// TODO: Add your control notification handler code here
   CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_WEIGHT_METHOD);
   int idx = pCB->GetCurSel();
   TrafficBarrierEntry::WeightMethod method = (TrafficBarrierEntry::WeightMethod)pCB->GetItemData(idx);

   GetDlgItem(IDC_WEIGHT_LABEL)->EnableWindow(method == TrafficBarrierEntry::Input );
   GetDlgItem(IDC_WEIGHT)->EnableWindow(method == TrafficBarrierEntry::Input );
   GetDlgItem(IDC_WEIGHT_UNIT)->EnableWindow(method == TrafficBarrierEntry::Input );

   GetDlgItem(IDC_WEIGHT_LABEL)->ShowWindow(method == TrafficBarrierEntry::Input ? SW_SHOW : SW_HIDE );
   GetDlgItem(IDC_WEIGHT)->ShowWindow(method == TrafficBarrierEntry::Input ? SW_SHOW : SW_HIDE );
   GetDlgItem(IDC_WEIGHT_UNIT)->ShowWindow(method == TrafficBarrierEntry::Input ? SW_SHOW : SW_HIDE );
	
   GetDlgItem(IDC_EC_LABEL)->EnableWindow(method == TrafficBarrierEntry::Input );
   GetDlgItem(IDC_EC)->EnableWindow(method == TrafficBarrierEntry::Input );
   GetDlgItem(IDC_EC_UNIT)->EnableWindow(method == TrafficBarrierEntry::Input );

   GetDlgItem(IDC_EC_LABEL)->ShowWindow(method == TrafficBarrierEntry::Input ? SW_SHOW : SW_HIDE );
   GetDlgItem(IDC_EC)->ShowWindow(method == TrafficBarrierEntry::Input ? SW_SHOW : SW_HIDE );
   GetDlgItem(IDC_EC_UNIT)->ShowWindow(method == TrafficBarrierEntry::Input ? SW_SHOW : SW_HIDE );
}

void CTrafficBarrierDlg::OnAdd()
{
   m_PointsGrid.AppendRow();
}

void CTrafficBarrierDlg::OnDelete()
{
   m_PointsGrid.RemoveRows();
}

void CTrafficBarrierDlg::OnView()
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   UpdateData(TRUE);

   TrafficBarrierEntry entry;
   entry.SetBarrierPoints(m_BarrierPoints);

   CComPtr<IPolyShape> polyshape;
   entry.CreatePolyShape(pgsTypes::tboLeft,&polyshape);
   CComQIPtr<IShape> shape(polyshape);

   CTrafficBarrierViewDialog dlg(shape,this);
   dlg.DoModal();
}

void CTrafficBarrierDlg::OnMoveUp()
{
   m_PointsGrid.MoveUp();
}

void CTrafficBarrierDlg::OnMoveDown()
{
   m_PointsGrid.MoveDown();
}

void CTrafficBarrierDlg::EnableMoveUp(BOOL bEnable)
{
   GetDlgItem(IDC_UP)->EnableWindow(bEnable);
}

void CTrafficBarrierDlg::EnableMoveDown(BOOL bEnable)
{
   GetDlgItem(IDC_DOWN)->EnableWindow(bEnable);
}