///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#include <PgsExt\PgsExtExp.h>

class PGSEXTCLASS CTimelineItemDataPtr
{
public:
   typedef enum State { Used, Unused } State;
   State m_State;
};

class PGSEXTCLASS CTimelineItemIndexDataPtr : public CTimelineItemDataPtr
{
public:
   IndexType m_Index;
};

class PGSEXTCLASS CTimelineItemIDDataPtr : public CTimelineItemDataPtr
{
public:
   IDType m_ID;
};

class PGSEXTCLASS CTimelineItemListBox : public CListBox
{
public:
   typedef enum ListType { Source, Target } ListType;
   typedef enum KeyType { Index, ID } KeyType;

   CTimelineItemListBox();
   void Initialize(ListType listType, KeyType keyType,CTimelineItemListBox* pBuddy);

   int AddItem(LPCTSTR strText,CTimelineItemDataPtr::State state,IndexType index);
   int AddItem(LPCTSTR strText,CTimelineItemDataPtr::State state,IDType id);
   int AddItem(LPCTSTR strText,CTimelineItemDataPtr* pItemData);

   void MoveSelectedItemsToBuddy();

   virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct) override;

   afx_msg void OnDestroy();

   DECLARE_MESSAGE_MAP()

private:
   CTimelineItemListBox* m_pBuddy;
   ListType  m_ListType;
   KeyType   m_KeyType;
};
