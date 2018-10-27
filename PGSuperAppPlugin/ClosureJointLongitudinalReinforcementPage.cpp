// ClosureJointLongutindalReinforcementPage.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\resource.h"
#include "PGSuperAppPlugin.h"
#include "ClosureJointLongitudinalReinforcementPage.h"
#include "ClosureJointDlg.h"
#include <EAF\EAFDisplayUnits.h>
#include <EAF\EAFDocument.h>
#include <IFace\BeamFactory.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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

   DDX_Control(pDX,IDC_MILD_STEEL_SELECTOR,m_cbRebar);

   CClosureJointDlg* pParent = (CClosureJointDlg*)GetParent();


   // longitudinal steel information from grid and store it
   if (pDX->m_bSaveAndValidate)
   {
      CLongitudinalRebarData& rebarData(pParent->m_ClosureJoint.GetRebar());
      DDX_RebarMaterial(pDX,IDC_MILD_STEEL_SELECTOR,rebarData.BarType,rebarData.BarGrade);
      m_Grid.GetRebarData(&rebarData);


      CComPtr<IBroker> pBroker;
      EAFGetBroker(&pBroker);

      const CPrecastSegmentData* pLeftSegment = pParent->m_ClosureJoint.GetLeftSegment();
      const CPrecastSegmentData* pRightSegment = pParent->m_ClosureJoint.GetRightSegment();

      CString strMsg;
      CComPtr<IPoint2d> point;
      point.CoCreateInstance(CLSID_Point2d);
      int rowIdx = 1;
      for (const auto& row : rebarData.RebarRows)
      {
         if (row.Cover < 0)
         {
            strMsg.Format(_T("The cover for row %d must be greater than zero."), rowIdx);
            AfxMessageBox(strMsg);
            pDX->Fail();
         }

         if (row.NumberOfBars == INVALID_INDEX)
         {
            strMsg.Format(_T("The number of bars in row %d must be greater than zero."), rowIdx);
            AfxMessageBox(strMsg);
            pDX->Fail();
         }

         if (1 < row.NumberOfBars && row.BarSpacing < 0)
         {
            strMsg.Format(_T("The bar spacing in row %d must be greater than zero."), rowIdx);
            AfxMessageBox(strMsg);
            pDX->Fail();
         }

         // make sure bars are inside of girder - use shape symmetry
         for (int i = 0; i < 2; i++)
         {
            const CPrecastSegmentData* pSegment = (i == 0 ? pLeftSegment : pRightSegment);
            pgsTypes::SegmentZoneType zone = (i == 0 ? pgsTypes::sztLeftPrismatic : pgsTypes::sztRightPrismatic);

            Float64 Lzone, height, tbf;
            pSegment->GetVariationParameters(zone, false, &Lzone, &height, &tbf);

            gpPoint2d testpnt;
            testpnt.X() = row.BarSpacing * (row.NumberOfBars - 1) / 2.;
            if (row.Face == pgsTypes::TopFace)
            {
               testpnt.Y() = -row.Cover;
            }
            else
            {
               testpnt.Y() = -(height - row.Cover);
            }

            point->Move(testpnt.X(), testpnt.Y());

            const GirderLibraryEntry* pGdrEntry = pSegment->GetGirder()->GetGirderLibraryEntry();
            CComPtr<IBeamFactory> factory;
            pGdrEntry->GetBeamFactory(&factory);

            CComPtr<IGirderSection> gdrSection;
            factory->CreateGirderSection(pBroker, INVALID_ID, pGdrEntry->GetDimensions(), height, tbf, &gdrSection);

            CComQIPtr<IShape> shape(gdrSection);

            VARIANT_BOOL bPointInShape;
            shape->PointInShape(point, &bPointInShape);
            if (bPointInShape == VARIANT_FALSE)
            {
               strMsg.Format(_T("One or more of the bars in row %d are outside of the girder section."), rowIdx);
               AfxMessageBox(strMsg);
               pDX->Fail();
            }
         }

         rowIdx++;
      }
   }
   else
   {
      DDX_RebarMaterial(pDX,IDC_MILD_STEEL_SELECTOR,pParent->m_ClosureJoint.GetStirrups().ShearBarType,pParent->m_ClosureJoint.GetStirrups().ShearBarGrade);
      m_Grid.FillGrid(pParent->m_ClosureJoint.GetRebar());
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
	EAFHelp( EAFGetDocument()->GetDocumentationSetName(), IDH_GIRDERDETAILS_LONGIT_REBAR );
}
