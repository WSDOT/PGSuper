///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2011  Washington State Department of Transportation
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
//
// 
#ifndef INCLUDED_PGSEXT_TxDOTOPTIONALDESIGNDATA_H_
#define INCLUDED_PGSEXT_TxDOTOPTIONALDESIGNDATA_H_

// SYSTEM INCLUDES
//
#include <WBFLCore.h>

#if !defined INCLUDED_STRDATA_H_
#include <StrData.h>
#endif
// PROJECT INCLUDES
//
#include "TxDOTOptionalDesignGirderData.h"

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//
class CTxDOTOptionalDesignData;
class pgsGirderArtifact;
// MISCELLANEOUS
//

/*****************************************************************************
INTERFACE
   IGetTogaData

   Interface to work through broker to get TOGA project data

DESCRIPTION
   Interface to work through broker to get TOGA project data
*****************************************************************************/
// {9454556F-E6F9-4c1c-BE34-6BF9027003D6}
DEFINE_GUID(IID_IGetTogaData, 
0x9454556f, 0xe6f9, 0x4c1c, 0xbe, 0x34, 0x6b, 0xf9, 0x2, 0x70, 0x3, 0xd6);
interface IGetTogaData : IUnknown
{
   virtual const CTxDOTOptionalDesignData* GetTogaData() = 0;
};


/*****************************************************************************
INTERFACE
   IGetTogaResults

   Interface to work through broker to get TOGA analysis results

DESCRIPTION
   Interface to work through broker to get TOGA analysis results
*****************************************************************************/
// {0D8A01DE-304F-48df-9650-EA797D059BF4}
DEFINE_GUID(IID_IGetTogaResults, 
0xd8a01de, 0x304f, 0x48df, 0x96, 0x50, 0xea, 0x79, 0x7d, 0x5, 0x9b, 0xf4);
interface IGetTogaResults : IUnknown
{
   // Values from "original design" configuration
   virtual void GetControllingTensileStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart)=0;
   virtual void GetControllingCompressiveStress(Float64* pStress, Float64* pStressFactor, Float64* pDistFromStart)=0;

   virtual Float64 GetRequiredUltimateMoment()=0;
   virtual Float64 GetUltimateMomentCapacity()=0;

   virtual Float64 GetMaximumCamber()=0;

   virtual Float64 GetRequiredFc()=0;
   virtual Float64 GetRequiredFci()=0;

   // Values from fabricator optional design
   virtual const pgsGirderArtifact* GetFabricatorDesignArtifact()=0;
   virtual Float64 GetFabricatorMaximumCamber()=0;

   virtual bool ShearPassed()=0;
};


/*****************************************************************************
CLASS 
   CTxDOTOptionalDesignData

  Class to manage input data for TxDOT Optional Girder Analysis plugin

DESCRIPTION
   Utility class for TxDOT Optional Girder Analysis description data. This class encapsulates all
   the input data and implements the IStructuredLoad and IStructuredSave persistence interfaces.


COPYRIGHT
   Copyright © 1997-2010
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 02.19.2010 : Created file
*****************************************************************************/

class CTxDOTOptionalDesignData
{
   friend CTxDOTOptionalDesignGirderData;
public:
   // Data
   // First, Template Data
   void SetGirderEntryName(const CString& value);
   CString GetGirderEntryName();

   void SetLeftConnectionEntryName(const CString& value);
   CString GetLeftConnectionEntryName();

   void SetRightConnectionEntryName(const CString& value);
   CString GetRightConnectionEntryName();

   void SetPGSuperFileName(const CString& value);
   CString GetPGSuperFileName();

   // Bridge Input Data
   void SetBridge(const CString& text);
   CString GetBridge() const;

   void SetBridgeID(const CString& text);
   CString GetBridgeID() const;

   void SetJobNumber(const CString& text);
   CString GetJobNumber() const;

   void SetEngineer(const CString& text);
   CString GetEngineer() const;

   void SetCompany(const CString& text);
   CString GetCompany() const;

   void SetComments(const CString& text);
   CString GetComments() const;

   void SetSpanNo(const CString& val);
   CString GetSpanNo() const;

   void SetBeamNo(const CString& val);
   CString GetBeamNo() const;

   // TRICKY: Beam type is also file name of .togt template file
   void SetBeamType(const CString& text, bool doFire=true);
   CString GetBeamType() const;

   void SetBeamSpacing(Float64 val);
   Float64 GetBeamSpacing() const;

   void SetSpanLength(Float64 val);
   Float64 GetSpanLength() const;

   void SetSlabThickness(Float64 val);
   Float64 GetSlabThickness() const;

