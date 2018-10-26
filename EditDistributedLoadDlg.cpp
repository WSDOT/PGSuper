///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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
// EditDistributedLoadDlg.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "EditDistributedLoadDlg.h"

#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <EAF\EAFDisplayUnits.h>
#include <MfcTools\CustomDDX.h>
#include <System\Tokenizer.h>
#include <..\htmlhelp\HelpTopics.hh>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditDistributedLoadDlg dialog


CEditDistributedLoadDlg::CEditDistributedLoadDlg(CDistributedLoadData load, IBroker* pBroker,CWnd* pParent /*=NULL*/)
	: CDialog(CEditDistributedLoadDlg::IDD, pParent),
   m_Load(load),
   m_pBroker(pBroker)
{
	//{{AFX_DATA_INIT(CEditDistributedLoadDlg)
	//}}AFX_DATA_INIT
}


void CEditDistributedLoadDlg::DoDataExchange(CDataExchange* pDX)
{
   const unitForcePerLength& usForce = unitMeasure::KipPerFoot;
   const unitForcePerLength& siForce = unitMeasure::KilonewtonPerMeter;

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditDistributedLoadDlg)
	DDX_Control(pDX, IDC_SPAN_LENGTH_CTRL, m_SpanLengthCtrl);
	DDX_Control(pDX, IDC_LOCATION_UNITS, m_LocationUnitCtrl);
	DDX_Control(pDX, IDC_RIGHT_LOCATION, m_RightLocationCtrl);
	DDX_Control(pDX, IDC_LEFT_LOCATION, m_LeftLocationCtrl);
	DDX_Control(pDX, IDC_LOADTYPE, m_LoadTypeCB);
	DDX_Control(pDX, IDC_FRACTIONAL, m_FractionalCtrl);
	DDX_Control(pDX, IDC_GIRDERS, m_GirderCB);
	DDX_Control(pDX, IDC_SPANS, m_SpanCB);
	DDX_Control(pDX, IDC_STAGE, m_StageCB);
	DDX_Control(pDX, IDC_LOADCASE, m_LoadCaseCB);
	//}}AFX_DATA_MAP
	DDX_String(pDX, IDC_DESCRIPTION, m_Load.m_Description);

   // magnitude is easy part
   DDX_UnitValueAndTag( pDX, IDC_LEFT_MAGNITUDE, IDC_MAGNITUDE_UNITS, m_Load.m_WStart, m_bUnitsSI, usForce, siForce );
   DDX_UnitValueAndTag( pDX, IDC_RIGHT_MAGNITUDE, IDC_MAGNITUDE_UNITS2, m_Load.m_WEnd, m_bUnitsSI, usForce, siForce );

   // other values need to be done manually
   if (pDX->m_bSaveAndValidate)
   {
      m_Load.m_LoadCase = UserLoads::GetLoadCase(m_LoadCaseCB.GetCurSel());

      if (m_Load.m_LoadCase != UserLoads::LL_IM)
      {
         m_Load.m_Stage    = UserLoads::GetStage(m_StageCB.GetCurSel());
      }
      else
      {
         m_Load.m_Stage    = UserLoads::BridgeSite3;
      }

      int ival = m_SpanCB.GetCurSel();

      if (ival == m_SpanCB.GetCount()-1)
         m_Load.m_Span = ALL_SPANS;
      else
         m_Load.m_Span = ival;

      ival = m_GirderCB.GetCurSel();
      if (ival == m_GirderCB.GetCount()-1 )
         m_Load.m_Girder = ALL_GIRDERS;
      else
         m_Load.m_Girder = ival;

      // first check if load is uniform or trapezoidal (much more work for trapezoidal)
      m_Load.m_Type    = UserLoads::GetDistributedLoadType(m_LoadTypeCB.GetCurSel());

      if (m_Load.m_Type!=UserLoads::Uniform)
      {
         // location takes some effort
         double lft_locval, rgt_locval;
         CString str;
         m_LeftLocationCtrl.GetWindowText(str);
         if (!sysTokenizer::ParseDouble(str, &lft_locval))
         {
      	   HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LEFT_LOCATION);
            ::AfxMessageBox(_T("Please enter a number"));
            pDX->Fail();
         }

         m_RightLocationCtrl.GetWindowText(str);
         if (!sysTokenizer::ParseDouble(str, &rgt_locval))
         {
      	   HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_RIGHT_LOCATION);
            ::AfxMessageBox(_T("Please enter a number"));
            pDX->Fail();
         }

         if (lft_locval >= rgt_locval)
         {
      	   HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LEFT_LOCATION);
            ::AfxMessageBox(_T("Invalid Value: The left location value must be greater than the right location value"));
            pDX->Fail();
         }

         m_Load.m_Fractional = m_FractionalCtrl.GetCheck()!=FALSE;

         if (m_Load.m_Fractional)
         {
            if (lft_locval>=0.0 && lft_locval<=1.0)
            {
               m_Load.m_StartLocation = lft_locval;
            }
            else
            {
      	      HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LEFT_LOCATION);
               ::AfxMessageBox(_T("Invalid Value: Fractional values must range from 0.0 to 1.0"));
               pDX->Fail();
            }

            if (rgt_locval>=0.0 && rgt_locval<=1.0)
            {
               m_Load.m_EndLocation = rgt_locval;
            }
            else
            {
      	      HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_RIGHT_LOCATION);
               ::AfxMessageBox(_T("Invalid Value: Fractional values must range from 0.0 to 1.0"));
               pDX->Fail();
            }
         }
         else
         {
            if (lft_locval>=0.0)
            {
               m_Load.m_StartLocation = ::ConvertToSysUnits(lft_locval, *m_pLengthUnit);
            }
            else
            {
      	      HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LEFT_LOCATION);
               ::AfxMessageBox(_T("Invalid Value: Location values must be zero or greater"));
               pDX->Fail();
            }

            if (rgt_locval>=0.0)
            {
               m_Load.m_EndLocation = ::ConvertToSysUnits(rgt_locval, *m_pLengthUnit);
            }
            else
            {
      	      HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_RIGHT_LOCATION);
               ::AfxMessageBox(_T("Invalid Value: Location values must be zero or greater"));
               pDX->Fail();
            }
         }
      }
   }
}


