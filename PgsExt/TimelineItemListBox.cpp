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

// BoundaryConditionComboBox.cpp : implementation file
//

#include <PgsExt\PgsExtLib.h>
#include "resource.h"
#include <PgsExt\TimelineItemListBox.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// NOTE: The list box resource most have the owner draw and has strings attributes set.

CTimelineItemListBox::CTimelineItemListBox()
{
}

BEGIN_MESSAGE_MAP(CTimelineItemListBox,CListBox)
   ON_WM_DESTROY()
END_MESSAGE_MAP()

void CTimelineItemListBox::Initialize(ListType listType, KeyType keyType,CTimelineItemListBox* pBuddy)
{
   m_ListType = listType;
   m_KeyType  = keyType;
   m_pBuddy   = pBuddy;
}

int CTimelineItemListBox::AddItem(LPCTSTR strText,CTimelineItemDataPtr::State state,IndexType index)
{
   ATLASSERT(m_KeyType == CTimelineItemListBox::Index);
   int idx = AddString(strText);
   CTimelineItemIndexDataPtr* pItemDataPtr = new CTimelineItemIndexDataPtr;
   pItemDataPtr->m_State = state;
   pItemDataPtr->m_Index = index;
   SetItemDataPtr(idx,(void*)pItemDataPtr);
   return idx;
}

int CTimelineItemListBox::AddItem(LPCTSTR strText,CTimelineItemDataPtr::State state,IDType id)
{
   ATLASSERT(m_KeyType == CTimelineItemListBox::ID);
   int idx = AddString(strText);
   CTimelineItemIDDataPtr* pItemDataPtr = new CTimelineItemIDDataPtr;
   pItemDataPtr->m_State = state;
   pItemDataPtr->m_ID = id;
   SetItemDataPtr(idx,(void*)pItemDataPtr);
   return idx;
}

int CTimelineItemListBox::AddItem(LPCTSTR strText,CTimelineItemDataPtr* pItemData)
{
   int idx = AddString(strText);
   SetItemDataPtr(idx,(void*)pItemData);
   return idx;
}

void CTimelineItemListBox::MoveSelectedItemsToBuddy()
{
   int nCount = GetSelCount();
   CArray<int,int> arrSelected;
   arrSelected.SetSize(nCount);
   GetSelItems(nCount,arrSelected.GetData());

   for ( int i = 0; i < nCount; i++ )
   {
      int sel = arrSelected.GetAt(i);

      CString strLabel;
      GetText(sel,strLabel);
      CTimelineItemDataPtr* pItemDataPtr = (CTimelineItemDataPtr*)GetItemDataPtr(sel);

      if ( m_pBuddy->m_ListType == CTimelineItemListBox::Target )
      {
         // when moving item into a target list, it becomes a "used" item
         pItemDataPtr->m_State = CTimelineItemDataPtr::Used;
      }
      else
      {
         // when moving item into a source list, it becomes "unused"
         pItemDataPtr->m_State = CTimelineItemDataPtr::Unused;
      }

      int idx = m_pBuddy->AddString(strLabel);
      m_pBuddy->SetItemDataPtr(idx,(void*)pItemDataPtr);
   }

   // removes items from this list
   for ( int i = nCount-1; 0 <= i; i-- )
   {
      DeleteString(arrSelected.GetAt(i));
   }
}

void CTimelineItemListBox::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   if ( lpDrawItemStruct->itemID == LB_ERR )
   {
      return;
   }

   CDC dc;
   dc.Attach(lpDrawItemStruct->hDC);
   COLORREF crOldTextColor = dc.GetTextColor();
   COLORREF crOldBkColor = dc.GetBkColor();

   CString strText;
   GetText(lpDrawItemStruct->itemID,strText);

   CTimelineItemDataPtr* pItemData = (CTimelineItemDataPtr*)GetItemDataPtr(lpDrawItemStruct->itemID);

   // If this item is selected, set the background color
   // and the text color to appropreate values. Also, erase
   // rect by filling it with the background color
   if ((lpDrawItemStruct->itemAction | ODA_SELECT) &&
       (lpDrawItemStruct->itemState & ODS_SELECTED) )
   {
      dc.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
      dc.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
      dc.FillSolidRect(&lpDrawItemStruct->rcItem,::GetSysColor(COLOR_HIGHLIGHT));
   }
   else
   {
      dc.FillSolidRect(&lpDrawItemStruct->rcItem,crOldBkColor);
   }

   if ( m_ListType == Source && pItemData->m_State == CTimelineItemDataPtr::Used )
   {
      dc.SetTextColor(::GetSysColor(COLOR_GRAYTEXT));
   }


   dc.DrawText(strText.LockBuffer(),strText.GetLength(),&lpDrawItemStruct->rcItem,DT_LEFT|DT_SINGLELINE);

   dc.SetTextColor(crOldTextColor);
   dc.SetBkColor(crOldBkColor);
   dc.Detach();
}

void CTimelineItemListBox::OnDestroy()
{
   int nItems = GetCount();
   for ( int i = 0; i < nItems; i++ )
   {
      CTimelineItemDataPtr* pItemData = (CTimelineItemDataPtr*)GetItemDataPtr(i);
      delete pItemData;
      pItemData = nullptr;
      SetItemDataPtr(i,nullptr);
   }

   m_pBuddy = nullptr;
}
