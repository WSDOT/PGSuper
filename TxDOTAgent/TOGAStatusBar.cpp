///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2021  Washington State Department of Transportation
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

#include "stdafx.h"
#include "TOGAStatusBar.h"
#include <EAF\EAFResources.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
   EAFID_INDICATOR_AUTOSAVE_ON,
   EAFID_INDICATOR_MODIFIED,
   ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};


BOOL CTOGAStatusBar::Create(CWnd* pParentWnd, DWORD dwStyle, UINT nID)
{
#if _WBFL_VERSION < 330
   // we have to override the __super class method as it will crash without a status center indicato
   // the problem is fixed in WBFL 3.3.0
   BOOL bResult = CStatusBar::Create(pParentWnd, dwStyle, nID);
   if ( !bResult )
      return bResult;

   const UINT* pIndicators;
   int nIndicators;
   GetStatusIndicators(&pIndicators,&nIndicators);
   bResult = SetIndicators(pIndicators,nIndicators);

   // not using status indicator... this will crash in __super class
   //SetPaneStyle( GetStatusPaneIndex(), SBPS_NORMAL | SBT_OWNERDRAW );
   //SetPaneStyle( GetModifiedPaneIndex(), SBPS_DISABLED );
   SetPaneStyle( 1, SBPS_DISABLED ); // GetModifiedPaneIndex() doesn't work correctly and it isn't virtual... 
                                     // just use the known index (and don't change it!)

   for ( int i = 0; i < nIndicators; i++ )
   {
      UINT style = GetPaneStyle(i);
      style |= SBPS_NOBORDERS;
      SetPaneStyle(i,style);
   }

   return bResult;
#else
   return __super::Create(pParentWnd,dwStyle,nID);
#endif
}

#if _WBFL_VERSION < 330
void CTOGAStatusBar::EnableModifiedFlag(BOOL bEnable)
{
   // _super class version doesn't work property because GetModifedPaneIndex() isn't going to return the
   // correct result
   UINT style = (bEnable ? SBPS_NORMAL : SBPS_DISABLED) | SBPS_NOBORDERS;
   SetPaneStyle(1, style );
}
#endif

void CTOGAStatusBar::GetStatusIndicators(const UINT** lppIDArray,int* pnIDCount)
{
   *lppIDArray = indicators;
   *pnIDCount = sizeof(indicators)/sizeof(UINT);
}