BEGIN_MESSAGE_MAP(CEditDistributedLoadDlg, CDialog)
	//{{AFX_MSG_MAP(CEditDistributedLoadDlg)
	ON_BN_CLICKED(IDC_FRACTIONAL, OnFractional)
	ON_CBN_SELCHANGE(IDC_LOADCASE, OnEditchangeLoadcase)
	ON_CBN_SELCHANGE(IDC_LOADTYPE, OnEditchangeLoadtype)
	ON_CBN_SELCHANGE(IDC_SPANS, OnEditchangeSpans)
	ON_CBN_SELCHANGE(IDC_GIRDERS, OnEditchangeGirders)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditDistributedLoadDlg message handlers

BOOL CEditDistributedLoadDlg::OnInitDialog() 
{
   // units
   GET_IFACE(IEAFDisplayUnits,pDisplayUnits);
   m_bUnitsSI = IS_SI_UNITS(pDisplayUnits);

   if (m_bUnitsSI)
      m_pLengthUnit = &unitMeasure::Meter;
   else
      m_pLengthUnit = &unitMeasure::Feet;

	CDialog::OnInitDialog();
	
	// fill up controls
   // stages, load cases

   for (Int32 ilc=0; ilc<UserLoads::GetNumLoadCases(); ilc++)
   {
      CString str(UserLoads::GetLoadCaseName(ilc).c_str());
      m_LoadCaseCB.AddString(str);
   }

   m_LoadCaseCB.SetCurSel(m_Load.m_LoadCase);

   m_WasLiveLoad = m_Load.m_LoadCase == UserLoads::LL_IM;

   UpdateStageLoadCase(true);

   // load types
   for (Int32 ilt=0; ilt<UserLoads::GetNumDistributedLoadTypes(); ilt++)
   {
      CString str(UserLoads::GetDistributedLoadTypeName(ilt).c_str());
      m_LoadTypeCB.AddString(str);
   }

   m_LoadTypeCB.SetCurSel(m_Load.m_Type);

   // spans, girders
   GET_IFACE(IBridge, pBridge);
   SpanIndexType nSpans = pBridge->GetSpanCount();

   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++)
   {
      CString str;
      str.Format(_T("Span %d"), LABEL_SPAN(spanIdx));
      m_SpanCB.AddString(str);
   }

    m_SpanCB.AddString(_T("All Spans"));

    if (m_Load.m_Span==ALL_SPANS)
    {
       m_SpanCB.SetCurSel((int)nSpans);
    }
    else
    {
      if ( 0 <= m_Load.m_Span && m_Load.m_Span < nSpans)
      {
         m_SpanCB.SetCurSel((int)m_Load.m_Span);
      }
      else
      {
         ::AfxMessageBox(_T("Warning - The Span for this load is out of range. Resetting to Span 1"));
         m_Load.m_Span = 0;
         m_SpanCB.SetCurSel((int)m_Load.m_Span);
      }
    }

   UpdateGirderList();

    if (m_Load.m_Girder==ALL_GIRDERS)
    {
       m_GirderCB.SetCurSel( m_GirderCB.GetCount()-1 );
    }
    else
    {
      if (0 <= m_Load.m_Girder && m_Load.m_Girder < GirderIndexType(m_GirderCB.GetCount()-1) )
      {
         m_GirderCB.SetCurSel((int)m_Load.m_Girder);
      }
      else
      {
         ::AfxMessageBox(_T("Warning - The Girder for this load is out of range. Resetting to girder A"));
         m_Load.m_Girder=0;
         m_GirderCB.SetCurSel((int)m_Load.m_Girder);
      }
    }
   
   m_FractionalCtrl.SetCheck(m_Load.m_Fractional);

   if (m_Load.m_Fractional)
   {
      sysNumericFormatTool tool;
      m_LeftLocationCtrl.SetWindowText(tool.AsString(m_Load.m_StartLocation).c_str());
      m_RightLocationCtrl.SetWindowText(tool.AsString(m_Load.m_EndLocation).c_str());
   }
   else
   {
      sysNumericFormatTool tool;
      Float64 val = ::ConvertFromSysUnits(m_Load.m_StartLocation, *m_pLengthUnit);
      m_LeftLocationCtrl.SetWindowText(tool.AsString(val).c_str());

      val = ::ConvertFromSysUnits(m_Load.m_EndLocation, *m_pLengthUnit);
      m_RightLocationCtrl.SetWindowText(tool.AsString(val).c_str());
   }

   UpdateLocationUnit();
   UpdateLoadType();
   UpdateSpanLength();

   AFX_MANAGE_STATE(AfxGetStaticModuleState());
   HICON hIcon = (HICON)LoadImage(AfxGetResourceHandle(),MAKEINTRESOURCE(IDI_DISTRIBUTED_LOAD),IMAGE_ICON,0,0,LR_DEFAULTSIZE);
   SetIcon(hIcon,FALSE);

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditDistributedLoadDlg::UpdateLocationUnit()
{
	int chk = m_FractionalCtrl.GetCheck();

   if (chk)
   {
      m_LocationUnitCtrl.SetWindowText(_T("(0.0 - 1.0)"));
   }
   else
   {
      m_LocationUnitCtrl.SetWindowText(m_pLengthUnit->UnitTag().c_str());
   }
}

