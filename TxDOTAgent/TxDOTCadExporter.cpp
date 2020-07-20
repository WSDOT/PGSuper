///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2020  Washington State Department of Transportation
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

// TxDOTAgentImp.cpp : Implementation of CTxDOTAgentImp

#include "stdafx.h"
#include "TxDOTCadExporter.h"
#include "TxDOTCadWriter.h"
#include "ExportCadData.h" 
#include "TxDOTLegacyCadWriter.h"

#include "TxExcelDataExporter.h"
#include "TxCSVDataExporter.h"

#include <EAF\EAFAutoProgress.h>
#include <EAF\EAFDocument.h>

#include <IFace\Selection.h>
#include <IFace\TestFileExport.h>

#include <MfcTools\XUnwind.h>
#include <MFCTools\VersionInfo.h>
#include <MFCTools\AutoRegistry.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


bool DoesFileExist(const CString& filename)
{
   if (filename.IsEmpty())
      return false;
   else
   {
      CFileFind finder;
      BOOL is_file;
      is_file = finder.FindFile(filename);
      return (is_file!=0);
   }
}

// void raised_strand_research(IBroker* pBroker,const std::vector<CGirderKey>& girderKeys);

// CTxDOTCadExporter

HRESULT CTxDOTCadExporter::FinalConstruct()
{
   CEAFApp* pApp = EAFGetApp();
   CString str = pApp->GetAppLocation();

   CString strDefaultLocation;
   if (-1 != str.Find(_T("RegFreeCOM")))
   {
      // application is on a development box
      strDefaultLocation = (_T("\\ARP\\PGSuper\\TxDOTAgent\\TxCADExport\\"));
   }
   else
   {
      strDefaultLocation = str + CString(_T("\\TxCadExport\\"));
   }

   // Get the user's setting, using the local machine setting as the default if not present
   m_strTemplateLocation = pApp->GetProfileString(_T("Settings"),_T("TxCADExportTemplateFolder"),strDefaultLocation);

   // make sure we have a trailing backslash
   if (_T('\\') != m_strTemplateLocation.GetAt(m_strTemplateLocation.GetLength() - 1))
   {
      m_strTemplateLocation += _T("\\");
   }

   return S_OK;
}

void CTxDOTCadExporter::FinalRelease()
{
   CEAFApp* pApp = EAFGetApp();
   VERIFY(pApp->WriteProfileString(_T("Settings"), _T("TxCADExportTemplateFolder"), m_strTemplateLocation));
}

/////////////////////////////////////////////////////////////////////////////
// IPGSuperExporter
STDMETHODIMP CTxDOTCadExporter::Init(UINT nCmdID)
{
   // we don't need to save our command ID
   return S_OK;
}

STDMETHODIMP CTxDOTCadExporter::GetMenuText(BSTR*  bstrText) const
{
   *bstrText = CComBSTR("TxDOT &CAD Data...");
   return S_OK;
}

STDMETHODIMP CTxDOTCadExporter::GetBitmapHandle(HBITMAP* phBmp) const
{
   *phBmp = nullptr;
   return S_OK;
}

STDMETHODIMP CTxDOTCadExporter::GetCommandHintText(BSTR*  bstrText) const
{
   *bstrText = CComBSTR("Export TxDOT CAD Data\nExport TxDOT CAD Data");
   return S_OK;
}

