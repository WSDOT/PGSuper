///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2010  Washington State Department of Transportation
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

#ifndef INCLUDED_PGSEXT_CPGSUPERCOMMANDLINEINFO_H_
#define INCLUDED_PGSEXT_CPGSUPERCOMMANDLINEINFO_H_

// SYSTEM INCLUDES
//

// PROJECT INCLUDES
//
#if !defined INCLUDED_PGSEXTEXP_H_
#include <PgsExt\PgsExtExp.h>
#endif

// LOCAL INCLUDES
//

// FORWARD DECLARATIONS
//

// MISCELLANEOUS
//

/*****************************************************************************
CLASS 
   CPgsuperCommandLineInfo

   Custom command line parser for PGSuper


DESCRIPTION
   Custorm command line parser for special pgsuper command line options


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

class  CPGSuperCommandLineInfo : public CCommandLineInfo
{
public:
   // Different types of Analysis/Design and level of detail (Ext==exteneded) for TxDOT CAD reports
   enum TxRunType {txrDesign, txrAnalysis, TxrDistributionFactors};
   enum TxFType {txfNormal, txfExtended, txfTest};


   // GROUP: LIFECYCLE
   //------------------------------------------------------------------------
   // Default constructor
   CPGSuperCommandLineInfo();

   //------------------------------------------------------------------------
   // Destructor
   virtual ~CPGSuperCommandLineInfo();

   // GROUP: OPERATORS
   // GROUP: OPERATIONS

   //------------------------------------------------------------------------
   // derive new version to parse new commands
   void ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast);

   bool  m_CommandLineMode; // true if we are batching

   bool   m_bAbort; // command line problem

   bool   m_bDo1250Test;
   long   m_SubdomainId;


   // txDOT CAD report from command line
   bool  m_DoTxCadReport;
   TxRunType m_TxRunType;
   TxFType   m_TxFType;
   CString m_TxOutputFile;
   SpanIndexType m_TxSpan;
   GirderIndexType m_TxGirder;
   bool m_DoAppendToFile;

   Uint32 m_Count; // parameter number

   // GROUP: ACCESS
   // GROUP: INQUIRY

protected:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE
   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY

private:
   // GROUP: DATA MEMBERS
   // GROUP: LIFECYCLE

   // Prevent accidental copying and assignment
   CPGSuperCommandLineInfo(const CPGSuperCommandLineInfo&);
   CPGSuperCommandLineInfo& operator=(const CPGSuperCommandLineInfo&);

   // GROUP: OPERATORS
   // GROUP: OPERATIONS
   // GROUP: ACCESS
   // GROUP: INQUIRY
};

// INLINE METHODS
//

// EXTERNAL REFERENCES
//

#endif // INCLUDED_PGSEXT_CPGSUPERCOMMANDLINEINFO_H_
