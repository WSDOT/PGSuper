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

#ifndef INCLUDED_EDITSPAN_H_
#define INCLUDED_EDITSPAN_H_

#include <System\Transaction.h>
#include <PgsExt\BridgeDescription.h>
#include <PgsExt\PierData.h>
#include <PgsExt\GirderSpacing.h>
#include <IFace\Project.h>

struct txnEditSpanData
{
   txnEditSpanData();
   txnEditSpanData(const CSpanData* pSpan);

   // whole bridge data
   bool bSameNumberOfGirdersInAllSpans;
   bool bSameGirderType;
   pgsTypes::SupportedBeamSpacing GirderSpacingType;
   pgsTypes::MeasurementLocation GirderMeasurementLocation;

   Float64 SpanLength;

   GirderIndexType nGirders;
   CGirderTypes GirderTypes;

   CGirderSpacing GirderSpacing[2];

   // Connections
   pgsTypes::PierConnectionType m_ConnectionType[2];

   // first index is MemberEndType, second index is PierFaceType
   // Example, bearing offset at back side of pier at the start of this span
   // m_BearingOffset[pgsTypes::metStart][pgsTypes::Back]
   ConnectionLibraryEntry::EndDistanceMeasurementType m_EndDistanceMeasurementType[2][2];
   Float64 m_EndDistance[2][2];
   ConnectionLibraryEntry::BearingOffsetMeasurementType m_BearingOffsetMeasurementType[2][2];
   Float64 m_BearingOffset[2][2]; 
   Float64 m_SupportWidth[2][2];

   Float64 m_DiaphragmHeight[2][2];
   Float64 m_DiaphragmWidth[2][2];
   ConnectionLibraryEntry::DiaphragmLoadType m_DiaphragmLoadType[2][2];
   Float64 m_DiaphragmLoadLocation[2][2];

   pgsTypes::SlabOffsetType SlabOffsetType;
   Float64 SlabOffset[2];
};

class txnEditSpan : public txnTransaction
{
public:
   txnEditSpan(SpanIndexType spanIdx,const txnEditSpanData& oldData,const txnEditSpanData& newData);

   ~txnEditSpan();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);
   SpanIndexType m_SpanIdx;
   txnEditSpanData m_SpanData[2];
};

#endif // INCLUDED_EDITSPAN_H_