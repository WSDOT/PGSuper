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

#ifndef INCLUDED_STATUSCENTERIMP_H_
#define INCLUDED_STATUSCENTERIMP_H_

#include <IFace\StatusCenter.h>
#include <set>
#include <map>

class iStatusCenterEventSink
{
public:
   virtual void OnStatusItemAdded(pgsStatusItem* pItem) = 0;
   virtual void OnStatusItemRemoved(StatusItemIDType id) = 0;
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

   AgentIDType GetAgentID();
   StatusItemIDType Add(pgsStatusItem* pItem);
   bool RemoveByID(StatusItemIDType id);
   bool RemoveByIndex(CollectionIndexType index);
   bool RemoveByAgentID(AgentIDType id);
   pgsStatusItem* GetByID(StatusItemIDType id);
   pgsStatusItem* GetByIndex(CollectionIndexType index);
   CollectionIndexType Count();

   pgsTypes::StatusSeverityType GetSeverity();

   StatusCallbackIDType RegisterCallbackItem(iStatusCallback* pCallback);
   pgsTypes::StatusSeverityType GetSeverity(StatusCallbackIDType callbackID);

   void EditItem(CollectionIndexType index);

   void SinkEvents(iStatusCenterEventSink* pSink);
   void UnSinkEvents(iStatusCenterEventSink* pSink);

private:
   AgentIDType m_NextAgentID;
   StatusItemIDType m_NextID;
   StatusCallbackIDType m_NextCallbackID;
   pgsStatusItem* m_pCurrentItem;

   typedef std::set<pgsStatusItem*,StatusItemCompare> Container;
   Container m_Items;

   typedef std::set<iStatusCenterEventSink*> Sinks;
   Sinks m_Sinks;

   typedef std::map<StatusCallbackIDType,iStatusCallback*> Callbacks;
   Callbacks m_Callbacks;

   iStatusCallback* GetCallback(StatusCallbackIDType callbackID);

   void NotifyAdded(pgsStatusItem* pNewItem);
   void NotifyRemoved(StatusItemIDType id);
};

#endif // INCLUDED_STATUSCENTERIMP_H_