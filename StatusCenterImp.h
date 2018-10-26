///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_STATUSCENTERIMP_H_
#define INCLUDED_STATUSCENTERIMP_H_

#include <IFace\StatusCenter.h>
#include <set>
#include <map>

class iStatusCenterEventSink
{
public:
   virtual void OnStatusItemAdded(pgsStatusItem* pItem) = 0;
   virtual void OnStatusItemRemoved(long id) = 0;
};

#define STATUS_OK       0
#define STATUS_WARNING  1
#define STATUS_ERROR    2

class iStatusCallback
{
public:
   virtual long GetSeverity() = 0;
   virtual void Execute(pgsStatusItem* pItem) = 0;
};

class StatusItemCompare
{
public:
   bool operator()(pgsStatusItem* a,pgsStatusItem* b);
};

class pgsStatusCenter
{
public:
   pgsStatusCenter();
   ~pgsStatusCenter();

   Int32 GetAgentID();
   Int32 Add(pgsStatusItem* pItem);
   bool RemoveByID(Int32 id);
   bool RemoveByIndex(Uint32 index);
   bool RemoveByAgentID(Int32 id);
   pgsStatusItem* GetByID(Int32 id);
   pgsStatusItem* GetByIndex(Uint32 index);
   Uint32 Count();

   Uint32 GetSeverity();

   void RegisterCallbackItem(Uint32 callbackID,iStatusCallback* pCallback);
   Uint32 GetSeverity(Uint32 callbackID);

   void EditItem(Uint32 index);

   void SinkEvents(iStatusCenterEventSink* pSink);
   void UnSinkEvents(iStatusCenterEventSink* pSink);

private:
   Int32 m_NextAgentID;
   Int32 m_NextID;
   pgsStatusItem* m_pCurrentItem;

   typedef std::set<pgsStatusItem*,StatusItemCompare> Container;
   Container m_Items;

   typedef std::set<iStatusCenterEventSink*> Sinks;
   Sinks m_Sinks;

   typedef std::map<Int32,iStatusCallback*> Callbacks;
   Callbacks m_Callbacks;

   iStatusCallback* GetCallback(long callbackID);

   void NotifyAdded(pgsStatusItem* pNewItem);
   void NotifyRemoved(Int32 id);
};

#endif // INCLUDED_STATUSCENTERIMP_H_