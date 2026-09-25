///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright © 1999-2026  Washington State Department of Transportation
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
#include "DesignLog.h"
#include <PgsExt\SegmentDesignArtifact.h>
#include <PGSuperTypes.h>
#include <exception>
#include <filesystem>

namespace pgsDesignLog
{
   // true while a designer log is open. Checked by every DLOG statement.
   static bool g_bEnabled = false;

   // Reads a setting from the environment. Returns false if it is not set.
   static bool GetSetting(LPCTSTR lpszName, std::_tstring* pValue)
   {
      DWORD n = ::GetEnvironmentVariable(lpszName, nullptr, 0);
      if (n == 0)
      {
         return false;
      }

      std::vector<TCHAR> buffer(n);
      n = ::GetEnvironmentVariable(lpszName, buffer.data(), (DWORD)buffer.size());
      if (n == 0 || buffer.size() <= n)
      {
         return false;
      }

      *pValue = buffer.data();
      return true;
   }

   LPCTSTR OutcomeName(int outcome)
   {
      // order must match pgsSegmentDesignArtifact::Outcome
      static LPCTSTR names[] =
      {
         _T("Success"),
         _T("SuccessButLongitudinalBarsNeeded4FlexuralTensionCy"),
         _T("SuccessButLongitudinalBarsNeeded4FlexuralTensionLifting"),
         _T("SuccessButLongitudinalBarsNeeded4FlexuralTensionHauling"),
         _T("TooManyStrandsReqd"),
         _T("ReleaseStrength"),
         _T("OverReinforced"),
         _T("UnderReinforced"),
         _T("UltimateMomentCapacity"),
         _T("StrandSlopeOutOfRange"),
         _T("ExceededMaxHoldDownForce"),
         _T("ShearExceedsMaxConcreteStrength"),
         _T("TooManyStirrupsReqd"),
         _T("TooManyStirrupsReqdForHorizontalInterfaceShear"),
         _T("TooManyStirrupsReqdForSplitting"),
         _T("ConflictWithLongReinforcementShearSpec"),
         _T("TooManyBarsForLongReinfShear"),
         _T("TooMuchStrandsForLongReinfShear"),
         _T("StrandsReqdForLongReinfShearAndFlexureTurnedOff"),
         _T("NoDevelopmentLengthForLongReinfShear"),
         _T("NoStrandDevelopmentLengthForLongReinfShear"),
         _T("RebarForceExceedsPretensionForceForLongReinfShear"),
         _T("MaxIterExceeded"),
         _T("GirderLiftingConcreteStrength"),
         _T("GirderLiftingStability"),
         _T("GirderShippingConcreteStrength"),
         _T("GirderShippingStability"),
         _T("GirderShippingConfiguration"),
         _T("StressExceedsConcreteStrength"),
         _T("DebondDesignFailed"),
         _T("DesignCancelled"),
         _T("NoDesignRequested"),
         _T("LldfRangeOfApplicabilityError"),
         _T("DesignNotSupported_Losses"),
         _T("DesignNotSupported_Strands"),
         _T("DesignNotSupported_Material"),
      };
      static_assert(sizeof(names)/sizeof(names[0]) == pgsSegmentDesignArtifact::DesignNotSupported_Material + 1, "OutcomeName table is out of sync with pgsSegmentDesignArtifact::Outcome");

      if (outcome < 0 || (int)(sizeof(names)/sizeof(names[0])) <= outcome)
      {
         return _T("UnknownOutcome");
      }

      return names[outcome];
   }

   bool IsDetailEnabled()
   {
      std::_tstring value;
      return GetSetting(_T("PGS_DESIGN_LOG_DETAIL"), &value) && !value.empty() && value[0] != _T('0');
   }

   bool IsEnabled()
   {
      return g_bEnabled;
   }

   bool IsRequested()
   {
      std::_tstring value;
      if (GetSetting(_T("PGS_DESIGN_LOG"), &value) && !value.empty())
      {
         return value[0] != _T('0');
      }

#if defined ENABLE_LOGGING
      return true;  // Debug builds log by default (this was the behavior before the log could be controlled)
#else
      return false; // Release builds only log on request
#endif
   }

