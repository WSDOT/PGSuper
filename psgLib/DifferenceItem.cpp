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

#include "StdAfx.h"
#include <PsgLib/DifferenceItem.h>
#include <PsgLib/GirderLabel.h>

using namespace PGS::Library;

DifferenceItem::DifferenceItem(LPCTSTR lpszItem) :
m_Item(lpszItem)
{
}

/////////////////////////////////////////////
DifferenceStringItem::DifferenceStringItem(LPCTSTR lpszItem,LPCTSTR lpszOldValue,LPCTSTR lpszNewValue) :
DifferenceItem(lpszItem),
m_OldValue(lpszOldValue),
m_NewValue(lpszNewValue)
{
}

void DifferenceStringItem::GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const
{
   *pItem = m_Item;
   *pOldValue = m_OldValue;
   *pNewValue = m_NewValue;
}

/////////////////////////////////////////////
DifferenceIndexItem::DifferenceIndexItem(LPCTSTR lpszItem,IndexType oldValue,IndexType newValue) :
DifferenceItem(lpszItem),
m_OldValue(oldValue),
m_NewValue(newValue)
{
}

void DifferenceIndexItem::GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const
{
   *pItem = m_Item;
   pOldValue->Format(_T("%d"),LABEL_INDEX(m_OldValue));
   pNewValue->Format(_T("%d"),LABEL_INDEX(m_NewValue));
}

/////////////////////////////////////////////
DifferenceDoubleItem::DifferenceDoubleItem(LPCTSTR lpszItem,Float64 oldValue,Float64 newValue) :
DifferenceItem(lpszItem),
m_OldValue(oldValue),
m_NewValue(newValue)
{
}

void DifferenceDoubleItem::GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const
{
   *pItem = m_Item;
   pOldValue->Format(_T("%f"),m_OldValue);
   pNewValue->Format(_T("%f"),m_NewValue);
}

/////////////////////////////////////////////
DifferenceBooleanItem::DifferenceBooleanItem(LPCTSTR lpszItem,bool oldValue,bool newValue,LPCTSTR strTrue,LPCTSTR strFalse) :
DifferenceItem(lpszItem),
m_OldValue(oldValue),
m_NewValue(newValue),
m_strTrue(strTrue),
m_strFalse(strFalse)
{
}

void DifferenceBooleanItem::GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const
{
   *pItem = m_Item;
   pOldValue->Format(_T("%s"),m_OldValue ? m_strTrue : m_strFalse);
   pNewValue->Format(_T("%s"),m_NewValue ? m_strTrue : m_strFalse);
}
