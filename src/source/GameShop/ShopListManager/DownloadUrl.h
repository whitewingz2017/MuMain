#pragma once
#include <string>

namespace GameShop
{
inline std::wstring BuildDownloadUrl(bool useFtp, const std::wstring& host, unsigned short port, const std::wstring& remotePath)
{
    std::wstring path = remotePath;
    for (wchar_t& ch : path)
    {
        if (ch == L'\\')
        {
            ch = L'/';
        }
    }
    while (!path.empty() && path.front() == L'/')
    {
        path.erase(path.begin());
    }

    const std::wstring scheme = useFtp ? L"ftp://" : L"http://";
    return scheme + host + L":" + std::to_wstring(port) + L"/" + path;
}
} // namespace
