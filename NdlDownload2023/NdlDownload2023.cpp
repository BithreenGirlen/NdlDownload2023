// NdlDownload2023.cpp : このファイルには 'main' 関数が含まれています。プログラム実行の開始と終了がそこで行われます。
//

#include <Windows.h>
#include <urlmon.h>
#include <atlbase.h>

#include <iostream>
#include <string>
#include <vector>

#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "shlwapi.lib")


class DownloadProgress : public IBindStatusCallback {
private:
    unsigned long m_progress = 0;
    unsigned long m_progress_max = 0;
public:
    HRESULT __stdcall QueryInterface(const IID&, void**) {
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef(void) {
        return 1;
    }
    ULONG STDMETHODCALLTYPE Release(void) {
        return 1;
    }
    HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, IBinding* pib) {
        m_progress = 0;
        m_progress_max = 0;
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetPriority(LONG* pnPriority) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved) {
        return S_OK;
    }
    virtual HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, LPCWSTR szError) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed) {
        return E_NOTIMPL;
    }
    virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID riid, IUnknown* punk) {
        return E_NOTIMPL;
    }

    virtual HRESULT __stdcall OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
    {
        std::wcout << ulProgress << L" of " << ulProgressMax << L" " << ulStatusCode;
        if (szStatusText) std::wcout << L" " << szStatusText;
        std::wcout << std::endl;
        m_progress = ulProgress;
        if (ulStatusCode == BINDSTATUS_BEGINDOWNLOADDATA)m_progress_max = ulProgressMax;
        return S_OK;
    }
    bool IsTrulyCompleted()
    {
        return m_progress == m_progress_max;
    }
};

struct ComInit
{
    HRESULT hr;
    ComInit() : hr(::CoInitialize(nullptr)) {}
    ~ComInit() { if (SUCCEEDED(hr)) ::CoUninitialize(); }
};


std::string GetFolderBasePath();

void GetResourcesFromPid(std::string pid, std::vector<std::string>& resources);
bool ReadManifestJson(char* src, std::vector<std::string>& resources);

bool ExtractJsonObject(char* src, const char* name, char** dst);
bool GetJsonElementValue(char* src, const char* name, char* dst, size_t dst_size);

int main()
{
    DownloadProgress progress;

    std::string pid;
    std::vector<std::string> resources;

    std::string strFile;
    std::string strUrl;
    std::string strPage;

    for (;;)
    {
        std::cout << "Enter pid...\r\n";
        std::cin >> pid;
        GetResourcesFromPid(pid, resources);
        if (!resources.empty())break;
    }

    std::string strFolder = GetFolderBasePath() + pid + "\\/";
    ::CreateDirectoryA(strFolder.c_str(), nullptr);

    size_t ullPageLength = std::to_string(resources.size()).length() + 1;

    for (size_t i = 0; i < resources.size(); ++i)
    {
        strPage = std::to_string(i + 1);
        strUrl = resources.at(i);
        strFile = strFolder + strPage.insert(0, ullPageLength - strPage.length(), '0') + ".jpg";;
        if (!::PathFileExistsA(strFile.c_str()))
        {
            for (;;) {
                HRESULT hr = ::URLDownloadToFileA(nullptr, strUrl.c_str(), strFile.c_str(), 0, static_cast<IBindStatusCallback*>(&progress));
                if (hr == S_OK && progress.IsTrulyCompleted())break;
                std::cout << "Sleeps for ten seconds...\r\n";
                ::Sleep(1000);
            }
        }
    }

    return 0;
}

/*実行プロセスの階層取得*/
std::string GetFolderBasePath()
{
    char application_path[MAX_PATH]{};
    ::GetModuleFileNameA(nullptr, application_path, MAX_PATH);
    std::string::size_type pos = std::string(application_path).find_last_of("\\/");
    return std::string(application_path).substr(0, pos) + "\\/";
}

