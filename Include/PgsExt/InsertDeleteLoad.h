///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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

class PGSEXTCLASS txnInsertPointLoad : public txnTransaction
{
public:
   txnInsertPointLoad(const CPointLoadData& loadData);
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   CollectionIndexType m_LoadIdx;
   CPointLoadData m_LoadData;
};

class PGSEXTCLASS txnDeletePointLoad : public txnTransaction
{
public:
   txnDeletePointLoad(CollectionIndexType loadIdx);
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   CollectionIndexType m_LoadIdx;
   CPointLoadData m_LoadData;
};

class PGSEXTCLASS txnEditPointLoad : public txnTransaction
{
public:
   txnEditPointLoad(CollectionIndexType loadIdx,const CPointLoadData& oldLoadData,const CPointLoadData& newLoadData);
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);
   CollectionIndexType m_LoadIdx;
   CPointLoadData m_LoadData[2];
};

class PGSEXTCLASS txnInsertDistributedLoad : public txnTransaction
{
public:
   txnInsertDistributedLoad(const CDistributedLoadData& loadData);
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   CollectionIndexType m_LoadIdx;
   CDistributedLoadData m_LoadData;
};

class PGSEXTCLASS txnDeleteDistributedLoad : public txnTransaction
{
public:
   txnDeleteDistributedLoad(CollectionIndexType loadIdx);
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   CollectionIndexType m_LoadIdx;
   CDistributedLoadData m_LoadData;
};

class PGSEXTCLASS txnEditDistributedLoad : public txnTransaction
{
public:
   txnEditDistributedLoad(CollectionIndexType loadIdx,const CDistributedLoadData& oldLoadData,const CDistributedLoadData& newLoadData);
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);
   CollectionIndexType m_LoadIdx;
   CDistributedLoadData m_LoadData[2];
};

class PGSEXTCLASS txnInsertMomentLoad : public txnTransaction
{
public:
   txnInsertMomentLoad(const CMomentLoadData& loadData);
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   CollectionIndexType m_LoadIdx;
   CMomentLoadData m_LoadData;
};

class PGSEXTCLASS txnDeleteMomentLoad : public txnTransaction
{
public:
   txnDeleteMomentLoad(CollectionIndexType loadIdx);
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   CollectionIndexType m_LoadIdx;
   CMomentLoadData m_LoadData;
};

class PGSEXTCLASS txnEditMomentLoad : public txnTransaction
{
public:
   txnEditMomentLoad(CollectionIndexType loadIdx,const CMomentLoadData& oldLoadData,const CMomentLoadData& newLoadData);
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);
   CollectionIndexType m_LoadIdx;
   CMomentLoadData m_LoadData[2];
};

#endif // INCLUDED_INSERTDELETELOAD_H_