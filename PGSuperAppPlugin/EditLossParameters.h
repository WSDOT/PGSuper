///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2018  Washington State Department of Transportation
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

#include <System\Transaction.h>
#include <PgsExt\LoadFactors.h>

struct txnEditLossParametersData
{
   bool    bIgnoreCreepEffects;
   bool    bIgnoreShrinkageEffects;
   bool    bIgnoreRelaxationEffects;

   bool    bUseLumpSumLosses;
   Float64 BeforeXferLosses;
   Float64 AfterXferLosses;
   Float64 LiftingLosses;
   Float64 ShippingLosses;
   Float64 BeforeTempStrandRemovalLosses;
   Float64 AfterTempStrandRemovalLosses;
   Float64 AfterDeckPlacementLosses;
   Float64 AfterSIDLLosses;
   Float64 FinalLosses;

   Float64 Dset_PT;
   Float64 WobbleFriction_PT;
   Float64 FrictionCoefficient_PT;

   Float64 Dset_TTS;
   Float64 WobbleFriction_TTS;
   Float64 FrictionCoefficient_TTS;
};

class txnEditLossParameters : public txnTransaction
{
public:
   txnEditLossParameters(const txnEditLossParametersData& oldData,const txnEditLossParametersData& newData);

   ~txnEditLossParameters();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);
   txnEditLossParametersData m_LossParameters[2];
};