void CEditDistributedLoadDlg::UpdateStageLoadCase(bool isInitial)
{
   if(m_LoadCaseCB.GetCurSel()==UserLoads::LL_IM)
   {
      m_StageCB.ResetContent();
      CString str(UserLoads::GetStageName(UserLoads::BridgeSite3).c_str());
      m_StageCB.AddString(str);
      m_StageCB.SetCurSel(0);
      m_StageCB.EnableWindow(FALSE);

      m_WasLiveLoad = true;
   }
   else
   {
      if (isInitial || m_WasLiveLoad)
      {
         m_StageCB.ResetContent();
         for (Int32 istg=0; istg<UserLoads::GetNumStages(); istg++)
         {
            CString str(UserLoads::GetStageName(istg).c_str());
            m_StageCB.AddString(str);
         }

         m_StageCB.EnableWindow(TRUE);

         if (isInitial)
         {
            m_StageCB.SetCurSel(m_Load.m_Stage);
         }
         else
         {
            m_StageCB.SetCurSel(0);
         }
      }
  
      m_WasLiveLoad = false;
   }
}

void CEditDistributedLoadDlg::OnFractional() 
{
   UpdateLocationUnit();
}

void CEditDistributedLoadDlg::OnEditchangeLoadcase() 
{
   UpdateStageLoadCase();
}

