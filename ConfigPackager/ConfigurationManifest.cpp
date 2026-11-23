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

#include <windows.h>
#include <bcrypt.h>

#include "ConfigurationManifest.h"

#include <algorithm>
#include <cwctype>
#include <fstream>
#include <sstream>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#pragma comment(lib, "bcrypt.lib")

namespace pt = boost::property_tree;

namespace ConfigManifest
{

std::wstring GetLegacyIniFileName(const std::wstring& appName)
{
   return appName + L"Packages.ini";
}

///////////////////////////////////////////////////////////////////////
// String helpers

std::wstring FromUtf8(const std::string& str)
{
   if (str.empty())
   {
      return std::wstring();
   }

   int len = ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
   std::wstring result(len, L'\0');
   ::MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), len);
   return result;
}

std::string ToUtf8(const std::wstring& str)
{
   if (str.empty())
   {
      return std::string();
   }

   int len = ::WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0, nullptr, nullptr);
   std::string result(len, '\0');
   ::WideCharToMultiByte(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), len, nullptr, nullptr);
   return result;
}

static std::wstring ToLower(const std::wstring& str)
{
   std::wstring result(str);
   std::transform(result.begin(), result.end(), result.begin(), [](wchar_t c) {return (wchar_t)std::towlower(c);});
   return result;
}

static std::wstring Trim(const std::wstring& str)
{
   auto first = str.find_first_not_of(L" \t\r\n");
   if (first == std::wstring::npos)
   {
      return std::wstring();
   }
   auto last = str.find_last_not_of(L" \t\r\n");
   return str.substr(first, last - first + 1);
}

bool EqualsNoCase(const std::wstring& a, const std::wstring& b)
{
   return _wcsicmp(a.c_str(), b.c_str()) == 0;
}

bool StartsWithNoCase(const std::wstring& str, const std::wstring& prefix)
{
   return prefix.size() <= str.size() && _wcsnicmp(str.c_str(), prefix.c_str(), prefix.size()) == 0;
}

bool EndsWithNoCase(const std::wstring& str, const std::wstring& suffix)
{
   return suffix.size() <= str.size() && _wcsicmp(str.c_str() + (str.size() - suffix.size()), suffix.c_str()) == 0;
}

bool IsAbsoluteLocation(const std::wstring& url)
{
   // scheme, e.g. https://
   auto scheme = url.find(L"://");
   if (scheme != std::wstring::npos && url.find_first_of(L"/\\") > scheme)
   {
      return true;
   }

   // UNC path
   if (StartsWithNoCase(url, L"\\\\"))
   {
      return true;
   }

   // drive letter
   if (2 < url.size() && std::iswalpha(url[0]) && url[1] == L':' && (url[2] == L'\\' || url[2] == L'/'))
   {
      return true;
   }

   return false;
}

bool IsMd5String(const std::wstring& str)
{
   return str.size() == 32 && std::all_of(str.begin(), str.end(), [](wchar_t c) {return std::iswxdigit(c) != 0;});
}

///////////////////////////////////////////////////////////////////////
// Versions

bool ParseVersion(const std::wstring& version, unsigned int parts[3])
{
   parts[0] = parts[1] = parts[2] = 0;

   std::vector<std::wstring> tokens;
   std::wstring token;
   std::wistringstream is(Trim(version));
   while (std::getline(is, token, L'.'))
   {
      tokens.push_back(token);
   }

   if (tokens.size() != 3 && tokens.size() != 4)
   {
      return false;
   }

   for (size_t i = 0; i < tokens.size(); i++)
   {
      const auto& t = tokens[i];
      if (t.empty() || 9 < t.size() || !std::all_of(t.begin(), t.end(), [](wchar_t c) {return std::iswdigit(c) != 0;}))
      {
         return false;
      }

      if (i < 3)
      {
         parts[i] = (unsigned int)std::stoul(t);
      }
   }

   return true;
}

int CompareVersions(const std::wstring& v1, const std::wstring& v2)
{
   unsigned int p1[3], p2[3];
   ParseVersion(v1, p1);
   ParseVersion(v2, p2);
   for (int i = 0; i < 3; i++)
   {
      if (p1[i] != p2[i])
      {
         return p1[i] < p2[i] ? -1 : 1;
      }
   }
   return 0;
}

