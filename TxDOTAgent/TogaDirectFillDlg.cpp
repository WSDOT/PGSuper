///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
// DirectFillDlg.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "TogaDirectFillDlg.h"
#include "TxDOTOptionalDesignUtilities.h"
#include "TOGAGirderSelectStrandsDlg.h"

#include <IFace\EditByUI.h>

#include <PgsExt\BridgeDescription2.h>

// CTogaDirectFillDlg dialog

IMPLEMENT_DYNAMIC(CTogaDirectFillDlg, CDialog)

CTogaDirectFillDlg::CTogaDirectFillDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTogaDirectFillDlg::IDD, pParent), 
   m_bFirstActive(true),
   m_SpanLength(0)
{

}

CTogaDirectFillDlg::~CTogaDirectFillDlg()
{
}

void CTogaDirectFillDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(CTogaDirectFillDlg, CDialog)
   ON_BN_CLICKED(IDC_SELECT_STRANDS, &CTogaDirectFillDlg::OnBnClickedSelectStrands)
END_MESSAGE_MAP()


// CTogaDirectFillDlg message handlers

void CTogaDirectFillDlg::OnBnClickedSelectStrands()
{
   CComPtr<IBroker> pBroker = m_pBrokerRetriever->GetClassicBroker();
   GET_IFACE2(pBroker, IBridge, pBridge);
   GET_IFACE2( pBroker, ISpecification, pSpec );

   // Max debond length is ~1/2 girder length
   Float64 maxDebondLength = m_SpanLength/2.0;

   std::_tstring spec_name = pSpec->GetSpecification();
   const libLibraryEntry* pLibEntry = m_pBrokerRetriever->GetSpecLibrary()->GetEntry(spec_name.c_str());
   const SpecLibraryEntry* pSpecEntry = dynamic_cast<const SpecLibraryEntry*>(pLibEntry);


   CTOGAGirderSelectStrandsDlg dlg;
   dlg.InitializeData(TOGA_SPAN, m_GirderIdx, m_pGirderData->GetDirectFilledStraightStrands(),
                      m_pGirderData->GetDirectFilledStraightDebond(), 
                      pSpecEntry, &m_GirderLibraryEntry, maxDebondLength);

   if ( dlg.DoModal() == IDOK )
   {
      // get data from dialog
      CDirectStrandFillCollection directFilledStraightStrands;
      std::vector<CDebondData> straightDebond;
      dlg.GetData(directFilledStraightStrands, straightDebond);

      m_pGirderData->SetDirectFilledStraightStrands(directFilledStraightStrands);
      m_pGirderData->SetDirectFilledStraightDebond(straightDebond);

      UpdateNoStrandsCtrls();
   }
}

BOOL CTogaDirectFillDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   // TODO:  Add extra initialization here

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CTogaDirectFillDlg::Init(CTxDOTOptionalDesignGirderData* pGirderData, ITxDOTBrokerRetriever* pBrokerRetriever, LPCTSTR entryName,
                              GirderIndexType girderIdx)
{
   m_pGirderData = pGirderData;
   m_pBrokerRetriever = pBrokerRetriever;
   m_GirderEntryName = entryName;
   m_GirderIdx = girderIdx;

   UpdateLibraryData();
}

BOOL CTogaDirectFillDlg::OnFillSetActive()
{
   if (m_bFirstActive)
   {
      m_bFirstActive = false;
   }
   else
   {
      if(!UpdateData(FALSE))
         return FALSE;
   }

   UpdateNoStrandsCtrls();

   return TRUE;
}

BOOL CTogaDirectFillDlg::OnFillKillActive()
{
   return UpdateData(TRUE);
}

void CTogaDirectFillDlg::UpdateNoStrandsCtrls()
{
   StrandIndexType nos = m_pGirderData->GetDirectFilledStraightStrands().GetFilledStrandCount();

   StrandIndexType ndb(0);
   std::vector<CDebondData>::const_iterator debond_iter;
   for ( debond_iter = m_pGirderData->GetDirectFilledStraightDebond().begin(); 
         debond_iter != m_pGirderData->GetDirectFilledStraightDebond().end(); debond_iter++ )
   {
      const CDebondData& debond_info = *debond_iter;

      ndb+= m_pGirderData->GetDirectFilledStraightStrands().GetFillCountAtIndex( debond_info.strandTypeGridIdx );
   }

   CString msg;
   Float64 ecc;
   if (m_pGirderData->ComputeDirectFillEccentricity(&m_GirderLibraryEntry, &ecc))
   {
      ecc = ::ConvertFromSysUnits(ecc, unitMeasure::Inch);
      msg.Format(_T("No. Strands = %d; No. Debonded = %d.\nEccentricity, C.L. = %.3f in"), nos, ndb, ecc);
   }
   else
   {
      msg.Format(_T("No. Strands = %d; No. Debonded = %d."), nos, ndb);
   }

   GetDlgItem(IDC_DIRECT_STATIC)->SetWindowText(msg);
}

void CTogaDirectFillDlg::SetGirderEntryName(LPCTSTR entryName)
{
   if (m_GirderEntryName != entryName)
   {
      m_GirderEntryName = entryName;

      UpdateLibraryData();
      UpdateNoStrandsCtrls();
   }
}

void CTogaDirectFillDlg::SetSpanLength(Float64 length)
{
   m_SpanLength = length;
}

bool  CTogaDirectFillDlg::UpdateLibraryData()
{
   if (m_GirderLibraryEntry.GetName().c_str() != m_GirderEntryName)
   {
      // our library entry must be modfied to match current settings
      const libLibraryEntry* pLibEntry = m_pBrokerRetriever->GetGirderLibrary()->GetEntry(m_GirderEntryName);
      const GirderLibraryEntry* pGdrEntry = dynamic_cast<const GirderLibraryEntry*>(pLibEntry);

      if (pGdrEntry == NULL)
      {
         CString msg, stmp;
         stmp.LoadString(IDS_GDR_ERROR);
         msg.Format(stmp,m_GirderEntryName);
         ::AfxMessageBox(msg);
         return false;
      }

      // Direct fill in toga is only for girders with all straight strands.
      // If we have a section with harped strands, we will need to convert them to straight and 
      // then allow harping of existing straight strands

      GirderLibraryEntry gdr_clone(*pGdrEntry); // a bit inefficient here if not needed, but it's a ui thing
      StrandIndexType nah = pGdrEntry->GetMaxHarpedStrands();
      if(nah > 0)
      {
         MakeHarpedCloneStraight(pGdrEntry, &gdr_clone);

         pGdrEntry = &gdr_clone; // use clone from here on out
      }

      m_GirderLibraryEntry = *pGdrEntry;
   }

   return true;
}