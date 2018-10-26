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

#include <PgsExt\PgsExtExp.h>
#include <EAF\EAFCommandLineInfo.h>
#include "PGSuperCatalogServer.h"

/*****************************************************************************
CLASS 
   CPGSBaseCommandLineInfo

   Custom command line parser for PGSuper and PGSplice


DESCRIPTION
   Custom command line parser for PGSuper and PGSplice. Handles all common
   command line options for both applications


COPYRIGHT
   Copyright © 1997-1998
   Washington State Department Of Transportation
   All Rights Reserved

LOG
   rdp : 09.24.1999 : Created file
*****************************************************************************/
// constants if All option is at command line
#define TXALLSPANS   -5 
#define TXALLGIRDERS -5
// Below is to take a sample exterior and interior girder (A and mid-most)
#define TXEIGIRDERS  -6 

class  CPGSBaseCommandLineInfo : public CEAFCommandLineInfo
{
public:
   //// Different types of Analysis/Design and level of detail (Ext==exteneded) for TxDOT CAD reports
   //enum TxRunType {txrDesign, txrAnalysis, TxrDistributionFactors};
   //enum TxFType {txfNormal, txfExtended, txfTest};


   CPGSBaseCommandLineInfo();
   virtual ~CPGSBaseCommandLineInfo();

   // derive new version to parse new commands
   virtual void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast);

   virtual CString GetUsageMessage();
   virtual CString GetErrorMessage();

   bool   m_bDo1250Test;
   long   m_SubdomainId;

   bool m_bSetUpdateLibrary;
   CString m_CatalogServerName;
   CString m_PublisherName;

   Uint32 m_Count; // parameter number

private:
   // Prevent accidental copying and assignment
   CPGSBaseCommandLineInfo(const CPGSBaseCommandLineInfo&);
   CPGSBaseCommandLineInfo& operator=(const CPGSBaseCommandLineInfo&);

   virtual LPCTSTR GetAppName() const = 0;
};

