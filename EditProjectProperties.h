///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2016  Washington State Department of Transportation
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

class txnEditProjectProperties : public txnTransaction
{
public:
   txnEditProjectProperties(const std::_tstring& oldBridgeName,const std::_tstring& newBridgeName,
      const std::_tstring& oldBridgeID, const std::_tstring& newBridgeID,
      const std::_tstring& oldJobNumber, const std::_tstring& newJobNumber,
      const std::_tstring& oldEngineer, const std::_tstring& newEngineer,
      const std::_tstring& oldCompany, const std::_tstring& newCompany,
      const std::_tstring& oldComment, const std::_tstring& newComment);
       
   ~txnEditProjectProperties();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void Execute(int i);

   std::_tstring m_BridgeName[2];
   std::_tstring m_BridgeID[2];
   std::_tstring m_JobNumber[2];
   std::_tstring m_Engineer[2];
   std::_tstring m_Company[2];
   std::_tstring m_Comment[2];
};