/*永続識別子から資源一覧取得*/
void GetResourcesFromPid(std::string pid, std::vector<std::string> &resources)
{
    std::string strUrl = "https://dl.ndl.go.jp/api/iiif/" + pid + "/manifest.json";
    ComInit init;
    CComPtr<IStream> pStream;

    HRESULT hr = ::URLOpenBlockingStreamA(nullptr, strUrl.c_str(), &pStream, 0, nullptr);
    if (hr == S_OK)
    {
        STATSTG stat;
        hr = pStream->Stat(&stat, STATFLAG_DEFAULT);
        if (hr == S_OK)
        {
            char* buffer = static_cast<char*>(malloc(stat.cbSize.LowPart + 1LL));
            if (buffer != nullptr)
            {
                DWORD dwReadBytes = 0;
                DWORD dwSize = 0;
                for (;;)
                {
                    hr = pStream->Read(buffer + dwSize, stat.cbSize.LowPart - dwSize, &dwReadBytes);
                    if (FAILED(hr))break;
                    dwSize += dwReadBytes;
                    if (dwSize >= stat.cbSize.LowPart)break;
                }
                *(buffer + dwSize) = '\0';

                ReadManifestJson(buffer, resources);

                free(buffer);
            }
        }
    }

}
/*対象資料のJSON読み取り*/
bool ReadManifestJson(char* src, std::vector<std::string>& resources)
{
    char* p = nullptr;
    char* pp = nullptr;
    char* buffer = nullptr;
    char* list = nullptr;
    char element[256]{};
    int iCount = 0;

    bool bRet = ExtractJsonObject(src, "sequences", &buffer);
    if (!bRet)return false;
    pp = buffer;

    for (;; ++iCount)
    {
        p = strstr(pp, "resource");
        if (p == nullptr)break;

        bRet = ExtractJsonObject(p, "resource", &list);
        if (!bRet)break;
        pp = p + strlen(list);

        bRet = GetJsonElementValue(list, "@id", element, sizeof(element));
        free(list);
        if (!bRet)break;
        resources.push_back(element);
    }

    free(buffer);

    return iCount > 0;
}

/*JSON特性値の抽出*/
bool ExtractJsonObject(char* src, const char* name, char** dst)
{
    char* p = nullptr;
    char* pp = src;
    char* q = nullptr;
    char* qq = nullptr;
    size_t len = 0;
    int iCount = 0;

    p = strstr(pp, name);
    if (p == nullptr)return false;

    pp = strstr(p, ":");
    if (pp == nullptr)false;
    pp += strlen(":");

    for (;;)
    {
        q = strstr(pp, "}");
        if (q == nullptr)return false;

        qq = strstr(pp, "{");
        if (qq == nullptr)break;

        if (q < qq)
        {
            --iCount;
            pp = q + strlen("}");
        }
        else
        {
            ++iCount;
            pp = qq + strlen("{");
        }

        if (iCount == 0)break;
    }

    len = q - p + strlen("}");
    char* buffer = static_cast<char*>(malloc(len + 1));
    if (buffer == nullptr)return false;
    memcpy(buffer, p, len);
    *(buffer + len) = '\0';
    *dst = buffer;

    return true;

}
/*JSON要素の値を取得*/
bool GetJsonElementValue(char* src, const char* name, char* dst, size_t dst_size)
{
    char* p = nullptr;
    char* pp = src;
    size_t len = 0;

    p = strstr(pp, name);
    if (p == nullptr)return false;

    pp = strstr(p, "\":");
    if (pp == nullptr)return false;
    pp += strlen("\":");

    p = strstr(pp, "\"");
    if (p == nullptr)return false;
    p += strlen("\"");

    pp = strstr(p, "\"");
    if (pp == nullptr)return false;

    len = pp - p;
    if (len > dst_size)return false;
    memcpy(dst, p, len);
    *(dst + len) = '\0';

    return true;
}