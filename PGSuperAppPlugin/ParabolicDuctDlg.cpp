// ParabolicDuctDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin.h"
#include "ParabolicDuctDlg.h"

#include <EAF\EAFDocument.h>


void DDX_DuctGeometry(CDataExchange* pDX,CParabolicDuctGrid& grid,CParabolicDuctGeometry& ductGeometry)
{
   if ( pDX->m_bSaveAndValidate )
   {
      ductGeometry = grid.GetData();
   }
   else
   {
      grid.SetData(ductGeometry);
   }
}

void DDV_DuctGeometry(CDataExchange* pDX,int nIDC,CParabolicDuctGeometry& ductGeometry)
{
   if ( pDX->m_bSaveAndValidate )
   {
      pDX->PrepareCtrl(nIDC);

      Float64 dist;
      Float64 offset;
      CDuctGeometry::OffsetType offsetType;
      ductGeometry.GetStartPoint(&dist,&offset,&offsetType);
      if ( offset <= 0.0 )
      {
         CString strMsg(_T("The start point offset must be greater than zero."));
         AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);
         pDX->Fail();
      }

      const CSplicedGirderData* pGirder = ductGeometry.GetGirder();
      PierIndexType startPierIdx = pGirder->GetPier(pgsTypes::metStart)->GetIndex();
      SpanIndexType startSpanIdx = pGirder->GetPier(pgsTypes::metStart)->GetNextSpan()->GetIndex();
      SpanIndexType endSpanIdx   = pGirder->GetPier(pgsTypes::metEnd)->GetPrevSpan()->GetIndex();

      ATLASSERT(ductGeometry.GetSpanCount() == endSpanIdx-startSpanIdx+1);

      for ( SpanIndexType spanIdx = startSpanIdx; spanIdx <= endSpanIdx; spanIdx++ )
      {
         Float64 distLow;
         Float64 offsetLow;
         CDuctGeometry::OffsetType lowOffsetType;
         ductGeometry.GetLowPoint(spanIdx,&distLow,&offsetLow,&lowOffsetType);

         if ( offsetLow <= 0.0 )
         {
            CString strMsg;
            strMsg.Format(_T("The low point offset in Span %d must be greater than zero."),LABEL_SPAN(spanIdx));
            AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);
            pDX->Fail();
         }

         PierIndexType pierIdx = (PierIndexType)spanIdx;
         if ( startPierIdx < pierIdx )
         {
            Float64 distLeftIP;
            Float64 highOffset;
            CDuctGeometry::OffsetType highOffsetType;
            Float64 distRightIP;
            ductGeometry.GetHighPoint(pierIdx,&distLeftIP,&highOffset,&highOffsetType,&distRightIP);
            if ( highOffset <= 0.0 )
            {
               CString strMsg;
               strMsg.Format(_T("The high point offset at Pier %d must be greater than zero."),LABEL_PIER(pierIdx));
               AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);
               pDX->Fail();
            }
         }
      }


      ductGeometry.GetEndPoint(&dist,&offset,&offsetType);
      if ( offset <= 0.0 )
      {
         CString strMsg(_T("The end point offset must be greater than zero."));
         AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);
         pDX->Fail();
      }
   }
}

// CParabolicDuctDlg dialog

IMPLEMENT_DYNAMIC(CParabolicDuctDlg, CDialog)

CParabolicDuctDlg::CParabolicDuctDlg(CSplicedGirderGeneralPage* pGdrDlg,CWnd* pParent /*=nullptr*/)
	: CDialog(CParabolicDuctDlg::IDD, pParent)
{
   m_pGirderlineDlg = pGdrDlg;
}

CParabolicDuctDlg::~CParabolicDuctDlg()
{
}

void CParabolicDuctDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
   DDX_DuctGeometry(pDX,m_Grid,m_DuctGeometry);
   DDV_DuctGeometry(pDX,IDC_POINT_GRID,m_DuctGeometry);
}


BEGIN_MESSAGE_MAP(CParabolicDuctDlg, CDialog)
   ON_BN_CLICKED(ID_HELP,&CParabolicDuctDlg::OnHelp)
END_MESSAGE_MAP()


// CParabolicDuctDlg message handlers

BOOL CParabolicDuctDlg::OnInitDialog()
{
   m_Grid.SubclassDlgItem(IDC_POINT_GRID,this);
   m_Grid.CustomInit(this);

   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}


void CParabolicDuctDlg::OnDuctChanged()
{
   m_pGirderlineDlg->OnDuctChanged();
}

void CParabolicDuctDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_PARABOLIC_DUCT);
}