STDMETHODIMP CTxDOTCadExporter::Export(IBroker* pBroker)
{
   GET_IFACE2(pBroker,ISelection,pSelection);
   CSelection selection = pSelection->GetSelection();

   CGirderKey girderKey;
   if ( selection.Type == CSelection::Span )
   {
      GET_IFACE2(pBroker,IBridge,pBridge);
      girderKey.groupIndex = pBridge->GetGirderGroupIndex(selection.SpanIdx);
      girderKey.girderIndex = 0;
   }
   else if ( selection.Type == CSelection::Girder || selection.Type == CSelection::Segment || selection.Type == CSelection::ClosureJoint )
   {
      girderKey.groupIndex  = selection.GroupIdx;
      girderKey.girderIndex = selection.GirderIdx;
   }
   else
   {
      girderKey.groupIndex  = 0;
      girderKey.girderIndex = 0;
   }

   std::vector<CGirderKey> girderKeys;
   girderKeys.push_back(girderKey);

	// Create ExportCADData dialog box object 
   exportCADData::ctxFileFormatType fileFormat = exportCADData::ctxExcel;;
   {
      AFX_MANAGE_STATE(AfxGetStaticModuleState());
	   exportCADData  caddlg (pBroker, nullptr);
      caddlg.m_GirderKeys = girderKeys;
      caddlg.m_FileFormatType = fileFormat;

	   // Open the ExportCADData dialog box 
	   
	   if (caddlg.DoModal() == IDOK)
	   {
		   // Get user's span & beam id values 
		   girderKeys  = caddlg.m_GirderKeys;
         fileFormat = caddlg.m_FileFormatType;
	   }
	   else
	   {
		   // Just do nothing if CANCEL
	       return S_OK;
	   }
   }

   // Generic class for writing CAD format (Excel or CSV) to a specific CTxDataExporter
   TxDOTCadWriter cadWriter;

   // Get strand layout and do some error checking
   bool isIBeam = IsIBeam(pBroker, girderKeys.front());

   bool isStraight(false), isHarped(false), isDebonded(false), isRaised(false);
   for (auto& key : girderKeys)
   {
      txcwStrandLayoutType strand_layout = GetStrandLayoutType(pBroker, key);

      isStraight |= strand_layout==tslStraight         || strand_layout == tslDebondedStraight || strand_layout == tslRaisedStraight;
      isHarped   |= strand_layout==tslHarped           || strand_layout==tslMixedHarpedRaised  || strand_layout==tslMixedHarpedDebonded;
      isDebonded |= strand_layout==tslDebondedStraight || strand_layout==tslMixedHarpedDebonded;
      isRaised   |= strand_layout == tslRaisedStraight   || strand_layout == tslMixedHarpedRaised;
   }

   // TxDOT always treats I-Beams as harped
   if (isIBeam)
   {
      isHarped = true;
   }

   if (isIBeam && isDebonded)
   {
      ::AfxMessageBox(_T("Error: The list of girders selected for export contains an I Beam with debonded strands. This is an invalid strand layout for CAD export. CAD export cannot continue."));
      return E_FAIL;
   }
   else if (isHarped && isDebonded)
   {
      ::AfxMessageBox(_T("Error: The list of girders selected for export have a mix of harped and debonded strands. CAD export cannot continue."));
      return E_FAIL;
   }

   // Factory the specific file format exporter and its associated information
   std::unique_ptr<CTxDataExporter> pExporter;
   TxDOTCadWriter::txcwNsTableLayout table_layout;
   CString default_name;
   CString strFilter;
   CString strSuffix;

   if (exportCADData::ctxLegacy == fileFormat)
   {
#pragma Reminder("Legacy TxDOT CAD Export is a hack an may no longer be useful. Remove this option at Version 6.0 if confirmed.")
      // This doesn't fit with our factory pattern, but the plan is to delete this functionality in the near future
      // Use the File and girder selection UI from this function and then jump to our old legacy version
      default_name = _T("CADexport.txt");
      strFilter = _T("Legacy Text File (*.txt)|*.txt||");
      strSuffix = _T("txt");
   }
   else if (exportCADData::ctxExcel == fileFormat)
   {
      table_layout = TxDOTCadWriter::ttlnTwoTables;
      default_name = _T("CADexport.xlsx");
      strFilter = _T("CAD Export Excel Worksheet (*.xlsx)|*.xlsx||");
      strSuffix = _T("xlsx");

      // Determine which template file to open
      CString templateFolder = this->GetExcelTemplateFolderLocation();
      CString templateName;
      if (isHarped)
      {
         templateName = templateFolder + _T("CADExport-Harped.xltx");
      }
      else
      {
         templateName = templateFolder + _T("CADExport-Straight.xltx");
      }

      if (!DoesFileExist(templateName))
      {
         CString errMsg = CString(_T("The Excel template file at ")) + templateName + CString(_T(" does not exist. This is likely an installation problem. CAD export cannot continue."));
         ::AfxMessageBox(errMsg);
         return E_FAIL;
      }

      // Make the exporter
      auto pExcelExporter = std::make_unique<CTxExcelDataExporter>();
      pExcelExporter->InitializeExporter(templateName);

      // move to base class ptr
      pExporter = std::move(pExcelExporter);
   }
   else if (exportCADData::ctxCSV == fileFormat)
   {
      table_layout = TxDOTCadWriter::ttlnSingleTable;
      default_name = _T("CADexport.txt");
      strFilter = _T("Semicolon Separated Value text File (*.txt)|*.txt||");
      strSuffix = _T("txt");

      // Make CSV exporter and give it the list of columns in its table
      auto pCSVDataExporter = std::make_unique<CTxCSVDataExporter>();

      pCSVDataExporter->SetSeparator(_T(";")); 

      // Main table
      std::vector<std::_tstring> cols;
      if (isHarped)
      {
         cols = std::vector<std::_tstring>({ _T("StructureName"),_T("SpanNum"),_T("GdrNum"),_T("GdrType"),_T("NonStd"),_T("NStot"),_T("Size"),_T("Strength"),_T("eCL"),_T("eEnd"),_T("B_1"),
                                            _T("NH"),_T("ToEnd"),_T("B_2"),_T("FCI"),_T("FC"),_T("B_3"),_T("fComp"),_T("fTens"),_T("UltMom"),_T("gMoment"),_T("gShear"),_T("NSArrangement") });
      }
      else
      {
         cols = std::vector<std::_tstring>({ _T("StructureName"),_T("SpanNum"),_T("GdrNum"),_T("GdrType"),_T("NonStd"),_T("NStot"),_T("Size"),_T("Strength"),_T("eCL"),_T("eEnd"),
                                             _T("NDBtot"),_T("DBBotDist"),_T("NSRow"),_T("NDBRow"),_T("DB_3"),_T("DB_6"),_T("DB_9"),_T("DB_12"),_T("DB_15"),_T("FCI"),_T("FC"),_T("B_3"),
                                             _T("fComp"),_T("fTens"),_T("UltMom"),_T("gMoment"),_T("gShear"),_T("NSArrangement") });
      }

      pCSVDataExporter->InitializeTable(1, cols);

      // move to base class ptr
      pExporter = std::move(pCSVDataExporter);
   }
   else
   {
      ATLASSERT(0); 
      return E_FAIL;
   }

   // Create SAVEAS file dialog box object 
	CFileDialog  fildlg(FALSE,strSuffix,default_name,OFN_HIDEREADONLY, strFilter);

	// Open the SAVEAS dialog box
	if ( fildlg.DoModal() == IDOK)
	{
		// Get full pathname of selected file 
		CString file_path = fildlg.GetPathName();

		// See if the file currently exists 
		if (DoesFileExist(file_path))
		{
			// See if the user wants to overwrite file 
			CString msg(_T(" The file: "));
			msg += file_path + _T(" exists. Overwrite it?");
			int stm = AfxMessageBox(msg,MB_YESNOCANCEL|MB_ICONQUESTION);
         if (stm != IDYES)
         {
            pExporter->Fail();
            return S_OK;
         }
         else
         {
            if (0 == ::DeleteFile( file_path ))
            {
               CString errMsg = CString(_T("Error deleting the file: \" ")) + file_path + CString(_T(" \". Could it be open in another program (e.g., Excel)? CAD export cannot continue."));
               ::AfxMessageBox(errMsg);
               pExporter->Fail();
               return E_FAIL;
            } 
         }
		}

      if (exportCADData::ctxLegacy == fileFormat)
      {
#pragma Reminder("Legacy TxDOT CAD Export is a hack. Consider removing this option at Version 6.0")
         // We have our file name and girder list. Create Legacy format in its own function so it can be removed later
         return TxDOT_WriteLegacyCADDataToFile(file_path, pBroker, girderKeys);
      }

      bool did_throw=false;

      // Create progress window in own scope
      try
      {
         GET_IFACE2(pBroker,IProgress,pProgress);

         bool multi = girderKeys.size()>1;
         DWORD mask = multi ? PW_ALL : PW_ALL|PW_NOGAUGE; // Progress window has a cancel button,
         CEAFAutoProgress ap(pProgress,0,mask); 

         if (multi)
            pProgress->Init(0,(short)girderKeys.size(),1);  // and for multi-girders, a gauge.

	      // Write CAD data to text file
         TxDOTCadWriter cadWriter;
         for (std::vector<CGirderKey>::iterator it = girderKeys.begin(); it!= girderKeys.end(); it++)
         {
            CGirderKey& girderKey(*it);

            txcwStrandLayoutType strand_layout = GetStrandLayoutType(pBroker, girderKey);

	         if (CAD_SUCCESS != cadWriter.WriteCADDataToFile(*pExporter, pBroker, girderKey, strand_layout, table_layout, isIBeam) )
            {
		         AfxMessageBox (_T("Warning: An error occured while writing to File"));
               pExporter->Fail();
		         return S_OK;
            }

            pProgress->Increment();
         }

		   // Close the open file
         if (!pExporter->Commit(file_path))
         {
            AfxMessageBox(_T("File Creation Aborted"), MB_ICONINFORMATION | MB_OK);
            pExporter->Fail();
            return E_FAIL;
         }
      } // autoprogress scope
      catch(...)
      {
         // must catch so progress window goes out of scope and gets destroyed
         // must rethrow to get the exception into MFC
         throw; 
      }

		// Notify completion 
	   CString msg(_T("File: "));
	   msg += file_path + _T(" file created successfully.");
      AfxMessageBox(msg,MB_ICONINFORMATION|MB_OK);

//         pProgress->UpdateMessage(_T("Writing Top Strand Research Data"));
//         raised_strand_research(pBroker, girderKeys);
	}

   return S_OK;
}

