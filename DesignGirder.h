///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 2008  Washington State Department of Transportation
//                     Bridge and Structures Office
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

#ifndef INCLUDED_DESIGNGIRDER_H_
#define INCLUDED_DESIGNGIRDER_H_

#include <System\Transaction.h>
#include <PgsExt\DesignArtifact.h>

class txnDesignGirder : public txnTransaction
{
public:
   txnDesignGirder(SpanIndexType spanIdx,GirderIndexType gdrIdx,arDesignOptions designOptions,const pgsDesignArtifact& artifact,pgsTypes::SlabOffsetType slabOffsetType);
   ~txnDesignGirder();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::string Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void Init();
   bool m_bInit;

   void CacheFlexureDesignResults();
   void CacheShearDesignResults();

   void DoExecute(int i);
   SpanIndexType m_SpanIdx;
   GirderIndexType m_GirderIdx;
   arDesignOptions m_DesignOptions;
   pgsDesignArtifact m_DesignArtifact;

   // index 0 = old data (before design), 1 = new data (design outcome)
   CGirderData m_GirderData[2];
   CShearData m_ShearData[2];
   double m_SlabOffset[2][2]; // first index is pgsTypes::MemberEndType
   pgsTypes::SlabOffsetType m_SlabOffsetType[2];
};

#endif // INCLUDED_DESIGNGIRDER_H_