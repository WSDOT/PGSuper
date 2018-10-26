///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

// LiveLoadDistFactorsDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "LLDFFillDlg.h"
#include "LiveLoadDistFactorsDlg.h"

#include <..\htmlhelp\helptopics.hh>
#include <IFace\Project.h>

// CLLDFFillDlg dialog

IMPLEMENT_DYNAMIC(CLLDFFillDlg, CDialog)

CLLDFFillDlg::CLLDFFillDlg(CWnd* pParent /*=NULL*/)
   : CDialog(CLLDFFillDlg::IDD, pParent)
     , m_pBridgeDesc(NULL)
     , m_UserInputValue(1.0)
     , m_GIRDER_SPAN_INT(0)
     , m_GIRDER_GIRDER_INT(0)
     , m_PIER_INT(0)
     , m_PIER_GIRDER_INT(0)
     , m_Method(0)
     , m_ROA_INT(0)
{

}

CLLDFFillDlg::~CLLDFFillDlg()
{
}

void CLLDFFillDlg::DoDataExchange(CDataExchange* pDX)
{
   // Translate df calculation method and action to get correct ordering
   int action;
   if ( !pDX->m_bSaveAndValidate )
   {
      action = GetIntForLldfAction(m_LldfRangeOfApplicabilityAction);
   }

   DDX_CBIndex(pDX, IDC_ROA_CB, action );
   DDX_Radio(pDX, IDC_LLDF_COMPUTE, m_Method );

   if ( pDX->m_bSaveAndValidate )
   {
      m_LldfRangeOfApplicabilityAction = GetLldfActionForInt(action);

      // Only save combo values on way out
      DDX_CBIndex(pDX, IDC_GIRDER_SPAN, m_GIRDER_SPAN_INT);
      DDX_CBIndex(pDX, IDC_GIRDER_GIRDER, m_GIRDER_GIRDER_INT);
      DDX_CBIndex(pDX, IDC_PIER, m_PIER_INT);
      DDX_CBIndex(pDX, IDC_PIER_GIRDER, m_PIER_GIRDER_INT);
      DDX_CBIndex(pDX, IDC_ROA_CB, m_ROA_INT);
   }

   CDialog::DoDataExchange(pDX);

   DDX_Control(pDX, IDC_GIRDER_SPAN, m_SpanCB);
   DDX_Control(pDX, IDC_GIRDER_GIRDER, m_GirderGirderCB);
   DDX_Control(pDX, IDC_PIER, m_PierCB);
   DDX_Control(pDX, IDC_PIER_GIRDER, m_PierGirderCB);

   DDX_Text(pDX, IDC_VALUE, m_UserInputValue);
   DDV_MinMaxDouble(pDX, m_UserInputValue, 0, 1000.00);
}


BEGIN_MESSAGE_MAP(CLLDFFillDlg, CDialog)
   ON_BN_CLICKED(IDC_LLDF_COMPUTE, &CLLDFFillDlg::OnMethod)
   ON_BN_CLICKED(IDC_LLDF_LEVER_RULE, &CLLDFFillDlg::OnMethod)
   ON_BN_CLICKED(IDC_LLDF_SINGLE_VALUE, &CLLDFFillDlg::OnMethod)
   ON_CBN_SELCHANGE(IDC_GIRDER_SPAN, &CLLDFFillDlg::OnCbnSelchangeGirderSpan)
   ON_CBN_SELCHANGE(IDC_PIER, &CLLDFFillDlg::OnCbnSelchangePier)
	ON_COMMAND(ID_HELP, OnHelp)
END_MESSAGE_MAP()


// CLLDFFillDlg message handlers

BOOL CLLDFFillDlg::OnInitDialog()
{
   CDialog::OnInitDialog();

   // fill span and pier combos
   ComputeMaxNumGirders();

   SpanIndexType nspans = m_pBridgeDesc->GetSpanCount();

   CString str;
   m_SpanCB.AddString(_T("None"));
   m_SpanCB.AddString(_T("All"));
   for (SpanIndexType ispan=0; ispan<nspans; ispan++)
   {
      str.Format(_T("%d"),LABEL_SPAN(ispan));
      m_SpanCB.AddString(str);
   }

   m_SpanCB.SelectString(-1,_T("All"));

   m_PierCB.AddString(_T("None"));
   m_PierCB.AddString(_T("All"));
   for (PierIndexType ipier=0; ipier<=nspans; ipier++)
   {
      str.Format(_T("%d"),LABEL_PIER(ipier));
      m_PierCB.AddString(str);
   }

   m_PierCB.SelectString(-1,_T("All"));

   OnCbnSelchangeGirderSpan();
   OnCbnSelchangePier();

   OnMethod();

   return TRUE;  // return TRUE unless you set the focus to a control
   // EXCEPTION: OCX Property Pages should return FALSE
}