//////////////////////////////////////////////////
// IPGSDocumentation
STDMETHODIMP CTxDOTCadExporter::GetDocumentationSetName(BSTR* pbstrName) const
{
   CComBSTR bstrDocSetName(_T("TxCADExport"));
   bstrDocSetName.CopyTo(pbstrName);
   return S_OK;
}

STDMETHODIMP CTxDOTCadExporter::LoadDocumentationMap()
{
   CComBSTR bstrDocSetName;
   GetDocumentationSetName(&bstrDocSetName);

   CString strDocSetName(OLE2T(bstrDocSetName));

   CEAFApp* pApp = EAFGetApp();

   CString strDocumentationURL = GetDocumentationURL();

   CString strDocMapFile = EAFGetDocumentationMapFile(strDocSetName,strDocumentationURL);

   VERIFY(EAFLoadDocumentationMap(strDocMapFile,m_HelpTopics));
   return S_OK;
}

STDMETHODIMP CTxDOTCadExporter::GetDocumentLocation(UINT nHID,BSTR* pbstrURL) const
{
   auto found = m_HelpTopics.find(nHID);
   if ( found == m_HelpTopics.end() )
   {
      return E_FAIL;
   }

   CString strURL;
   strURL.Format(_T("%s%s"),GetDocumentationURL(),found->second);
   CComBSTR bstrURL(strURL);
   bstrURL.CopyTo(pbstrURL);
   return S_OK;
}

