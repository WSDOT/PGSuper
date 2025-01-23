///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

// ColumnFixityComboBox.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include <PgsExt\ColumnFixityComboBox.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CColumnFixityComboBox::CColumnFixityComboBox(UINT fixity) :
m_Fixity(fixity)
{
}

void CColumnFixityComboBox::SetFixityTypes(UINT fixity)
{
   m_Fixity = fixity;
   UpdateFixity();
}

int CColumnFixityComboBox::AddFixity(pgsTypes::ColumnLongitudinalBaseFixityType fixityType)
{
   int idx = AddString( fixityType == pgsTypes::cftFixed ? _T("Fixed") : _T("Pinned") );
   SetItemData(idx,(DWORD)fixityType);
   return idx;
}

void CColumnFixityComboBox::PreSubclassWindow()
{
   CComboBox::PreSubclassWindow();
   UpdateFixity();
}

void CColumnFixityComboBox::UpdateFixity()
{
   if ( GetSafeHwnd() )
   {
      if ( WBFL::System::Flags<UINT>::IsSet(m_Fixity,COLUMN_FIXITY_FIXED) )
      {
         AddFixity(pgsTypes::cftFixed);
      }

      if ( WBFL::System::Flags<UINT>::IsSet(m_Fixity,COLUMN_FIXITY_PINNED) )
      {
         AddFixity(pgsTypes::cftPinned);
      }
   }
}

void CColumnFixityComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   // NOTE: need to learn how to draw using the windows style engine
   // so that the combo box matches the rest of the UI

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsTypes::ColumnLongitudinalBaseFixityType fixityType = (pgsTypes::ColumnLongitudinalBaseFixityType)(lpDrawItemStruct->itemData);
   CString strText = (fixityType == pgsTypes::cftFixed ? _T("Fixed") : _T("Pinned"));

   CDC dc;
   dc.Attach(lpDrawItemStruct->hDC);

   // save these values to restore them when done drawing
   COLORREF crOldTextColor = dc.GetTextColor();
   COLORREF crOldBkColor   = dc.GetBkColor();

   // If this item is selected, set the background color
   // and the text color is appropriate values, Erase
   // the rect by filling it with the background color.
   DWORD dwRop = SRCCOPY;
   if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
       (lpDrawItemStruct->itemState  & ODS_SELECTED))
   {
      dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
      dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
      dc.FillSolidRect(&lpDrawItemStruct->rcItem,::GetSysColor(COLOR_HIGHLIGHT));
      dwRop = MERGEPAINT;
   }
   else
   {
      dc.FillSolidRect(&lpDrawItemStruct->rcItem,crOldBkColor);
      dwRop = SRCCOPY;
   }

   // draw the text
   CBitmap bmpFixity;
   CDC dcMemory;

   dcMemory.CreateCompatibleDC(&dc);
   BITMAP bmpInfo;

   int xSrc, ySrc;
   ySrc = 0;
   
   bmpFixity.LoadBitmap(IDB_COLUMN_FIXITY);
   dcMemory.SelectObject(&bmpFixity);
   bmpFixity.GetBitmap(&bmpInfo);
   int bmWidth = bmpInfo.bmHeight+1;
   xSrc = ((int)fixityType)*bmWidth;

   CRect rcItem = lpDrawItemStruct->rcItem;
   int xDest = rcItem.left + 2;
   int yDest = rcItem.top  + 2;

   dc.BitBlt(xDest,yDest,bmpInfo.bmHeight,bmpInfo.bmHeight,&dcMemory,xSrc,ySrc,dwRop);

   int xText = xDest + bmpInfo.bmHeight + 5;
   int yText = yDest;
   dc.TextOut(xText, yText, strText);

   // reset the dc
   dc.SetTextColor(crOldTextColor);
   dc.SetBkColor(crOldBkColor);

   dc.Detach();
}
