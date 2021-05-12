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

// LibraryEntryConflict.cpp : implementation file
//

#include "stdafx.h"
#include <psgLib\LibraryEntryDifferenceItem.h>

#include <PgsExt\GirderLabel.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////
pgsLibraryEntryDifferenceItem::pgsLibraryEntryDifferenceItem(LPCTSTR lpszItem) :
m_Item(lpszItem)
{
}

/////////////////////////////////////////////
pgsLibraryEntryDifferenceStringItem::pgsLibraryEntryDifferenceStringItem(LPCTSTR lpszItem,LPCTSTR lpszOldValue,LPCTSTR lpszNewValue) :
pgsLibraryEntryDifferenceItem(lpszItem),
m_OldValue(lpszOldValue),
m_NewValue(lpszNewValue)
{
}

void pgsLibraryEntryDifferenceStringItem::GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const
{
   *pItem = m_Item;
   *pOldValue = m_OldValue;
   *pNewValue = m_NewValue;
}

/////////////////////////////////////////////
pgsLibraryEntryDifferenceIndexItem::pgsLibraryEntryDifferenceIndexItem(LPCTSTR lpszItem,IndexType oldValue,IndexType newValue) :
pgsLibraryEntryDifferenceItem(lpszItem),
m_OldValue(oldValue),
m_NewValue(newValue)
{
}

void pgsLibraryEntryDifferenceIndexItem::GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const
{
   *pItem = m_Item;
   pOldValue->Format(_T("%d"),LABEL_INDEX(m_OldValue));
   pNewValue->Format(_T("%d"),LABEL_INDEX(m_NewValue));
}

/////////////////////////////////////////////
pgsLibraryEntryDifferenceDoubleItem::pgsLibraryEntryDifferenceDoubleItem(LPCTSTR lpszItem,Float64 oldValue,Float64 newValue) :
pgsLibraryEntryDifferenceItem(lpszItem),
m_OldValue(oldValue),
m_NewValue(newValue)
{
}

void pgsLibraryEntryDifferenceDoubleItem::GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const
{
   *pItem = m_Item;
   pOldValue->Format(_T("%f"),m_OldValue);
   pNewValue->Format(_T("%f"),m_NewValue);
}

/////////////////////////////////////////////
pgsLibraryEntryDifferenceBooleanItem::pgsLibraryEntryDifferenceBooleanItem(LPCTSTR lpszItem,bool oldValue,bool newValue,LPCTSTR strTrue,LPCTSTR strFalse) :
pgsLibraryEntryDifferenceItem(lpszItem),
m_OldValue(oldValue),
m_NewValue(newValue),
m_strTrue(strTrue),
m_strFalse(strFalse)
{
}

void pgsLibraryEntryDifferenceBooleanItem::GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const
{
   *pItem = m_Item;
   pOldValue->Format(_T("%s"),m_OldValue ? m_strTrue : m_strFalse);
   pNewValue->Format(_T("%s"),m_NewValue ? m_strTrue : m_strFalse);
}
