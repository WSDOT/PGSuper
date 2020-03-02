///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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
   txnDeleteSpan(PierIndexType refPierIdx,pgsTypes::PierFaceType face,pgsTypes::BoundaryConditionType boundaryCondition);
   txnDeleteSpan(PierIndexType refPierIdx, pgsTypes::PierFaceType face, pgsTypes::PierSegmentConnectionType segmentConnection, EventIndexType castClosureEventIdx);
   ~txnDeleteSpan();
   virtual std::_tstring Name() const;
   virtual txnTransaction* CreateClone() const;
   virtual bool Execute();
   virtual void Undo();
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   txnDeleteSpan(const txnDeleteSpan& other);
   PierIndexType m_RefPierIdx;
   pgsTypes::PierFaceType m_PierFace;
   pgsTypes::BoundaryConditionType m_BoundaryCondition;
   pgsTypes::PierSegmentConnectionType m_SegmentConnection;
   EventIndexType m_CastClosureEventIdx;

   bool m_bIsBoundaryPier;

   CBridgeDescription2 m_BridgeDescription;
   GroupIndexType m_StartGroupIdx;
   GroupIndexType m_EndGroupIdx;

   std::map<CGirderKey,CPTData> m_PTData;
};

#endif // INCLUDED_INSERTDELETESPAN_H_