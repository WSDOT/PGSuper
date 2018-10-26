///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\PierData.h>
#include <PgsExt\GirderSpacing.h>
#include <IFace\Project.h>

class txnInsertSpan : public txnTransaction
{
public:
   txnInsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face);
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   PierIndexType m_RefPierIdx;
   pgsTypes::PierFaceType m_PierFace;
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
   CSpanData* m_pDeletedSpan;
   CPierData* m_pDeletedPier;
   Float64  m_SpanLength; // length of deleted span
};

#endif // INCLUDED_INSERTDELETESPAN_H_