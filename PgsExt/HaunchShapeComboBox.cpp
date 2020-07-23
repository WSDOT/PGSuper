///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// HaunchShapeComboBox.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include <PgsExt\HaunchShapeComboBox.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

inline CString HaunchAsString(pgsTypes::HaunchShapeType type)
{
   if (type == pgsTypes::hsSquare)
   {
      return _T("Square Corners");
   }
   else
   {
      return _T("Filleted Corners");
   }
}


CHaunchShapeComboBox::CHaunchShapeComboBox()
{
   m_HaunchShape = pgsTypes::hsSquare;
}

void CHaunchShapeComboBox::Initialize(pgsTypes::HaunchShapeType type)
{
   m_HaunchShape = type;

   ResetContent();

   int sqidx = AddString( HaunchAsString(pgsTypes::hsSquare));
   SetItemData(sqidx,(DWORD)pgsTypes::hsSquare);

   int fidx = AddString(  HaunchAsString(pgsTypes::hsFilleted) );
   SetItemData(fidx,(DWORD)pgsTypes::hsFilleted);
}

void CHaunchShapeComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   // NOTE: need to learn how to draw using the windows style engine
   // so that the combo box matches the rest of the UI

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsTypes::HaunchShapeType HaunchShapeType = (pgsTypes::HaunchShapeType)(lpDrawItemStruct->itemData);
   CString strText = HaunchAsString(HaunchShapeType);

   CDC dc;
   dc.Attach(lpDrawItemStruct->hDC);

   // save these values to restore them when done drawing
   COLORREF crOldTextColor = dc.GetTextColor();
   COLORREF crOldBkColor   = dc.GetBkColor();

   // If this item is selected, set the background color
   // and the text color to appropriate values, Erase
   // the rect by filling it with the background color.
   DWORD dwRop = SRCCOPY;
   if ((lpDrawItemStruct->itemAction & ODA_SELECT) &&
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
   CBitmap bmpSquare, bmpFilleted;
   CDC dcMemory;

   dcMemory.CreateCompatibleDC(&dc);
   BITMAP bmpInfo;

   int xSrc(0), ySrc(0);
   
   if ( HaunchShapeType == pgsTypes::hsSquare )
   {
      bmpSquare.LoadBitmap(IDB_HAUNCH_SQUARE);
      dcMemory.SelectObject(&bmpSquare);
      bmpSquare.GetBitmap(&bmpInfo);
   }
   else if ( HaunchShapeType == pgsTypes::hsFilleted )
   {
      bmpFilleted.LoadBitmap(IDB_HAUNCH_FILLETED);
      dcMemory.SelectObject(&bmpFilleted);
      bmpFilleted.GetBitmap(&bmpInfo);
   }

   CRect rcItem = lpDrawItemStruct->rcItem;
   int xDest = rcItem.left + 2;
   int yDest = rcItem.top  + 2;

   dc.BitBlt(xDest,yDest,bmpInfo.bmWidth,bmpInfo.bmHeight,&dcMemory,xSrc,ySrc,dwRop);

   int xText = xDest + bmpInfo.bmWidth + 10;
   int yText = yDest;
   dc.TextOut(xText, yText, strText);

   // reset the dc
   dc.SetTextColor(crOldTextColor);
   dc.SetBkColor(crOldBkColor);

   dc.Detach();
}
