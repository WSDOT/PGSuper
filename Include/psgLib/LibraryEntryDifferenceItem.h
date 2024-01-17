///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#pragma once

#include "psgLibLib.h"
#include <MFCTools\Format.h>
#include <Units\Units.h>


class PSGLIBCLASS pgsLibraryEntryDifferenceItem
{
public:
   pgsLibraryEntryDifferenceItem(LPCTSTR lpszItem);
   virtual void GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const = 0;

protected:
   CString m_Item;
};

class PSGLIBCLASS pgsLibraryEntryDifferenceStringItem : public pgsLibraryEntryDifferenceItem
{
public:
   pgsLibraryEntryDifferenceStringItem(LPCTSTR lpszItem,LPCTSTR lpszOldValue,LPCTSTR lpszNewValue);
   virtual void GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const override;

protected:
   CString m_OldValue;
   CString m_NewValue;
};

class PSGLIBCLASS pgsLibraryEntryDifferenceIndexItem : public pgsLibraryEntryDifferenceItem
{
public:
   pgsLibraryEntryDifferenceIndexItem(LPCTSTR lpszItem,IndexType oldValue,IndexType newValue);
   virtual void GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const override;

protected:
   IndexType m_OldValue;
   IndexType m_NewValue;
};

class PSGLIBCLASS pgsLibraryEntryDifferenceDoubleItem : public pgsLibraryEntryDifferenceItem
{
public:
   pgsLibraryEntryDifferenceDoubleItem(LPCTSTR lpszItem,Float64 oldValue,Float64 newValue);
   virtual void GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const override;

protected:
   Float64 m_OldValue;
   Float64 m_NewValue;
};

class PSGLIBCLASS pgsLibraryEntryDifferenceBooleanItem : public pgsLibraryEntryDifferenceItem
{
public:
   pgsLibraryEntryDifferenceBooleanItem(LPCTSTR lpszItem,bool oldValue,bool newValue,LPCTSTR strTrue=_T("true"),LPCTSTR strFalse=_T("false"));
   virtual void GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const override;

protected:
   bool m_OldValue;
   bool m_NewValue;
   CString m_strTrue; // string for true state, could be "true", "checked", "enabled", etc.
   CString m_strFalse;
};

template <class U>
class pgsLibraryEntryDifferenceUnitValueItemT : public pgsLibraryEntryDifferenceItem
{
public:
   pgsLibraryEntryDifferenceUnitValueItemT(LPCTSTR lpszItem,Float64 oldValue,Float64 newValue,const U& unit) :
   pgsLibraryEntryDifferenceItem(lpszItem),
   m_OldValue(oldValue),
   m_NewValue(newValue),
   m_Unit(unit)
   {
   }

   virtual void GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const override
   {
      *pItem = m_Item;
      pOldValue->Format(_T("%s"),::FormatDimension(m_OldValue,m_Unit));
      pNewValue->Format(_T("%s"),::FormatDimension(m_NewValue,m_Unit));
   }

protected:
   Float64 m_OldValue;
   Float64 m_NewValue;
   const U& m_Unit;
};

template <class U>
class pgsLibraryEntryDifferenceUnitValueKeywordItemT : public pgsLibraryEntryDifferenceUnitValueItemT<U>
{
public:
   pgsLibraryEntryDifferenceUnitValueKeywordItemT(LPCTSTR lpszItem,Float64 oldValue,Float64 newValue,const U& unit,LPCTSTR lpszKeyword) :
   pgsLibraryEntryDifferenceUnitValueItemT<U>(lpszItem,oldValue,newValue,unit),
   m_strKeyword(lpszKeyword)
   {
   }

   virtual void GetConflict(CString* pItem,CString* pOldValue,CString* pNewValue) const override
   {
      *pItem = this->m_Item;
      if ( this->m_OldValue < 0 )
      {
         pOldValue->Format(_T("%s"),m_strKeyword);
      }
      else
      {
         pOldValue->Format(_T("%s"),::FormatDimension(this->m_OldValue,this->m_Unit));
      }

      if ( this->m_NewValue < 0 )
      {
         pNewValue->Format(_T("%s"),m_strKeyword);
      }
      else
      {
         pNewValue->Format(_T("%s"),::FormatDimension(this->m_NewValue,this->m_Unit));
      }
   }

protected:
   CString m_strKeyword;
};

#define DECLARE_UNIT_VALUE_DIFFERENCE_ITEM(u,t) \
   PSGLIBTPL pgsLibraryEntryDifferenceUnitValueItemT<u>; \
   using t = pgsLibraryEntryDifferenceUnitValueItemT<u>;

DECLARE_UNIT_VALUE_DIFFERENCE_ITEM(WBFL::Units::LengthData,pgsLibraryEntryDifferenceLengthItem);
DECLARE_UNIT_VALUE_DIFFERENCE_ITEM(WBFL::Units::ForceData,pgsLibraryEntryDifferenceForceItem);
DECLARE_UNIT_VALUE_DIFFERENCE_ITEM(WBFL::Units::ForcePerLengthData,pgsLibraryEntryDifferenceForcePerLengthItem);
DECLARE_UNIT_VALUE_DIFFERENCE_ITEM(WBFL::Units::StressData,pgsLibraryEntryDifferenceStressItem);
DECLARE_UNIT_VALUE_DIFFERENCE_ITEM(WBFL::Units::DensityData,pgsLibraryEntryDifferenceDensityItem);


#define DECLARE_UNIT_VALUE_KEYWORD_DIFFERENCE_ITEM(u,t) \
   PSGLIBTPL pgsLibraryEntryDifferenceUnitValueKeywordItemT<u>; \
   using t = pgsLibraryEntryDifferenceUnitValueKeywordItemT<u>;

DECLARE_UNIT_VALUE_KEYWORD_DIFFERENCE_ITEM(WBFL::Units::LengthData,pgsLibraryEntryDifferenceLengthKeywordItem);

#define RETURN_ON_DIFFERENCE if ( bReturnOnFirstDifference ) { ATLASSERT(vDifferences.size() == 0); return false; }
