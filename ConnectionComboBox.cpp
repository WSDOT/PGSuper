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

// ConnectionComboBox.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PGSuperAppPlugin\PGSuperApp.h"
#include "ConnectionComboBox.h"
#include <PgsExt\PierData.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CConnectionComboBox::CConnectionComboBox()
{
   m_PierType = PIERTYPE_INTERMEDIATE;
}

void CConnectionComboBox::SetPierType(int pierType)
{
   m_PierType = pierType;
}

void CConnectionComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   pgsTypes::PierConnectionType connectionType = (pgsTypes::PierConnectionType)(lpDrawItemStruct->itemData);
   CString strText = CPierData::AsString(connectionType);

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
   CBitmap bmpHinges, bmpRollers, bmpContinuous;
   CDC dcMemory;

   dcMemory.CreateCompatibleDC(&dc);
   BITMAP bmpInfo;

   int xSrc, ySrc;
   ySrc = 0;
   
   if ( connectionType == pgsTypes::Hinged )
   {
      bmpHinges.LoadBitmap(IDB_HINGES);
      dcMemory.SelectObject(&bmpHinges);
      bmpHinges.GetBitmap(&bmpInfo);

      int bmWidth = bmpInfo.bmHeight + 1;

      switch( m_PierType )
      {
      case PIERTYPE_START:
         xSrc = bmWidth;
         break;

      case PIERTYPE_INTERMEDIATE:
         xSrc = 0;
         break;

      case PIERTYPE_END:
         xSrc = 2*bmWidth;
         break;
      }
   }
   else if ( connectionType == pgsTypes::Roller )
   {
      bmpRollers.LoadBitmap(IDB_ROLLERS);
      dcMemory.SelectObject(&bmpRollers);
      bmpRollers.GetBitmap(&bmpInfo);

      int bmWidth = bmpInfo.bmHeight + 1;

      switch( m_PierType )
      {
      case PIERTYPE_START:
         xSrc = bmWidth;
         break;

      case PIERTYPE_INTERMEDIATE:
         xSrc = 0;
         break;

      case PIERTYPE_END:
         xSrc = 2*bmWidth;
         break;
      }
   }
   else
   {
      bmpContinuous.LoadBitmap(IDB_CONTINUOUS);
      dcMemory.SelectObject(&bmpContinuous);
      bmpContinuous.GetBitmap(&bmpInfo);

      int bmWidth = bmpInfo.bmHeight + 1;

      if ( connectionType == pgsTypes::ContinuousBeforeDeck || connectionType == pgsTypes::ContinuousAfterDeck )
      {
         ASSERT(m_PierType == PIERTYPE_INTERMEDIATE);
         xSrc = 0;
      }
      else if ( connectionType == pgsTypes::IntegralBeforeDeck || connectionType == pgsTypes::IntegralAfterDeck )
      {
         switch( m_PierType )
         {
         case PIERTYPE_START:
            xSrc = 2*bmWidth;
            break;

         case PIERTYPE_INTERMEDIATE:
            xSrc = bmWidth;
            break;

         case PIERTYPE_END:
            xSrc = 3*bmWidth;
            break;
         }
      }
      else if ( connectionType == pgsTypes::IntegralAfterDeckHingeBack || connectionType == pgsTypes::IntegralBeforeDeckHingeBack )
      {
         ASSERT(m_PierType == PIERTYPE_INTERMEDIATE);
         xSrc = 5*bmWidth;
      }
      else if ( connectionType == pgsTypes::IntegralAfterDeckHingeAhead || connectionType == pgsTypes::IntegralBeforeDeckHingeAhead )
      {
         ASSERT(m_PierType == PIERTYPE_INTERMEDIATE);
         xSrc = 4*bmWidth;
      }
      else
      {
         ASSERT(0); // ??? should never get here
      }
   }

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
