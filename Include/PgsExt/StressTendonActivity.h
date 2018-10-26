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
#pragma once

#include <PgsExt\PgsExtExp.h>
#include <PgsExt\SegmentKey.h>

/*****************************************************************************
CLASS 
   CStressTendonActivity

DESCRIPTION
   Encapsulates the data for the stress tension activity
*****************************************************************************/

class PGSEXTCLASS CStressTendonActivity
{
public:
   CStressTendonActivity();
   CStressTendonActivity(const CStressTendonActivity& rOther);
   ~CStressTendonActivity();

   CStressTendonActivity& operator= (const CStressTendonActivity& rOther);
   bool operator==(const CStressTendonActivity& rOther) const;
   bool operator!=(const CStressTendonActivity& rOther) const;

   void Enable(bool bEnable=true);
   bool IsEnabled() const;

   void Clear();

   // Add a tendon to the list of tendons that are stressed during this event
   void AddTendon(const CGirderKey& girderKey,DuctIndexType ductIdx);
   void AddTendon(const CTendonKey& tendonKey);

   // Add multiple tendons to the list of tendons that are stressed during this event
   void AddTendons(const std::set<CTendonKey>& tendons);

   // Removes a tendon from the stressing list
   void RemoveTendon(const CGirderKey& girderKey,DuctIndexType ductIdx);

   // Removes all tendons for the specified girder from the stressing list
   void RemoveTendons(const CGirderKey& girderKey);

   // Returns true if a specific tendon is stressed
   bool IsTendonStressed(const CGirderKey& girderKey,DuctIndexType ductIdx) const;

   // Returns true if any tendons are stressed
   bool IsTendonStressed() const;

   // Returns the tendons that are stressed in this activity
   // the second data member holds a duct index
   const std::set<CTendonKey>& GetTendons() const;

   // Returns the number of tendons in this activity
   IndexType GetTendonCount() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CStressTendonActivity& rOther);
   virtual void MakeAssignment(const CStressTendonActivity& rOther);
   bool m_bEnabled;

   std::set<CTendonKey> m_Tendons;
};
