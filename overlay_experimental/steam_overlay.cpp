#include "steam_overlay.h"

#ifdef EMU_OVERLAY

#include <thread>
#include <string>
#include <sstream>
#include <cctype>
#include <imgui.h>

#include "../dll/dll.h"

#include "Renderer_Detector.h"

static constexpr int max_window_id = 10000;
static constexpr int base_notif_window_id  = 0 * max_window_id;
static constexpr int base_friend_window_id = 1 * max_window_id;
static constexpr int base_friend_item_id   = 2 * max_window_id;

int find_free_id(std::vector<int> & ids, int base)
{
    std::sort(ids.begin(), ids.end());

    int id = base;
    for (auto i : ids)
    {
        if (id < i)
            break;
        id = i + 1;
    }

    return id > (base+max_window_id) ? 0 : id;
}

int find_free_friend_id(std::map<Friend, friend_window_state, Friend_Less> const& friend_windows)
{
    std::vector<int> ids;
    ids.reserve(friend_windows.size());

    std::for_each(friend_windows.begin(), friend_windows.end(), [&ids](std::pair<Friend const, friend_window_state> const& i)
    {
        ids.emplace_back(i.second.id);
    });
    
    return find_free_id(ids, base_friend_window_id);
}

int find_free_notification_id(std::vector<Notification> const& notifications)
{
    std::vector<int> ids;
    ids.reserve(notifications.size());

    std::for_each(notifications.begin(), notifications.end(), [&ids](Notification const& i)
    {
        ids.emplace_back(i.id);
    });
    

    return find_free_id(ids, base_friend_window_id);
}

#ifdef __WINDOWS__
#include "windows/Windows_Hook.h"
#endif

#include "notification.h"

void Steam_Overlay::steam_overlay_run_every_runcb(void* object)
{
    Steam_Overlay* _this = reinterpret_cast<Steam_Overlay*>(object);
    _this->RunCallbacks();
}

void Steam_Overlay::steam_overlay_callback(void* object, Common_Message* msg)
{
    Steam_Overlay* _this = reinterpret_cast<Steam_Overlay*>(object);
    _this->Callback(msg);
}

Steam_Overlay::Steam_Overlay(Settings* settings, SteamCallResults* callback_results, SteamCallBacks* callbacks, RunEveryRunCB* run_every_runcb, Networking* network) :
    settings(settings),
    callback_results(callback_results),
    callbacks(callbacks),
    run_every_runcb(run_every_runcb),
    network(network),
    setup_overlay_called(false),
    show_overlay(false),
    is_ready(false),
    notif_position(ENotificationPosition::k_EPositionBottomLeft),
    h_inset(0),
    v_inset(0),
    overlay_state_changed(false),
    i_have_lobby(false)
{
    run_every_runcb->add(&Steam_Overlay::steam_overlay_run_every_runcb, this);
    this->network->setCallback(CALLBACK_ID_STEAM_MESSAGES, settings->get_local_steam_id(), &Steam_Overlay::steam_overlay_callback, this);
}

Steam_Overlay::~Steam_Overlay()
{
    run_every_runcb->remove(&Steam_Overlay::steam_overlay_run_every_runcb, this);
}

bool Steam_Overlay::Ready() const
{
    return is_ready;
}

bool Steam_Overlay::NeedPresent() const
{
    return true;
}

void Steam_Overlay::SetNotificationPosition(ENotificationPosition eNotificationPosition)
{
    notif_position = eNotificationPosition;
}

void Steam_Overlay::SetNotificationInset(int nHorizontalInset, int nVerticalInset)
{
    h_inset = nHorizontalInset;
    v_inset = nVerticalInset;
}

void Steam_Overlay::SetupOverlay()
{
    PRINT_DEBUG("%s\n", __FUNCTION__);
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    if (!setup_overlay_called)
    {
        setup_overlay_called = true;
        Renderer_Detector::Inst().find_renderer();
    }
}

void Steam_Overlay::HookReady()
{
    if (!is_ready)
    {
        // TODO: Uncomment this and draw our own cursor (cosmetics)
        //ImGuiIO &io = ImGui::GetIO();
        //io.WantSetMousePos = false;
        //io.MouseDrawCursor = false;
        //io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

        is_ready = true;
    }
}

void Steam_Overlay::OpenOverlayInvite(CSteamID lobbyId)
{
    ShowOverlay(true);
}

