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

#include "StdAfx.h"

#include "TOGATestFileWriter.h"
#include "TxDOTOptionalDesignData.h"
#include "TxDOTOptionalDesignUtilities.h"

#include <IFace\Tools.h>
#include <IFace\Project.h>
#include <IFace\AnalysisResults.h>
#include <IFace\Artifact.h>
#include <IFace\MomentCapacity.h>
#include <IFace\DistributionFactors.h>
#include <EAF\EAFDisplayUnits.h>
#include <IFace\PrestressForce.h>
#include <IFace\DistFactorEngineer.h>
#include <IFace\GirderHandling.h>
#include <IFace\Intervals.h>

#if defined _DEBUG
#include <IFace\DocumentType.h>
#endif

#include <PgsExt\GirderArtifact.h>
#include <PsgLib\BridgeDescription2.h>
#include <PgsExt\GirderArtifactTool.h>
#include <PsgLib\GirderLabel.h>



//////// TOGA Report
int TxDOT_WriteTOGAReportToFile (FILE *fp, std::shared_ptr<WBFL::EAF::Broker> pBroker)
{
   // Use our worker bee to write results
   CadWriterWorkerBee workerB(true);

   GET_IFACE2(pBroker,IGetTogaResults,pGetTogaResults);
   GET_IFACE2(pBroker,IGetTogaData,pGetTogaData);
   const CTxDOTOptionalDesignData* pProjectData = pGetTogaData->GetTogaData();

   // Compressive stress - top
   Float64 stress_val_calc, stress_fac, stress_loc;
   pGetTogaResults->GetControllingCompressiveStress(&stress_val_calc, &stress_fac, &stress_loc);

   Float64 stress_val_input = WBFL::Units::ConvertFromSysUnits( pProjectData->GetFt(), WBFL::Units::Measure::KSI );
   stress_val_calc = WBFL::Units::ConvertFromSysUnits( -stress_val_calc, WBFL::Units::Measure::KSI );

   workerB.WriteFloat64(stress_val_input, _T("ftinp "),8,6,_T("%6.2f"));
   workerB.WriteFloat64(stress_val_calc, _T("ftcalc"),8,6,_T("%6.2f"));
   workerB.WriteFloat64(stress_fac, _T("ftfact"),8,6,_T("%6.2f"));

   // Tensile stress - bottom
   pGetTogaResults->GetControllingTensileStress(&stress_val_calc, &stress_fac, &stress_loc);

   stress_val_input = WBFL::Units::ConvertFromSysUnits( pProjectData->GetFb(), WBFL::Units::Measure::KSI );
   stress_val_calc = WBFL::Units::ConvertFromSysUnits( -stress_val_calc, WBFL::Units::Measure::KSI );

   workerB.WriteFloat64(stress_val_input, _T("fbinp "),8,6,_T("%6.2f"));
   workerB.WriteFloat64(stress_val_calc, _T("fbcalc"),8,6,_T("%6.2f"));
   workerB.WriteFloat64(stress_fac, _T("fbfact"),8,6,_T("%6.2f"));

   // Ultimate moment
   Float64 mu_input = WBFL::Units::ConvertFromSysUnits( pProjectData->GetMu(), WBFL::Units::Measure::KipFeet);
   Float64 mu_orig  = WBFL::Units::ConvertFromSysUnits( pGetTogaResults->GetRequiredUltimateMoment(), WBFL::Units::Measure::KipFeet );
   Float64 mu_fabr  = WBFL::Units::ConvertFromSysUnits( pGetTogaResults->GetUltimateMomentCapacity(), WBFL::Units::Measure::KipFeet );

   workerB.WriteFloat64(mu_input,_T(" muinp  "),10,8,_T("%8.2f"));
   workerB.WriteFloat64(mu_orig, _T(" muorig "),10,8,_T("%8.2f"));
   workerB.WriteFloat64(mu_fabr, _T(" mufabr "),10,8,_T("%8.2f"));

   // Required concrete strengths
   Float64 input_fci = WBFL::Units::ConvertFromSysUnits(pProjectData->GetPrecasterDesignGirderData()->GetFci(), WBFL::Units::Measure::KSI );
   Float64 reqd_fci  = WBFL::Units::ConvertFromSysUnits(pGetTogaResults->GetRequiredFci(), WBFL::Units::Measure::KSI );

   workerB.WriteFloat64(input_fci,_T("fciinp"),8,6,_T("%6.2f"));
   workerB.WriteFloat64(reqd_fci, _T("fcireq"),8,6,_T("%6.2f"));

   Float64 input_fc = WBFL::Units::ConvertFromSysUnits(pProjectData->GetPrecasterDesignGirderData()->GetFc(), WBFL::Units::Measure::KSI );
   Float64 reqd_fc =  WBFL::Units::ConvertFromSysUnits(pGetTogaResults->GetRequiredFc(), WBFL::Units::Measure::KSI );

   workerB.WriteFloat64(input_fc,_T("fc inp"),8,6,_T("%6.2f"));
   workerB.WriteFloat64(reqd_fc, _T("fc req"),8,6,_T("%6.2f"));

   // Camber
   Float64 cbr_orig = WBFL::Units::ConvertFromSysUnits(pGetTogaResults->GetMaximumCamber(), WBFL::Units::Measure::Feet );
   Float64 cbr_fabr = WBFL::Units::ConvertFromSysUnits(pGetTogaResults->GetFabricatorMaximumCamber(), WBFL::Units::Measure::Feet );

   workerB.WriteFloat64(cbr_orig,_T("cbr orig"),10,8,_T("%8.4f"));
   workerB.WriteFloat64(cbr_fabr,_T("cbr fabr"),10,8,_T("%8.4f"));

   // Shear check
   bool passed = pGetTogaResults->ShearPassed();
   workerB.WriteString(passed?_T("Ok\0"):_T("Fail\n"),_T("Shear"),8,7,_T("%7s"));

   workerB.WriteToFile(fp);

   _ftprintf(fp, _T("\n"));

   return S_OK;
}

