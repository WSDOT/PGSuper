///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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
// EditMomentLoadDlg.cpp : implementation file
//

#include "stdafx.h"
#include "pgsuper.h"
#include "EditMomentLoadDlg.h"
#include <IFace\Bridge.h>
#include <IFace\Project.h>
#include <MfcTools\CustomDDX.h>
#include <System\Tokenizer.h>
#include <..\htmlhelp\HelpTopics.hh>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditMomentLoadDlg dialog


CEditMomentLoadDlg::CEditMomentLoadDlg(CMomentLoadData load, IBroker* pBroker, CWnd* pParent /*=NULL*/):
	CDialog(CEditMomentLoadDlg::IDD, pParent),
   m_Load(load),
   m_pBroker(pBroker)
{
	//{{AFX_DATA_INIT(CEditMomentLoadDlg)
	//}}AFX_DATA_INIT
}


void CEditMomentLoadDlg::DoDataExchange(CDataExchange* pDX)
{
   const unitMoment& usForce = unitMeasure::KipFeet;
   const unitMoment& siForce = unitMeasure::KilonewtonMeter;

   if ( !pDX->m_bSaveAndValidate )
   {
      CComboBox* pCB = (CComboBox*)GetDlgItem(IDC_GIRDER_END);
      m_LocationIdx = IsZero(m_Load.m_Location) ? 0 : 1;
   }

	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CEditMomentLoadDlg)
	DDX_Control(pDX, IDC_SPAN_LENGTH_CTRL, m_SpanLengthCtrl);
	DDX_Control(pDX, IDC_LOCATION, m_LocationCtrl);
	DDX_Control(pDX, IDC_LOCATION_UNITS, m_LocationUnitCtrl);
	DDX_Control(pDX, IDC_FRACTIONAL, m_FractionalCtrl);
	DDX_Control(pDX, IDC_GIRDERS, m_GirderCB);
	DDX_Control(pDX, IDC_STAGE, m_StageCB);
	DDX_Control(pDX, IDC_SPANS, m_SpanCB);
	DDX_Control(pDX, IDC_LOADCASE, m_LoadCaseCB);
	//}}AFX_DATA_MAP
	DDX_String(pDX, IDC_DESCRIPTION, m_Load.m_Description);
   DDX_CBIndex(pDX,IDC_GIRDER_END,m_LocationIdx);

   // magnitude is easy part
   DDX_UnitValueAndTag( pDX, IDC_MAGNITUDE, IDC_MAGNITUDE_UNITS, m_Load.m_Magnitude, m_bUnitsSI, usForce, siForce );

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
         m_Load.m_Span = UserLoads::AllSpans;
      else
         m_Load.m_Span = ival;

      ival = m_GirderCB.GetCurSel();
      if (ival == m_GirderCB.GetCount()-1)
         m_Load.m_Girder = UserLoads::AllGirders;
      else
         m_Load.m_Girder = ival;

      // location takes some effort
      double locval;
      CString str;
      m_LocationCtrl.GetWindowText(str);
      if (!sysTokenizer::ParseDouble(str, &locval))
      {
      	HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LOCATION);
         ::AfxMessageBox("Please enter a number");
         pDX->Fail();
      }

      m_Load.m_Fractional = m_FractionalCtrl.GetCheck()!=FALSE;

      if (m_Load.m_Fractional)
      {
         if (locval>=0.0 && locval<=1.0)
         {
            m_Load.m_Location = locval;
         }
         else
         {
      	   HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LOCATION);
            ::AfxMessageBox("Invalid Value: Fractional values must range from 0.0 to 1.0");
            pDX->Fail();
         }
      }
      else
      {
         if (locval>=0.0)
         {
            m_Load.m_Location = ::ConvertToSysUnits(locval, *m_pLengthUnit);
         }
         else
         {
      	   HWND hWndCtrl = pDX->PrepareEditCtrl(IDC_LOCATION);
            ::AfxMessageBox("Invalid Value: Location values must be zero or greater");
            pDX->Fail();
         }
      }

      // this must always be a fractional measure at the start or end of the girder
      if ( m_LocationIdx == 0 )
      {
         m_Load.m_Location = 0;
         m_Load.m_Fractional = true;
      }
      else
      {
         m_Load.m_Location = 1.0;
         m_Load.m_Fractional = true;
      }
   }
}


BEGIN_MESSAGE_MAP(CEditMomentLoadDlg, CDialog)
	//{{AFX_MSG_MAP(CEditMomentLoadDlg)
	ON_BN_CLICKED(IDC_FRACTIONAL, OnFractional)
	ON_CBN_SELCHANGE(IDC_LOADCASE, OnEditchangeLoadcase)
	ON_CBN_SELCHANGE(IDC_SPANS, OnEditchangeSpans)
	ON_CBN_SELCHANGE(IDC_GIRDERS, OnEditchangeGirders)
	ON_BN_CLICKED(ID_HELP, OnHelp)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEditMomentLoadDlg message handlers

