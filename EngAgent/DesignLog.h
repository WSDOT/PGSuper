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

#pragma once

// Structured logging for the girder designer (Designer_x64.log)
//
// The designer log is meant to be read by a person trying to follow the design
// algorithm and by tools (grep, scripts, LLMs) trying to diagnose a failed or
// non-converging design. See devdocs/DesignerLog.md for details. Conventions:
//
//  * Every line inside an outer design iteration starts with "[iNN]". Lines logged
//    outside the outer loop start with "[---]". Grep for "[i07]" to see iteration 7.
//  * Design steps are logged as scopes. A scope opens with ">> Name" and closes with
//    "<< Name (1.234 s) result". Lines inside a scope are indented with "|  " so
//    nesting is visible. Scopes are exception safe; a cancelled design still closes them.
//  * Important events are tagged so they can be found with a single search:
//       [FAIL]     a check did not pass (what, where, demand vs. limit)
//       [ACTION]   the designer changed the design (strands, f'c, f'ci, debonding, offsets...)
//       [RESTART]  the outer design loop restarts; the reason follows
//       [ABORT]    the design gave up; the outcome code follows
//       [WARN]     an unexpected condition or a possible non-convergence
//       [OK]       a step or check completed successfully
//  * Units are always stated: locations in ft (from the start of the segment unless
//    noted), section dimensions in in, stress in ksi, force in kip, moment kip-ft.
//
// The designer log is compiled into all builds and controlled at run time with
// environment variables (read when a design starts):
//
//    PGS_DESIGN_LOG=1|0         enable/disable the log. Default: enabled in Debug builds
//                               (ENABLE_LOGGING), disabled in Release builds.
//    PGS_DESIGN_LOG_FILE=path   log file name and location. May be a directory (ends with \ or
//                               is an existing folder), in which case the file name is
//                               "{project}_Designer.log". Tokens: {project} (project file title),
//                               {projectdir} (folder containing the project), {timestamp}
//                               (yyyymmdd_hhmmss when the log is opened), {pid} (process id).
//                               Default: Designer_x64.log (Designer.log for 32-bit) in the
//                               current working folder, same as before.
//    PGS_DESIGN_LOG_DETAIL=1    also log bulky per-calculation detail (LOG_DETAIL)
//
// When the log is disabled, DLOG statements do not evaluate their arguments, so there is
// no cost beyond testing a flag. Designer code uses DLOG (not LOG) so that it is independent
// of ENABLE_LOGGING, which controls the other agent logs.

#include <IFace/Tools.h>
#include <Units\Convert.h>
#include <WBFLTools\LogContext.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>

struct arDesignOptions;

// Designer logging can be completely removed from the build by commenting this out
#define ENABLE_DESIGN_LOGGING

namespace pgsDesignLog
{
   // Unit conversions for log messages. These keep log statements short and make
   // the unit that is logged obvious at the call site.
   inline Float64 ksi(Float64 v)   { return WBFL::Units::ConvertFromSysUnits(v, WBFL::Units::Measure::KSI); }
   inline Float64 ft(Float64 v)    { return WBFL::Units::ConvertFromSysUnits(v, WBFL::Units::Measure::Feet); }
   inline Float64 in(Float64 v)    { return WBFL::Units::ConvertFromSysUnits(v, WBFL::Units::Measure::Inch); }
   inline Float64 kip(Float64 v)   { return WBFL::Units::ConvertFromSysUnits(v, WBFL::Units::Measure::Kip); }
   inline Float64 kipft(Float64 v) { return WBFL::Units::ConvertFromSysUnits(v, WBFL::Units::Measure::KipFeet); }

   // Formats a value with a fixed number of decimals and an optional minimum width.
   // Use in tabular output so columns line up.
   inline std::_tstring Fixed(Float64 v, int decimals, int width = 0)
   {
      std::_tostringstream os;
      os << std::fixed << std::setprecision(decimals);
      if (0 < width)
      {
         os << std::setw(width);
      }
      os << v;
      return os.str();
   }

   // Name of a pgsSegmentDesignArtifact::Outcome value (passed as int so this header does not
   // depend on the artifact header)
   LPCTSTR OutcomeName(int outcome);

   // One line description of design options, e.g. "flexure=Debonding, fill=GridOrder, shear=LayoutStirrups, ..."
   std::_tstring DescribeDesignOptions(const arDesignOptions& options);