void Steam_Overlay::OpenOverlay(const char* pchDialog)
{
    // TODO: Show pages depending on pchDialog
    ShowOverlay(true);
}

bool Steam_Overlay::ShowOverlay() const
{
    return show_overlay;
}

void Steam_Overlay::ShowOverlay(bool state)
{
    if (!Ready() || show_overlay == state)
        return;

    ImGuiIO &io = ImGui::GetIO();

    if(state)
    {
        io.MouseDrawCursor = true;
    }
    else
    {
        io.MouseDrawCursor = false;
    }

#ifdef __WINDOWS__
    static RECT old_clip;

    if (state)
    {
        HWND game_hwnd = Windows_Hook::Inst()->GetGameHwnd();
        RECT cliRect, wndRect, clipRect;

        GetClipCursor(&old_clip);
        // The window rectangle has borders and menus counted in the size
        GetWindowRect(game_hwnd, &wndRect);
        // The client rectangle is the window without borders
        GetClientRect(game_hwnd, &cliRect);

        clipRect = wndRect; // Init clip rectangle

        // Get Window width with borders
        wndRect.right -= wndRect.left;
        // Get Window height with borders & menus
        wndRect.bottom -= wndRect.top;
        // Compute the border width
        int borderWidth = (wndRect.right - cliRect.right) / 2;
        // Client top clip is the menu bar width minus bottom border
        clipRect.top += wndRect.bottom - cliRect.bottom - borderWidth;
        // Client left clip is the left border minus border width
        clipRect.left += borderWidth;
        // Here goes the same for right and bottom
        clipRect.right -= borderWidth;
        clipRect.bottom -= borderWidth;

        ClipCursor(&clipRect);
    }
    else
    {
        ClipCursor(&old_clip);
    }

#else

#endif

    show_overlay = state;
    overlay_state_changed = true;
}

void Steam_Overlay::NotifyUser(friend_window_state& friend_state)
{
    if (!(friend_state.window_state & window_state_show) || !show_overlay)
    {
        friend_state.window_state |= window_state_need_attention;
#ifdef __WINDOWS__
        PlaySound((LPCSTR)notif_invite_wav, NULL, SND_ASYNC | SND_MEMORY);
#endif
    }
}

void Steam_Overlay::SetLobbyInvite(Friend friendId, uint64 lobbyId)
{
    if (!Ready())
        return;

    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    auto i = friends.find(friendId);
    if (i != friends.end())
    {
        auto& frd = i->second;
        frd.lobbyId = lobbyId;
        frd.window_state |= window_state_lobby_invite;
        // Make sure don't have rich presence invite and a lobby invite (it should not happen but who knows)
        frd.window_state &= ~window_state_rich_invite;
        AddInviteNotification(*i);
        NotifyUser(i->second);
    }
}

void Steam_Overlay::SetRichInvite(Friend friendId, const char* connect_str)
{
    if (!Ready())
        return;

    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    auto i = friends.find(friendId);
    if (i != friends.end())
    {
        auto& frd = i->second;
        strncpy(frd.connect, connect_str, k_cchMaxRichPresenceValueLength - 1);
        frd.window_state |= window_state_rich_invite;
        // Make sure don't have rich presence invite and a lobby invite (it should not happen but who knows)
        frd.window_state &= ~window_state_lobby_invite;
        AddInviteNotification(*i);
        NotifyUser(i->second);
    }
}

void Steam_Overlay::FriendConnect(Friend _friend)
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    int id = find_free_friend_id(friends);
    if (id != 0)
    {
        auto& item = friends[_friend];
        item.window_title = std::move(_friend.name() + " playing " + std::to_string(_friend.appid()));
        item.window_state = window_state_none;
        item.id = id;
        memset(item.chat_input, 0, max_chat_len);
        item.joinable = false;
    }
    else
        PRINT_DEBUG("No more free id to create a friend window\n");
}

void Steam_Overlay::FriendDisconnect(Friend _friend)
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    auto it = friends.find(_friend);
    if (it != friends.end())
        friends.erase(it);
}

void Steam_Overlay::AddMessageNotification(std::string const& message)
{
    std::lock_guard<std::recursive_mutex> lock(notifications_mutex);
    int id = find_free_notification_id(notifications);
    if (id != 0)
    {
        Notification notif;
        notif.id = id;
        notif.type = notification_type_message;
        notif.message = message;
        notif.start_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
        notifications.emplace_back(notif);
    }
    else
        PRINT_DEBUG("No more free id to create a notification window\n");
}