const Package* FindCompatiblePackage(const Configuration& configuration, const std::wstring& programVersion)
{
   unsigned int parts[3];
   if (!ParseVersion(programVersion, parts))
   {
      return nullptr;
   }

   const Package* pBest = nullptr;
   for (const auto& package : configuration.Packages)
   {
      if (!ParseVersion(package.AppVersion, parts))
      {
         continue;
      }

      if (CompareVersions(package.AppVersion, programVersion) <= 0 &&
          (pBest == nullptr || 0 < CompareVersions(package.AppVersion, pBest->AppVersion)))
      {
         pBest = &package;
      }
   }

   return pBest;
}

///////////////////////////////////////////////////////////////////////
// Manifest data model

Configuration* Application::FindConfiguration(const std::wstring& name)
{
   auto found = std::find_if(Configurations.begin(), Configurations.end(), [&name](const auto& c) {return c.Name == name;});
   return found == Configurations.end() ? nullptr : &(*found);
}

const Configuration* Application::FindConfiguration(const std::wstring& name) const
{
   auto found = std::find_if(Configurations.begin(), Configurations.end(), [&name](const auto& c) {return c.Name == name;});
   return found == Configurations.end() ? nullptr : &(*found);
}

Application* Manifest::FindApplication(const std::wstring& name)
{
   auto found = std::find_if(Applications.begin(), Applications.end(), [&name](const auto& a) {return EqualsNoCase(a.Name, name);});
   return found == Applications.end() ? nullptr : &(*found);
}

const Application* Manifest::FindApplication(const std::wstring& name) const
{
   auto found = std::find_if(Applications.begin(), Applications.end(), [&name](const auto& a) {return EqualsNoCase(a.Name, name);});
   return found == Applications.end() ? nullptr : &(*found);
}

Application& Manifest::GetOrAddApplication(const std::wstring& name)
{
   Application* pApp = FindApplication(name);
   if (pApp == nullptr)
   {
      Application app;
      app.Name = name;
      Applications.push_back(app);
      pApp = &Applications.back();
   }
   return *pApp;
}

///////////////////////////////////////////////////////////////////////
// Legacy INI file
//
//    [WSDOT]
//    Format=Pgz
//    WebLink=https://...
//    Version_9.0.0_PgzFiles=https://.../WSDOT.pgz

Application ReadLegacyIni(const std::wstring& fullPath, const std::wstring& appName)
{
   Application application;
   application.Name = appName;

   // section names (the buffer grows until the list fits)
   std::vector<wchar_t> names(4096);
   while (true)
   {
      DWORD n = ::GetPrivateProfileSectionNamesW(names.data(), (DWORD)names.size(), fullPath.c_str());
      if (n < names.size() - 2)
      {
         break;
      }
      names.resize(names.size() * 2);
   }

   for (const wchar_t* name = names.data(); *name != L'\0'; name += wcslen(name) + 1)
   {
      // key=value pairs of the section
      std::vector<wchar_t> keys(8192);
      while (true)
      {
         DWORD n = ::GetPrivateProfileSectionW(name, keys.data(), (DWORD)keys.size(), fullPath.c_str());
         if (n < keys.size() - 2)
         {
            break;
         }
         keys.resize(keys.size() * 2);
      }

      Configuration configuration;
      configuration.Name = Trim(name);
      bool bIsPgzFormat = false;

      for (const wchar_t* entry = keys.data(); *entry != L'\0'; entry += wcslen(entry) + 1)
      {
         std::wstring strEntry(entry);
         auto eq = strEntry.find(L'=');
         if (eq == std::wstring::npos)
         {
            continue;
         }

         std::wstring key = Trim(strEntry.substr(0, eq));
         std::wstring value = Trim(strEntry.substr(eq + 1));
         if (2 <= value.size() && value.front() == L'"' && value.back() == L'"')
         {
            value = value.substr(1, value.size() - 2);
         }

         if (EqualsNoCase(key, L"Format"))
         {
            bIsPgzFormat = EqualsNoCase(value, L"pgz");
         }
         else if (EqualsNoCase(key, L"WebLink"))
         {
            configuration.WebLink = value;
         }
         else if (StartsWithNoCase(key, L"Version_") && EndsWithNoCase(key, L"_PgzFiles") && !value.empty() && 17 < key.size())
         {
            std::wstring version = key.substr(8, key.size() - 8 - 9);
            unsigned int parts[3];
            if (ParseVersion(version, parts))
            {
               Package package;
               package.AppVersion = version;
               package.Url = value;
               configuration.Packages.push_back(package);
            }
         }
      }

      if (bIsPgzFormat)
      {
         application.Configurations.push_back(configuration);
      }
   }

   return application;
}

