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

// PGSuperStatusBar.cpp : implementation file
//

#include "PGSuperAppPlugin\stdafx.h"
#include "resource.h"
#include "pgsuper.h"
#include "PGSuperStatusBar.h"
#include "PGSuperColors.h"
#include <IFace\StatusCenter.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static UINT indicators[] =
{
   ID_SEPARATOR,           // status line indicator
   ID_INDICATOR_ANALYSIS,
   ID_INDICATOR_STATUS,
   ID_INDICATOR_MODIFIED,
   ID_INDICATOR_AUTOCALC_ON,
   ID_INDICATOR_CAPS,
   ID_INDICATOR_NUM,
   ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CPGSuperStatusBar

CPGSuperStatusBar::CPGSuperStatusBar()
{
   m_AnalysisModePaneIdx = -1;
   m_AutoCalcPaneIdx     = -1;
}

CPGSuperStatusBar::~CPGSuperStatusBar()
{
}

BEGIN_MESSAGE_MAP(CPGSuperStatusBar, CEAFStatusBar)
	//{{AFX_MSG_MAP(CPGSuperStatusBar)
#if !defined _EAF_USING_MFC_FEATURE_PACK
	ON_WM_LBUTTONDBLCLK()
#endif
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CPGSuperStatusBar::GetStatusIndicators(const UINT** lppIDArray,int* pnIDCount)
{
   *lppIDArray = indicators;
   *pnIDCount = sizeof(indicators)/sizeof(UINT);
}

/////////////////////////////////////////////////////////////////////////////
// CPGSuperStatusBar message handlers

#if !defined _EAF_USING_MFC_FEATURE_PACK
void CPGSuperStatusBar::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
   CRect rect;

   // Analysis Type
   GetStatusBarCtrl().GetRect(GetAnalysisModePaneIndex(),&rect);
   if (rect.PtInRect(point))
   {
      PostMessage(WM_COMMAND,ID_PROJECT_ANALYSIS,0);
   }

   // AutoCalc Mode
   GetStatusBarCtrl().GetRect(GetAutoCalcPaneIndex(),&rect);
   if (rect.PtInRect(point))
   {
      PostMessage(WM_COMMAND,ID_PROJECT_AUTOCALC,0);
   }

   CEAFStatusBar::OnLButtonDblClk(nFlags, point);
}
#endif

int CPGSuperStatusBar::GetAnalysisModePaneIndex()
{
   if ( m_AnalysisModePaneIdx < 0 )
   {
      for ( int i = 0; i < GetPaneCount(); i++ )
      {
         UINT nID;
         UINT nStyle;
         int cxWidth;
         GetPaneInfo(i,nID,nStyle,cxWidth);

         if ( nID == ID_INDICATOR_ANALYSIS )
         {
            m_AnalysisModePaneIdx = i;
            break;
         }
      }
   }

   return m_AnalysisModePaneIdx;
}

int CPGSuperStatusBar::GetAutoCalcPaneIndex()
{
   if ( m_AutoCalcPaneIdx < 0 )
   {
      for ( int i = 0; i < GetPaneCount(); i++ )
      {
         UINT nID;
         UINT nStyle;
         int cxWidth;
         GetPaneInfo(i,nID,nStyle,cxWidth);

         if ( nID == ID_INDICATOR_AUTOCALC_ON )
         {
            m_AutoCalcPaneIdx = i;
            break;
         }
      }
   }

   return m_AutoCalcPaneIdx;
}

void CPGSuperStatusBar::Reset()
{
   CEAFStatusBar::Reset();

   int idx = GetAnalysisModePaneIndex();
   SetPaneText( idx, _T("") );

   idx = GetAutoCalcPaneIndex();
   SetPaneText( idx, _T("") );
}

void CPGSuperStatusBar::AutoCalcEnabled( bool bEnable )
{
   CString status_text;
   CString tip_text;
   if ( bEnable )
   {
      status_text.LoadString(ID_INDICATOR_AUTOCALC_ON);
      tip_text = _T("Automatic Calculation Mode is enabled. All output views will be automatically refreshed whenever the input parameters change. Double click to toggle Auto-Calculation Mode");
   }
   else
   {
      status_text.LoadString(ID_INDICATOR_AUTOCALC_OFF);
      tip_text = _T("Automatic Calculation Mode is disabled. Output views will be marked as out-of-date whenever the input parameters change. Double click to toggle Auto-Calculation Mode");
   }

   int idx = GetAutoCalcPaneIndex();
   SetPaneText(idx, status_text, TRUE);
#if defined _EAF_USING_MFC_FEATURE_PACK
   SetTipText(idx,tip_text);
#endif
}

void CPGSuperStatusBar::SetAnalysisTypeStatusIndicator(pgsTypes::AnalysisType analysisType)
{
   CString strAnalysisType;
   CString tip_text;
   switch( analysisType )
   {
   case pgsTypes::Simple:
      strAnalysisType = _T("Simple Span");
      tip_text = _T("The bridge will be analyzed as a simple span structure regardless of boundary conditions. Double click to modify the Analysis Mode");
      break;
   case pgsTypes::Continuous:
      strAnalysisType = _T("Continuous");
      tip_text = _T("The bridge will be analyzed as simple spans made continuous. Double click to modify the Analysis Mode");
      break;
   case pgsTypes::Envelope:
      strAnalysisType = _T("Envelope");
      tip_text = _T("The bridge will be analyzed as simple spans and simple spans made continuous. The governing results will be used. Double click to modify the Analysis Mode");
      break;
   }

   int idx = GetAnalysisModePaneIndex();
   SetPaneText(idx,strAnalysisType,TRUE);
#if defined _EAF_USING_MFC_FEATURE_PACK
   SetTipText(idx,tip_text);
#endif
}