void CLLDFFillDlg::OnMethod()
{
   BOOL bEnable = IsDlgButtonChecked(IDC_LLDF_COMPUTE);

   GetDlgItem(IDC_ROA_STATIC)->EnableWindow(bEnable);
   GetDlgItem(IDC_ROA_CB)->EnableWindow(bEnable);

   bEnable = IsDlgButtonChecked(IDC_LLDF_SINGLE_VALUE);
   GetDlgItem(IDC_FILL_STATIC)->EnableWindow(bEnable);
   GetDlgItem(IDC_VALUE)->EnableWindow(bEnable);
}

void CLLDFFillDlg::OnCbnSelchangeGirderSpan()
{
   int sel = m_SpanCB.GetCurSel();
   if(sel!=CB_ERR)
   {
      int gsel = m_GirderGirderCB.GetCurSel();
      gsel = gsel==CB_ERR ? 0:gsel;

      m_GirderGirderCB.ResetContent();

      BOOL enable = sel>0 ? TRUE : FALSE; // disable girder control if "None" selected

      m_GirderGirderCB.EnableWindow(enable);
      GetDlgItem(IDC_GIRDER_GIRDER_STATIC)->EnableWindow(enable);

      if (enable)
      {
         GirderIndexType ngdrs;
         if (sel==1) // All
         {
            ngdrs = m_MaxNumGirders;
         }
         else
         {
            SpanIndexType spidx = sel-2;
            const CSpanData2* pSpan = m_pBridgeDesc->GetSpan(spidx);
            const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(pSpan);
            ngdrs = pGroup->GetGirderCount();
         }

         m_GirderGirderCB.AddString(_T("All"));

         CString str;
         for (GirderIndexType igdr=0; igdr<ngdrs; igdr++)
         {
            str.Format(_T("%s"),LABEL_GIRDER(igdr));
            m_GirderGirderCB.AddString(str);
         }

         // Go back to previous selection if possible
         gsel = gsel>(int)ngdrs ? 0:gsel;
         m_GirderGirderCB.SetCurSel(gsel);
      }
   }
   else
   {
      ATLASSERT(0);
   }
}

void CLLDFFillDlg::OnCbnSelchangePier()
{
   int sel = m_PierCB.GetCurSel();
   if(sel!=CB_ERR)
   {
      int gsel = m_PierGirderCB.GetCurSel();
      gsel = gsel==CB_ERR ? 0:gsel;

      m_PierGirderCB.ResetContent();

      BOOL enable = sel>0 ? TRUE : FALSE; // disable girder control if "None" selected

      m_PierGirderCB.EnableWindow(enable);
      GetDlgItem(IDC_PIER_GIRDER_STATIC)->EnableWindow(enable);

      if (enable)
      {
         GirderIndexType ngdrs;
         if (sel==1) // All
         {
            ngdrs = m_MaxNumGirders;
         }
         else
         {
            PierIndexType pieridx = sel-2;
            const CPierData2* pPier = m_pBridgeDesc->GetPier(pieridx);
            ngdrs = GetPierGirderCount(pPier);
         }

         m_PierGirderCB.AddString(_T("All"));

         CString str;
         for (GirderIndexType igdr=0; igdr<ngdrs; igdr++)
         {
            str.Format(_T("%s"),LABEL_GIRDER(igdr));
            m_PierGirderCB.AddString(str);
         }

         // Go back to previous selection if possible
         gsel = gsel>(int)ngdrs ? 0:gsel;
         m_PierGirderCB.SetCurSel(gsel);
      }
   }
   else
   {
      ATLASSERT(0);
   }
}

void CLLDFFillDlg::ComputeMaxNumGirders()
{
   m_MaxNumGirders = 0;

   SpanIndexType nspans = m_pBridgeDesc->GetSpanCount();
   for (SpanIndexType ispan=0; ispan<nspans; ispan++)
   {
      const CSpanData2* pSpan = m_pBridgeDesc->GetSpan(ispan);
      const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(pSpan);
      GirderIndexType ngdrs = pGroup->GetGirderCount();

      m_MaxNumGirders = Max(m_MaxNumGirders, ngdrs);
   }
}

