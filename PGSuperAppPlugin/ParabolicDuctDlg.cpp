// ParabolicDuctDlg.cpp : implementation file
//

#include "stdafx.h"
#include "PGSuperAppPlugin.h"
#include "ParabolicDuctDlg.h"

#include <EAF\EAFDocument.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


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

      PierIndexType startPierIdx, endPierIdx;
      ductGeometry.GetRange(&startPierIdx, &endPierIdx);

      PierIndexType pierIdx;
      Float64 dist;
      Float64 offset;
      CDuctGeometry::OffsetType offsetType;
      ductGeometry.GetStartPoint(&pierIdx,&dist,&offset,&offsetType);
      if ( offset <= 0.0 )
      {
         CString strMsg(_T("The start point offset must be greater than zero."));
         AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);
         pDX->Fail();
      }

      SpanIndexType startSpanIdx = (SpanIndexType)(startPierIdx);
      SpanIndexType endSpanIdx = (SpanIndexType)(endPierIdx - 1);
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
            strMsg.Format(_T("The low point offset in Span %s must be greater than zero."),LABEL_SPAN(spanIdx));
            AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);
            pDX->Fail();
         }

         if (distLow == -1.0)
         {
            CString strMsg;
            strMsg.Format(_T("The low point in Span %s cannot be coincident with the high point. Use a relative distance that is less than 100%%."),LABEL_SPAN(spanIdx));
            AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK);
            pDX->Fail();
         }

         if (distLow == 0.0)
         {
            CString strMsg;
            strMsg.Format(_T("The low point in Span %s cannot be zero."), LABEL_SPAN(spanIdx));
            AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK);
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
               strMsg.Format(_T("The high point offset at Pier %s must be greater than zero."),LABEL_PIER(pierIdx));
               AfxMessageBox(strMsg,MB_ICONEXCLAMATION | MB_OK);
               pDX->Fail();
            }

            if (distLeftIP == -1.0)
            {
               CString strMsg;
               strMsg.Format(_T("The inflection point before Pier %s cannot be coincident with the high point. Use a relative distance that is less than 100%%."), LABEL_PIER(pierIdx));
               AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK);
               pDX->Fail();
            }

            if (distRightIP == -1.0)
            {
               CString strMsg;
               strMsg.Format(_T("The inflection point after Pier %s cannot be coincident with the high point. Use a relative distance that is less than 100%%."), LABEL_PIER(pierIdx));
               AfxMessageBox(strMsg, MB_ICONEXCLAMATION | MB_OK);
               pDX->Fail();
            }
         }
      }


      ductGeometry.GetEndPoint(&pierIdx,&dist,&offset,&offsetType);
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

CParabolicDuctDlg::CParabolicDuctDlg(CSplicedGirderGeneralPage* pGdrDlg,CPTData* pPTData,DuctIndexType ductIdx,CWnd* pParent /*=nullptr*/)
	: CDialog(CParabolicDuctDlg::IDD, pParent)
{
   m_pGirderlineDlg = pGdrDlg;
   m_PTData = *pPTData;
   m_PTData.SetGirder(m_pGirderlineDlg->GetGirder(),false/*don't initialize pt data*/);
   m_DuctIdx = ductIdx;
}

CParabolicDuctDlg::~CParabolicDuctDlg()
{
}

void CParabolicDuctDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

   PierIndexType startPierIdx, endPierIdx;
   if (!pDX->m_bSaveAndValidate)
   {
      m_PTData.GetDuct(m_DuctIdx)->ParabolicDuctGeometry.GetRange(&startPierIdx, &endPierIdx);
   }
   DDX_CBItemData(pDX, IDC_START_PIER, startPierIdx);
   DDX_CBItemData(pDX, IDC_END_PIER, endPierIdx);

   DDX_DuctGeometry(pDX,m_Grid, m_PTData.GetDuct(m_DuctIdx)->ParabolicDuctGeometry);
   DDV_DuctGeometry(pDX,IDC_POINT_GRID, m_PTData.GetDuct(m_DuctIdx)->ParabolicDuctGeometry);

#if defined _DEBUG
   if (pDX->m_bSaveAndValidate)
   {
      PierIndexType p1, p2;
      m_PTData.GetDuct(m_DuctIdx)->ParabolicDuctGeometry.GetRange(&p1, &p2);
      ATLASSERT(p1 == startPierIdx);
      ATLASSERT(p2 == endPierIdx);
   }
#endif
}


BEGIN_MESSAGE_MAP(CParabolicDuctDlg, CDialog)
   ON_BN_CLICKED(ID_HELP,&CParabolicDuctDlg::OnHelp)
   ON_CBN_SELCHANGE(IDC_START_PIER, &CParabolicDuctDlg::OnRangeChanged)
   ON_CBN_SELCHANGE(IDC_END_PIER, &CParabolicDuctDlg::OnRangeChanged)
   ON_BN_CLICKED(IDC_SCHEMATIC, &CParabolicDuctDlg::OnSchematicButton)
END_MESSAGE_MAP()


// CParabolicDuctDlg message handlers

