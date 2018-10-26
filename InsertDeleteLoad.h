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

#ifndef INCLUDED_INSERTDELETELOAD_H_
#define INCLUDED_INSERTDELETELOAD_H_

#include <System\Transaction.h>
#include <IFace\Project.h>
#include <PgsExt\PointLoadData.h>
#include <PgsExt\DistributedLoadData.h>
#include <PgsExt\MomentLoadData.h>

class txnInsertPointLoad : public txnTransaction
{
public:
   txnInsertPointLoad(const CPointLoadData& loadData);
   virtual txnTransaction* CreateClone() const;
   virtual std::string Name() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   Uint32 m_LoadIdx;
   CPointLoadData m_LoadData;
};

class txnDeletePointLoad : public txnTransaction
{
public:
   txnDeletePointLoad(Uint32 loadIdx);
   virtual std::string Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   Uint32 m_LoadIdx;
   CPointLoadData m_LoadData;
};

class txnEditPointLoad : public txnTransaction
{
public:
   txnEditPointLoad(Uint32 loadIdx,const CPointLoadData& oldLoadData,const CPointLoadData& newLoadData);
   virtual std::string Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);
   Uint32 m_LoadIdx;
   CPointLoadData m_LoadData[2];
};

class txnInsertDistributedLoad : public txnTransaction
{
public:
   txnInsertDistributedLoad(const CDistributedLoadData& loadData);
   virtual txnTransaction* CreateClone() const;
   virtual std::string Name() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   Uint32 m_LoadIdx;
   CDistributedLoadData m_LoadData;
};

class txnDeleteDistributedLoad : public txnTransaction
{
public:
   txnDeleteDistributedLoad(Uint32 loadIdx);
   virtual std::string Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   Uint32 m_LoadIdx;
   CDistributedLoadData m_LoadData;
};

class txnEditDistributedLoad : public txnTransaction
{
public:
   txnEditDistributedLoad(Uint32 loadIdx,const CDistributedLoadData& oldLoadData,const CDistributedLoadData& newLoadData);
   virtual std::string Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);
   Uint32 m_LoadIdx;
   CDistributedLoadData m_LoadData[2];
};

class txnInsertMomentLoad : public txnTransaction
{
public:
   txnInsertMomentLoad(const CMomentLoadData& loadData);
   virtual txnTransaction* CreateClone() const;
   virtual std::string Name() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   Uint32 m_LoadIdx;
   CMomentLoadData m_LoadData;
};

class txnDeleteMomentLoad : public txnTransaction
{
public:
   txnDeleteMomentLoad(Uint32 loadIdx);
   virtual std::string Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   Uint32 m_LoadIdx;
   CMomentLoadData m_LoadData;
};

class txnEditMomentLoad : public txnTransaction
{
public:
   txnEditMomentLoad(Uint32 loadIdx,const CMomentLoadData& oldLoadData,const CMomentLoadData& newLoadData);
   virtual std::string Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);
   Uint32 m_LoadIdx;
   CMomentLoadData m_LoadData[2];
};

#endif // INCLUDED_INSERTDELETELOAD_H_