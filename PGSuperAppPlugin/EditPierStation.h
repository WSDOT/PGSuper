///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2024  Washington State Department of Transportation
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

#ifndef INCLUDED_EDITPIERSTATION_H_
#define INCLUDED_EDITPIERSTATION_H_

#include <EAF\EAFTransaction.h>

class txnEditPierStation : public CEAFTransaction
{
public:
   txnEditPierStation(PierIndexType pierIdx,Float64 oldStation,Float64 newStation,pgsTypes::MovePierOption moveOption);

   ~txnEditPierStation();

   virtual bool Execute();
   virtual void Undo();
   virtual std::unique_ptr<CEAFTransaction>CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() const;
   virtual bool IsRepeatable() const;

private:
   void DoExecute(int i);
   PierIndexType m_PierIdx;
   Float64 m_Station[2];
   pgsTypes::MovePierOption m_MoveOption;
};

#endif // INCLUDED_EDITPIERSTATION_H_