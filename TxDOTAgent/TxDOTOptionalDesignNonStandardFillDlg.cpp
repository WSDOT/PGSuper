///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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

// TxDOTOptionalDesignNonStandardFillDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TxDOTOptionalDesignNonStandardFillDlg.h"
#include <WBFLGenericBridge.h>

#include <IFace\BeamFactory.h>


// CTxDOTOptionalDesignNonStandardFillDlg dialog

IMPLEMENT_DYNAMIC(CTxDOTOptionalDesignNonStandardFillDlg, CDialog)

CTxDOTOptionalDesignNonStandardFillDlg::CTxDOTOptionalDesignNonStandardFillDlg(CWnd* pParent /*=nullptr*/)
	: CDialog(CTxDOTOptionalDesignNonStandardFillDlg::IDD, pParent),
   m_pGirderData(nullptr),
   m_pBrokerRetriever(nullptr),
   m_UseDepressed(true),
   m_bFirstActive(true)
{

}

CTxDOTOptionalDesignNonStandardFillDlg::~CTxDOTOptionalDesignNonStandardFillDlg()
{
}

void CTxDOTOptionalDesignNonStandardFillDlg::Init(CTxDOTOptionalDesignGirderData* pGirderData, ITxDOTBrokerRetriever* pBrokerRetriever)
{
   m_pGirderData = pGirderData;
   m_pBrokerRetriever = pBrokerRetriever;

   // compute ybottom
   GirderLibrary* pLib = m_pBrokerRetriever->GetGirderLibrary();

   CString girder_name = m_pGirderData->GetGirderEntryName();
   const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pLib->GetEntry(girder_name));

   // first need cg location of outer shape - this requires some work
   CComPtr<IBeamFactory> pFactory;
   pGdrEntry->GetBeamFactory(&pFactory);
   GirderLibraryEntry::Dimensions dimensions = pGdrEntry->GetDimensions();

   long DUMMY_AGENT_ID = -1;
   CComPtr<IGirderSection> gdrSection;
   pFactory->CreateGirderSection(nullptr,INVALID_ID,dimensions,-1,-1,&gdrSection);

   CComPtr<IShape>  pShape;
   gdrSection.QueryInterface(&pShape);

   CComPtr<IShapeProperties> pShapeProps;
   pShape->get_ShapeProperties(&pShapeProps);

   pShapeProps->get_Ybottom(&m_yBottom);

   m_yBottom = ::ConvertFromSysUnits(m_yBottom,unitMeasure::Inch);
}

void CTxDOTOptionalDesignNonStandardFillDlg::DoDataExchange(CDataExchange* pDX)
{
   CDialog::DoDataExchange(pDX);

   ASSERT(m_pGirderData);
   ASSERT(m_pBrokerRetriever);

   GirderLibrary* pLib = m_pBrokerRetriever->GetGirderLibrary();

   CString girder_name = m_pGirderData->GetGirderEntryName();
   const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pLib->GetEntry(girder_name));
   if (pGdrEntry==nullptr)
   {
      CString msg, stmp;
      stmp.LoadString(IDS_GDR_ERROR);
      msg.Format(stmp,girder_name);
      ::AfxMessageBox(msg);
      return pDX->Fail();
   }

   if (pDX->m_bSaveAndValidate)
   {
      CTxDOTOptionalDesignGirderData::StrandRowContainer cl_rows = m_GridAtCL.GetData();

      if (m_UseDepressed)
      {
         CTxDOTOptionalDesignGirderData::StrandRowContainer end_rows = m_GridAtEnds.GetData();

         CString msg;
         if (!m_pGirderData->CheckAndBuildStrandRows(pGdrEntry, cl_rows, end_rows, msg))
         {
            pDX->PrepareCtrl(IDC_CL_GRID);
            ::AfxMessageBox(msg, MB_OK|MB_ICONEXCLAMATION);
            pDX->Fail();
         }

         m_pGirderData->SetStrandsAtEnds(end_rows);
      }

      m_pGirderData->SetStrandsAtCL(cl_rows);


      // Don't allow exit if cl/end strand no's don't match
      if (m_UseDepressed)
      {
         Float64 cg;
         StrandIndexType clnos;
         m_GridAtCL.ComputeStrands(&clnos, &cg);

         StrandIndexType endnos;
         m_GridAtEnds.ComputeStrands(&endnos, &cg);

         if (clnos != endnos)
         {
            pDX->PrepareCtrl(IDC_CL_GRID);
            ::AfxMessageBox(_T("No. Strands must match at girder C.L. and Ends"),MB_OK|MB_ICONEXCLAMATION);
            pDX->Fail();
         }
      }
   }
   else
   {
      if (pGdrEntry->IsDifferentHarpedGridAtEndsUsed() && pGdrEntry->GetMaxHarpedStrands()>0)
      {
         CString msg;
         msg.Format(_T("The girder entry with name: \"%s\" has harped strands with different locations at the ends and C.L. Cannot continue"),girder_name);
         ::AfxMessageBox(msg);
         return pDX->Fail();
      }

      CTxDOTOptionalDesignGirderData::AvailableStrandsInRowContainer available_rows =m_pGirderData->ComputeAvailableStrandRows(pGdrEntry);

      m_GridAtCL.FillGrid(available_rows,   m_pGirderData->GetStrandsAtCL());
      m_GridAtEnds.FillGrid(available_rows, m_pGirderData->GetStrandsAtEnds());
   }

   DDX_Control(pDX, IDC_CL_NO_STRANDS, m_CLNoStrandsCtrl);
   DDX_Control(pDX, IDC_END_NO_STRANDS, m_EndsNoStrandsCtrl);
}


