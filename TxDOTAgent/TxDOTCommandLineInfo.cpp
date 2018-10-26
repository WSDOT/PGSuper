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

/****************************************************************************
CLASS
   CTxDOTCommandLineInfo
****************************************************************************/
#include "stdafx.h"
#include "TxDOTCommandLineInfo.h"
#include <Iface\Test1250.h>

#include <system\tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CTxDOTCommandLineInfo::CTxDOTCommandLineInfo() :
CEAFCommandLineInfo(),
m_DoTxCadReport(false),
m_TxRunType(txrAnalysis),
m_TxFType(txfNormal),
m_TxSpan(-1),
m_TxGirder(-1),
m_DoAppendToFile(false)
{
   m_Count=0;
}

CTxDOTCommandLineInfo::~CTxDOTCommandLineInfo()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CTxDOTCommandLineInfo::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
{
   bool bMyParameter = false;

   CString strParam(lpszParam);
   if ( bFlag )
   {
      // Parameter is a flag (-flag or /flag)
      if ( m_Count != 0 )
      {
         // the flag must be the first and only flag on the command line
         m_bError = TRUE;
         m_strErrorMsg = GetUsageString();
      }
      else if ( strParam.CompareNoCase("TestDF") == 0 )
      {
         // Run distribution factor regression tests suite
         m_TxRunType        = TxrDistributionFactors;
         m_TxFType          = txfNormal;
         m_DoTxCadReport    = true;
         m_bCommandLineMode = TRUE;
         m_bShowSplash      = FALSE;
         bMyParameter       = true;
      }
      else if (strParam.Left(2).CompareNoCase("Tx") == 0)
      {   
         // probable TxDOT CAD report

         // see if we append or overwrite file
         // if flag ends in "o" we are overwriting, not appending
         m_DoAppendToFile = (strParam.Right(1).CompareNoCase("o") != 0);
       
         // Set main command option
         if (strParam.CompareNoCase("TxA") == 0 || strParam.CompareNoCase("TxAo") == 0)
         {
            m_TxRunType = txrAnalysis;
            m_TxFType   = txfNormal;
         }
         else if (strParam.CompareNoCase("TxAx") == 0 || strParam.CompareNoCase("TxAxo") == 0)
         {
            m_TxRunType = txrAnalysis;
            m_TxFType   = txfExtended;
         }
         else if (strParam.CompareNoCase("TxAT") == 0 || strParam.CompareNoCase("TxATo") == 0)
         {
            m_TxRunType = txrAnalysis;
            m_TxFType   = txfTest;
            m_DoAppendToFile = false;  // always delete test file
         }
         else if (strParam.CompareNoCase("TxD") == 0 || strParam.CompareNoCase("TxDo") == 0)
         {
            m_TxRunType = txrDesign;
            m_TxFType   = txfNormal;
         }
         else if (strParam.CompareNoCase("TxDx") == 0 || strParam.CompareNoCase("TxDxo") == 0)
         {
            m_TxRunType = txrDesign;
            m_TxFType   = txfExtended;
         }
         else if (strParam.CompareNoCase("TxDT") == 0 || strParam.CompareNoCase("TxDTo") == 0)
         {
            m_TxRunType = txrDesign;
            m_TxFType   = txfTest;
            m_DoAppendToFile = false;  // always delete test file
         }
         else
         {
            // invalid flag
            m_bError = true;
            return;
         }

         m_DoTxCadReport    = true;
         m_bCommandLineMode = true;
         m_bShowSplash      = FALSE;
         bMyParameter       = true;
       }
   }
   else
   {
      // Not a flag, just a regular parameter
      if ( m_Count == 0 )
      {
         // first parameter must be a flag
         m_bError = true;
         m_strErrorMsg.Format("The first parameter must be a flag.\n%s",GetUsageString());
      }
      else if ( m_DoTxCadReport )
      {
         if ( m_Count == 2 )
         {
            // output file name
            m_TxOutputFile = lpszParam;
            bMyParameter = true;
         }
         else if ( m_Count == 3 )
         {
            // span
            bMyParameter = true;
            long lsp;
            if (sysTokenizer::ParseLong(lpszParam, &lsp))
            {
               // Span number is one based on command line, but zero based inside the program
               m_TxSpan = SpanIndexType(lsp - 1);
            }
            else
            {
               if (strParam.CompareNoCase("ALL") == 0)
               {
                  m_TxSpan = ALL_SPANS;
               }
               else
               {
                  // Error parsing span number
                  m_strErrorMsg.Format("An invalid span number was encountered.\n\n%s",GetUsageString());
                  m_bError = true;

                  return;
               }
            }
         }
         else if ( m_Count == 4 )
         {
            // girder
            bMyParameter = true;
            if (strParam.CompareNoCase("ALL") == 0)
            {
               m_TxGirder = TXALLGIRDERS;
            }
            else if (strParam.CompareNoCase("EI") == 0)
            {
               m_TxGirder = TXEIGIRDERS;
            }
            else
            {
               int gdrIdx = (char)strParam.GetAt(0) - 'A';
               if (0 <= gdrIdx && gdrIdx <= 28)
               {
                  m_TxGirder = gdrIdx;
               }
               else
               {
                  // error parsing girder number
                  m_strErrorMsg.Format("An invalid girder number was encountered.\n\n%s",GetUsageString());
                  m_bError = true;
                  return;
               }
            }
         }
      } // m_DoTxCadReport
   }

   if ( !bMyParameter )
     CEAFCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);

   m_Count++;
}

CString CTxDOTCommandLineInfo::GetUsageString()
{
   return CString("Valid parameters are\n/flag filename.pgs outputfile span girder\nwhere\nflag can be TxA, TxAx, TxAt, TxD, TxDx, or TxDT\nspan can be a span number or the keyword ALL\ngirder can be a girder letter (A-Z), the keyword ALL or the keyword EI");
}
