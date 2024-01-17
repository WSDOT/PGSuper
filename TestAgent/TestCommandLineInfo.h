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

#pragma once

#include <PgsExt\PgsExtExp.h>
#include "PgsExt\BaseCommandLineInfo.h"

/*****************************************************************************
CLASS 
   CTestCommandLineInfo

   Custom command line parser for Test Extension Agent
*****************************************************************************/

// constants if All option is at command line
#define TXALLSPANS   -5 
#define TXALLGIRDERS -5

// Below is to take a sample exterior and interior girder (A and mid-most)
#define TXEIGIRDERS  -6 

class  CTestCommandLineInfo : public CPGSBaseCommandLineInfo
{
public:
   // Different types of Analysis/Design and level of detail (Ext==extended) for TxDOT CAD reports
   enum TxRunType {txrDesign, txrAnalysis, TxrDistributionFactors, txrDesignShear,txrGeometry};
   enum TxFType {txfNormal, txfExtended, txfTest};


   CTestCommandLineInfo();
   virtual ~CTestCommandLineInfo();

   // derive new version to parse new commands
   virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast) override;

   virtual CString GetUsageMessage() override;

   // txDOT CAD report from command line
   bool  m_DoTxCadReport;
   TxRunType m_TxRunType;
   TxFType   m_TxFType;
   CString m_TxOutputFile;
   SpanIndexType m_TxSpan;
   GirderIndexType m_TxGirder;
   bool m_DoAppendToFile;

   Uint32 m_Count; // parameter number

private:
   CString GetUsageString();
   virtual LPCTSTR GetAppName() const override;

   // Prevent accidental copying and assignment
   CTestCommandLineInfo(const CTestCommandLineInfo&) = delete;
   CTestCommandLineInfo& operator=(const CTestCommandLineInfo&) = delete;
};

