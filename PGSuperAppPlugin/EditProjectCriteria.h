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

#ifndef INCLUDED_EDITPROJECTCRITERIATXN_H_
#define INCLUDED_EDITPROJECTCRITERIATXN_H_

#include <EAF\EAFTransaction.h>

class txnEditProjectCriteria : public CEAFTransaction
{
public:
   txnEditProjectCriteria(LPCTSTR strOldCriteria,LPCTSTR strNewCriteria,pgsTypes::AnalysisType oldAnalysisType,pgsTypes::AnalysisType newAnalysisType,pgsTypes::WearingSurfaceType oldWearingSurfaceType,pgsTypes::WearingSurfaceType newWearingSurfaceType);

   ~txnEditProjectCriteria();

   virtual bool Execute();
   virtual void Undo();
   virtual std::unique_ptr<CEAFTransaction>CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable() const;
   virtual bool IsRepeatable() const;

private:
   void Execute(int i);

   std::_tstring m_strProjectCriteria[2];
   pgsTypes::AnalysisType m_AnalysisType[2];
   pgsTypes::WearingSurfaceType m_WearingSurfaceType[2]; // future overlay is not a valid
};

#endif // INCLUDED_EDITPROJECTCRITERIATXN_H_