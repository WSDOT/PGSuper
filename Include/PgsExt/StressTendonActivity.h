///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2017  Washington State Department of Transportation
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
#include <PgsExt\Keys.h>

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
   void AddTendon(GirderIDType gdrID,DuctIndexType ductIdx);
   void AddTendon(const CTendonKey& tendonKey);

   // Add multiple tendons to the list of tendons that are stressed during this event
   void AddTendons(const std::vector<CTendonKey>& tendons);

   // Removes a tendon from the stressing list.
   // Pass a value of true for bRemovedFromBridge if the tendon has been removed from the
   // bridge model. This will update the indices of the ducts. Use false
   // if the tendon is simply not stressed in this activity any longer
   void RemoveTendon(GirderIDType gdrID,DuctIndexType ductIdx,bool bRemovedFromBridge);

   // Removes all tendons for the specified girder from the stressing list
   void RemoveTendons(GirderIDType gdrID);

   // Returns true if a specific tendon is stressed
   bool IsTendonStressed(GirderIDType gdrID,DuctIndexType ductIdx) const;

   // Returns true if any tendons are stressed
   bool IsTendonStressed() const;

   // Returns the tendons that are stressed in this activity
   const std::vector<CTendonKey>& GetTendons() const;

   // Returns the number of tendons in this activity
   IndexType GetTendonCount() const;

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

protected:
   void MakeCopy(const CStressTendonActivity& rOther);
   void MakeAssignment(const CStressTendonActivity& rOther);
   bool m_bEnabled;

   // tendon keys must use the gdrID parameter!
   // this is a sorted collection
   std::vector<CTendonKey> m_Tendons;
};
