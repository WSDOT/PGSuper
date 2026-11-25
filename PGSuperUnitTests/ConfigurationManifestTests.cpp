///////////////////////////////////////////////////////////////////////
// PGSuper - Prestressed Girder SUPERstructure Design and Analysis
// Copyright (C) 1999-2026  Washington State Department of Transportation
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

// Tests for ConfigurationPackages.json, the legacy <App>Packages.ini reader, version selection
// and the MD5 helpers (ConfigPackager\ConfigurationManifest.h). These are shared by the programs
// (PgsExt) and ConfigPackager.

#include <windows.h>
#include "CppUnitTest.h"
#include "ConfigurationManifest.h"

#include <filesystem>
#include <fstream>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using namespace ConfigManifest;
namespace fs = std::filesystem;

namespace
{
   // A folder in %TEMP% for test files, removed when the test ends
   class TempFolder
   {
   public:
      TempFolder()
      {
         wchar_t name[64];
         swprintf_s(name, L"PGSuperUnitTests_%lu_%llu", ::GetCurrentProcessId(), ::GetTickCount64());
         m_Path = fs::temp_directory_path() / name;
         fs::create_directories(m_Path);
      }
      ~TempFolder()
      {
         std::error_code ec;
         fs::remove_all(m_Path, ec);
      }
      fs::path Write(const std::wstring& name, const std::string& content) const
      {
         fs::path file = m_Path / name;
         fs::create_directories(file.parent_path());
         std::ofstream out(file, std::ios::binary);
         out << content;
         return file;
      }
      const fs::path& Path() const { return m_Path; }
   private:
      fs::path m_Path;
   };

   const char* g_Sample = R"({
  "schemaVersion": 1,
  "applications": {
    "PGSuper": {
      "configurations": [
        {
          "name": "WSDOT",
          "webLink": "https://wsdot.wa.gov/eesc/bridge/software/configurations/LibraryInfo.html",
          "packages": [
            { "appVersion": "8.0.0", "url": "PGSuper/Version_8.0.0/WSDOT.zip", "md5": "0123456789ABCDEF0123456789abcdef" },
            { "appVersion": "9.0.0", "url": "PGSuper/Version_9.0.0/WSDOT.zip" },
            { "appVersion": "9.1.0", "url": "https://example.org/WSDOT.zip" }
          ]
        },
        {
          "name": "PCI UHPC",
          "packages": [
            { "appVersion": "7.1.0", "url": "PGSuper/Version_7.1.0/PCI_UHPC.zip" }
          ]
        }
      ]
    },
    "PGSplice": {
      "configurations": [
        { "name": "WSDOT", "packages": [ { "appVersion": "9.0.0", "url": "PGSplice/Version_9.0.0/WSDOT.zip" } ] }
      ]
    }
  }
})";

   std::string MakeManifest(const std::string& package)
   {
      return R"({"schemaVersion":1,"applications":{"PGSuper":{"configurations":[{"name":"A","packages":[)" + package + "]}]}}}";
   }
}

