#include "stdafx.h"
#include "Data/GameConfig/GameConfig.h"

#include <algorithm>

namespace
{
constexpr wchar_t AccountsSection[] = L"SavedAccounts";
constexpr int MaxSavedAccounts = 64;
std::wstring AccountKey(const wchar_t* prefix, int index) { return prefix + std::to_wstring(index); }
}

void GameConfig::LoadSavedAccounts()
{
    m_savedAccounts.clear();
    const int count = std::clamp(ReadInt(AccountsSection, L"Count", 0), 0, MaxSavedAccounts);
    for (int i = 0; i < count; ++i)
    {
        auto user = ReadString(AccountsSection, AccountKey(L"Username", i).c_str(), L"");
        auto password = ReadString(AccountsSection, AccountKey(L"Password", i).c_str(), L"");
        const auto name = DecryptSetting(user);
        if (!name.empty() && name.size() <= MAX_USERNAME_SIZE)
            m_savedAccounts.emplace_back(std::move(user), std::move(password));
    }
    // Keep the previous single saved login available after upgrading.
    const auto previousUser = DecryptSetting(m_encryptedUsername);
    if (m_savedAccounts.empty() && m_rememberMe && !previousUser.empty() && previousUser.size() <= MAX_USERNAME_SIZE)
        m_savedAccounts.emplace_back(m_encryptedUsername, m_savePassword ? m_encryptedPassword : L"");
}

void GameConfig::SaveSavedAccounts()
{
    const int oldCount = std::clamp(ReadInt(AccountsSection, L"Count", 0), 0, MaxSavedAccounts);
    const int count = static_cast<int>(m_savedAccounts.size());
    for (int i = 0; i < std::max(oldCount, count); ++i)
    {
        const auto userKey = AccountKey(L"Username", i), passwordKey = AccountKey(L"Password", i);
        if (i < count)
        {
            WriteString(AccountsSection, userKey.c_str(), m_savedAccounts[i].first);
            WriteString(AccountsSection, passwordKey.c_str(), m_savedAccounts[i].second);
        }
        else
        {
            RemoveObsoleteKey(AccountsSection, userKey.c_str());
            RemoveObsoleteKey(AccountsSection, passwordKey.c_str());
        }
    }
    WriteInt(AccountsSection, L"Count", count);
}

std::vector<std::wstring> GameConfig::GetSavedAccountNames()
{
    std::vector<std::wstring> names;
    for (const auto& account : m_savedAccounts) names.push_back(DecryptSetting(account.first));
    return names;
}

bool GameConfig::SelectSavedAccount(std::size_t index, std::wstring& user, std::wstring& password)
{
    if (index >= m_savedAccounts.size()) return false;
    user = DecryptSetting(m_savedAccounts[index].first);
    password = DecryptSetting(m_savedAccounts[index].second);
    if (user.empty() || user.size() > MAX_USERNAME_SIZE || password.size() > MAX_PASSWORD_SIZE) return false;
    m_encryptedUsername = m_savedAccounts[index].first;
    m_encryptedPassword = m_savedAccounts[index].second;
    m_rememberMe = true;
    m_savePassword = !password.empty();
    return true;
}

void GameConfig::RememberCurrentAccount()
{
    const auto user = DecryptSetting(m_encryptedUsername);
    if (!m_rememberMe || user.empty() || user.size() > MAX_USERNAME_SIZE) return;
    auto found = std::find_if(m_savedAccounts.begin(), m_savedAccounts.end(), [&](const auto& account)
        { return wcsicmp(DecryptSetting(account.first).c_str(), user.c_str()) == 0; });
    const auto encrypted = std::make_pair(m_encryptedUsername, m_savePassword ? m_encryptedPassword : std::wstring{});
    if (found != m_savedAccounts.end()) *found = encrypted;
    else if (m_savedAccounts.size() < MaxSavedAccounts) m_savedAccounts.push_back(encrypted);
}

void GameConfig::ForgetSavedAccount(const std::wstring& user, bool passwordOnly)
{
    auto found = std::find_if(m_savedAccounts.begin(), m_savedAccounts.end(), [&](const auto& account)
        { return wcsicmp(DecryptSetting(account.first).c_str(), user.c_str()) == 0; });
    if (found == m_savedAccounts.end()) return;
    if (passwordOnly) found->second.clear();
    else m_savedAccounts.erase(found);
    Save();
}
