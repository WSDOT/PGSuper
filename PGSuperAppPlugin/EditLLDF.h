///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2022  Washington State Department of Transportation
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

#ifndef INCLUDED_EDITLLDFTXN_H_
#define INCLUDED_EDITLLDFTXN_H_

#include <System\Transaction.h>
#include <PgsExt\BridgeDescription2.h>
#include <IFace\Project.h>

class txnEditLLDF : public txnTransaction
{
public:
   txnEditLLDF(const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc,
               LldfRangeOfApplicabilityAction OldROA,LldfRangeOfApplicabilityAction newROA);

   ~txnEditLLDF();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   // index 0 = old data (before edit), index 1 = new data (after edit)
	CBridgeDescription2* m_pBridgeDesc[2];
   LldfRangeOfApplicabilityAction m_ROA[2];

   void DoExecute(int i);
};

#endif // INCLUDED_EDITLLDFTXN_H_