///////////////////////////////////////////////////////////////////////
// Reading

static std::wstring GetString(const pt::ptree& node, const char* key)
{
   return Trim(FromUtf8(node.get<std::string>(key, std::string())));
}

bool ParseManifest(const std::string& utf8Text, Manifest& manifest, std::wstring& error)
{
   manifest = Manifest();
   error.clear();

   std::string text(utf8Text);
   if (3 <= text.size() && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF)
   {
      text.erase(0, 3);
   }

   pt::ptree root;
   try
   {
      std::istringstream is(text);
      pt::read_json(is, root);
   }
   catch (const pt::json_parser_error& e)
   {
      error = L"The file is not valid JSON: " + FromUtf8(e.message()) + L" (line " + std::to_wstring(e.line()) + L").";
      return false;
   }
   catch (...)
   {
      error = L"The file is not valid JSON.";
      return false;
   }

   std::wstring schemaVersion = GetString(root, "schemaVersion");
   if (schemaVersion.empty() || !std::all_of(schemaVersion.begin(), schemaVersion.end(), [](wchar_t c) {return std::iswdigit(c) != 0;}) || 9 < schemaVersion.size())
   {
      error = L"\"schemaVersion\" is missing or is not a whole number.";
      return false;
   }
   manifest.SchemaVersion = std::stoi(schemaVersion);

   auto applications = root.get_child_optional("applications");
   if (!applications || applications->empty())
   {
      error = L"\"applications\" is missing or empty.";
      return false;
   }

   for (const auto& [appKey, appNode] : *applications)
   {
      Application application;
      application.Name = Trim(FromUtf8(appKey));
      if (application.Name.empty())
      {
         error = L"\"applications\" must contain one section per program, such as \"PGSuper\": { ... }.";
         return false;
      }

      if (manifest.FindApplication(application.Name))
      {
         error = L"The program \"" + application.Name + L"\" is listed more than once.";
         return false;
      }

      auto configurations = appNode.get_child_optional("configurations");
      if (!configurations)
      {
         error = L"\"" + application.Name + L"\" has no \"configurations\" list.";
         return false;
      }

      for (const auto& [configKey, configNode] : *configurations)
      {
         if (!configKey.empty())
         {
            error = L"\"configurations\" for \"" + application.Name + L"\" must be a list: [ { ... }, { ... } ].";
            return false;
         }

         Configuration configuration;
         configuration.Name = GetString(configNode, "name");
         configuration.WebLink = GetString(configNode, "webLink");

         if (configuration.Name.empty())
         {
            error = L"A configuration for \"" + application.Name + L"\" has no \"name\".";
            return false;
         }

         if (application.FindConfiguration(configuration.Name))
         {
            error = L"The configuration \"" + configuration.Name + L"\" is listed more than once for \"" + application.Name + L"\".";
            return false;
         }

         auto packages = configNode.get_child_optional("packages");
         if (!packages)
         {
            error = L"The configuration \"" + configuration.Name + L"\" has no \"packages\" list.";
            return false;
         }

         for (const auto& [packageKey, packageNode] : *packages)
         {
            std::wstring where = L"A package for \"" + configuration.Name + L"\" (" + application.Name + L")";
            if (!packageKey.empty())
            {
               error = L"\"packages\" for \"" + configuration.Name + L"\" must be a list: [ { ... }, { ... } ].";
               return false;
            }

            Package package;
            package.AppVersion = GetString(packageNode, "appVersion");
            package.Url = GetString(packageNode, "url");
            package.Md5 = ToLower(GetString(packageNode, "md5"));

            unsigned int parts[3];
            if (!ParseVersion(package.AppVersion, parts))
            {
               error = where + L" has an \"appVersion\" that is missing or not in the form 1.2.3: \"" + package.AppVersion + L"\".";
               return false;
            }

            if (package.Url.empty())
            {
               error = where + L" has no \"url\".";
               return false;
            }

            if (StartsWithNoCase(package.Url, L"http://"))
            {
               error = where + L" has a \"url\" that starts with http://. Package addresses must start with https:// or be relative to this file: " + package.Url;
               return false;
            }

            if (!package.Md5.empty() && !IsMd5String(package.Md5))
            {
               error = where + L" has an \"md5\" that is not 32 hexadecimal digits: \"" + package.Md5 + L"\".";
               return false;
            }

            configuration.Packages.push_back(package);
         }

         application.Configurations.push_back(configuration);
      }

      manifest.Applications.push_back(application);
   }

   return true;
}