BEGIN_MESSAGE_MAP(CTxDOTOptionalDesignNonStandardFillDlg, CDialog)
   ON_WM_CTLCOLOR()
   ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// CTxDOTOptionalDesignNonStandardFillDlg message handlers
BOOL CTxDOTOptionalDesignNonStandardFillDlg::OnInitDialog()
{
	m_GridAtCL.SubclassDlgItem(IDC_CL_GRID, this);
   m_GridAtCL.CustomInit(this);

	m_GridAtEnds.SubclassDlgItem(IDC_END_GRID, this);
   m_GridAtEnds.CustomInit(this);

   if(!CDialog::OnInitDialog())
      return FALSE;

   DisplayEndCtrls();

   return TRUE;
}

BOOL CTxDOTOptionalDesignNonStandardFillDlg::OnFillSetActive()
{
   if (m_bFirstActive)
   {
      m_bFirstActive = false;
   }
   else
   {
      if(!UpdateData(FALSE))
         return FALSE;

      UpdateNoStrandsCtrls();
   }

   return TRUE;
}

BOOL CTxDOTOptionalDesignNonStandardFillDlg::OnFillKillActive()
{
   return UpdateData(TRUE);
}

void CTxDOTOptionalDesignNonStandardFillDlg::UpdateNoStrandsCtrls()
{
   // Centerline
   Float64 clcg;
   StrandIndexType clnos;
   m_GridAtCL.ComputeStrands(&clnos,&clcg);
   CString msg;
   if (clnos==0)
      msg.Format(_T("No. Strands = %d"), clnos);
   else
      msg.Format(_T("No. Strands = %d\ne = %.3f in"), clnos,m_yBottom-clcg);

   m_CLNoStrandsCtrl.SetWindowText(msg);

   // Ends
   Float64 endcg;
   StrandIndexType endnos;
   m_GridAtEnds.ComputeStrands(&endnos,&endcg);
   if (endnos==0)
      msg.Format(_T("No. Strands = %d"), endnos);
   else
      msg.Format(_T("No. Strands = %d\ne = %.3f in"), endnos,m_yBottom-endcg);

   m_EndsNoStrandsCtrl.SetWindowText(msg);
}

HBRUSH CTxDOTOptionalDesignNonStandardFillDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
   // Set strand no text red if no's dont' match
   if (m_UseDepressed)
   {
      int id = pWnd->GetDlgCtrlID();
      if (id==IDC_CL_NO_STRANDS || id==IDC_END_NO_STRANDS)
      {
         Float64 cg;
         StrandIndexType clnos;
         m_GridAtCL.ComputeStrands(&clnos,&cg);
         StrandIndexType endnos;
         m_GridAtEnds.ComputeStrands(&endnos,&cg);

         if (clnos != endnos)
         {
            pDC->SetTextColor(RGB(255, 0, 0));
         }
      }
   }

   pDC->SetBkColor(TXDOT_BACK_COLOR);

   CBrush backBrush;
   backBrush.CreateSolidBrush(TXDOT_BACK_COLOR);

   return (HBRUSH)backBrush;
}

void CTxDOTOptionalDesignNonStandardFillDlg::OnGridDataChanged()
{
   // grid data change - update controls
   UpdateNoStrandsCtrls();
}

void CTxDOTOptionalDesignNonStandardFillDlg::DoUseDepressed(bool useDepr)
{
   m_UseDepressed = useDepr;

   DisplayEndCtrls();
}

void CTxDOTOptionalDesignNonStandardFillDlg::DisplayEndCtrls()
{
   // show or hide end grid and assoc controls
   int do_show = m_UseDepressed ? SW_SHOW : SW_HIDE;

   m_GridAtEnds.ShowWindow( do_show );
   m_EndsNoStrandsCtrl.ShowWindow( do_show );

   CWnd* pGroup = GetDlgItem(IDC_ENDS_GROUP);
   pGroup->ShowWindow( do_show );

   // strand no colors need to be updated
   this->Invalidate(TRUE);
}

BOOL CTxDOTOptionalDesignNonStandardFillDlg::OnEraseBkgnd(CDC* pDC)
{
    CRect r;
    GetClientRect(&r);

    CBrush brush;
    brush.CreateSolidBrush(TXDOT_BACK_COLOR);

    pDC->FillRect(&r, &brush);
    return TRUE;
}
