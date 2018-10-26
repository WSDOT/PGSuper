///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2014  Washington State Department of Transportation
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
   txnInsertSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face,Float64 spanLength,bool bCreateNewGroup,EventIndexType pierErectionEventIdx);
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   CBridgeDescription2 m_BridgeDescription[2];

   PierIndexType m_RefPierIdx;
   pgsTypes::PierFaceType m_PierFace;
   Float64 m_SpanLength;
   bool m_bCreateNewGroup;
   EventIndexType m_PierErectionEventIndex;

   // access array with pgsTypes::PierFaceType enum
   Float64 m_BrgOffset[2];
   ConnectionLibraryEntry::BearingOffsetMeasurementType m_BrgOffsetMeasure[2];
   Float64 m_EndDist[2];
   ConnectionLibraryEntry::EndDistanceMeasurementType m_EndDistMeasure[2];
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
   CSpanData2* m_pDeletedSpan;
   CPierData2* m_pDeletedPier;
   Float64  m_SpanLength; // length of deleted span
   bool m_bCreateNewGroup;
   EventIndexType m_PierErectionEventIdx;
};

#endif // INCLUDED_INSERTDELETESPAN_H_