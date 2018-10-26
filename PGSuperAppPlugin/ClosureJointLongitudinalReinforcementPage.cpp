// ClosureJointLongutindalReinforcementPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin.h"
#include "ClosureJointLongitudinalReinforcementPage.h"
#include "ClosureJointDlg.h"
#include <EAF\EAFDisplayUnits.h>

#include "HtmlHelp\HelpTopics.hh"
#include <PsgLib\RebarUIUtils.h>


// CClosureJointLongitudinalReinforcementPage dialog

IMPLEMENT_DYNAMIC(CClosureJointLongitudinalReinforcementPage, CPropertyPage)

CClosureJointLongitudinalReinforcementPage::CClosureJointLongitudinalReinforcementPage()
	: CPropertyPage(CClosureJointLongitudinalReinforcementPage::IDD)
{

}

CClosureJointLongitudinalReinforcementPage::~CClosureJointLongitudinalReinforcementPage()
{
}

void CClosureJointLongitudinalReinforcementPage::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CGirderDescLongitudinalRebar)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
	DDV_GXGridWnd(pDX, &m_Grid);

   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();

   // longitudinal steel information from grid and store it
   if (pDX->m_bSaveAndValidate)
   {
      CLongitudinalRebarData rebarData;

      ROWCOL nrows = m_Grid.GetRowCount();
      for (ROWCOL i=1; i<=nrows; i++)
      {
         CLongitudinalRebarData::RebarRow row;
         if (m_Grid.GetRowData(i,&row))
         {
            // values are in display units - must convert to system
            CComPtr<IBroker> pBroker;
            EAFGetBroker(&pBroker);
            GET_IFACE2(pBroker,IEAFDisplayUnits,pDisplayUnits);

            row.Cover      = ::ConvertToSysUnits(row.Cover,      pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
            row.BarSpacing = ::ConvertToSysUnits(row.BarSpacing, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
            rebarData.RebarRows.push_back(row);
         }
      }


      int idx;
      DDX_CBIndex(pDX,IDC_MILD_STEEL_SELECTOR,idx);
      GetStirrupMaterial(idx,rebarData.BarType,rebarData.BarGrade);

      pParent->m_ClosureJoint.GetRebar() = rebarData;
   }
   else
   {
      int idx = GetStirrupMaterialIndex(pParent->m_ClosureJoint.GetStirrups().ShearBarType,pParent->m_ClosureJoint.GetStirrups().ShearBarGrade);
      DDX_CBIndex(pDX,IDC_MILD_STEEL_SELECTOR,idx);

      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);
      GET_IFACE2_NOCHECK(pBroker,IEAFDisplayUnits,pDisplayUnits); // only used if there are rebar rows

      CLongitudinalRebarData rebardata;
      std::vector<CLongitudinalRebarData::RebarRow>::iterator iter;
      for ( iter = pParent->m_ClosureJoint.GetRebar().RebarRows.begin(); iter != pParent->m_ClosureJoint.GetRebar().RebarRows.end(); iter++ )
      {
         CLongitudinalRebarData::RebarRow row = *iter;
         row.BarSpacing = ::ConvertFromSysUnits(row.BarSpacing, pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);
         row.Cover      = ::ConvertFromSysUnits(row.Cover,      pDisplayUnits->GetComponentDimUnit().UnitOfMeasure);

         rebardata.RebarRows.push_back(row);
      }
      m_Grid.FillGrid(rebardata);
   }
}


BEGIN_MESSAGE_MAP(CClosureJointLongitudinalReinforcementPage, CPropertyPage)
	ON_BN_CLICKED(IDC_INSERTROW, OnInsertrow)
	ON_BN_CLICKED(IDC_APPEND_ROW, OnAppendRow)
	ON_BN_CLICKED(IDC_REMOVEROWS, OnRemoveRows)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CClosureJointLongitudinalReinforcementPage message handlers

void CClosureJointLongitudinalReinforcementPage::OnEnableDelete(bool canDelete)
{
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(canDelete);
}

BOOL CClosureJointLongitudinalReinforcementPage::OnInitDialog() 
{
	m_Grid.SubclassDlgItem(IDC_LONG_GRID, this);
   m_Grid.CustomInit();

   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();
   CComboBox* pc = (CComboBox*)GetDlgItem(IDC_MILD_STEEL_SELECTOR);
   FillRebarMaterialComboBox(pc);

   CPropertyPage::OnInitDialog();
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CClosureJointLongitudinalReinforcementPage::OnInsertrow() 
{
   m_Grid.Insertrow();
}

void CClosureJointLongitudinalReinforcementPage::OnAppendRow() 
{
   m_Grid.Appendrow();
}

void CClosureJointLongitudinalReinforcementPage::OnRemoveRows() 
{
	m_Grid.Removerows();

   // selection is gone after row is deleted
   CWnd* pdel = GetDlgItem(IDC_REMOVEROWS);
   ASSERT(pdel);
   pdel->EnableWindow(FALSE);
}

void CClosureJointLongitudinalReinforcementPage::OnHelp() 
{
	::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_GIRDERWIZ_REBAR );
	
}
