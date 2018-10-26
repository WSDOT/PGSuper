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
   CPGSuperCommandLineInfo
****************************************************************************/
#include "stdafx.h"
#include "PgsuperCommandLineInfo.h"
#include <Iface\Test1250.h>

#include <system\tokenizer.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////// PUBLIC     ///////////////////////////////////////

//======================== LIFECYCLE  =======================================
CPGSuperCommandLineInfo::CPGSuperCommandLineInfo() :
CEAFCommandLineInfo(),
m_bDo1250Test(false),
m_SubdomainId(0),
m_DoTxCadReport(false),
m_TxRunType(txrAnalysis),
m_TxFType(txfNormal),
m_TxSpan(-1),
m_TxGirder(-1),
m_DoAppendToFile(false)
{
   m_Count=0;
}

CPGSuperCommandLineInfo::~CPGSuperCommandLineInfo()
{
}

//======================== OPERATORS  =======================================
//======================== OPERATIONS =======================================
void CPGSuperCommandLineInfo::ParseParam(LPCTSTR lpszParam, BOOL bFlag, BOOL bLast)
{
   m_Count++;

  // check if this is a test run.
  // we are using a flag parameter to
  // pass the desired subdomain id.
   bool bOurFlag = false;

   if( bFlag )
   {
       CString sParam(lpszParam);
       CString sFlag(sParam.Left(4));
       CString txFlag(sParam.Left(2));

       if (sParam.CompareNoCase("testdf")==0)
       {
            // Use txdot command path to handle df report
            m_TxRunType = TxrDistributionFactors;
            m_TxFType   = txfNormal;
            m_DoTxCadReport = true;
            m_CommandLineMode = true;
            m_bShowSplash = FALSE;
            bOurFlag = true;
       }
       else if (sFlag.CompareNoCase("test")==0)
       {
         try
         {
            sParam = sParam.Right(sParam.GetLength() - 4);
            if (sParam[0]=='R'||sParam[0]=='r')
            {
               // regression test
               m_SubdomainId = RUN_REGRESSION;
            }
            else
            {
               CComVariant vdid(sParam);
               vdid.ChangeType(VT_I4);
               m_SubdomainId = vdid.lVal;
            }

            m_bDo1250Test = true;
            m_CommandLineMode = true;
            m_bShowSplash = FALSE;
            bOurFlag = true;
         }
         catch(...)
         {
            ::AfxMessageBox("Error - Parsing subdomain id on command line");
            m_bAbort = true;
         }
         return;
       }
       else if (txFlag.CompareNoCase("tx")==0)
       {   // probable TxDOT CAD report

         // see if we append or overwrite file
         CString appFlag(sParam.Right(1));
         m_DoAppendToFile = appFlag.CompareNoCase("o")!=0;
       
         // Set main command option
         if (sParam.CompareNoCase("txa")==0 || sParam.CompareNoCase("txao")==0)
         {
            m_TxRunType = txrAnalysis;
            m_TxFType   = txfNormal;
         }
         else if (sParam.CompareNoCase("txax")==0 || sParam.CompareNoCase("txaxo")==0)
         {
            m_TxRunType = txrAnalysis;
            m_TxFType   = txfExtended;
         }
         else if (sParam.CompareNoCase("txat")==0 || sParam.CompareNoCase("txato")==0)
         {
            m_TxRunType = txrAnalysis;
            m_TxFType   = txfTest;
            m_DoAppendToFile = false;  // always delete test file
         }
         else if (sParam.CompareNoCase("txd")==0 || sParam.CompareNoCase("txdo")==0)
         {
            m_TxRunType = txrDesign;
            m_TxFType   = txfNormal;
         }
         else if (sParam.CompareNoCase("txdx")==0 || sParam.CompareNoCase("txdxo")==0)
         {
            m_TxRunType = txrDesign;
            m_TxFType   = txfExtended;
         }
         else if (sParam.CompareNoCase("txdt")==0 || sParam.CompareNoCase("txdto")==0)
         {
            m_TxRunType = txrDesign;
            m_TxFType   = txfTest;
            m_DoAppendToFile = false;  // always delete test file
         }
         else
         {
            ::AfxMessageBox("Error - Parsing Texas CAD command on command line. Available options are /TxA, /TxAx, /TxAt /TxD, /TxDx /TxDt");
            m_bAbort = true;
            return;
         }

         m_DoTxCadReport = true;
         m_CommandLineMode = true;
         m_bShowSplash = FALSE;
         bOurFlag = true;
       }
     }
     else
     {
        // not a flag

        if (m_DoTxCadReport && m_Count==3)
        {
           // output file name
           m_TxOutputFile = lpszParam;
        }
        else if (m_DoTxCadReport && m_Count==4)
        {
           // span
           long lsp;
           if (sysTokenizer::ParseLong(lpszParam, &lsp))
           {
               m_TxSpan = SpanIndexType(lsp - 1);
           }
           else
           {
              CString cparam(lpszParam);
              cparam.MakeUpper();
              if (cparam=="ALL")
              {
                 m_TxSpan = ALL_SPANS;
              }
              else
              {
                  ::AfxMessageBox("Error - Parsing span number in Texas CAD command from command line");
                  m_bAbort = true;
                  return;
              }
           }
        }
        else if (m_DoTxCadReport && m_Count==5)
        {
           // girder
           CString cparam(lpszParam);
           cparam.MakeUpper();
           if (cparam=="ALL")
           {
              m_TxGirder = TXALLGIRDERS;
           }
           else if (cparam=="EI")
           {
              m_TxGirder = TXEIGIRDERS;
           }
           else
           {
              int gd = (char)cparam[0] - 'A';
              if (gd>=0 && gd<=28)
              {
                 m_TxGirder = gd;
              }
              else
              {
                  ::AfxMessageBox("Error - Parsing girder number in Texas CAD command from command line");
                  m_bAbort = true;
                  return;
              }
           }
        }
   }

   if ( bFlag && !bOurFlag || !bFlag )
     CEAFCommandLineInfo::ParseParam(lpszParam, bFlag, bLast);
}

CString CPGSuperCommandLineInfo::GetUsageMessage()
{
   CString strMsg;
   strMsg.Format("Command Line Options:\n%s\n%s\n%s",
      "PGSuper filename.pgs",
      "PGSuper [/TxA, /TxAx, /TxAt /TxD, /TxDx /TxDt] filename.pgs outputfile span girder",
      "See Command Line Options in the PGSuper User Guide for more information");
   return strMsg;
}
