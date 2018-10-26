///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2015  Washington State Department of Transportation
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
m_TxSpan(INVALID_INDEX),
m_TxGirder(INVALID_INDEX),
m_DoAppendToFile(false),
m_DoTogaTest(false)
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
         // Not necessarily an error... might be a different extension's command line option
         //// the flag must be the first and only flag on the command line
         //m_bError = TRUE;
         //m_strErrorMsg = GetUsageString();
      }
      else if ( strParam.CompareNoCase(_T("TestDF")) == 0 )
      {
         // Run distribution factor regression tests suite
         m_TxRunType        = TxrDistributionFactors;
         m_TxFType          = txfNormal;
         m_DoTxCadReport    = true;
         m_bCommandLineMode = TRUE;
         m_bShowSplash      = FALSE;
         bMyParameter       = true;
      }
      else if (strParam.Left(2).CompareNoCase(_T("Tx")) == 0)
      {   
         // probable TxDOT CAD or TOGA report

         if (strParam.CompareNoCase(_T("TxTOGA")) == 0 )
         {
            // TOGA test file
            m_DoTogaTest = true;
         }
         else
         {
            // see if we append or overwrite file
            // if flag ends in "o" we are overwriting, not appending
            m_DoAppendToFile = (strParam.Right(1).CompareNoCase(_T("o")) != 0);
      
            // Set main command option
            if (strParam.CompareNoCase(_T("TxA")) == 0 || strParam.CompareNoCase(_T("TxAo")) == 0)
            {
               m_TxRunType = txrAnalysis;
               m_TxFType   = txfNormal;
            }
            else if (strParam.CompareNoCase(_T("TxAx")) == 0 || strParam.CompareNoCase(_T("TxAxo")) == 0)
            {
               m_TxRunType = txrAnalysis;
               m_TxFType   = txfExtended;
            }
            else if (strParam.CompareNoCase(_T("TxAT")) == 0 || strParam.CompareNoCase(_T("TxATo")) == 0)
            {
               m_TxRunType = txrAnalysis;
               m_TxFType   = txfTest;
               m_DoAppendToFile = false;  // always delete test file
            }
            else if (strParam.CompareNoCase(_T("TxD")) == 0 || strParam.CompareNoCase(_T("TxDo")) == 0)
            {
               m_TxRunType = txrDesign;
               m_TxFType   = txfNormal;
            }
            else if (strParam.CompareNoCase(_T("TxDx")) == 0 || strParam.CompareNoCase(_T("TxDxo")) == 0)
            {
               m_TxRunType = txrDesign;
               m_TxFType   = txfExtended;
            }
            else if (strParam.CompareNoCase(_T("TxDT")) == 0 || strParam.CompareNoCase(_T("TxDTo")) == 0)
            {
               m_TxRunType = txrDesign;
               m_TxFType   = txfTest;
               m_DoAppendToFile = false;  // always delete test file
            }
            else if (strParam.CompareNoCase(_T("TxDS")) == 0 )
            {
               m_TxRunType = txrDesignShear;
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
         }

         m_bCommandLineMode = true;
         m_bShowSplash      = FALSE;
         bMyParameter       = true;
       }
   }
   else
   {
      // Not a flag, just a regular parameter
      if ( m_Count == 0 && !bLast )
      {
         // first parameter must be a flag
         m_bError = true;
         m_strErrorMsg.Format(_T("The first parameter must be a flag.\n%s"),GetUsageString());
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
               if (strParam.CompareNoCase(_T("ALL")) == 0)
               {
                  m_TxSpan = ALL_SPANS;
               }
               else
               {
                  // Error parsing span number
                  m_strErrorMsg.Format(_T("An invalid span number was encountered.\n\n%s"),GetUsageString());
                  m_bError = true;

                  return;
               }
            }
         }
         else if ( m_Count == 4 )
         {
            // girder
            bMyParameter = true;
            if (strParam.CompareNoCase(_T("ALL")) == 0)
            {
               m_TxGirder = TXALLGIRDERS;
            }
            else if (strParam.CompareNoCase(_T("EI")) == 0)
            {
               m_TxGirder = TXEIGIRDERS;
            }
            else
            {
               int gdrIdx = (TCHAR)strParam.GetAt(0) - _T('A');
               if (0 <= gdrIdx && gdrIdx <= 28)
               {
                  m_TxGirder = gdrIdx;
               }
               else
               {
                  // error parsing girder number
                  m_strErrorMsg.Format(_T("An invalid girder number was encountered.\n\n%s"),GetUsageString());
                  m_bError = true;
                  return;
               }
            }
         }
      }
      else if ( m_DoTogaTest )
      {
         if ( m_Count == 2 )
         {
            // output file name
            m_TxOutputFile = lpszParam;
            bMyParameter = true;
         }
      }
   }

   if ( !bMyParameter )
     CEAFCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);

   m_Count++;
}

CString CTxDOTCommandLineInfo::GetUsageMessage()
{
   return CString(_T("/TxTOGA filename.toga outputfile"));
}

CString CTxDOTCommandLineInfo::GetUsageString()
{
   return CString(_T("Valid parameters are\n/flag filename.pgs outputfile span girder\nwhere\nflag can be TxA, TxAx, TxAt, TxD, TxDx, or TxDT\nspan can be a span number or the keyword ALL\ngirder can be a girder letter (A-Z), the keyword ALL or the keyword EI\nOr /TxTOGA filename.toga outputfile"));
}