CString CTxDOTCadExporter::GetDocumentationURL() const
{
   CComBSTR bstrDocSetName;
   GetDocumentationSetName(&bstrDocSetName);
   CString strDocSetName(OLE2T(bstrDocSetName));

   CEAFApp* pApp = EAFGetApp();
   CString strDocumentationRootLocation = pApp->GetDocumentationRootLocation();

   CString strDocumentationURL;
   strDocumentationURL.Format(_T("%s%s/"), strDocumentationRootLocation, strDocSetName);

   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CVersionInfo verInfo;
   CString strAppName = AfxGetAppName(); // needs module state
   strAppName += _T(".dll");
   verInfo.Load(strAppName);

   CString strVersion = verInfo.GetProductVersionAsString(false);

   std::_tstring v(strVersion);
   auto count = std::count(std::begin(v), std::end(v), _T('.'));

   for (auto i = 0; i < count - 1; i++)
   {
      int pos = strVersion.ReverseFind(_T('.')); // find the last '.'
      strVersion = strVersion.Left(pos);
   }
   CString strURL;
   strURL.Format(_T("%s%s/"), strDocumentationURL, strVersion);
   strDocumentationURL = strURL;

   return strDocumentationURL;
}

CString CTxDOTCadExporter::GetExcelTemplateFolderLocation() const
{
   return m_strTemplateLocation;
}


