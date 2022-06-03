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

#ifndef INCLUDED_INSERTDELETELOAD_H_
#define INCLUDED_INSERTDELETELOAD_H_
#include <PgsExt\PgsExtExp.h>

#include <System\Transaction.h>
#include <IFace\Project.h>
#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>
#include <PgsExt\TimelineManager.h>

class PGSEXTCLASS txnInsertPointLoad : public txnTransaction
{
public:
   txnInsertPointLoad(const CPointLoadData& loadData,EventIDType loadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnInsertPointLoad();
   virtual txnTransaction* CreateClone() const override;
   virtual std::_tstring Name() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() override;
   virtual bool IsRepeatable() override;

private:
   CollectionIndexType m_LoadIdx;
   CPointLoadData m_LoadData;
   EventIDType m_LoadingEventID;
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};

class PGSEXTCLASS txnDeletePointLoad : public txnTransaction
{
public:
   txnDeletePointLoad(LoadIDType loadID);
   virtual std::_tstring Name() const override;
   virtual txnTransaction* CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() override;
   virtual bool IsRepeatable() override;

private:
   LoadIDType m_LoadID;
   CPointLoadData m_LoadData;
   EventIDType m_LoadingEventID;
};

class PGSEXTCLASS txnEditPointLoad : public txnTransaction
{
public:
   txnEditPointLoad(LoadIDType loadID,const CPointLoadData& oldLoadData,EventIDType oldLoadingEventID,const CPointLoadData& newLoadData,EventIDType newLoadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnEditPointLoad();
   virtual std::_tstring Name() const override;
   virtual txnTransaction* CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() override;
   virtual bool IsRepeatable() override;

private:
   void DoExecute(int i);
   LoadIDType m_LoadID;
   CPointLoadData m_LoadData[2];
   EventIDType m_LoadingEventID[2];
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};

class PGSEXTCLASS txnInsertDistributedLoad : public txnTransaction
{
public:
   txnInsertDistributedLoad(const CDistributedLoadData& loadData,EventIDType loadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnInsertDistributedLoad();
   virtual txnTransaction* CreateClone() const override;
   virtual std::_tstring Name() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() override;
   virtual bool IsRepeatable() override;

private:
   CollectionIndexType m_LoadIdx;
   CDistributedLoadData m_LoadData;
   EventIDType m_LoadingEventID;
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};

class PGSEXTCLASS txnDeleteDistributedLoad : public txnTransaction
{
public:
   txnDeleteDistributedLoad(LoadIDType loadID);
   virtual std::_tstring Name() const override;
   virtual txnTransaction* CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() override;
   virtual bool IsRepeatable() override;

private:
   LoadIDType m_LoadID;
   CDistributedLoadData m_LoadData;
   EventIDType m_LoadingEventID;
};

class PGSEXTCLASS txnEditDistributedLoad : public txnTransaction
{
public:
   txnEditDistributedLoad(LoadIDType loadID,const CDistributedLoadData& oldLoadData,EventIDType oldLoadingEventID,const CDistributedLoadData& newLoadData,EventIDType newLoadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnEditDistributedLoad();
   virtual std::_tstring Name() const override;
   virtual txnTransaction* CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() override;
   virtual bool IsRepeatable() override;

private:
   void DoExecute(int i);
   LoadIDType m_LoadID;
   CDistributedLoadData m_LoadData[2];
   EventIDType m_LoadingEventID[2];
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};

class PGSEXTCLASS txnInsertMomentLoad : public txnTransaction
{
public:
   txnInsertMomentLoad(const CMomentLoadData& loadData,EventIDType loadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnInsertMomentLoad();
   virtual txnTransaction* CreateClone() const override;
   virtual std::_tstring Name() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() override;
   virtual bool IsRepeatable() override;

private:
   CollectionIndexType m_LoadIdx;
   CMomentLoadData m_LoadData;
   EventIDType m_LoadingEventID;
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};

class PGSEXTCLASS txnDeleteMomentLoad : public txnTransaction
{
public:
   txnDeleteMomentLoad(LoadIDType loadID);
   virtual std::_tstring Name() const override;
   virtual txnTransaction* CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() override;
   virtual bool IsRepeatable() override;

private:
   LoadIDType m_LoadID;
   CMomentLoadData m_LoadData;
   EventIDType m_LoadingEventID;
};

class PGSEXTCLASS txnEditMomentLoad : public txnTransaction
{
public:
   txnEditMomentLoad(LoadIDType loadID,const CMomentLoadData& oldLoadData,EventIDType oldLoadingEventID,const CMomentLoadData& newLoadData,EventIDType newLoadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnEditMomentLoad();
   virtual std::_tstring Name() const override;
   virtual txnTransaction* CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() override;
   virtual bool IsRepeatable() override;

private:
   void DoExecute(int i);
   LoadIDType m_LoadID;
   CMomentLoadData m_LoadData[2];
   EventIDType m_LoadingEventID[2];
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};

#endif // INCLUDED_INSERTDELETELOAD_H_