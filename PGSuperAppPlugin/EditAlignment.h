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

#ifndef INCLUDED_EDITALIGNMENTTXN_H_
#define INCLUDED_EDITALIGNMENTTXN_H_

#include <System\Transaction.h>
#include <IFace\Project.h>

class txnEditAlignment : public txnTransaction
{
public:
   txnEditAlignment(const AlignmentData2& oldAlignmentData,const AlignmentData2& newAlignmentData,
      const ProfileData2& oldProfileData, const ProfileData2& newProfileData,
	  const RoadwaySectionData& oldSectionData, const RoadwaySectionData& newSectionData);

   ~txnEditAlignment();

   virtual bool Execute();
   virtual void Undo();
   virtual txnTransaction* CreateClone() const;
   virtual std::_tstring Name() const;
   virtual bool IsUndoable();
   virtual bool IsRepeatable();

private:
   void Execute(int i);

   AlignmentData2     m_AlignmentData[2];
   ProfileData2       m_ProfileData[2];
   RoadwaySectionData m_SectionData[2];
};

#endif // INCLUDED_EDITALIGNMENTTXN_H_