void Steam_Overlay::AddAchievementNotification(nlohmann::json const& ach)
{
    std::lock_guard<std::recursive_mutex> lock(notifications_mutex);
    int id = find_free_notification_id(notifications);
    if (id != 0)
    {
        Notification notif;
        notif.id = id;
        notif.type = notification_type_achievement;
        // Load achievement image
        notif.message = ach["displayName"].get<std::string>() + "\n" + ach["description"].get<std::string>();
        notif.start_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
        notifications.emplace_back(notif);
    }
    else
        PRINT_DEBUG("No more free id to create a notification window\n");
}

void Steam_Overlay::AddInviteNotification(std::pair<const Friend, friend_window_state>& wnd_state)
{
    std::lock_guard<std::recursive_mutex> lock(notifications_mutex);
    int id = find_free_notification_id(notifications);
    if (id != 0)
    {
        Notification notif;
        notif.id = id;
        notif.type = notification_type_invite;
        notif.frd = &wnd_state;
        notif.message = wnd_state.first.name() + " invited you to join a game";
        notif.start_time = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch());
        notifications.emplace_back(notif);
    }
    else
        PRINT_DEBUG("No more free id to create a notification window\n");
}

bool Steam_Overlay::FriendJoinable(std::pair<const Friend, friend_window_state> &f)
{
    Steam_Friends* steamFriends = get_steam_client()->steam_friends;

    if( std::string(steamFriends->GetFriendRichPresence(f.first.id(), "connect")).length() > 0 )
        return true;

    FriendGameInfo_t friend_game_info = {};
    steamFriends->GetFriendGamePlayed(f.first.id(), &friend_game_info);
    if (friend_game_info.m_steamIDLobby.IsValid() && (f.second.window_state & window_state_lobby_invite))
        return true;

    return false;
}

bool Steam_Overlay::IHaveLobby()
{
    Steam_Friends* steamFriends = get_steam_client()->steam_friends;
    if (std::string(steamFriends->GetFriendRichPresence(settings->get_local_steam_id(), "connect")).length() > 0)
        return true;

    if (settings->get_lobby().IsValid())
        return true;

    return false;
}

