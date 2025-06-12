///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2025  Washington State Department of Transportation
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

#include "PsgLibLib.h"
#include <PsgLib\GirderMaterial.h>
#include <PsgLib\StrandData.h>
#include <PsgLib\ShearData.h> // CShearData, CShearData2
#include <PsgLib\DebondData.h>
#include <PsgLib\LongitudinalRebarData.h>
#include <PsgLib\HandlingData.h>

#include <StrData.h>



namespace WBFL
{
   namespace Materials
   {
      class PsStrand;
   };
};

class ConcreteLibraryEntry;

/*****************************************************************************
CLASS 
   CGirderData

   Utility class for prestressing description data.

DESCRIPTION
   Utility class for prestressing description data. This class encapsulates all
   the input data the Prestress page of the Bridge Description Dialog, and 
   implements the IStructuredLoad and IStructuredSave persistence interfaces.

   NOTE: This class is obsolete. It is only used for loading data out of old files

LOG
   rab : 09.30.1998 : Created file
*****************************************************************************/
class PSGLIBCLASS CGirderData
{
public:
   CStrandData Strands;
   CGirderMaterial Material; // concrete and strand data
   CShearData ShearData; 
   CShearData2 ShearData2; 
   bool m_bUsedShearData2; // if true, the shear data is contained in ShearData2, otherwise ShearData
   CLongitudinalRebarData LongitudinalRebarData;
   CHandlingData HandlingData;

   // Rating and Condition
   pgsTypes::ConditionFactorType Condition;
   Float64 ConditionFactor;

   std::_tstring m_GirderName;
   const GirderLibraryEntry* m_pLibraryEntry;

   CGirderData();
   CGirderData(const CGirderData& rOther) = default;
   ~CGirderData();

   CGirderData& operator=(const CGirderData& rOther) = default;

   void SetGirderName(LPCTSTR strName);
   LPCTSTR GetGirderName() const;

   void SetGirderLibraryEntry(const GirderLibraryEntry* pEntry);
   const GirderLibraryEntry* GetGirderLibraryEntry() const;

   // An == operator is not enough. We must know the type of change that was
   // made in order to fire the right events. That's what the following enum 
   // and function do.
   enum ChangeType {ctNone         = 0x0000,
                    ctPrestress    = 0x0001,  // # or configuration of prestress changed
                    ctConcrete     = 0x0002,  // concrete material properties changed
                    ctStrand       = 0x0004,
                    ctLifting      = 0x0008,
                    ctShipping     = 0x0010,
                    ctCondition    = 0x0020
   };

   // return or'ed enums above 
   int GetChangeType(const CGirderData& rOther) const;

   bool operator==(const CGirderData& rOther) const;

   bool operator!=(const CGirderData& rOther) const;

   void CopyPrestressingFrom(const CGirderData& rOther);
   void CopyMaterialFrom(const CGirderData& rOther);

   // GROUP: OPERATIONS

   // the last 4 parameters in Load is are concrete properties. Concrete data for the girder used
   // to be stored elsewhere. Passing the default data makes it possible to handle older data structures
	HRESULT Load(IStructuredLoad* pStrLoad,std::shared_ptr<IEAFProgress> pProgress, Float64 fc=0,Float64 weightDensity=0,Float64 strengthDensity=0,Float64 maxAggSize=0);
	HRESULT Save(IStructuredSave* pStrSave,std::shared_ptr<IEAFProgress> pProgress);
};