BOOL CEditMomentLoadDlg::OnInitDialog() 
{
   // units
   GET_IFACE(IProjectSettings,pProjSettings);
   Int32 units = pProjSettings->GetUnitsMode();
   m_bUnitsSI = (units == pgsTypes::umSI) ? true : false;

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

   // spans, girders
   GET_IFACE(IBridge, pBridge);
   SpanIndexType nSpans   = pBridge->GetSpanCount();

   for (SpanIndexType spanIdx = 0; spanIdx < nSpans; spanIdx++ )
   {
      CString str;
      str.Format("Span %d", LABEL_SPAN(spanIdx));
      m_SpanCB.AddString(str);
   }

   m_SpanCB.AddString("All Spans");

   if (m_Load.m_Span==UserLoads::AllSpans)
   {
      m_SpanCB.SetCurSel(nSpans);
   }
   else
   {
      if ( 0 <= m_Load.m_Span && m_Load.m_Span < nSpans)
      {
         m_SpanCB.SetCurSel(m_Load.m_Span);
      }
      else
      {
         ::AfxMessageBox("Warning - The Span for this load is out of range. Resetting to Span 1");
         m_Load.m_Span = 0;
         m_SpanCB.SetCurSel(m_Load.m_Span);
      }
   }

   UpdateGirderList();

   if (m_Load.m_Girder==UserLoads::AllGirders)
   {
      m_GirderCB.SetCurSel(m_GirderCB.GetCount()-1);
   }
   else
   {
      if (0 <= m_Load.m_Girder && m_Load.m_Girder < GirderIndexType(m_GirderCB.GetCount()-1) )
      {
         m_GirderCB.SetCurSel(m_Load.m_Girder);
      }
      else
      {
         ::AfxMessageBox("Warning - The Girder for this load is out of range. Resetting to girder A");
         m_Load.m_Girder=0;
         m_GirderCB.SetCurSel(m_Load.m_Girder);
      }
   }

   // location
   m_FractionalCtrl.SetCheck(m_Load.m_Fractional);

   if (m_Load.m_Fractional)
   {
      sysNumericFormatTool tool;
      m_LocationCtrl.SetWindowText(tool.AsString(m_Load.m_Location).c_str());
   }
   else
   {
      Float64 val = ::ConvertFromSysUnits(m_Load.m_Location, *m_pLengthUnit);
      sysNumericFormatTool tool;
      m_LocationCtrl.SetWindowText(tool.AsString(val).c_str());
   }

   UpdateLocationUnit();
   UpdateStageLoadCase();
   UpdateSpanLength();

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CEditMomentLoadDlg::OnFractional() 
{
   UpdateLocationUnit();
}

void CEditMomentLoadDlg::UpdateLocationUnit()
{
	int chk = m_FractionalCtrl.GetCheck();
   if (chk)
   {
      m_LocationUnitCtrl.SetWindowText("(0.0 - 1.0)");
   }
   else
   {
      m_LocationUnitCtrl.SetWindowText(m_pLengthUnit->UnitTag().c_str());
   }
}

void CEditMomentLoadDlg::UpdateStageLoadCase(bool isInitial)
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

void CEditMomentLoadDlg::OnEditchangeLoadcase() 
{
   UpdateStageLoadCase();
}

void CEditMomentLoadDlg::OnEditchangeSpans() 
{
   UpdateGirderList();
   UpdateSpanLength();
}

void CEditMomentLoadDlg::OnEditchangeGirders() 
{
   UpdateSpanLength();
}

void CEditMomentLoadDlg::UpdateSpanLength() 
{
	int spn = m_SpanCB.GetCurSel();
	int gdr = m_GirderCB.GetCurSel();

   if (spn == m_SpanCB.GetCount()-1 || gdr == m_GirderCB.GetCount()-1)
   {
      m_SpanLengthCtrl.SetWindowText("N/A");
   }
   else
   {
      GET_IFACE(IBridge, pBridge);
      Float64 span_length = pBridge->GetSpanLength(spn, gdr);
      Float64 val = ::ConvertFromSysUnits(span_length, *m_pLengthUnit);
      sysNumericFormatTool tool;
      std::string str = tool.AsString(val) + std::string(" ") +  m_pLengthUnit->UnitTag();
      m_SpanLengthCtrl.SetWindowText(str.c_str());
   }
}

void CEditMomentLoadDlg::OnHelp() 
{
   ::HtmlHelp( *this, AfxGetApp()->m_pszHelpFilePath, HH_HELP_CONTEXT, IDH_EDIT_MOMENT_LOADS );
}

void CEditMomentLoadDlg::UpdateGirderList()
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
      str.Format("Girder %s", LABEL_GIRDER(gdrIdx));
      m_GirderCB.AddString(str);
   }

    m_GirderCB.AddString("All Girders");
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