void Steam_Overlay::BuildContextMenu(Friend const& frd, friend_window_state& state)
{
    if (ImGui::BeginPopupContextItem("Friends_ContextMenu", 1))
    {
        bool close_popup = false;

        if (ImGui::Button("Chat"))
        {
            state.window_state |= window_state_show;
            close_popup = true;
        }
        // If we have the same appid, activate the invite/join buttons
        if (settings->get_local_game_id().AppID() == frd.appid())
        {
            if (i_have_lobby && ImGui::Button("Invite###PopupInvite"))
            {
                state.window_state |= window_state_invite;
                has_friend_action.push(frd);
                close_popup = true;
            }
            if (state.joinable && ImGui::Button("Join###PopupJoin"))
            {
                state.window_state |= window_state_join;
                has_friend_action.push(frd);
                close_popup = true;
            }
        }
        if( close_popup)
        {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void Steam_Overlay::BuildFriendWindow(Friend const& frd, friend_window_state& state)
{
    if (!(state.window_state & window_state_show))
        return;

    bool show = true;
    bool send_chat_msg = false;

    float width = ImGui::CalcTextSize("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA").x;
    
    if (state.window_state & window_state_need_attention && ImGui::IsWindowFocused())
    {
        state.window_state &= ~window_state_need_attention;
    }
    ImGui::SetNextWindowSizeConstraints(ImVec2{ width, ImGui::GetFontSize()*8 + ImGui::GetFrameHeightWithSpacing()*4 },
        ImVec2{ std::numeric_limits<float>::max() , std::numeric_limits<float>::max() });

    // Window id is after the ###, the window title is the friend name
    std::string friend_window_id = std::move("###" + std::to_string(state.id));
    if (ImGui::Begin((state.window_title + friend_window_id).c_str(), &show))
    {
        if (state.window_state & window_state_need_attention && ImGui::IsWindowFocused())
        {
            state.window_state &= ~window_state_need_attention;
        }

        // Fill this with the chat box and maybe the invitation
        if (state.window_state & (window_state_lobby_invite | window_state_rich_invite))
        {
            ImGui::LabelText("##label", "%s invited you to join the game.", frd.name().c_str());
            ImGui::SameLine();
            if (ImGui::Button("Accept"))
            {
                state.window_state |= window_state_join;
                this->has_friend_action.push(frd);
            }
            ImGui::SameLine();
            if (ImGui::Button("Refuse"))
            {
                state.window_state &= ~(window_state_lobby_invite | window_state_rich_invite);
            }
        }

        ImGui::InputTextMultiline("##chat_history", &state.chat_history[0], state.chat_history.length(), { -1.0f, 0 }, ImGuiInputTextFlags_ReadOnly);
        // TODO: Fix the layout of the chat line + send button.
        // It should be like this: chat input should fill the window size minus send button size (button size is fixed)
        // |------------------------------|
        // | /--------------------------\ |
        // | |                          | |
        // | |       chat history       | |
        // | |                          | |
        // | \--------------------------/ |
        // | [____chat line______] [send] |
        // |------------------------------|
        //
        // And it is like this
        // |------------------------------|
        // | /--------------------------\ |
        // | |                          | |
        // | |       chat history       | |
        // | |                          | |
        // | \--------------------------/ |
        // | [__chat line__] [send]       |
        // |------------------------------|
        float wnd_width = ImGui::GetWindowContentRegionWidth();
        ImGuiStyle &style = ImGui::GetStyle();
        wnd_width -= ImGui::CalcTextSize("Send").x + style.FramePadding.x * 2 + style.ItemSpacing.x + 1;

        ImGui::PushItemWidth(wnd_width);
        if (ImGui::InputText("##chat_line", state.chat_input, max_chat_len, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            send_chat_msg = true;
        }
        ImGui::PopItemWidth();

        ImGui::SameLine();

        if (ImGui::Button("Send"))
        {
            send_chat_msg = true;
        }

        if (send_chat_msg)
        {
            if (!(state.window_state & window_state_send_message))
            {
                has_friend_action.push(frd);
                state.window_state |= window_state_send_message;
            }
        }
    }
    // User closed the friend window
    if (!show)
        state.window_state &= ~window_state_show;

    ImGui::End();
}

ImFont *font_default;
ImFont *font_notif;

void Steam_Overlay::BuildNotifications(int width, int height)
{
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    int i = 0;

    int font_size = ImGui::GetFontSize();

    std::queue<Friend> friend_actions_temp;

    {
        std::lock_guard<std::recursive_mutex> lock(notifications_mutex);

        for (auto it = notifications.begin(); it != notifications.end(); ++it, ++i)
        {
            auto elapsed_notif = now - it->start_time;

            if ( elapsed_notif < Notification::fade_in)
            {
                float alpha = Notification::max_alpha * (elapsed_notif.count() / static_cast<float>(Notification::fade_in.count()));
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, alpha));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(Notification::r, Notification::g, Notification::b, alpha));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 255, alpha*2));
            }
            else if ( elapsed_notif > Notification::fade_out_start)
            {
                float alpha = Notification::max_alpha * ((Notification::show_time - elapsed_notif).count() / static_cast<float>(Notification::fade_out.count()));
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, alpha));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(Notification::r, Notification::g, Notification::b, alpha));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 255, alpha*2));
            }
            else
            {
                ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, Notification::max_alpha));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(Notification::r, Notification::g, Notification::b, Notification::max_alpha));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(255, 255, 255, Notification::max_alpha*2));
            }
            
            ImGui::SetNextWindowPos(ImVec2((float)width - width * Notification::width, Notification::height * font_size * i ));
            ImGui::SetNextWindowSize(ImVec2( width * Notification::width, Notification::height * font_size ));
            ImGui::Begin(std::to_string(it->id).c_str(), nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | 
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoDecoration);

            switch (it->type)
            {
                case notification_type_achievement:
                    ImGui::TextWrapped("%s", it->message.c_str());
                    break;
                case notification_type_invite:
                    {
                        ImGui::TextWrapped("%s", it->message.c_str());
                        if (ImGui::Button("Join"))
                        {
                            it->frd->second.window_state |= window_state_join;
                            friend_actions_temp.push(it->frd->first);
                            it->start_time = std::chrono::seconds(0);
                        }
                    }
                    break;
                case notification_type_message:
                    ImGui::TextWrapped("%s", it->message.c_str()); break;
            }

            ImGui::End();

            ImGui::PopStyleColor(3);
        }
        notifications.erase(std::remove_if(notifications.begin(), notifications.end(), [&now](Notification &item) {
            return (now - item.start_time) > Notification::show_time;
        }), notifications.end());
    }

    if (!friend_actions_temp.empty()) {
        std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
        while (!friend_actions_temp.empty()) {
            has_friend_action.push(friend_actions_temp.front());
            friend_actions_temp.pop();
        }
    }
}

