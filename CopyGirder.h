///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2012  Washington State Department of Transportation
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

#ifndef INCLUDED_COPYGIRDERTXN_H_
#define INCLUDED_COPYGIRDERTXN_H_

#include <System\Transaction.h>
#include <PgsExt\BridgeDescription.h>
#include <IFace\Project.h>
#include <PgsExt\GirderData.h>

class txnCopyGirderData
{
public:
   std::_tstring m_strGirderName;
   CGirderData m_GirderData;
   CShearData m_ShearData;
   CLongitudinalRebarData m_LongitudinalRebarData;
   double m_LeftLiftPoint;
   double m_RightLiftPoint;
   double m_TrailingOverhang;
   double m_LeadingOverhang;
   double m_SlabOffset[2];
};

class txnCopyGirder : public txnTransaction
{
public:
   txnCopyGirder(SpanGirderHashType fromHash,std::vector<SpanGirderHashType> toHash,BOOL bGirder,BOOL bTransverse,BOOL bLongitudinalRebar,BOOL bPrestress,BOOL bHandling, BOOL bMaterial,BOOL bSlabOffset);

   ~txnCopyGirder();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   SpanGirderHashType m_From; // span/gdr hash value of source girder
   std::vector<SpanGirderHashType> m_To; // span/gdr has values of destination girders
   BOOL m_bGirder;
   BOOL m_bTransverse;
   BOOL m_bLongitudinalRebar;
   BOOL m_bPrestress;
   BOOL m_bHandling;
   BOOL m_bMaterial;
   BOOL m_bSlabOffset;

   txnCopyGirderData m_SourceGirderData;
   std::vector<txnCopyGirderData> m_DestinationGirderData;

   void GetGirderData(SpanIndexType spanIdx,GirderIndexType gdrIdx,txnCopyGirderData* ptxnGirderData);
};

#endif // INCLUDED_COPYGIRDERTXN_H_