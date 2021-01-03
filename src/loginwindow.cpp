#include "loginwindow.h"
#include "imgui.h"
#include "fonts/IconsFontAwesome4.h"
#include "imgui_internal.h"
#include "spinner/spinner.h"

#include <string>
#include <string_view>

namespace
{
constexpr std::string_view LOGIN_WINDOW_POPUP_TITLE = "Reddit Login";
constexpr std::string_view LABEL_TEMPLATE = "XXXXXXXXXXX";
constexpr std::string_view FIELD_TEMPLATE = "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
}

LoginWindow::LoginWindow(RedditClientProducer* client,
                         const boost::asio::any_io_executor& executor):
    client(client),loginConnection(client->makeLoginClientConnection()),uiExecutor(executor)
{
    loginConnection->connectionCompleteHandler([this](const boost::system::error_code& ec,const client_response<access_token>& token){
        boost::asio::post(this->uiExecutor,std::bind(&LoginWindow::testingCompleted,this,ec,token));
    });
}
void LoginWindow::testingCompleted(const boost::system::error_code ec,
                                   const client_response<access_token> token)
{
    tested = !ec;
    testingInProgress = false;
    if(ec)
    {
        testingErrorMessage = ec.message();
    }
    else
    {
        configuredUser = user(username,password,clientId,secret,website,appName);
        this->token = token;
    }
}
void LoginWindow::setShowLoginWindow(bool flag)
{
    showLoginInfoWindow = flag;
}

bool LoginWindow::showLoginWindow()
{
    bool okPressed = false;
    if(showLoginInfoWindow)
    {
        ImGui::OpenPopup(LOGIN_WINDOW_POPUP_TITLE.data());
        showLoginInfoWindow = false;
    }
    if (ImGui::BeginPopupModal(LOGIN_WINDOW_POPUP_TITLE.data(),nullptr,ImGuiWindowFlags_AlwaysAutoResize))
    {
        auto fieldsPosition = ImGui::CalcTextSize(LABEL_TEMPLATE.data()).x;

        ImGui::Text("Username:");
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !ImGui::IsAnyItemActive()
                && !ImGui::IsMouseClicked(0))
        {
           ImGui::SetKeyboardFocusHere(0);
        }
        ImGui::SameLine(fieldsPosition);
        ImGui::PushItemWidth(ImGui::CalcTextSize(FIELD_TEMPLATE.data()).x);
        ImGui::InputText("##login_username",username,IM_ARRAYSIZE(username),ImGuiInputTextFlags_None);
        ImGui::PopItemWidth();

        ImGui::Text("Password:");
        ImGui::SameLine(fieldsPosition);
        ImGui::PushItemWidth(ImGui::CalcTextSize(FIELD_TEMPLATE.data()).x);
        ImGui::InputText("##login_password",password,IM_ARRAYSIZE(password),ImGuiInputTextFlags_Password);
        ImGui::PopItemWidth();

        ImGui::Text("Client Id:");
        ImGui::SameLine(fieldsPosition);
        ImGui::PushItemWidth(ImGui::CalcTextSize(FIELD_TEMPLATE.data()).x);
        ImGui::InputText("##login_clientid",clientId,IM_ARRAYSIZE(clientId),ImGuiInputTextFlags_None);
        ImGui::PopItemWidth();

        ImGui::Text("Secret:");
        ImGui::SameLine(fieldsPosition);
        ImGui::PushItemWidth(ImGui::CalcTextSize(FIELD_TEMPLATE.data()).x);
        ImGui::InputText("##login_secret",secret,IM_ARRAYSIZE(secret),ImGuiInputTextFlags_None);
        ImGui::PopItemWidth();

        ImGui::Text("Website:");
        ImGui::SameLine(fieldsPosition);
        ImGui::PushItemWidth(ImGui::CalcTextSize(FIELD_TEMPLATE.data()).x);
        ImGui::InputText("##login_website",website,IM_ARRAYSIZE(website),ImGuiInputTextFlags_None);
        ImGui::PopItemWidth();

        ImGui::Text("App Name:");
        ImGui::SameLine(fieldsPosition);
        ImGui::PushItemWidth(ImGui::CalcTextSize(FIELD_TEMPLATE.data()).x);
        ImGui::InputText("##login_appname",appName,IM_ARRAYSIZE(appName),ImGuiInputTextFlags_None);
        ImGui::PopItemWidth();

        ImGui::Spacing();
        int spacing = ImGui::GetWindowContentRegionMax().x - 100;
        if(testingInProgress)
        {
            spacing -= (ImGui::GetTextLineHeight()*1.5);
        }
        ImGui::SameLine(spacing);
        bool testDisabled = std::string_view(username).empty() ||
                std::string_view(password).empty() ||
                std::string_view(clientId).empty() ||
                std::string_view(secret).empty() ||
                std::string_view(website).empty() ||
                std::string_view(appName).empty() ||
                testingInProgress;

        if(testDisabled)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }

        if(testingInProgress)
        {
            constexpr std::string_view id = "###spinner_test";
            ImGui::Spinner(id.data(),ImGui::GetTextLineHeight()/2.0,1,ImGui::GetColorU32(ImGuiCol_ButtonActive));
            ImGui::SameLine();
        }
        if (ImGui::Button("Test", ImVec2(100, 0)))
        {
            testingInProgress = true;
            tested = false;
            testingErrorMessage.clear();
            loginConnection->login(user(username,password,clientId,secret,website,appName));
        }
        if (testDisabled)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        if(!testingErrorMessage.empty())
        {
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),"%s",testingErrorMessage.c_str());
        }
        if(tested && !testingInProgress)
        {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f),"OK");
        }

        ImGui::TextWrapped(
            "To obtain this information, go to https://www.reddit.com/prefs/apps/ and add "
            "an app of type <script>. Then insert here the values that you obtain from reddit.");

        ImGui::Separator();
        bool okDisabled = !tested;
        if(okDisabled)
        {
            ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        if (ImGui::Button("OK", ImVec2(120, 0)) ||
                (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Enter)) && !okDisabled))
        {
            okPressed = true;

            ImGui::CloseCurrentPopup();
        }
        if (okDisabled)
        {
            ImGui::PopItemFlag();
            ImGui::PopStyleVar();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0)) ||
                ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Escape)))
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
    return okPressed;
}