void Steam_Overlay::CreateFonts()
{
    ImGuiIO& io = ImGui::GetIO();
    ImFontConfig fontcfg;

    fontcfg.OversampleH = fontcfg.OversampleV = 1;
    fontcfg.PixelSnapH = true;
    fontcfg.GlyphRanges = io.Fonts->GetGlyphRangesDefault();

    fontcfg.SizePixels = std::round(io.DisplaySize.y / 68);
    font_default = io.Fonts->AddFontDefault(&fontcfg);

    fontcfg.SizePixels = std::round(io.DisplaySize.y / 60);
    font_notif = io.Fonts->AddFontDefault(&fontcfg);

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0; // Disable round window
}

// Try to make this function as short as possible or it might affect game's fps.
void Steam_Overlay::OverlayProc()
{
    if (!Ready())
        return;

    ImGuiIO& io = ImGui::GetIO();

    ImGui::PushFont(font_notif);
    BuildNotifications(io.DisplaySize.x, io.DisplaySize.y);
    ImGui::PopFont();

    if (show_overlay)
    {
        io.ConfigFlags &= ~ImGuiConfigFlags_NoMouseCursorChange;
        // Set the overlay windows to the size of the game window
        ImGui::SetNextWindowPos({ 0,0 });
        ImGui::SetNextWindowSize({ static_cast<float>(io.DisplaySize.x),
                                   static_cast<float>(io.DisplaySize.y) });

        ImGui::SetNextWindowBgAlpha(0.50);

        ImGui::PushFont(font_default);

        bool show = true;

        if (ImGui::Begin("SteamOverlay", &show, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus))
        {
            ImGui::LabelText("##label", "Username: %s(%llu) playing %u",
                settings->get_local_name(),
                settings->get_local_steam_id().ConvertToUint64(),
                settings->get_local_game_id().AppID());
            ImGui::SameLine();
            Base_Hook* hook = Renderer_Detector::Inst().get_renderer();
            ImGui::LabelText("##label", "Renderer: %s", (hook == nullptr ? "Unknown" : hook->get_lib_name()));

            ImGui::Spacing();

            ImGui::LabelText("##label", "Friends");

            std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
            if (!friends.empty())
            {
                ImGui::ListBoxHeader("##label", friends.size());
                std::for_each(friends.begin(), friends.end(), [this](std::pair<Friend const, friend_window_state> &i)
                {
                    ImGui::PushID(i.second.id-base_friend_window_id+base_friend_item_id);

                    ImGui::Selectable(i.second.window_title.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick);
                    BuildContextMenu(i.first, i.second);
                    if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(0))
                    {
                        i.second.window_state |= window_state_show;
                    }
                    ImGui::PopID();

                    BuildFriendWindow(i.first, i.second);
                });
                ImGui::ListBoxFooter();
            }
        }
        ImGui::End();

        ImGui::PopFont();

        if (!show)
            ShowOverlay(false);
    } else {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    }
}

void Steam_Overlay::Callback(Common_Message *msg)
{
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    if (msg->has_steam_messages())
    {
        Friend frd;
        frd.set_id(msg->source_id());
        auto friend_info = friends.find(frd);
        if (friend_info != friends.end())
        {
            Steam_Messages const& steam_message = msg->steam_messages();
            // Change color to cyan for friend
            friend_info->second.chat_history.append("\x1""00FFFFFF", 9).append(steam_message.message()).append("\n", 1);
            if (!(friend_info->second.window_state & window_state_show))
            {
                friend_info->second.window_state |= window_state_need_attention;
            }

            AddMessageNotification(friend_info->first.name() + " says: " + steam_message.message());
            NotifyUser(friend_info->second);
        }
    }
}