SpanGirderList CLLDFFillDlg::GetSpanGirders()
{
   SpanGirderList theList;

   int spansel = m_GIRDER_SPAN_INT;
   if (spansel>0)
   {
      if(spansel==1)
      {
         // all spans
         SpanIndexType nspans = m_pBridgeDesc->GetSpanCount();
         for (SpanIndexType ispan=0; ispan<nspans; ispan++)
         {
            int gdrsel = m_GIRDER_GIRDER_INT;
            if (gdrsel==0)
            {
               // all girders
               const CSpanData2* pSpan = m_pBridgeDesc->GetSpan(ispan);
               const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(pSpan);
               GirderIndexType ngdrs = pGroup->GetGirderCount();

               for (GirderIndexType igdr=0; igdr<ngdrs; igdr++)
               {
                  theList.push_back( SpanGirderType(ispan, igdr) );
               }
            }
            else
            {
               // single girder
               GirderIndexType gdr = gdrsel-1;
               theList.push_back( SpanGirderType(ispan, gdr) );
            }
         }
      }
      else
      {
         // single span
         SpanIndexType span = spansel-2;
         int gdrsel = m_GIRDER_GIRDER_INT;
         if (gdrsel==0)
         {
            // all girders
            const CSpanData2* pSpan = m_pBridgeDesc->GetSpan(span);
            const CGirderGroupData* pGroup = m_pBridgeDesc->GetGirderGroup(pSpan);
            GirderIndexType ngdrs = pGroup->GetGirderCount();

            for (GirderIndexType igdr=0; igdr<ngdrs; igdr++)
            {
               theList.push_back( SpanGirderType(span, igdr) );
            }
         }
         else
         {
            // single girder
            GirderIndexType gdr = gdrsel-1;
            theList.push_back( SpanGirderType(span, gdr) );
         }
      }
   }

   return theList;
}

PierGirderList CLLDFFillDlg::GetPierGirders()
{
   PierGirderList theList;

   int Piersel = m_PIER_INT;
   if (Piersel>0)
   {
      if(Piersel==1)
      {
         // all Piers
         PierIndexType nPiers = m_pBridgeDesc->GetPierCount();
         for (PierIndexType iPier=0; iPier<nPiers; iPier++)
         {
            int gdrsel = m_PIER_GIRDER_INT;
            if (gdrsel==0)
            {
               // all girders
               const CPierData2* pPier = m_pBridgeDesc->GetPier(iPier);
               GirderIndexType ngdrs = GetPierGirderCount(pPier);

               for (GirderIndexType igdr=0; igdr<ngdrs; igdr++)
               {
                  theList.push_back( PierGirderType(iPier, igdr) );
               }
            }
            else
            {
               // single girder
               GirderIndexType gdr = gdrsel-1;
               theList.push_back( PierGirderType(iPier, gdr) );
            }
         }
      }
      else
      {
         // single Pier
         PierIndexType Pier = Piersel-2;
         int gdrsel = m_PIER_GIRDER_INT;
         if (gdrsel==0)
         {
            // all girders
            const CPierData2* pPier = m_pBridgeDesc->GetPier(Pier);
            GirderIndexType ngdrs = GetPierGirderCount(pPier);

            for (GirderIndexType igdr=0; igdr<ngdrs; igdr++)
            {
               theList.push_back( PierGirderType(Pier, igdr) );
            }
         }
         else
         {
            // single girder
            GirderIndexType gdr = gdrsel-1;
            theList.push_back( PierGirderType(Pier, gdr) );
         }
      }
   }

   return theList;
}

pgsTypes::DistributionFactorMethod CLLDFFillDlg::GetDistributionFactorMethod()
{
   if(m_Method==0)
   {
      return pgsTypes::Calculated;
   }
   else if(m_Method==1)
   {
      return pgsTypes::LeverRule;
   }
   else
   {
      return pgsTypes::DirectlyInput;
   }
}

LldfRangeOfApplicabilityAction CLLDFFillDlg::GetLldfRangeOfApplicabilityAction()
{
   if(m_ROA_INT==0)
   {
      return roaEnforce;
   }
   else if(m_ROA_INT== 1)
   {
      return roaIgnore;
   }
   else
   {
      return roaIgnoreUseLeverRule;
   }
}

void CLLDFFillDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_FILL_DISTRIBUTION_FACTORS);
}