   static void ReplaceToken(std::_tstring& str, LPCTSTR lpszToken, const std::_tstring& value)
   {
      std::_tstring token(lpszToken);
      std::_tstring::size_type pos;
      while ((pos = str.find(token)) != std::_tstring::npos)
      {
         str.replace(pos, token.size(), value);
      }
   }

   std::_tstring GetLogFilePath(LPCTSTR lpszProjectTitle, LPCTSTR lpszProjectFolder)
   {
#if defined _WIN64
      std::_tstring strDefaultName(_T("Designer_x64.log"));
#else
      std::_tstring strDefaultName(_T("Designer.log"));
#endif

      std::_tstring strProject(lpszProjectTitle ? lpszProjectTitle : _T(""));
      std::_tstring strPath;
      if (!GetSetting(_T("PGS_DESIGN_LOG_FILE"), &strPath) || strPath.empty())
      {
         return strDefaultName; // current working folder, same as before this setting existed
      }

      // A folder was given, use a file name based on the project name
      std::error_code ec;
      TCHAR last = strPath.back();
      if (last == _T('\\') || last == _T('/') || std::filesystem::is_directory(std::filesystem::path(strPath), ec))
      {
         if (last != _T('\\') && last != _T('/'))
         {
            strPath += _T('\\');
         }
         strPath += strProject.empty() ? strDefaultName : std::_tstring(_T("{project}_Designer.log"));
      }

      SYSTEMTIME st;
      ::GetLocalTime(&st);
      TCHAR timestamp[32];
      _stprintf_s(timestamp, _T("%04d%02d%02d_%02d%02d%02d"), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

      TCHAR pid[16];
      _stprintf_s(pid, _T("%lu"), ::GetCurrentProcessId());

      ReplaceToken(strPath, _T("{project}"), strProject.empty() ? std::_tstring(_T("Untitled")) : strProject);
      ReplaceToken(strPath, _T("{projectdir}"), std::_tstring(lpszProjectFolder ? lpszProjectFolder : _T(".")));
      ReplaceToken(strPath, _T("{timestamp}"), std::_tstring(timestamp));
      ReplaceToken(strPath, _T("{pid}"), std::_tstring(pid));

      return strPath;
   }

   WBFL::Debug::LogContext& NullLog()
   {
      // a log context without a log file discards everything written to it
      static LogContext nullLog;
      return nullLog;
   }

   std::_tstring DescribeDesignOptions(const arDesignOptions& options)
   {
      static LPCTSTR flexure[] = { _T("None"), _T("Harping"), _T("Debonding"), _T("FullyBonded"), _T("FullyBondedRaised"), _T("DebondingRaised") };
      static LPCTSTR concrete[] = { _T("PreserveStrength"), _T("DesignForMinStrength") };
      static LPCTSTR fill[] = { _T("GridOrder"), _T("MinimizeHarping"), _T("DirectFill") };
      static LPCTSTR slabOffset[] = { _T("PreserveHaunch"), _T("DesignHaunch"), _T("Default") };
      static LPCTSTR shear[] = { _T("None"), _T("LayoutStirrups"), _T("RetainExistingLayout") };

      auto name = [](LPCTSTR* names, size_t n, int value) { return (0 <= value && (size_t)value < n) ? names[value] : _T("?"); };

      std::_tostringstream os;
      os << _T("flexure=") << name(flexure, sizeof(flexure)/sizeof(flexure[0]), options.doDesignForFlexure)
         << _T(", concrete=") << name(concrete, sizeof(concrete)/sizeof(concrete[0]), options.doDesignConcreteStrength)
         << _T(", fill=") << name(fill, sizeof(fill)/sizeof(fill[0]), options.doStrandFillType)
         << (options.doForceHarpedStrandsStraight ? _T(" (harped forced straight)") : _T(""))
         << _T(", slab offset=") << name(slabOffset, sizeof(slabOffset)/sizeof(slabOffset[0]), options.doDesignSlabOffset)
         << _T(", shear=") << name(shear, sizeof(shear)/sizeof(shear[0]), options.doDesignForShear)
         << _T(", lifting=") << (options.doDesignLifting ? _T("Y") : _T("N"))
         << _T(", hauling=") << (options.doDesignHauling ? _T("Y") : _T("N"))
         << _T(", slope=") << (options.doDesignSlope ? _T("Y") : _T("N"))
         << _T(", hold down=") << (options.doDesignHoldDown ? _T("Y") : _T("N"));

      if (0.0 < options.maxFc || 0.0 < options.maxFci)
      {
         os << _T(", max f'ci=") << ksi(options.maxFci) << _T(" ksi, max f'c=") << ksi(options.maxFc) << _T(" ksi");
      }

      return os.str();
   }

   void WriteLegend(WBFL::Debug::LogContext& log)
   {
      log << _T("Log conventions:") << WBFL::Debug::endl;
      log << _T("  [iNN]      outer design iteration (all lines within an iteration); [---] outside the iteration loop") << WBFL::Debug::endl;
      log << _T("  >> / <<    start / end of a design step; end line gives elapsed time and result; nested lines indented with '|'") << WBFL::Debug::endl;
      log << _T("  [FAIL]     a check did not pass     [ACTION]  the designer changed the design") << WBFL::Debug::endl;
      log << _T("  [RESTART]  outer iteration restarts [ABORT]   design gave up (outcome follows)") << WBFL::Debug::endl;
      log << _T("  [WARN]     unexpected condition     [OK]      step or check passed") << WBFL::Debug::endl;
      log << _T("  Units: x locations in ft from start of segment, section dimensions in in, stress ksi, force kip, moment kip-ft") << WBFL::Debug::endl;
      log << _T("  Set environment variable PGS_DESIGN_LOG_DETAIL=1 for per-calculation detail (") << (IsDetailEnabled() ? _T("currently ON") : _T("currently OFF")) << _T(")") << WBFL::Debug::endl;
      log << _T("  Each girder design ends with a DESIGN SUMMARY block (outcome, restarts, final state)") << WBFL::Debug::endl;
   }
};

pgsDesignLogContext::pgsDesignLogContext() :
   LogContext()
{
}

pgsDesignLogContext::~pgsDesignLogContext()
{
   Close();
}

bool pgsDesignLogContext::Open(const std::_tstring& strFilePath)
{
   Close();

   CComPtr<ILogFile> logFile;
   HRESULT hr = logFile.CoCreateInstance(CLSID_LogFile);
   if (FAILED(hr))
   {
      return false;
   }

   // the folder must exist for the log file to be created
   std::error_code ec;
   std::filesystem::path parent = std::filesystem::path(strFilePath).parent_path();
   if (!parent.empty())
   {
      std::filesystem::create_directories(parent, ec);
   }

   DWORD dwCookie;
   hr = logFile->Open(strFilePath.c_str(), &dwCookie);
   if (FAILED(hr))
   {
      std::_tstring msg(_T("PGSuper: Unable to open designer log file ") + strFilePath + _T("\n"));
      ::OutputDebugString(msg.c_str());
      return false;
   }

   LogContext::SetLog(logFile, dwCookie);
   m_strFilePath = strFilePath;
   m_Depth = 0;
   m_Iteration = INVALID_INDEX;
   pgsDesignLog::g_bEnabled = true;
   return true;
}

bool pgsDesignLogContext::IsOpen() const
{
   return !m_strFilePath.empty();
}

const std::_tstring& pgsDesignLogContext::GetFilePath() const
{
   return m_strFilePath;
}

void pgsDesignLogContext::Close()
{
   if (!IsOpen())
   {
      return;
   }

   if (m_bLinePending)
   {
      WriteLine();
   }

   CComPtr<ILogFile> logFile;
   DWORD dwCookie;
   GetLog(&logFile, &dwCookie);
   if (logFile)
   {
      logFile->Close(dwCookie); // writes the "Log closed" time stamp
   }

   LogContext::SetLog(nullptr, 0);
   m_strFilePath.clear();
   pgsDesignLog::g_bEnabled = false;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(const std::_tstring& s)
{
   m_Line << s;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(LPCTSTR s)
{
   if (s)
   {
      m_Line << s;
   }
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(TCHAR c)
{
   m_Line << c;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(DWORD n)
{
   m_Line << n;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(bool n)
{
   m_Line << (n ? _T("True") : _T("False"));
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(Int16 n)
{
   m_Line << n;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(Uint16 n)
{
   m_Line << n;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(Int32 n)
{
   m_Line << n;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(Uint32 n)
{
   m_Line << n;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(Int64 n)
{
   m_Line << n;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(Uint64 n)
{
   m_Line << n;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(Float32 n)
{
   m_Line << n;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(Float64 n)
{
   m_Line << n;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(Float80 n)
{
   m_Line << n;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(void* n)
{
   m_Line << n;
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::operator<<(const WBFL::System::SectionValue& n)
{
   m_Line << n.AsString();
   m_bLinePending = true;
   return *this;
}

WBFL::Debug::LogContext& pgsDesignLogContext::EndLine()
{
   WriteLine();
   return *this;
}

void pgsDesignLogContext::BeginScope()
{
   m_Depth++;
}

void pgsDesignLogContext::EndScope()
{
   ATLASSERT(0 < m_Depth);
   if (0 < m_Depth)
   {
      m_Depth--;
   }
}

void pgsDesignLogContext::SetIteration(IndexType iter)
{
   m_Iteration = iter;
}

void pgsDesignLogContext::WriteLine()
{
   std::_tostringstream osPrefix;
   if (m_Iteration == INVALID_INDEX)
   {
      osPrefix << _T("[---] ");
   }
   else
   {
      osPrefix << _T("[i") << std::setw(2) << std::setfill(_T('0')) << m_Iteration << _T("] ");
   }

   for (IndexType i = 0; i < m_Depth; i++)
   {
      osPrefix << _T("|  ");
   }
   std::_tstring prefix(osPrefix.str());

   std::_tstring text(m_Line.str());
   m_Line.str(std::_tstring());
   m_Line.clear();
   m_bLinePending = false;

   // Prefix every physical line, including those created by new-line characters embedded in the message
   std::_tstring out;
   std::_tstring::size_type start = 0;
   while (true)
   {
      std::_tstring::size_type pos = text.find(_T('\n'), start);
      std::_tstring line = prefix + text.substr(start, pos == std::_tstring::npos ? std::_tstring::npos : pos - start);
      std::_tstring::size_type last = line.find_last_not_of(_T(" \t\r"));
      line.erase(last == std::_tstring::npos ? 0 : last + 1);
      out += line;
      out += _T('\n');

      if (pos == std::_tstring::npos || pos + 1 == text.size())
      {
         break;
      }

      start = pos + 1;
   }

   LogContext::operator<<(out.c_str());
}

pgsDesignLogScope::pgsDesignLogScope(WBFL::Debug::LogContext& log, std::_tstring&& title) :
   m_bEnabled(pgsDesignLog::IsEnabled()),
   m_Log(log),
   m_pDesignLog(nullptr),
   m_Title(std::move(title)),
   m_Start(std::chrono::steady_clock::now())
{
   if (!m_bEnabled)
   {
      return;
   }

   m_pDesignLog = dynamic_cast<pgsDesignLogContext*>(&log);
   m_Log << _T(">> ") << m_Title << WBFL::Debug::endl;
   if (m_pDesignLog)
   {
      m_pDesignLog->BeginScope();
   }
}

pgsDesignLogScope::~pgsDesignLogScope()
{
   if (!m_bEnabled)
   {
      return;
   }

   try
   {
      if (m_pDesignLog)
      {
         m_pDesignLog->EndScope();
      }

      std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - m_Start;

      std::_tostringstream os;
      os << _T("<< ") << m_Title << _T(" (") << std::fixed << std::setprecision(3) << elapsed.count() << _T(" s)");
      if (!m_Result.empty())
      {
         os << _T(" ") << m_Result;
      }

      if (0 < std::uncaught_exceptions())
      {
         os << _T(" [scope exited by exception - design cancelled or error]");
      }

      m_Log << os.str() << WBFL::Debug::endl;
   }
   catch (...)
   {
      // never throw from a destructor
   }
}

void pgsDesignLogScope::SetResult(const std::_tstring& result)
{
   m_Result = result;
}

