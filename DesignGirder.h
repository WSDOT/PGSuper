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

#ifndef INCLUDED_DESIGNGIRDER_H_
#define INCLUDED_DESIGNGIRDER_H_

#include <System\Transaction.h>
#include <PgsExt\DesignArtifact.h>
#include <PgsExt\StrandData.h>
#include <PgsExt\GirderMaterial.h>
#include <PsgLib\ShearData.h>
#include <PgsExt\HandlingData.h>

class txnDesignGirder : public txnTransaction
{
public:
   txnDesignGirder(std::vector<const pgsDesignArtifact*>& artifacts, pgsTypes::SlabOffsetType slabOffsetType);
   ~txnDesignGirder();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   txnDesignGirder();

   void Init();
   bool m_bInit;

   struct DesignData;

   void CacheFlexureDesignResults(DesignData& rdata);
   void CacheShearDesignResults(DesignData& rdata);

   void DoExecute(int i);

   // Store all design data for a girder
   struct DesignData
   {
      pgsDesignArtifact m_DesignArtifact;

      // index 0 = old data (before design), 1 = new data (design outcome)
      CGirderMaterial m_Material[2];
      CStrandData m_Strands[2];
      CShearData2 m_ShearData[2];
      CHandlingData m_HandlingData[2];
      Float64 m_SlabOffset[2][2]; // first index is pgsTypes::MemberEndType
      pgsTypes::SlabOffsetType m_SlabOffsetType[2];
   };

   typedef std::vector<DesignData> DesignDataColl;
   typedef DesignDataColl::iterator DesignDataIter;
   typedef DesignDataColl::const_iterator DesignDataConstIter;

   DesignDataColl m_DesignDataColl;
};

#endif // INCLUDED_DESIGNGIRDER_H_