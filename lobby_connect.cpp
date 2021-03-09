/* Copyright (C) 2019 Mr Goldberg
   This file is part of the Goldberg Emulator

   The Goldberg Emulator is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 3 of the License, or (at your option) any later version.

   The Goldberg Emulator is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the Goldberg Emulator; if not, see
   <http://www.gnu.org/licenses/>.  */

/*
*/

#include "sdk_includes/steam_api.h"
#include "dll/common_includes.h"

#include <iostream>
#include <chrono>
#include <thread>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#else

#endif
int main() {
    if (SteamAPI_Init()) {
        //Set appid to: LOBBY_CONNECT_APPID
        SteamAPI_RestartAppIfNecessary(LOBBY_CONNECT_APPID);
        std::cout << "This is a program to find lobbies and run the game with lobby connect parameters" << std::endl;
        std::cout << "Api initialized, ";
top:
        std::cout << "waiting a few seconds for connections:" << std::endl;
        for (int i = 0; i < 10; ++i) {
            SteamAPI_RunCallbacks();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        int friend_count = SteamFriends()->GetFriendCount(k_EFriendFlagAll);
        std::cout << "People on the network: " << friend_count << std::endl;
        for (int i = 0; i < friend_count; ++i) {
            CSteamID id = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagAll);
            const char *name = SteamFriends()->GetFriendPersonaName(id);

            FriendGameInfo_t friend_info = {};
            SteamFriends()->GetFriendGamePlayed(id, &friend_info);
            std::cout << name << " is playing: " << friend_info.m_gameID.AppID() << std::endl;
        }

        std::cout << std::endl << "--------------Menu-------------" << std::endl << "\tappid\tname\tcommand line" << std::endl;

        std::vector<std::string> arguments;
        for (int i = 0; i < friend_count; ++i) {
            CSteamID id = SteamFriends()->GetFriendByIndex(i, k_EFriendFlagAll);
            const char *name = SteamFriends()->GetFriendPersonaName(id);
            const char *connect = SteamFriends()->GetFriendRichPresence( id, "connect");
            FriendGameInfo_t friend_info = {};
            SteamFriends()->GetFriendGamePlayed(id, &friend_info);

            if (strlen(connect) > 0) {
                std::cout << arguments.size() << "\t" << friend_info.m_gameID.AppID() << "\t" << name << "\t" << connect << std::endl;
                arguments.push_back(connect);
            } else {
                if (friend_info.m_steamIDLobby != k_steamIDNil) {
                    std::string connect = "+connect_lobby " + std::to_string(friend_info.m_steamIDLobby.ConvertToUint64());
                    std::cout << arguments.size() << "\t" << friend_info.m_gameID.AppID() << "\t" << name << "\t" << connect << std::endl;
                    arguments.push_back(connect);
                }
            }
        }

        std::cout << arguments.size() << ": Retry." << std::endl;
        std::cout << std::endl << "Enter the number corresponding to your choice then press Enter." << std::endl;
        unsigned int choice;
        std::cin >> choice;

        if (choice >= arguments.size()) goto top;

#ifdef _WIN32
        std::cout << "starting the game with: " << arguments[choice] << std::endl << "Please select the game exe" << std::endl;

        OPENFILENAMEA ofn;
        char szFileName[MAX_PATH] = "";
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = 0;
        ofn.lpstrFilter = "Exe Files (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
        ofn.lpstrFile = szFileName;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
        ofn.lpstrDefExt = "txt";
        if(GetOpenFileNameA(&ofn))
        {
            std::string filename = szFileName;
            filename = "\"" + filename + "\" " + arguments[choice];
            std::cout << filename << std::endl;
            STARTUPINFOA lpStartupInfo;
            PROCESS_INFORMATION lpProcessInfo;

            ZeroMemory( &lpStartupInfo, sizeof( lpStartupInfo ) );
            lpStartupInfo.cb = sizeof( lpStartupInfo );
            ZeroMemory( &lpProcessInfo, sizeof( lpProcessInfo ) );

            CreateProcessA( NULL,
                        const_cast<char *>(filename.c_str()), NULL, NULL,
                        NULL, NULL, NULL, NULL,
                        &lpStartupInfo,
                        &lpProcessInfo
                        );
        }
#else
        std::cout << "Please launch the game with these arguments: " << arguments[choice] << std::endl;
#endif
    }
}
