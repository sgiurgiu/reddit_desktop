#include "networkproxysettingswindow.h"
#include "database.h"
#include "imgui.h"

#include "connections/proxy/proxy.h"

namespace
{
    constexpr auto NETWORK_PROXY_SETTINGS_TITLE = "Proxy settings";
    constexpr auto LABEL_TEMPLATE = "XXXXXXXXXXX";
    constexpr auto FIELD_TEMPLATE = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
}
NetworkProxySettingsWindow::NetworkProxySettingsWindow()
{
}

bool NetworkProxySettingsWindow::showProxySettingsWindow()
{
    bool okPressed = false;
    if(openWindow)
    {
        ImGui::OpenPopup(NETWORK_PROXY_SETTINGS_TITLE);
        openWindow = false;
    }
    if (ImGui::BeginPopupModal(NETWORK_PROXY_SETTINGS_TITLE,nullptr,ImGuiWindowFlags_AlwaysAutoResize))
    {
//        auto fieldsPosition = ImGui::CalcTextSize(LABEL_TEMPLATE).x;
        auto fieldSize = ImGui::CalcTextSize(FIELD_TEMPLATE).x;

        ImGui::PushItemWidth(fieldSize);
        ImGui::InputText("Host##proxy_host",host,IM_ARRAYSIZE(host),ImGuiInputTextFlags_None);
        ImGui::PopItemWidth();

        ImGui::PushItemWidth(fieldSize);
        ImGui::InputText("Port##proxy_port",port,IM_ARRAYSIZE(port),ImGuiInputTextFlags_None);
        ImGui::PopItemWidth();

        if(proxyType==proxy::PROXY_TYPE_SOCKS5)
        {
            ImGui::PushItemWidth(fieldSize);
            ImGui::InputText("Username##proxy_username",username,IM_ARRAYSIZE(username),ImGuiInputTextFlags_None);
            ImGui::PopItemWidth();

            ImGui::PushItemWidth(fieldSize);
            ImGui::InputText("Password##proxy_password",password,IM_ARRAYSIZE(password),ImGuiInputTextFlags_Password);
            ImGui::PopItemWidth();
        }

        if (ImGui::BeginCombo("Proxy Type", proxyType))
        {
            if(ImGui::Selectable(proxy::PROXY_TYPE_SOCKS5, proxyType==proxy::PROXY_TYPE_SOCKS5)) proxyType=proxy::PROXY_TYPE_SOCKS5;
            if(proxyType==proxy::PROXY_TYPE_SOCKS5) ImGui::SetItemDefaultFocus();
            if(ImGui::Selectable(proxy::PROXY_TYPE_SOCKS4, proxyType==proxy::PROXY_TYPE_SOCKS4)) proxyType=proxy::PROXY_TYPE_SOCKS4;
            if(proxyType==proxy::PROXY_TYPE_SOCKS4) ImGui::SetItemDefaultFocus();
            ImGui::EndCombo();
        }

        ImGui::Checkbox("Use proxy",&useProxy);

        ImGui::Separator();
        if (ImGui::Button("OK", ImVec2(120, 0)) ||
                ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)))
        {
            ImGui::CloseCurrentPopup();
            auto existingProxy = Database::getInstance()->getProxy();
            proxy::proxy_t proxy {host,port,username,password,proxyType,useProxy};
            if(!existingProxy ||  (existingProxy && existingProxy.value() != proxy))
            {
                okPressed = true;
                Database::getInstance()->setProxy(proxy);
            }
            else
            {
                okPressed = false;
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)) ||
                ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
        {
            okPressed = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    return okPressed;
}
void NetworkProxySettingsWindow::setShowProxySettingsWindow(bool flag)
{
    openWindow = flag;
    if(openWindow)
    {
        auto proxy = Database::getInstance()->getProxy();
        if(proxy)
        {
            strncpy(host,proxy->host.c_str(),sizeof(host)-1);
            strncpy(port,proxy->port.c_str(),sizeof(port)-1);
            strncpy(username,proxy->username.c_str(),sizeof(username)-1);
            strncpy(password,proxy->password.c_str(),sizeof(password)-1);
            useProxy = proxy->useProxy;
            if(proxy->proxyType == proxy::PROXY_TYPE_SOCKS5) proxyType=proxy::PROXY_TYPE_SOCKS5;
            else if(proxy->proxyType == proxy::PROXY_TYPE_SOCKS4) proxyType=proxy::PROXY_TYPE_SOCKS4;
            else proxyType=proxy::PROXY_TYPE_SOCKS5;
        }
        else
        {
            proxyType=proxy::PROXY_TYPE_SOCKS5;
        }
    }
}