   void SetRelativeHumidity(Float64 val);
   Float64 GetRelativeHumidity() const;

   void SetLldfMoment(Float64 val);
   Float64 GetLldfMoment() const;

   void SetLldfShear(Float64 val);
   Float64 GetLldfShear() const;

   void SetEcSlab(Float64 val);
   Float64 GetEcSlab() const;

   void SetEcBeam(Float64 val);
   Float64 GetEcBeam() const;

   void SetFcSlab(Float64 val);
   Float64 GetFcSlab() const;

   void SetFt(Float64 val);
   Float64 GetFt() const;

   void SetFb(Float64 val);
   Float64 GetFb() const;

   void SetMu(Float64 val);
   Float64 GetMu() const;

   void SetWNonCompDc(Float64 val);
   Float64 GetWNonCompDc() const;

   void SetWCompDc(Float64 val);
   Float64 GetWCompDc() const;

   void SetWCompDw(Float64 val);
   Float64 GetWCompDw() const;

   void SetUseHigherCompressionAllowable(BOOL val);
   BOOL GetUseHigherCompressionAllowable() const;

   // our girders
   CTxDOTOptionalDesignGirderData* GetOriginalDesignGirderData();
   const CTxDOTOptionalDesignGirderData* GetOriginalDesignGirderData() const;

   CTxDOTOptionalDesignGirderData* GetPrecasterDesignGirderData();
   const CTxDOTOptionalDesignGirderData* GetPrecasterDesignGirderData() const;

   // GROUP: LIFECYCLE

   // Let our observers listen
   void Attach(ITxDataObserver* pObserver);
   void Detach(ITxDataObserver* pObserver);

   //------------------------------------------------------------------------
   // Default constructor
   CTxDOTOptionalDesignData();

   //------------------------------------------------------------------------
   // Copy constructor
   CTxDOTOptionalDesignData(const CTxDOTOptionalDesignData& rOther);

   //------------------------------------------------------------------------
   // Destructor
   ~CTxDOTOptionalDesignData();

   // GROUP: OPERATORS
   //------------------------------------------------------------------------
   // Assignment operator
   CTxDOTOptionalDesignData& operator = (const CTxDOTOptionalDesignData& rOther);

   //------------------------------------------------------------------------
   // Resets all the input to default values.
   void ResetData();

   //------------------------------------------------------------------------
   // Resets all number of strand input to zero
   void ResetStrandNoData();

   ////------------------------------------------------------------------------
   //bool operator==(const CTxDOTOptionalDesignData& rOther) const;

   ////------------------------------------------------------------------------
   //bool operator!=(const CTxDOTOptionalDesignData& rOther) const;

   // GROUP: OPERATIONS

	HRESULT Load(IStructuredLoad* pStrLoad,IProgress* pProgress);
	HRESULT Save(IStructuredSave* pStrSave,IProgress* pProgress);

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   //------------------------------------------------------------------------
   void MakeCopy(const CTxDOTOptionalDesignData& rOther);

   //------------------------------------------------------------------------
   void MakeAssignment(const CTxDOTOptionalDesignData& rOther);

   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   CString m_GirderEntryName;
   CString m_LeftConnectionEntryName;
   CString m_RightConnectionEntryName;
   CString m_PGSuperFileName;

   CString m_Bridge;
   CString m_BridgeID;
   CString m_JobNumber;
   CString m_Engineer;
   CString m_Company;
   CString m_Comments;

   CString m_SpanNo;
   CString m_BeamNo;
   CString m_BeamType;
   Float64 m_BeamSpacing;
   Float64 m_SpanLength;
   Float64 m_SlabThickness;
   Float64 m_RelativeHumidity;
   Float64 m_LldfMoment;
   Float64 m_LldfShear;

   Float64 m_EcSlab;
   Float64 m_EcBeam;
   Float64 m_FcSlab;

   Float64 m_Ft;
   Float64 m_Fb;
   Float64 m_Mu;

   Float64 m_WNonCompDc;
   Float64 m_WCompDc;
   Float64 m_WCompDw;

   BOOL m_UseHigherCompressionAllowable;

   // Girder data
   CTxDOTOptionalDesignGirderData m_OriginalDesignGirderData;
   CTxDOTOptionalDesignGirderData m_PrecasterDesignGirderData;

   // our observers
   std::set<ITxDataObserver*> m_pObservers;

   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   void FireChanged(ITxDataObserver::ChangeType change);

   // GROUP: ACCESS
   // GROUP: INQUIRY

};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//


#endif // INCLUDED_PGSEXT_TxDOTOPTIONALDESIGNDATA_H_