void Steam_Overlay::RunCallbacks()
{
    if (overlay_state_changed)
    {
        GameOverlayActivated_t data = { 0 };
        data.m_bActive = show_overlay;
        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));

        overlay_state_changed = false;
    }

    Steam_Friends* steamFriends = get_steam_client()->steam_friends;
    Steam_Matchmaking* steamMatchmaking = get_steam_client()->steam_matchmaking;

    i_have_lobby = IHaveLobby();
    std::lock_guard<std::recursive_mutex> lock(overlay_mutex);
    std::for_each(friends.begin(), friends.end(), [this](std::pair<Friend const, friend_window_state> &i)
    {
        i.second.joinable = FriendJoinable(i);
    });

    while (!has_friend_action.empty())
    {
        auto friend_info = friends.find(has_friend_action.front());
        if (friend_info != friends.end())
        {
            uint64 friend_id = friend_info->first.id();
            // The user clicked on "Send"
            if (friend_info->second.window_state & window_state_send_message)
            {
                char* input = friend_info->second.chat_input;
                char* end_input = input + strlen(input);
                char* printable_char = std::find_if(input, end_input, [](char c) {
                    return std::isgraph(c);
                });
                // Check if the message contains something else than blanks
                if (printable_char != end_input)
                {
                    // Handle chat send
                    Common_Message msg;
                    Steam_Messages* steam_messages = new Steam_Messages;
                    steam_messages->set_type(Steam_Messages::FRIEND_CHAT);
                    steam_messages->set_message(friend_info->second.chat_input);
                    msg.set_allocated_steam_messages(steam_messages);
                    msg.set_source_id(settings->get_local_steam_id().ConvertToUint64());
                    msg.set_dest_id(friend_id);
                    network->sendTo(&msg, true);

                    friend_info->second.chat_history.append("\x1""00FF00FF", 9).append(input).append("\n", 1);
                }
                *input = 0; // Reset the input field
                friend_info->second.window_state &= ~window_state_send_message;
            }
            // The user clicked on "Invite"
            if (friend_info->second.window_state & window_state_invite)
            {
                std::string connect = steamFriends->GetFriendRichPresence(settings->get_local_steam_id(), "connect");
                if (connect.length() > 0)
                    steamFriends->InviteUserToGame(friend_id, connect.c_str());
                else if (settings->get_lobby().IsValid())
                    steamMatchmaking->InviteUserToLobby(settings->get_lobby(), friend_id);

                friend_info->second.window_state &= ~window_state_invite;
            }
            // The user clicked on "Join"
            if (friend_info->second.window_state & window_state_join)
            {
                std::string connect = steamFriends->GetFriendRichPresence(friend_id, "connect");
                // The user got a lobby invite and accepted it
                if (friend_info->second.window_state & window_state_lobby_invite)
                {
                    GameLobbyJoinRequested_t data;
                    data.m_steamIDLobby.SetFromUint64(friend_info->second.lobbyId);
                    data.m_steamIDFriend.SetFromUint64(friend_id);
                    callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));

                    friend_info->second.window_state &= ~window_state_lobby_invite;
                } else {
                // The user got a rich presence invite and accepted it
                    if (friend_info->second.window_state & window_state_rich_invite)
                    {
                        GameRichPresenceJoinRequested_t data = {};
                        data.m_steamIDFriend.SetFromUint64(friend_id);
                        strncpy(data.m_rgchConnect, friend_info->second.connect, k_cchMaxRichPresenceValueLength - 1);
                        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));

                        friend_info->second.window_state &= ~window_state_rich_invite;
                    } else if (connect.length() > 0)
                    {
                        GameRichPresenceJoinRequested_t data = {};
                        data.m_steamIDFriend.SetFromUint64(friend_id);
                        strncpy(data.m_rgchConnect, connect.c_str(), k_cchMaxRichPresenceValueLength - 1);
                        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));
                    }

                    //Not sure about this but it fixes sonic racing transformed invites
                    FriendGameInfo_t friend_game_info = {};
                    steamFriends->GetFriendGamePlayed(friend_id, &friend_game_info);
                    uint64 lobby_id = friend_game_info.m_steamIDLobby.ConvertToUint64();
                    if (lobby_id) {
                        GameLobbyJoinRequested_t data;
                        data.m_steamIDLobby.SetFromUint64(lobby_id);
                        data.m_steamIDFriend.SetFromUint64(friend_id);
                        callbacks->addCBResult(data.k_iCallback, &data, sizeof(data));
                    }
                }

                friend_info->second.window_state &= ~window_state_join;
            }
        }
        has_friend_action.pop();
    }
}

#endif
