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

#ifndef INCLUDED_EDITGIRDER_H_
#define INCLUDED_EDITGIRDER_H_

#include <System\Transaction.h>
#include <PgsExt\ShearData.h>
#include <PgsExt\GirderData.h>
#include <PgsExt\LongitudinalRebarData.h>
#include <IFace\Project.h>

class txnEditGirder : public txnTransaction
{
public:
   txnEditGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx,
                 bool bOldUseSameGirder, bool bNewUseSameGirder,
                 const std::string& strOldGirderName, const std::string& newGirderName,
                 const CGirderData& oldGirderData,const CGirderData& newGirderData,
                 const CShearData& oldShearData,const CShearData& newShearData,
                 const CLongitudinalRebarData& oldRebarData,const CLongitudinalRebarData& newRebarData,
                 double oldLiftingLocation,  double newLiftingLocation,
                 double oldTrailingOverhang, double newTrailingOverhang,
                 double oldLeadingOverhang,  double newLeadingOverhang,
                 pgsTypes::SlabOffsetType oldSlabOffsetType,pgsTypes::SlabOffsetType newSlabOffsetType,
                 Float64 oldSlabOffsetStart,Float64 newSlabOffsetStart,
                 Float64 oldSlabOffsetEnd, Float64 newSlabOffsetEnd
                 );

   ~txnEditGirder();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::string Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);
   SpanIndexType m_SpanIdx;
   GirderIndexType m_GirderIdx;
   bool m_bUseSameGirder[2];
   std::string m_strGirderName[2];
   CGirderData m_GirderData[2];
   CShearData m_ShearData[2];
   CLongitudinalRebarData m_RebarData[2];
   double m_LiftingLocation[2];
   double m_TrailingOverhang[2];
   double m_LeadingOverhang[2];

   pgsTypes::SlabOffsetType m_SlabOffsetType[2];
   Float64 m_SlabOffset[2][2]; // first index is new/old, second index is pgsTypes::MemberEndType
   // if slab offset is whole bridge then m_SlabOffset[i][pgsTypes::metStart] contains the value
};

#endif // INCLUDED_EDITGIRDER_H_