void CEditDistributedLoadDlg::OnEditchangeLoadtype() 
{
   UpdateLoadType();
}

void CEditDistributedLoadDlg::UpdateLoadType()
{
   int sel = m_LoadTypeCB.GetCurSel();

   // need to hide a bunch of stuff if uniform
   int cmdshw = (UserLoads::GetDistributedLoadType(sel) == UserLoads::Uniform)? SW_HIDE : SW_SHOW;

   CWnd* pst = this->GetDlgItem(IDC_LEFT_ENDSTATIC);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_RIGHT_END_STATIC);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_RIGHT_MAGNITUDE);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_RIGHT_MAGNITUDE);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_MAGNITUDE_UNITS);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_LOCATION_STATIC);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_LEFT_LOCATION);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_RIGHT_LOCATION);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_LOCATION_UNITS);
   pst->ShowWindow(cmdshw);
   pst = this->GetDlgItem(IDC_FRACTIONAL);
   pst->ShowWindow(cmdshw);
   
   // units for uniform
   int cmdswt = cmdshw==SW_HIDE? SW_SHOW:SW_HIDE;
   pst = this->GetDlgItem(IDC_MAGNITUDE_UNITS2);
   pst->ShowWindow(cmdswt);
}


void CEditDistributedLoadDlg::OnEditchangeSpans() 
{
   UpdateGirderList();
   UpdateSpanLength();
}

void CEditDistributedLoadDlg::OnEditchangeGirders() 
{
   UpdateSpanLength();
}

void CEditDistributedLoadDlg::UpdateSpanLength() 
{
	int spn = m_SpanCB.GetCurSel();
	int gdr = m_GirderCB.GetCurSel();

   if (spn == m_SpanCB.GetCount()-1 || gdr == m_GirderCB.GetCount()-1)
   {
      m_SpanLengthCtrl.SetWindowText(_T("N/A"));
   }
   else
   {
      GET_IFACE(IBridge, pBridge);
      Float64 span_length = pBridge->GetSpanLength(spn, gdr);
      Float64 val = ::ConvertFromSysUnits(span_length, *m_pLengthUnit);
      sysNumericFormatTool tool;
      std::_tstring str = tool.AsString(val) + std::_tstring(_T(" ")) +  m_pLengthUnit->UnitTag();
      m_SpanLengthCtrl.SetWindowText(str.c_str());
   }
}

void CEditDistributedLoadDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_EDIT_DISTRIBUTED_LOADS );
}

void CEditDistributedLoadDlg::UpdateGirderList()
{
   GET_IFACE(IBridge, pBridge);
   SpanIndexType spanIdx = m_SpanCB.GetCurSel();
   SpanIndexType nSpans = pBridge->GetSpanCount();

   int curSel = m_GirderCB.GetCurSel();
   BOOL bAllSelected = (curSel == m_GirderCB.GetCount()-1);
   m_GirderCB.ResetContent();

   GirderIndexType nMaxGirders = 9999;
   if ( spanIdx == nSpans )
   {
      // loading applies to all spans
      // need to find the span with the fewest girders
      for ( SpanIndexType i = 0; i < nSpans; i++ )
      {
         GirderIndexType cGirders = pBridge->GetGirderCount(i);
         if ( cGirders < nMaxGirders )
         {
            nMaxGirders = cGirders;
            spanIdx = i;
         }
      }
   }
   else
   {
      nMaxGirders = pBridge->GetGirderCount(spanIdx);
   }

   GirderIndexType nGirders = nMaxGirders;

   for (GirderIndexType gdrIdx = 0; gdrIdx < nGirders; gdrIdx++)
   {
      CString str;
      str.Format(_T("Girder %s"), LABEL_GIRDER(gdrIdx));
      m_GirderCB.AddString(str);
   }

    m_GirderCB.AddString(_T("All Girders"));
    if ( curSel != CB_ERR )
    {
       if ( bAllSelected )
       {
         m_GirderCB.SetCurSel( m_GirderCB.GetCount()-1 );
       }
       else
       {
          if ( m_GirderCB.GetCount()-1 == curSel )
             curSel = 0;

         curSel = m_GirderCB.SetCurSel( curSel );
       }
    }

    if ( curSel == CB_ERR )
       m_GirderCB.SetCurSel(0);
}
