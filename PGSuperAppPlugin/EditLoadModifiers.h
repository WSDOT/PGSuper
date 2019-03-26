///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2019  Washington State Department of Transportation
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
#include <IFace\Project.h>

class txnEditLoadModifiers : public txnTransaction
{
public:
   struct LoadModifiers
   {
      ILoadModifiers::Level ImportanceLevel;
      ILoadModifiers::Level DuctilityLevel;
      ILoadModifiers::Level RedundancyLevel;

      Float64 ImportanceFactor;
      Float64 DuctilityFactor;
      Float64 RedundancyFactor;
   };

   txnEditLoadModifiers(const LoadModifiers& oldLoadModifiers,const LoadModifiers& newLoadModifiers);

   ~txnEditLoadModifiers();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void DoExecute(int i);
   LoadModifiers m_LoadModifiers[2];
};
