///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright � 1999-2025  Washington State Department of Transportation
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
#pragma once
#include <PgsExt\BridgeDescription2.h>
#include <EAF\EAFTransaction.h>

class txnEditBridgeDescription :
   public CEAFTransaction
{
public:
   txnEditBridgeDescription(IBroker* pBroker,const CBridgeDescription2& oldBridgeDesc,const CBridgeDescription2& newBridgeDesc);
   ~txnEditBridgeDescription(void);

   virtual bool Execute() override;
   virtual void Undo() override;
   virtual std::unique_ptr<CEAFTransaction>CreateClone() const override;
   virtual std::_tstring Name() const override;
   virtual bool IsUndoable() const override;
   virtual bool IsRepeatable() const override;

private:
   void Execute(int i);

   IBroker* m_pBroker;
	CBridgeDescription2 m_BridgeDesc[2];
};
