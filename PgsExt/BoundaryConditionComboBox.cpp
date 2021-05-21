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

// BoundaryConditionComboBox.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include <PgsExt\BoundaryConditionComboBox.h>
#include <PgsExt\PierData2.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CBoundaryConditionComboBox::CBoundaryConditionComboBox()
{
   m_PierType = PIERTYPE_INTERMEDIATE;
}

void CBoundaryConditionComboBox::SetPierType(int pierType)
{
   m_PierType = pierType;
}

int CBoundaryConditionComboBox::AddBoundaryCondition(pgsTypes::BoundaryConditionType type)
{
   int idx = AddString( CPierData2::AsString(type,m_bNoDeck) );
   SetItemData(idx,(DWORD)type);
   return idx;
}

void CBoundaryConditionComboBox::Initialize(bool bIsBoundaryPier,const std::vector<pgsTypes::BoundaryConditionType>& connections,bool bNoDeck)
{
   m_bIsBoundaryPier = bIsBoundaryPier;
   m_bNoDeck = bNoDeck;

   int curSel = GetCurSel();
   pgsTypes::BoundaryConditionType currentType;
   if ( curSel != CB_ERR )
   {
      currentType = (pgsTypes::BoundaryConditionType)GetItemData(curSel);
   }

   ResetContent();

   int currentTypeIdx = CB_ERR;
   std::vector<pgsTypes::BoundaryConditionType>::const_iterator iter;
   for ( iter = connections.begin(); iter != connections.end(); iter++ )
   {
      pgsTypes::BoundaryConditionType type = *iter;
      int idx = AddBoundaryCondition(type);
      if ( curSel != CB_ERR && currentType == type )
      {
         currentTypeIdx = idx;
      }
   }

   if ( currentTypeIdx == CB_ERR )
   {
      SetCurSel(0);
   }
   else
   {
      SetCurSel(currentTypeIdx);
   }
}

void CBoundaryConditionComboBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   // NOTE: need to learn how to draw using the windows style engine
   // so that the combo box matches the rest of the UI

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   pgsTypes::BoundaryConditionType boundaryConditionType = (pgsTypes::BoundaryConditionType)(lpDrawItemStruct->itemData);
   CString strText = CPierData2::AsString(boundaryConditionType, m_bNoDeck);

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
   
   if ( boundaryConditionType == pgsTypes::bctHinge )
   {
      bmpHinges.LoadBitmap(m_bIsBoundaryPier ? IDB_HINGES : IDB_IP_HINGES);
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
   else if ( boundaryConditionType == pgsTypes::bctRoller )
   {
      bmpRollers.LoadBitmap(m_bIsBoundaryPier ? IDB_ROLLERS : IDB_IP_ROLLERS);
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

      if ( boundaryConditionType == pgsTypes::bctContinuousBeforeDeck || boundaryConditionType == pgsTypes::bctContinuousAfterDeck )
      {
         xSrc = 0;
      }
      else if ( boundaryConditionType == pgsTypes::bctIntegralBeforeDeck || boundaryConditionType == pgsTypes::bctIntegralAfterDeck )
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
      else if ( boundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeBack || boundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeBack )
      {
         ASSERT(m_PierType == PIERTYPE_INTERMEDIATE);
         xSrc = 5*bmWidth;
      }
      else if ( boundaryConditionType == pgsTypes::bctIntegralAfterDeckHingeAhead || boundaryConditionType == pgsTypes::bctIntegralBeforeDeckHingeAhead )
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

BEGIN_MESSAGE_MAP(CBoundaryConditionComboBox, CComboBox)
   ON_CONTROL_REFLECT(CBN_DROPDOWN, &CBoundaryConditionComboBox::OnCbnDropdown)
END_MESSAGE_MAP()


void CBoundaryConditionComboBox::OnCbnDropdown()
{
    // Reset the dropped width
    int nNumEntries = GetCount();
    int nWidth = 0;
    CString str;

    CClientDC dc(this);
    int nSave = dc.SaveDC();
    dc.SelectObject(GetFont());

    int nScrollWidth = ::GetSystemMetrics(SM_CXVSCROLL);
    for (int i = 0; i < nNumEntries; i++)
    {
        GetLBText(i, str);
        int nLength = dc.GetTextExtent(str).cx + nScrollWidth;
        nWidth = max(nWidth, nLength);
    }
    
    // Add margin space to the calculations
    nWidth += 20*dc.GetTextExtent(CString(_T("0"))).cx;

    dc.RestoreDC(nSave);
    SetDroppedWidth(nWidth);
}