   // Returns true if a designer log is open and being written. DLOG statements do nothing otherwise.
   bool IsEnabled();

   // Returns true if detailed logging (LOG_DETAIL) is requested (PGS_DESIGN_LOG_DETAIL)
   bool IsDetailEnabled();

   // Returns true if the designer log should be written (PGS_DESIGN_LOG, or the build default)
   bool IsRequested();

   // Resolves the log file path from PGS_DESIGN_LOG_FILE (or the default) for the given project
   std::_tstring GetLogFilePath(LPCTSTR lpszProjectTitle, LPCTSTR lpszProjectFolder);

   // Writes the log conventions (legend) to the log
   void WriteLegend(WBFL::Debug::LogContext& log);

   // A log that discards everything. Used by callers of the design tools that don't have a designer log.
   WBFL::Debug::LogContext& NullLog();
};

// Designer log. Buffers each line and writes it with the iteration prefix and scope indentation.
// Anything holding a WBFL::Debug::LogContext& to it (strand design tool, hauling checkers, etc)
// gets the same formatting for free.
class pgsDesignLogContext : public LogContext
{
public:
   pgsDesignLogContext();
   virtual ~pgsDesignLogContext();

   pgsDesignLogContext(const pgsDesignLogContext&) = delete;
   pgsDesignLogContext& operator=(const pgsDesignLogContext&) = delete;

   // Opens the log file. Returns false if it could not be opened.
   bool Open(const std::_tstring& strFilePath);
   bool IsOpen() const;
   const std::_tstring& GetFilePath() const;
   void Close();

   virtual WBFL::Debug::LogContext& operator<<(const std::_tstring& s) override;
   virtual WBFL::Debug::LogContext& operator<<(LPCTSTR s) override;
   virtual WBFL::Debug::LogContext& operator<<(TCHAR c) override;
   virtual WBFL::Debug::LogContext& operator<<(DWORD n) override;
   virtual WBFL::Debug::LogContext& operator<<(bool n) override;
   virtual WBFL::Debug::LogContext& operator<<(Int16 n) override;
   virtual WBFL::Debug::LogContext& operator<<(Uint16 n) override;
   virtual WBFL::Debug::LogContext& operator<<(Int32 n) override;
   virtual WBFL::Debug::LogContext& operator<<(Uint32 n) override;
   virtual WBFL::Debug::LogContext& operator<<(Int64 n) override;
   virtual WBFL::Debug::LogContext& operator<<(Uint64 n) override;
   virtual WBFL::Debug::LogContext& operator<<(Float32 n) override;
   virtual WBFL::Debug::LogContext& operator<<(Float64 n) override;
   virtual WBFL::Debug::LogContext& operator<<(Float80 n) override;
   virtual WBFL::Debug::LogContext& operator<<(void* n) override;
   virtual WBFL::Debug::LogContext& operator<<(const WBFL::System::SectionValue& n) override;

   virtual WBFL::Debug::LogContext& EndLine() override;

   // Scope depth controls indentation. Use pgsDesignLogScope rather than calling these directly.
   void BeginScope();
   void EndScope();

   // Outer design iteration number written at the start of every line. INVALID_INDEX clears it.
   void SetIteration(IndexType iter);

private:
   void WriteLine();

   std::_tstring m_strFilePath;
   std::_tostringstream m_Line;
   bool m_bLinePending = false;
   IndexType m_Depth = 0;
   IndexType m_Iteration = INVALID_INDEX;
};

// RAII helper that opens a scope in the log, indents everything logged while it is alive,
// and closes the scope with the elapsed time and an optional result. Does nothing when the
// designer log is not enabled.
class pgsDesignLogScope
{
public:
   pgsDesignLogScope(WBFL::Debug::LogContext& log, std::_tstring&& title);
   ~pgsDesignLogScope();

   pgsDesignLogScope(const pgsDesignLogScope&) = delete;
   pgsDesignLogScope& operator=(const pgsDesignLogScope&) = delete;

   // Text appended to the closing line (e.g. "RESTART - f'c changed")
   void SetResult(const std::_tstring& result);

private:
   bool m_bEnabled;
   WBFL::Debug::LogContext& m_Log;
   pgsDesignLogContext* m_pDesignLog; // nullptr if m_Log isn't a design log (no indentation)
   std::_tstring m_Title;
   std::_tstring m_Result;
   std::chrono::steady_clock::time_point m_Start;
};