BOOL CParabolicDuctDlg::OnInitDialog()
{
   CString strTitle;
   strTitle.Format(_T("Parabolic Duct - Duct %d"), LABEL_DUCT(m_DuctIdx));
   SetWindowText(strTitle);

   m_Grid.SubclassDlgItem(IDC_POINT_GRID,this);
   m_Grid.CustomInit(this);
   m_Grid.SetData(m_PTData.GetDuct(m_DuctIdx)->ParabolicDuctGeometry);

   // subclass the schematic drawing of the tendons
   m_DrawTendons.SubclassDlgItem(IDC_TENDONS, this);
   m_DrawTendons.CustomInit(m_pGirderlineDlg->GetGirder()->GetGirderKey(), m_pGirderlineDlg->GetGirder(), &m_PTData);
   m_DrawTendons.SetDuct(m_DuctIdx);
   m_DrawTendons.DrawAllDucts(true);
   m_DrawTendons.SetMapMode(m_pGirderlineDlg->GetTendonControlMapMode());

   FillPierLists();

   CDialog::OnInitDialog();

   HINSTANCE hInstance = AfxGetInstanceHandle();
   CButton* pSchematicBtn = (CButton*)GetDlgItem(IDC_SCHEMATIC);
   pSchematicBtn->SetIcon((HICON)::LoadImage(hInstance, MAKEINTRESOURCE(IDI_SCHEMATIC), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR | LR_SHARED));

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CParabolicDuctDlg::FillPierLists()
{
   PierIndexType startPierIdx = m_pGirderlineDlg->GetGirder()->GetPierIndex(pgsTypes::metStart);
   PierIndexType endPierIdx = m_pGirderlineDlg->GetGirder()->GetPierIndex(pgsTypes::metEnd);

   CComboBox* pcbStart = (CComboBox*)GetDlgItem(IDC_START_PIER);
   CComboBox* pcbEnd = (CComboBox*)GetDlgItem(IDC_END_PIER);
   int curSelStart = pcbStart->GetCurSel();
   int curSelEnd = pcbEnd->GetCurSel();

   PierIndexType curStartPierIdx = (curSelStart == CB_ERR ? startPierIdx : (PierIndexType)pcbStart->GetItemData(curSelStart));
   PierIndexType curEndPierIdx = (curSelEnd == CB_ERR ? endPierIdx : (PierIndexType)pcbEnd->GetItemData(curSelEnd));

   pcbStart->ResetContent();
   pcbEnd->ResetContent();

   PierIndexType ductStartPierIdx, ductEndPierIdx;
   m_Grid.GetTendonRange(&ductStartPierIdx, &ductEndPierIdx);

   for (PierIndexType pierIdx = startPierIdx; pierIdx <= endPierIdx; pierIdx++)
   {
      CString strText;
      strText.Format(_T("%s"), LABEL_PIER_EX(m_pGirderlineDlg->GetGirder()->GetGirderGroup()->GetBridgeDescription()->GetPier(pierIdx)->IsAbutment(), pierIdx));

      if (pierIdx < ductEndPierIdx)
      {
         // the start list can't have a pier index at or after the last pier (can't start after the end)
         int idx = pcbStart->AddString(strText);
         pcbStart->SetItemData(idx, (DWORD_PTR)pierIdx);

         if (pierIdx == curStartPierIdx)
         {
            pcbStart->SetCurSel(idx);
         }
      }

      if (ductStartPierIdx < pierIdx)
      {
         // the end list can't have a pier index at or before the first pier (can't end before the start)
         int idx = pcbEnd->AddString(strText);
         pcbEnd->SetItemData(idx, (DWORD_PTR)pierIdx);

         if (pierIdx == curEndPierIdx)
         {
            pcbEnd->SetCurSel(idx);
         }
      }
   }
}

void CParabolicDuctDlg::OnDuctChanged()
{
   m_PTData.GetDuct(m_DuctIdx)->ParabolicDuctGeometry = m_Grid.GetData(); // must update the PTData so the tendon control is drawing the most current data

   //m_pGirderlineDlg->OnDuctChanged();
   m_DrawTendons.Invalidate();
   m_DrawTendons.UpdateWindow();
}

void CParabolicDuctDlg::OnRangeChanged()
{
   CComboBox* pcbStart = (CComboBox*)GetDlgItem(IDC_START_PIER);
   int curSel = pcbStart->GetCurSel();
   PierIndexType startPierIdx = (PierIndexType)(pcbStart->GetItemData(curSel));
   CComboBox* pcbEnd = (CComboBox*)GetDlgItem(IDC_END_PIER);
   curSel = pcbEnd->GetCurSel();
   PierIndexType endPierIdx = (PierIndexType)(pcbEnd->GetItemData(curSel));

   m_Grid.SetTendonRange(startPierIdx, endPierIdx);

   FillPierLists();

   OnDuctChanged();
}

void CParabolicDuctDlg::OnHelp()
{
   EAFHelp(EAFGetDocument()->GetDocumentationSetName(),IDH_PARABOLIC_DUCT);
}

void CParabolicDuctDlg::OnSchematicButton()
{
   auto mm = m_DrawTendons.GetMapMode();
   mm = (mm == grlibPointMapper::Isotropic ? grlibPointMapper::Anisotropic : grlibPointMapper::Isotropic);
   m_DrawTendons.SetMapMode(mm);
}

const CParabolicDuctGeometry& CParabolicDuctDlg::GetDuctGeometry() const
{
   return m_PTData.GetDuct(m_DuctIdx)->ParabolicDuctGeometry;
}

grlibPointMapper::MapMode CParabolicDuctDlg::GetTendonControlMapMode() const
{
   return m_DrawTendons.GetMapMode();
}