namespace ConfigurationTests
{
   TEST_CLASS(ManifestReading)
   {
   public:
      TEST_METHOD(ReadsSample)
      {
         Manifest manifest;
         std::wstring error;
         Assert::IsTrue(ParseManifest(g_Sample, manifest, error), error.c_str());
         Assert::AreEqual(1, manifest.SchemaVersion);
         Assert::AreEqual(size_t(2), manifest.Applications.size());

         const Application* pApp = manifest.FindApplication(L"PGSuper");
         Assert::IsNotNull(pApp);
         Assert::AreEqual(size_t(2), pApp->Configurations.size());

         const Configuration* pConfig = pApp->FindConfiguration(L"WSDOT");
         Assert::IsNotNull(pConfig);
         Assert::AreEqual(std::wstring(L"https://wsdot.wa.gov/eesc/bridge/software/configurations/LibraryInfo.html"), pConfig->WebLink);
         Assert::AreEqual(size_t(3), pConfig->Packages.size());
         Assert::AreEqual(std::wstring(L"0123456789abcdef0123456789abcdef"), pConfig->Packages[0].Md5, L"md5 is stored in lower case");
         Assert::IsTrue(pConfig->Packages[1].Md5.empty(), L"md5 is optional");
      }

      TEST_METHOD(ApplicationNamesIgnoreCase)
      {
         Manifest manifest;
         std::wstring error;
         Assert::IsTrue(ParseManifest(g_Sample, manifest, error));
         Assert::IsNotNull(manifest.FindApplication(L"pgsplice"));
         Assert::IsNotNull(manifest.FindApplication(L"PGSUPER"));
      }

      TEST_METHOD(MissingApplicationIsNotAnError)
      {
         Manifest manifest;
         std::wstring error;
         Assert::IsTrue(ParseManifest(g_Sample, manifest, error));
         Assert::IsNull(manifest.FindApplication(L"TOGA"));
      }

      TEST_METHOD(AcceptsByteOrderMarkAndUnicode)
      {
         Manifest manifest;
         std::wstring error;
         std::string text = "\xEF\xBB\xBF" + MakeManifest(R"({"appVersion":"1.0.0","url":"a.zip"})");
         text.replace(text.find("\"A\""), 3, "\"Caf\xC3\xA9\"");
         Assert::IsTrue(ParseManifest(text, manifest, error), error.c_str());
         Assert::AreEqual(std::wstring(L"Caf\u00e9"), manifest.Applications[0].Configurations[0].Name);
      }

      TEST_METHOD(AcceptsNewerSchemaVersion)
      {
         Manifest manifest;
         std::wstring error;
         std::string text = MakeManifest(R"({"appVersion":"1.0.0","url":"a.zip"})");
         text.replace(text.find("\"schemaVersion\":1"), 17, "\"schemaVersion\":2");
         Assert::IsTrue(ParseManifest(text, manifest, error), error.c_str());
         Assert::AreEqual(2, manifest.SchemaVersion);
      }

      TEST_METHOD(RejectsInvalidFiles)
      {
         Manifest manifest;
         std::wstring error;
         Assert::IsFalse(ParseManifest("not json", manifest, error), L"not JSON");
         Assert::IsFalse(ParseManifest(R"({"applications":{"PGSuper":{"configurations":[]}}})", manifest, error), L"no schemaVersion");
         Assert::IsFalse(ParseManifest(R"({"schemaVersion":1})", manifest, error), L"no applications");
         Assert::IsFalse(ParseManifest(R"({"schemaVersion":1,"applications":{"PGSuper":{}}})", manifest, error), L"no configurations");
         Assert::IsFalse(ParseManifest(R"({"schemaVersion":1,"applications":{"PGSuper":{"configurations":[{"packages":[]}]}}})", manifest, error), L"no name");
         Assert::IsFalse(ParseManifest(R"({"schemaVersion":1,"applications":{"PGSuper":{"configurations":[{"name":"A"}]}}})", manifest, error), L"no packages");
         Assert::IsFalse(ParseManifest(R"({"schemaVersion":1,"applications":{"PGSuper":{"configurations":[{"name":"A","packages":[]},{"name":"A","packages":[]}]}}})", manifest, error), L"duplicate configuration");
      }

      TEST_METHOD(RejectsInvalidPackages)
      {
         Manifest manifest;
         std::wstring error;
         Assert::IsFalse(ParseManifest(MakeManifest(R"({"appVersion":"9.1","url":"a.zip"})"), manifest, error), L"two-part version");
         Assert::IsFalse(ParseManifest(MakeManifest(R"({"appVersion":"x.1.0","url":"a.zip"})"), manifest, error), L"bad version");
         Assert::IsFalse(ParseManifest(MakeManifest(R"({"appVersion":"9.1.0"})"), manifest, error), L"no url");
         Assert::IsFalse(ParseManifest(MakeManifest(R"({"appVersion":"9.1.0","url":"http://example.org/a.zip"})"), manifest, error), L"http:// package address");
         Assert::IsTrue(error.find(L"https://") != std::wstring::npos, L"message says https:// is required");
         Assert::IsFalse(ParseManifest(MakeManifest(R"({"appVersion":"9.1.0","url":"a.zip","md5":"1234"})"), manifest, error), L"bad md5");
         Assert::IsTrue(ParseManifest(MakeManifest(R"({"appVersion":"9.1.0","url":"https://example.org/a.zip"})"), manifest, error), L"https:// package address");
         Assert::IsTrue(ParseManifest(MakeManifest(R"({"appVersion":"9.1.0","url":"ftp://ftp.example.org/a.zip"})"), manifest, error), L"ftp:// package address");
      }

      TEST_METHOD(WriteAndReadBack)
      {
         Manifest manifest;
         std::wstring error;
         Assert::IsTrue(ParseManifest(g_Sample, manifest, error));
         manifest.Applications[0].Configurations[0].Name = L"Name with \"quotes\" and \\ backslash";

         std::string text = FormatManifest(manifest);
         Manifest copy;
         Assert::IsTrue(ParseManifest(text, copy, error), error.c_str());
         Assert::AreEqual(text, FormatManifest(copy));
         Assert::AreEqual(manifest.Applications[0].Configurations[0].Name, copy.Applications[0].Configurations[0].Name);
         Assert::IsTrue(text.find("\\/") == std::string::npos, L"forward slashes are not escaped");
      }

      TEST_METHOD(ReadsAndWritesFiles)
      {
         TempFolder folder;
         fs::path file = folder.Write(L"ConfigurationPackages.json", g_Sample);

         Manifest manifest;
         std::wstring error;
         Assert::IsTrue(ReadManifest(file.wstring(), manifest, error), error.c_str());
         manifest.GetOrAddApplication(L"TOGA");
         Assert::IsTrue(WriteManifest(file.wstring(), manifest, error), error.c_str());

         Manifest copy;
         Assert::IsTrue(ReadManifest(file.wstring(), copy, error), L"an application with no configurations is allowed");
         Assert::AreEqual(size_t(3), copy.Applications.size());
         Assert::IsFalse(ReadManifest((folder.Path() / L"missing.json").wstring(), copy, error));
      }
   };

   TEST_CLASS(VersionSelection)
   {
   public:
      TEST_METHOD(ParsesVersions)
      {
         unsigned int parts[3];
         Assert::IsTrue(ParseVersion(L"9.1.0", parts));
         Assert::AreEqual(9u, parts[0]);
         Assert::AreEqual(1u, parts[1]);
         Assert::AreEqual(0u, parts[2]);
         Assert::IsTrue(ParseVersion(L"9.1.0.27", parts), L"build number is accepted and ignored");
         Assert::IsFalse(ParseVersion(L"9.1", parts));
         Assert::IsFalse(ParseVersion(L"9.1.x", parts));
         Assert::IsFalse(ParseVersion(L"", parts));
      }

      TEST_METHOD(ComparesNumerically)
      {
         Assert::IsTrue(CompareVersions(L"4.0.11", L"4.0.9") > 0, L"not compared as text");
         Assert::IsTrue(CompareVersions(L"10.0.0", L"9.1.0") > 0);
         Assert::AreEqual(0, CompareVersions(L"9.1.0", L"9.1.0.5"));
      }

      TEST_METHOD(PicksNewestNotNewerThanProgram)
      {
         Manifest manifest;
         std::wstring error;
         Assert::IsTrue(ParseManifest(g_Sample, manifest, error));
         const Configuration& wsdot = *manifest.FindApplication(L"PGSuper")->FindConfiguration(L"WSDOT");

         auto pick = [&wsdot](const wchar_t* version)
         {
            const Package* p = FindCompatiblePackage(wsdot, version);
            return p ? p->AppVersion : std::wstring(L"none");
         };

         Assert::AreEqual(std::wstring(L"9.0.0"), pick(L"9.0.0"), L"exact match");
         Assert::AreEqual(std::wstring(L"9.0.0"), pick(L"9.0.5"), L"version in between");
         Assert::AreEqual(std::wstring(L"9.1.0"), pick(L"10.2.0"), L"program newer than every package");
         Assert::AreEqual(std::wstring(L"none"), pick(L"7.0.0"), L"every package newer than the program");
         Assert::AreEqual(std::wstring(L"9.1.0"), pick(L"9.1.0.12"), L"four-part program version");
         Assert::AreEqual(std::wstring(L"none"), pick(L"bad"), L"bad program version");
      }
   };

   TEST_CLASS(LegacyIni)
   {
   public:
      TEST_METHOD(ReadsPgzSections)
      {
         TempFolder folder;
         fs::path ini = folder.Write(L"PGSuperPackages.ini",
            "[WSDOT]\r\n"
            "Format=\"pgz\"\r\n"
            "WebLink=\"https://www.wsdot.wa.gov/eesc/bridge/software/PGSuper/LibraryInfo.html\"\r\n"
            "Version_8.0.0_PgzFiles=\"ftp://ftp.wsdot.wa.gov/public/bridge/Software/PGSuper/Version_8.0.0/WSDOT.pgz\"\r\n"
            "Version_9.1.0_PgzFiles=\"ftp://ftp.wsdot.wa.gov/public/bridge/Software/PGSuper/Version_9.1.0/WSDOT.pgz\"\r\n"
            "Version_bad_PgzFiles=\"ftp://x/bad.pgz\"\r\n"
            "\r\n"
            "[Original]\r\n"
            "Version_9.1.0_MasterLibrary=\"ftp://x/a.lbr\"\r\n"
            "\r\n"
            ";[Commented]\r\n"
            ";Format=\"pgz\"\r\n"
            "\r\n"
            "[Utah]\r\n"
            "format=PGZ\r\n"
            "version_7.0.0_pgzfiles=https://www.pgsuper.com/ProConfigurations/PGSuper/7.0.0/Utah.pgz\r\n");

         Application app = ReadLegacyIni(ini.wstring(), L"PGSuper");
         Assert::AreEqual(std::wstring(L"PGSuper"), app.Name);
         Assert::AreEqual(size_t(2), app.Configurations.size(), L"the original-format section is skipped");

         const Configuration* pWSDOT = app.FindConfiguration(L"WSDOT");
         Assert::IsNotNull(pWSDOT);
         Assert::AreEqual(size_t(2), pWSDOT->Packages.size(), L"bad version key is skipped");
         Assert::AreEqual(std::wstring(L"https://www.wsdot.wa.gov/eesc/bridge/software/PGSuper/LibraryInfo.html"), pWSDOT->WebLink, L"quotes removed");
         Assert::AreEqual(std::wstring(L"ftp://ftp.wsdot.wa.gov/public/bridge/Software/PGSuper/Version_9.1.0/WSDOT.pgz"), FindCompatiblePackage(*pWSDOT, L"9.1.0")->Url);

         const Configuration* pUtah = app.FindConfiguration(L"Utah");
         Assert::IsNotNull(pUtah, L"keys are not case sensitive");
         Assert::AreEqual(std::wstring(L"7.0.0"), pUtah->Packages[0].AppVersion);
      }

      TEST_METHOD(ReadsLongFiles)
      {
         // The old reader used 256-character buffers and lost configurations from long files
         std::string text;
         for (int i = 0; i < 60; i++)
         {
            char section[512];
            sprintf_s(section, "[Configuration number %02d with a long name]\r\nFormat=pgz\r\nVersion_9.1.0_PgzFiles=https://www.example.org/a/very/long/path/to/the/configuration/packages/folder/Configuration%02d.zip\r\n\r\n", i, i);
            text += section;
         }

         TempFolder folder;
         fs::path ini = folder.Write(L"PGSplicePackages.ini", text);
         Application app = ReadLegacyIni(ini.wstring(), L"PGSplice");
         Assert::AreEqual(size_t(60), app.Configurations.size());
         Assert::AreEqual(std::wstring(L"https://www.example.org/a/very/long/path/to/the/configuration/packages/folder/Configuration59.zip"), app.Configurations[59].Packages[0].Url);
      }

      TEST_METHOD(LegacyFileName)
      {
         Assert::AreEqual(std::wstring(L"PGSplicePackages.ini"), GetLegacyIniFileName(L"PGSplice"));
         Assert::AreEqual(std::wstring(L"ConfigurationPackages.json"), std::wstring(GetManifestFileName()));
      }
   };

   TEST_CLASS(Md5)
   {
   public:
      TEST_METHOD(KnownValues)
      {
         TempFolder folder;
         Assert::AreEqual(std::wstring(L"900150983cd24fb0d6963f7d28e17f72"), ComputeFileMd5(folder.Write(L"abc.txt", "abc").wstring()));
         Assert::AreEqual(std::wstring(L"d41d8cd98f00b204e9800998ecf8427e"), ComputeFileMd5(folder.Write(L"empty.txt", "").wstring()));
         Assert::IsTrue(ComputeFileMd5((folder.Path() / L"missing.txt").wstring()).empty());
      }

      TEST_METHOD(LargeFile)
      {
         // larger than the 64 KB read buffer
         TempFolder folder;
         std::string text(200000, 'a');
         Assert::AreEqual(std::wstring(L"561b1994f6baacd6e5eaf4baaa12849f"), ComputeFileMd5(folder.Write(L"large.bin", text).wstring()));
      }

      TEST_METHOD(ReadsChecksumFiles)
      {
         TempFolder folder;
         std::vector<std::wstring> hashes;

         // md5deep -q output (hash only), as written for packages
         Assert::IsTrue(ReadMd5List(folder.Write(L"a.zip.md5", "900150983CD24FB0D6963F7D28E17F72\r\n").wstring(), hashes));
         Assert::AreEqual(size_t(1), hashes.size());
         Assert::AreEqual(std::wstring(L"900150983cd24fb0d6963f7d28e17f72"), hashes[0]);

         // md5deep list (hash  path)
         Assert::IsTrue(ReadMd5List(folder.Write(L"list.md5", "900150983cd24fb0d6963f7d28e17f72  C:\\a\\abc.txt\r\nd41d8cd98f00b204e9800998ecf8427e  C:\\a\\empty.txt\r\nnot a hash\r\n").wstring(), hashes));
         Assert::AreEqual(size_t(2), hashes.size());

         Assert::IsFalse(ReadMd5List((folder.Path() / L"missing.md5").wstring(), hashes));
      }

      TEST_METHOD(DetectsChangedTemplate)
      {
         // The "Local or Network File System" server checks the cached templates against WorkgroupTemplates.md5
         TempFolder folder;
         folder.Write(L"Templates\\I-Beams\\W42G.pgt", "template 1");
         folder.Write(L"Templates\\I-Beams\\W42G.ico", "icon");
         folder.Write(L"Templates\\W58G.pgt", "template 2");

         std::wstring list = (folder.Path() / L"WorkgroupTemplates.md5").wstring();
         std::wstring error;
         Assert::IsTrue(WriteMd5List((folder.Path() / L"Templates").wstring(), list, error), error.c_str());

         std::vector<std::wstring> hashes;
         Assert::IsTrue(ReadMd5List(list, hashes));
         Assert::AreEqual(size_t(3), hashes.size());
         Assert::IsTrue(IsInMd5List((folder.Path() / L"Templates").wstring(), hashes), L"unchanged");

         folder.Write(L"Templates\\W58G.pgt", "template 2, revised");
         Assert::IsFalse(IsInMd5List((folder.Path() / L"Templates").wstring(), hashes), L"one template changed");
      }
   };

   TEST_CLASS(Locations)
   {
   public:
      TEST_METHOD(AbsoluteAndRelative)
      {
         Assert::IsTrue(IsAbsoluteLocation(L"https://www.pgsuper.com/ProConfigurations/PGSuper/9.0.0/Oregon.pgz"));
         Assert::IsTrue(IsAbsoluteLocation(L"ftp://ftp.wsdot.wa.gov/public/a.pgz"));
         Assert::IsTrue(IsAbsoluteLocation(L"\\\\server\\share\\a.zip"));
         Assert::IsTrue(IsAbsoluteLocation(L"C:\\Shared\\a.zip"));
         Assert::IsFalse(IsAbsoluteLocation(L"PGSuper/Version_9.1.0/WSDOT.zip"));
         Assert::IsFalse(IsAbsoluteLocation(L"PGSuper/odd://name.zip"));
      }

      TEST_METHOD(CaseInsensitiveHelpers)
      {
         Assert::IsTrue(StartsWithNoCase(L"HTTP://x", L"http://"));
         Assert::IsTrue(EndsWithNoCase(L"ConfigurationPackages.JSON", L".json"));
         Assert::IsFalse(EndsWithNoCase(L"json", L".json"));
         Assert::IsTrue(EqualsNoCase(L"PGSuper", L"pgsuper"));
      }
   };
}