/*
#include <IFace\Project.h>
#include <PgsExt\BridgeDescription2.h>
#include <IFace\Artifact.h>
#include <PgsExt\GirderArtifact.h>

void raised_strand_research(IBroker* pBroker, const std::vector<CGirderKey>& girderKeys)
{
   GET_IFACE2(pBroker, IStrandGeometry, pStrandGeometry);
   GET_IFACE2(pBroker, IPointOfInterest, pIPOI);
   GET_IFACE2(pBroker, IBridgeDescription, pIBridgeDesc);

   dbgLogDumpContext m_Log;
   ILogFile* __pLogFile__;
   DWORD __dwCookie__;
   HRESULT _xxHRxx_ = ::CoCreateInstance(CLSID_SysAgent, 0, CLSCTX_INPROC_SERVER, IID_ILogFile, (void**)&__pLogFile__);
   ATLASSERT(SUCCEEDED(_xxHRxx_));
   __pLogFile__->Open(_T("RaisedStrandResearch.log"), &__dwCookie__);
   m_Log.SetLog(__pLogFile__, __dwCookie__);
   __pLogFile__->Release();
   __pLogFile__ = 0;

   m_Log << _T("Span,Gdr,GdrType,Lspan,GdrDepth,DesignType,NS,NH,NDB,f'ci,f'c,Spec") << endl;


   // Since girder types, design info, etc can be different for each girder, process information for all
   // artifacts to get control data

   // Titles are now printed. Print results information
   ColumnIndexType idx = 0;
   ColumnIndexType col = 1;
   for (const auto& girderKey : girderKeys)
   {
      SegmentIndexType segIdx = 0;

      m_Log << LABEL_GROUP(girderKey.groupIndex) << _T(",") << girderKey.girderIndex+1;

      CSegmentKey segmentKey(girderKey, 0);

      const CBridgeDescription2* pBridgeDesc = pIBridgeDesc->GetBridgeDescription();
      const CGirderGroupData*    pGroup = pBridgeDesc->GetGirderGroup(segmentKey.groupIndex);
      const GirderLibraryEntry*  pGirderEntry = pGroup->GetGirder(segmentKey.girderIndex)->GetGirderLibraryEntry();
      const CSplicedGirderData*  pGdr = pGroup->GetGirder(girderKey.girderIndex);
      const CPrecastSegmentData* pSegment = pGdr->GetSegment(0);
      const CStrandData& strands = pSegment->Strands;

      m_Log << _T(",") << pGirderEntry->GetName();

      GET_IFACE2(pBroker, IBridge, pBridge);
      Float64 span_length = pBridge->GetSpanLength(segmentKey.groupIndex);

      span_length = ConvertFromSysUnits(span_length, unitMeasure::Feet);
      m_Log << _T(",") << span_length;

      Float64 gdr_hght = pGirderEntry->GetBeamHeight(pgsTypes::metEnd);
      gdr_hght = ConvertFromSysUnits(gdr_hght, unitMeasure::Inch);
      m_Log << _T(",") << gdr_hght;

      GET_IFACE2(pBroker, IArtifact, pIArtifact);
      const pgsGirderArtifact* pGirderArtifact = pIArtifact->GetGirderArtifact(girderKey);

      bool passed = pGirderArtifact->Passed();

      // Design type
      pgsTypes::AdjustableStrandType adjType = strands.GetAdjustableStrandType();
      StrandIndexType ns = strands.GetStrandCount(pgsTypes::Straight);
      StrandIndexType nh = strands.GetStrandCount(pgsTypes::Harped);
      StrandIndexType ndb = strands.GetDebondCount(pgsTypes::Straight, pgsTypes::metEnd, pGirderEntry);
      CStrandData::StrandDefinitionType sdt = strands.GetStrandDefinitionType();
      if (!passed || ns + nh == 0)
      {
         m_Log << _T(",") << 0;// _T("noStrands");
      }
      else if (CStrandData::sdtTotal == sdt || CStrandData::sdtStraightHarped == sdt)
      {
         if (nh > 0 && pgsTypes::asHarped == adjType)
         {
            m_Log << _T(",") << 1;// _T("harped");
         }
         else if (ndb > 0)
         {
            m_Log << _T(",") << 2;// _T("debond");
         }
         else
         {
            m_Log << _T(",") << 3;// _T("straight");
         }
      }
      else
      {
         // direct filled patterns use raised straight strands
         if (nh > 0 && pgsTypes::asHarped == adjType)
         {
            m_Log << _T(",") << 4;// _T("harpedRaised");
         }
         else if (ndb > 0)
         {
            m_Log << _T(",") << 5;// _T("debondRaised");
         }
         else
         {
            m_Log << _T(",") << 6;// _T("straightRaised");
         }
      }

      m_Log << _T(",") << ns;
      m_Log << _T(",") << nh;
      m_Log << _T(",") << ndb;

      Float64 fci = pSegment->Material.Concrete.Fci;
      fci = ::ConvertFromSysUnits(fci, unitMeasure::KSI);
      m_Log << _T(",") << fci;

      Float64 fc = pSegment->Material.Concrete.Fc;
      fc = ::ConvertFromSysUnits(fc, unitMeasure::KSI);
      m_Log << _T(",") << fc;

      if (passed)
      {
         m_Log << _T(", Passed");
      }
      else
      {
         m_Log << _T(", Failed");
      }

      m_Log << endl;
   }
}
*/