static bool ReadFileBytes(const std::wstring& fileName, std::string& bytes)
{
   std::ifstream file(fileName, std::ios::binary);
   if (!file)
   {
      return false;
   }
   std::ostringstream os;
   os << file.rdbuf();
   bytes = os.str();
   return true;
}

bool ReadManifest(const std::wstring& fileName, Manifest& manifest, std::wstring& error)
{
   std::string bytes;
   if (!ReadFileBytes(fileName, bytes))
   {
      error = L"Could not open " + fileName + L".";
      return false;
   }
   return ParseManifest(bytes, manifest, error);
}

///////////////////////////////////////////////////////////////////////
// Writing

static std::string Quote(const std::wstring& value)
{
   std::string utf8 = ToUtf8(value);
   std::string result("\"");
   for (char c : utf8)
   {
      switch (c)
      {
      case '"':  result += "\\\""; break;
      case '\\': result += "\\\\"; break;
      case '\n': result += "\\n";  break;
      case '\r': result += "\\r";  break;
      case '\t': result += "\\t";  break;
      default:
         if ((unsigned char)c < 0x20)
         {
            char buffer[8];
            sprintf_s(buffer, "\\u%04x", (unsigned int)(unsigned char)c);
            result += buffer;
         }
         else
         {
            result += c;
         }
      }
   }
   result += "\"";
   return result;
}

std::string FormatManifest(const Manifest& manifest)
{
   std::ostringstream os;
   os << "{\n";
   os << "  \"schemaVersion\": " << manifest.SchemaVersion << ",\n";
   os << "  \"applications\": {\n";
   for (size_t a = 0; a < manifest.Applications.size(); a++)
   {
      const auto& application = manifest.Applications[a];
      os << "    " << Quote(application.Name) << ": {\n";
      os << "      \"configurations\": [\n";
      for (size_t c = 0; c < application.Configurations.size(); c++)
      {
         const auto& configuration = application.Configurations[c];
         os << "        {\n";
         os << "          \"name\": " << Quote(configuration.Name) << ",\n";
         if (!configuration.WebLink.empty())
         {
            os << "          \"webLink\": " << Quote(configuration.WebLink) << ",\n";
         }
         os << "          \"packages\": [\n";
         for (size_t p = 0; p < configuration.Packages.size(); p++)
         {
            const auto& package = configuration.Packages[p];
            os << "            { \"appVersion\": " << Quote(package.AppVersion) << ", \"url\": " << Quote(package.Url);
            if (!package.Md5.empty())
            {
               os << ", \"md5\": " << Quote(package.Md5);
            }
            os << " }" << (p + 1 < configuration.Packages.size() ? "," : "") << "\n";
         }
         os << "          ]\n";
         os << "        }" << (c + 1 < application.Configurations.size() ? "," : "") << "\n";
      }
      os << "      ]\n";
      os << "    }" << (a + 1 < manifest.Applications.size() ? "," : "") << "\n";
   }
   os << "  }\n";
   os << "}\n";
   return os.str();
}

bool WriteManifest(const std::wstring& fileName, const Manifest& manifest, std::wstring& error)
{
   std::ofstream file(fileName, std::ios::binary | std::ios::trunc);
   if (!file)
   {
      error = L"Could not write " + fileName + L".";
      return false;
   }
   file << FormatManifest(manifest);
   if (!file)
   {
      error = L"Could not write " + fileName + L".";
      return false;
   }
   return true;
}

///////////////////////////////////////////////////////////////////////
// MD5

