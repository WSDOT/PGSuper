///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2013  Washington State Department of Transportation
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

#ifndef INCLUDED_INSERTDELETESPAN_H_
#define INCLUDED_INSERTDELETESPAN_H_

#include <System\Transaction.h>
#include <PgsExt\BridgeDescription2.h>
#include <PgsExt\PierData2.h>
#include <PgsExt\GirderSpacing2.h>
#include <IFace\Project.h>

class txnInsertSpan : public txnTransaction
{
public:
   txnInsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face,Float64 spanLength,bool bCreateNewGroup,EventIndexType eventIdx);
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   PierIndexType m_RefPierIdx;
   pgsTypes::PierFaceType m_PierFace;
   Float64 m_SpanLength;
   bool m_bCreateNewGroup;
   EventIndexType m_EventIdx;
   pgsTypes::PierConnectionType m_RefPierConnectionType;
};

class txnDeleteSpan : public txnTransaction
{
public:
   txnDeleteSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face);
   ~txnDeleteSpan();
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   PierIndexType m_RefPierIdx;
   pgsTypes::PierFaceType m_PierFace;
   EventIndexType m_EventIdx;
   GroupIndexType m_nGirderGroups;
   Float64  m_SpanLength; // length of deleted span
   pgsTypes::PierConnectionType m_RefPierConnectionType;

   CSpanData2* m_pDeletedSpan;
   CPierData2* m_pDeletedPier;

   struct TSItem
   {
      CTemporarySupportData TempSupport;
      EventIndexType ErectionEventIdx;
      EventIndexType RemovalEventIdx;
   };
   std::vector<TSItem> m_TempSupports; // temporary supports that were in the span that got deleted
};

#endif // INCLUDED_INSERTDELETESPAN_H_