// Log types used by the designer and the tools and checkers it shares its log with.
// These are the same in all builds, unlike SHARED_LOGFILE and friends in IFace/Tools.h.
#define DESIGN_SHARED_LOGFILE WBFL::Debug::LogContext&
#define DESIGN_LOGFILE m_Log
#define DECLARE_DESIGN_SHARED_LOGFILE WBFL::Debug::LogContext& m_Log
#define DECLARE_DESIGN_LOGFILE mutable pgsDesignLogContext m_Log
#define DESIGN_LOGGER static_cast<WBFL::Debug::LogContext&>(m_Log)

// For code outside the designer (e.g. EngAgentImp) that calls design tools/checkers.
// Passes the caller's own log in builds with ENABLE_LOGGING, otherwise a log that discards everything.
#if defined ENABLE_LOGGING
#define AGENT_LOGGER_FOR_DESIGN_TOOLS static_cast<WBFL::Debug::LogContext&>(const_cast<LogContext&>(static_cast<const LogContext&>(m_Log)))
#else
#define AGENT_LOGGER_FOR_DESIGN_TOOLS pgsDesignLog::NullLog()
#endif

// The LOGGER macro in Tools.h uses const_cast<LogContext*>, which does not compile when m_Log
// is derived from LogContext. This version works for LogContext and any class derived from it.
#if defined ENABLE_LOGGING
#undef LOGGER
#define LOGGER const_cast<LogContext&>(static_cast<const LogContext&>(m_Log))
#endif

#if defined ENABLE_DESIGN_LOGGING

// Writes a line to the designer log. Arguments are only evaluated if the log is enabled.
#define DLOG(_x_) do { if (pgsDesignLog::IsEnabled()) { m_Log << _x_ << WBFL::Debug::endl; } } while(0)

// Bulky, per-calculation detail. Only written when PGS_DESIGN_LOG_DETAIL=1
#define LOG_DETAIL(_x_) do { if (pgsDesignLog::IsEnabled() && pgsDesignLog::IsDetailEnabled()) { m_Log << _x_ << WBFL::Debug::endl; } } while(0)

// Opens a log scope that lasts until the end of the enclosing C++ block.
// _x_ is a stream expression, e.g. DESIGN_LOG_SCOPE(_T("DesignMidZone trial ") << iter)
// Only one DESIGN_LOG_SCOPE is allowed per C++ block. The title is not built if the log is disabled.
#define DESIGN_LOG_SCOPE(_x_) \
   pgsDesignLogScope _design_log_scope_(DESIGN_LOGFILE, pgsDesignLog::IsEnabled() ? [&]() { std::_tostringstream _os_; _os_ << _x_; return _os_.str(); }() : std::_tstring())

// Sets the text written on the closing line of the innermost DESIGN_LOG_SCOPE
#define DESIGN_LOG_SCOPE_RESULT(_x_) \
   do { if (pgsDesignLog::IsEnabled()) { std::_tostringstream _os_; _os_ << _x_; _design_log_scope_.SetResult(_os_.str()); } } while(0)

// Sets the outer iteration prefix. Only valid where m_Log is a pgsDesignLogContext (the designer)
#define DESIGN_LOG_SET_ITERATION(_i_) m_Log.SetIteration(_i_)

// Code that only exists for logging, e.g. building up a message over a loop
#define DESIGN_LOG_ONLY(_x_) _x_

#else

#define DLOG(_x_)
#define LOG_DETAIL(_x_)
#define DESIGN_LOG_SCOPE(_x_)
#define DESIGN_LOG_SCOPE_RESULT(_x_)
#define DESIGN_LOG_SET_ITERATION(_i_)
#define DESIGN_LOG_ONLY(_x_)

#endif // ENABLE_DESIGN_LOGGING

// Tagged log messages (see conventions above)
#define LOG_FAIL(_x_)    DLOG(_T("[FAIL] ")    << _x_)
#define LOG_ACTION(_x_)  DLOG(_T("[ACTION] ")  << _x_)
#define LOG_RESTART(_x_) DLOG(_T("[RESTART] ") << _x_)
#define LOG_ABORT(_x_)   DLOG(_T("[ABORT] ")   << _x_)
#define LOG_WARN(_x_)    DLOG(_T("[WARN] ")    << _x_)
#define LOG_OK(_x_)      DLOG(_T("[OK] ")      << _x_)
