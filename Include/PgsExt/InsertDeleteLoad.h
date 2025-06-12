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

#pragma once

#include <PgsExt\PgsExtExp.h>

#include <EAF\Transaction.h>
#include <IFace\Project.h>
#include <PsgLib/PointLoadData.h>
#include <PsgLib/DistributedLoadData.h>
#include <PsgLib/MomentLoadData.h>
#include <PsgLib/TimelineManager.h>

class PGSEXTCLASS txnInsertPointLoad : public WBFL::EAF::Transaction
{
public:
   txnInsertPointLoad(const CPointLoadData& loadData,EventIDType loadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnInsertPointLoad();
   virtual std::unique_ptr<WBFL::EAF::Transaction> CreateClone() const override;
   virtual std::_tstring Name() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() const override;
   virtual bool IsRepeatable() const override;

private:
   IndexType m_LoadIdx;
   CPointLoadData m_LoadData;
   EventIDType m_LoadingEventID;
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};

class PGSEXTCLASS txnDeletePointLoad : public WBFL::EAF::Transaction
{
public:
   txnDeletePointLoad(LoadIDType loadID);
   virtual std::_tstring Name() const override;
   virtual std::unique_ptr<WBFL::EAF::Transaction> CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() const override;
   virtual bool IsRepeatable() const override;

private:
   LoadIDType m_LoadID;
   CPointLoadData m_LoadData;
   EventIDType m_LoadingEventID;
};

class PGSEXTCLASS txnEditPointLoad : public WBFL::EAF::Transaction
{
public:
   txnEditPointLoad(LoadIDType loadID,const CPointLoadData& oldLoadData,EventIDType oldLoadingEventID,const CPointLoadData& newLoadData,EventIDType newLoadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnEditPointLoad();
   virtual std::_tstring Name() const override;
   virtual std::unique_ptr<WBFL::EAF::Transaction> CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() const override;
   virtual bool IsRepeatable() const override;

private:
   void DoExecute(int i);
   LoadIDType m_LoadID;
   CPointLoadData m_LoadData[2];
   EventIDType m_LoadingEventID[2];
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};

class PGSEXTCLASS txnInsertDistributedLoad : public WBFL::EAF::Transaction
{
public:
   txnInsertDistributedLoad(const CDistributedLoadData& loadData,EventIDType loadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnInsertDistributedLoad();
   virtual std::unique_ptr<WBFL::EAF::Transaction> CreateClone() const override;
   virtual std::_tstring Name() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() const override;
   virtual bool IsRepeatable() const override;

private:
   IndexType m_LoadIdx;
   CDistributedLoadData m_LoadData;
   EventIDType m_LoadingEventID;
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};

class PGSEXTCLASS txnDeleteDistributedLoad : public WBFL::EAF::Transaction
{
public:
   txnDeleteDistributedLoad(LoadIDType loadID);
   virtual std::_tstring Name() const override;
   virtual std::unique_ptr<WBFL::EAF::Transaction> CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() const override;
   virtual bool IsRepeatable() const override;

private:
   LoadIDType m_LoadID;
   CDistributedLoadData m_LoadData;
   EventIDType m_LoadingEventID;
};

class PGSEXTCLASS txnEditDistributedLoad : public WBFL::EAF::Transaction
{
public:
   txnEditDistributedLoad(LoadIDType loadID,const CDistributedLoadData& oldLoadData,EventIDType oldLoadingEventID,const CDistributedLoadData& newLoadData,EventIDType newLoadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnEditDistributedLoad();
   virtual std::_tstring Name() const override;
   virtual std::unique_ptr<WBFL::EAF::Transaction> CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() const override;
   virtual bool IsRepeatable() const override;

private:
   void DoExecute(int i);
   LoadIDType m_LoadID;
   CDistributedLoadData m_LoadData[2];
   EventIDType m_LoadingEventID[2];
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};

class PGSEXTCLASS txnInsertMomentLoad : public WBFL::EAF::Transaction
{
public:
   txnInsertMomentLoad(const CMomentLoadData& loadData,EventIDType loadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnInsertMomentLoad();
   virtual std::unique_ptr<WBFL::EAF::Transaction> CreateClone() const override;
   virtual std::_tstring Name() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() const override;
   virtual bool IsRepeatable() const override;

private:
   IndexType m_LoadIdx;
   CMomentLoadData m_LoadData;
   EventIDType m_LoadingEventID;
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};

class PGSEXTCLASS txnDeleteMomentLoad : public WBFL::EAF::Transaction
{
public:
   txnDeleteMomentLoad(LoadIDType loadID);
   virtual std::_tstring Name() const override;
   virtual std::unique_ptr<WBFL::EAF::Transaction> CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() const override;
   virtual bool IsRepeatable() const override;

private:
   LoadIDType m_LoadID;
   CMomentLoadData m_LoadData;
   EventIDType m_LoadingEventID;
};

class PGSEXTCLASS txnEditMomentLoad : public WBFL::EAF::Transaction
{
public:
   txnEditMomentLoad(LoadIDType loadID,const CMomentLoadData& oldLoadData,EventIDType oldLoadingEventID,const CMomentLoadData& newLoadData,EventIDType newLoadingEventID,CTimelineManager* pTimelineMgr);
   virtual ~txnEditMomentLoad();
   virtual std::_tstring Name() const override;
   virtual std::unique_ptr<WBFL::EAF::Transaction> CreateClone() const override;
   virtual bool Execute() override;
   virtual void Undo() override;
   virtual bool IsUndoable() const override;
   virtual bool IsRepeatable() const override;

private:
   void DoExecute(int i);
   LoadIDType m_LoadID;
   CMomentLoadData m_LoadData[2];
   EventIDType m_LoadingEventID[2];
   CTimelineManager* m_pTimelineMgr;
   CTimelineManager m_OldTimelineMgr;
};