std::wstring ComputeFileMd5(const std::wstring& fileName)
{
   HANDLE hFile = ::CreateFileW(fileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);
   if (hFile == INVALID_HANDLE_VALUE)
   {
      return std::wstring();
   }

   std::wstring result;
   BCRYPT_ALG_HANDLE hAlg = nullptr;
   BCRYPT_HASH_HANDLE hHash = nullptr;
   if (BCRYPT_SUCCESS(::BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_MD5_ALGORITHM, nullptr, 0)) &&
       BCRYPT_SUCCESS(::BCryptCreateHash(hAlg, &hHash, nullptr, 0, nullptr, 0, 0)))
   {
      std::vector<BYTE> buffer(64 * 1024);
      DWORD nRead = 0;
      bool bOK = true;
      while (::ReadFile(hFile, buffer.data(), (DWORD)buffer.size(), &nRead, nullptr) && 0 < nRead)
      {
         if (!BCRYPT_SUCCESS(::BCryptHashData(hHash, buffer.data(), nRead, 0)))
         {
            bOK = false;
            break;
         }
      }

      BYTE digest[16];
      if (bOK && BCRYPT_SUCCESS(::BCryptFinishHash(hHash, digest, sizeof(digest), 0)))
      {
         wchar_t hex[33];
         for (int i = 0; i < 16; i++)
         {
            swprintf_s(hex + 2 * i, 3, L"%02x", digest[i]);
         }
         result = hex;
      }
   }

   if (hHash)
   {
      ::BCryptDestroyHash(hHash);
   }

   if (hAlg)
   {
      ::BCryptCloseAlgorithmProvider(hAlg, 0);
   }

   ::CloseHandle(hFile);
   return result;
}

bool ReadMd5List(const std::wstring& fileName, std::vector<std::wstring>& hashes)
{
   hashes.clear();

   std::string bytes;
   if (!ReadFileBytes(fileName, bytes))
   {
      return false;
   }

   std::wstring text;
   if (2 <= bytes.size() && (unsigned char)bytes[0] == 0xFF && (unsigned char)bytes[1] == 0xFE)
   {
      // UTF-16 (written by some command shells)
      text.assign((const wchar_t*)(bytes.data() + 2), (bytes.size() - 2) / sizeof(wchar_t));
   }
   else
   {
      text = FromUtf8(bytes);
   }

   std::wistringstream is(text);
   std::wstring line;
   while (std::getline(is, line))
   {
      line = Trim(line);
      if (line.size() < 32)
      {
         continue;
      }

      std::wstring hash = ToLower(line.substr(0, 32));
      if (IsMd5String(hash) && (line.size() == 32 || std::iswspace(line[32])))
      {
         hashes.push_back(hash);
      }
   }

   return true;
}

static bool IsFolder(const std::wstring& path)
{
   DWORD attributes = ::GetFileAttributesW(path.c_str());
   return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

static void ListFiles(const std::wstring& folder, std::vector<std::wstring>& files)
{
   std::wstring root(folder);
   if (!root.empty() && root.back() != L'\\' && root.back() != L'/')
   {
      root += L'\\';
   }

   WIN32_FIND_DATAW data;
   HANDLE hFind = ::FindFirstFileW((root + L"*").c_str(), &data);
   if (hFind == INVALID_HANDLE_VALUE)
   {
      return;
   }

   do
   {
      std::wstring name(data.cFileName);
      if (name == L"." || name == L"..")
      {
         continue;
      }

      if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
      {
         ListFiles(root + name, files);
      }
      else
      {
         files.push_back(root + name);
      }
   } while (::FindNextFileW(hFind, &data));

   ::FindClose(hFind);
}

bool WriteMd5List(const std::wstring& fileOrFolder, const std::wstring& listFileName, std::wstring& error)
{
   std::vector<std::wstring> files;
   if (IsFolder(fileOrFolder))
   {
      ListFiles(fileOrFolder, files);
   }
   else
   {
      files.push_back(fileOrFolder);
   }

   std::ostringstream os;
   for (const auto& file : files)
   {
      if (EqualsNoCase(file, listFileName))
      {
         continue; // don't hash the list itself
      }

      std::wstring hash = ComputeFileMd5(file);
      if (hash.empty())
      {
         error = L"Could not read " + file + L".";
         return false;
      }
      os << ToUtf8(hash) << "  " << ToUtf8(file) << "\r\n";
   }

   std::ofstream out(listFileName, std::ios::binary | std::ios::trunc);
   if (!out)
   {
      error = L"Could not write " + listFileName + L".";
      return false;
   }
   out << os.str();
   return true;
}

bool IsInMd5List(const std::wstring& fileOrFolder, const std::vector<std::wstring>& hashes)
{
   std::vector<std::wstring> files;
   if (IsFolder(fileOrFolder))
   {
      ListFiles(fileOrFolder, files);
   }
   else
   {
      files.push_back(fileOrFolder);
   }

   for (const auto& file : files)
   {
      std::wstring hash = ComputeFileMd5(file);
      if (hash.empty() || std::find(hashes.begin(), hashes.end(), hash) == hashes.end())
      {
         return false;
      }
   }

   return true;
}

}; // namespace ConfigManifest