void CadWriterWorkerBee::WriteFloat64(Float64 val, LPCTSTR title, Int16 colWidth, Int16 nChars, LPCTSTR format)
{
   // write string to local buffer
   TCHAR buf[32];
   int nr = _stprintf_s(buf, 32, format, val);

   ATLASSERT(nr==nChars);

   this->WriteString(buf, title, colWidth, nChars,_T("%s"));
}

void CadWriterWorkerBee::WriteInt32(Int32 val, LPCTSTR title, Int16 colWidth, Int16 nchars, LPCTSTR format)
{
   // write string to local buffer
   TCHAR buf[32];
   int nr = _stprintf_s(buf, 32, format, val);

   ATLASSERT(nr==nchars);

   this->WriteString(buf, title, colWidth, nchars,_T("%s"));
}



void CadWriterWorkerBee::WriteString(LPCTSTR val, LPCTSTR title, Int16 colWidth, Int16 nchars, LPCTSTR format)
{
   ATLASSERT(nchars<=colWidth);
   ATLASSERT(std::_tstring(title).size()<=colWidth);

   // determine where to write string.
   // Center string in column, biased to right
   Int16 slack = colWidth - nchars;
   Int16 right = slack/2;
   Int16 left = (slack % 2 == 0) ? right : right + 1;
   ATLASSERT(colWidth == left+nchars+right);

   WriteBlankSpacesNoTitle(left);

   int nr = _stprintf_s(m_DataLineCursor, DataBufferRemaining(), format, val);
   ATLASSERT(nr==nchars);

   m_DataLineCursor += nchars;

   WriteBlankSpacesNoTitle(right);

   if (m_DoWriteTitles)
   {
      // Write title lines
      WriteTitle(title, colWidth);
   }
}

void CadWriterWorkerBee::WriteStringEx(LPCTSTR val, LPCTSTR title, Int16 lftPad, Int16 nchars, Int16 rgtPad, LPCTSTR format)
{
   int colWidth = lftPad + nchars + rgtPad;
   ATLASSERT(std::_tstring(title).size()<=colWidth);

   WriteBlankSpacesNoTitle(lftPad);

   int nr = _stprintf_s(m_DataLineCursor, DataBufferRemaining(), format, val);
   ATLASSERT(nr==nchars);

   m_DataLineCursor += nchars;

   WriteBlankSpacesNoTitle(rgtPad);

   if (m_DoWriteTitles)
   {
      // Write title lines
      WriteTitle(title, colWidth);
   }
}

void CadWriterWorkerBee::WriteBlankSpaces(Int16 ns)
{
   for (Int16 is=0; is<ns; is++)
   {
      *(m_DataLineCursor++) = _T(' ');

      if (m_DoWriteTitles)
      {
         *(m_TitleLineCursor++) = _T(' ');
         *(m_DashLineCursor++) = _T('-');
      }
   }

   *m_DataLineCursor = _T('\0');

   if (m_DoWriteTitles)
   {
      *m_TitleLineCursor = _T('\0');
      *m_DashLineCursor = _T('\0');
   }
}

void CadWriterWorkerBee::WriteBlankSpacesNoTitle(Int16 ns)
{
   for (Int16 is=0; is<ns; is++)
   {
      *(m_DataLineCursor++) = _T(' ');
   }

   *m_DataLineCursor = _T('\0');
}

void CadWriterWorkerBee::WriteToFile(FILE* fp)
{
   // Now that we've filled up our buffers, write them to the file
   if (m_DoWriteTitles)
   {
      _ftprintf(fp, _T("%s"), m_TitleLine);
      _ftprintf(fp, _T("\n"));

      _ftprintf(fp, _T("%s"), m_DashLine);
      _ftprintf(fp, _T("\n"));
   }

   _ftprintf(fp, _T("%s"), m_DataLine);
   _ftprintf(fp, _T("\n"));
}

CadWriterWorkerBee::CadWriterWorkerBee(bool doWriteTitles):
m_DoWriteTitles(doWriteTitles)
{
    m_DataLineCursor  = m_DataLine;
    m_TitleLineCursor = m_TitleLine;
    m_DashLineCursor  = m_DashLine;
}

void CadWriterWorkerBee::WriteTitle(LPCTSTR title, Int16 colWidth)
{
   // Write title line and dash line 
   size_t nchars = _tcslen(title);

   // Center string in column, biased to right
   size_t slack = colWidth - nchars;
   size_t right = slack/2;
   size_t left = (slack % 2 == 0) ? right : right + 1;
   ATLASSERT(colWidth == left+nchars+right);

   // left 
   for (size_t is=0; is<left; is++)
   {
      *(m_TitleLineCursor++) = _T(' ');
   }

   // center
   for (size_t is=0; is<nchars; is++)
   {
      *(m_TitleLineCursor++) = *(title++);
   }

   // right
   for (size_t is=0; is<right; is++)
   {
      *(m_TitleLineCursor++) = _T(' ');
   }

   for (size_t is=0; is<(size_t)colWidth; is++)
   {
      *(m_DashLineCursor++) = _T('-');
   }

   *m_TitleLineCursor = _T('\0');
   *m_DashLineCursor = _T('\0');
}

