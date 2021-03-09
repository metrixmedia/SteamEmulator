#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <list>

#include <curl/curl.h>
#include <json/json.hpp>
#include <json/fifo_map.hpp>

class CurlGlobal
{
    bool _init;

    CurlGlobal() :_init(false) {}

    ~CurlGlobal() { cleanup(); }

public:
    static CurlGlobal& Inst()
    {
        static CurlGlobal _this;
        return _this;
    }

    CURLcode init(long flags = CURL_GLOBAL_DEFAULT) { return curl_global_init(flags); }
    void cleanup()
    {
        if (_init)
        {
            curl_global_cleanup();
            _init = false;
        }
    }
};

class CurlEasy
{
    CURL* _me;
    bool _init;
    std::string _buffer;

    static int writer(char* data, size_t size, size_t nmemb,
        CurlEasy *_this)
    {
        if (_this == nullptr)
            return 0;

        _this->_buffer.append(data, size * nmemb);

        return size * nmemb;
    }

public:
    CurlEasy() :_me(nullptr), _init(false) {}
    ~CurlEasy() { cleanup(); }

    bool init()
    {
        _init = (_me = curl_easy_init()) != nullptr;
        if (_init)
        {
            if (curl_easy_setopt(_me, CURLOPT_WRITEFUNCTION, writer) != CURLE_OK)
            {
                cleanup();
                return false;
            }

            if (curl_easy_setopt(_me, CURLOPT_WRITEDATA, this) != CURLE_OK)
            {
                cleanup();
                return false;
            }
        }
        return _init;
    }

    void cleanup()
    {
        if (_init)
        {
            curl_easy_cleanup(_me);
        }
    }

    CURLcode set_url(const std::string& url)
    {
        return curl_easy_setopt(_me, CURLOPT_URL, url.c_str());
    }

    CURLcode skip_verifypeer(bool skip = true)
    {
        return curl_easy_setopt(_me, CURLOPT_SSL_VERIFYPEER, skip ? 0L : 1L);
    }

    CURLcode skip_verifhost(bool skip = true)
    {
        return curl_easy_setopt(_me, CURLOPT_SSL_VERIFYHOST, skip ? 0L : 1L);
    }

    CURLcode connect_only(bool connect = true)
    {
        return curl_easy_setopt(_me, CURLOPT_CONNECT_ONLY, connect ? 1L : 0L);
    }

    CURLcode perform()
    {
        _buffer.clear();
        return curl_easy_perform(_me);
    }

    CURLcode recv(void *buffer, size_t buflen, size_t* read_len)
    {
        return curl_easy_recv(_me, buffer, buflen, read_len);
    }

    CURLcode get_html_code(long &code)
    {
        return curl_easy_getinfo(_me, CURLINFO_RESPONSE_CODE, &code);
    }

    std::string const& get_answer() const { return _buffer; }
};

// Get all steam appid with their name: http://api.steampowered.com/ISteamApps/GetAppList/v2/
// Steam storefront webapi: https://wiki.teamfortress.com/wiki/User:RJackson/StorefrontAPI
// http://api.steampowered.com/ISteamUserStats/GetSchemaForGame/v2/?key=<key>&appid=<appid>
/*
{
  "game" : {
    "gameName" : "<name>",
    "availableGameStats" : {
      "achievements" : {
        ("<id>" : {
          "name" : "achievement_name",
          "displayName" : "achievement name on screen",
          "hidden" : (0|1),
          ["description" : "<desc>",]
          "icon" : "<url to icon when achievement is earned>",
          "icongray" : "<url to icon when achievement is not earned>"
        },
        ...)
      }
    }
  }
}
*/
// Get appid infos: http://store.steampowered.com/api/appdetails/?appids=218620
/*
"appid" : {
  "success" : (true|false),
  (success == true "data" : {
    ...
    "name" : "<name>",
    "steam_appid" : <appid>,
    (OPT "dlc" : [<dlc id>, <dlc id>]),
    "header_image" : "<miniature url>" <-- Use this in the overlay ?
    (OPT "achievements" : {
      "total" : <num of achievements>
    }),
    "background" : "<background url>" <-- Use this as the overlay background ?
    (OPT "packages" : [<package id>, <package id>])
  })
}
*/
// ---------------------------------
// -- Special thanks to psychonic --
// ---------------------------------
// Get game items definition digest (Phase1): https://api.steampowered.com/IInventoryService/GetItemDefMeta/v1?key=<webapi_key>&appid=218620
/*
{
  "response": {
    "modified": 1566848385,
    "digest": "3CDFC1CC1AC2B0D55D12C1C130F4294BDD6DF8D0"
  }
}
*/

// Get game items definition: https://api.steampowered.com/IGameInventory/GetItemDefArchive/v0001?appid=218620&digest=<digest>
/*
[
  { 
    "appid":"218620",
    "itemdefid":"0",
    "Timestamp":"2016-04-08T18:00:21.3643085Z",
    "modified":"20160408T180021Z",
    "date_created":"20160408T180021Z",
    "type":"",
    "display_type":"",
    "name":"",
    "quantity":0,
    "description":"",
    "tradable":false,
    "marketable":false,
    "commodity":false,
    "drop_interval":0,
    "drop_max_per_window":0,
    "workshopid":"0"
  },
  {
    "appid":"218620",
    "itemdefid":"50002",
    "Timestamp":"2015-11-13T16:01:18.0338618Z",
    "modified":"20151113T160117Z",
    "date_created":"20151113T160117Z",
    "type":"item",
    "display_type":"",
    "name":"Sputnik Safe",
    "quantity":0,
    "description":"[color=#2360D8]THE JUDGE SHOTGUN | Pixel [/color]\n[color=#2360D8]KOBUS 90 SUBMACHINE GUN | Red Stars[/color]\n[color=#2360D8]PLAINSRIDER BOW | Arctic Plains[/color]\n[color=#2360D8]GRUBER KURZ PISTOL | Little Leopard[/color]\n[color=#2360D8]HRL-7 ROCKET LAUNCHER | Headline[/color]\n[color=#2360D8]LOCOMOTIVE 12G SHOTGUN | Cosmonaut[/color]\n[color=#9900FF]FLAMETHROWER | St. Basil[/color]\n[color=#9900FF]JP36 RIFLE | Ice Leopard [/color]\n[color=#9900FF]CAR-4 RIFLE | Stripe On[/color]\n[color=#9900FF]BRONCO .44 REVOLVER | Black Bull[/color]\n[color=#FF00FF]BERNETTI 9 PISTOL | Angry Bear[/color]\n[color=#FF00FF]THANATOS .50 CAL SNIPER RIFLE | Matrjoschka[/color]\n[color=#FF00FF]M308 RIFLE | Helmet Space Program[/color]\n[color=#FF0000]CLARION RIFLE | Breaching Owl[/color]\n[color=#FF0000]MOSCONI 12G SHOTGUN | Bullet Bear Gun[/color]\n[color=#FFAA00]or an exceedingly rare special item![/color]",
    "icon_url":"http://media.overkillsoftware.com/economy42gF2Y/safes_weapon_01.png",
    "icon_url_large":"http://media.overkillsoftware.com/economy42gF2Y/safes_weapon_01.png",
    "store_tags":"safe;sputnik safe;",
    "tradable":true,
    "marketable":true,
    "commodity":false,
    "drop_interval":0,
    "drop_max_per_window":0,
    "workshopid":"0",
    "dsl_bonus":"false",
    "item_name":"weapon_01",
    "item_slot":"safes"
  }
*/

#ifdef max
#undef max
#endif

std::string steam_apikey;
std::string app_id;
std::string output_path;

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>

static bool create_directory(std::string const& strPath)
{
    DWORD dwAttrib = GetFileAttributesA(strPath.c_str());

    if (dwAttrib != INVALID_FILE_ATTRIBUTES && dwAttrib & FILE_ATTRIBUTE_DIRECTORY)
        return true;
        
    return CreateDirectoryA(strPath.c_str(), NULL);
}
#elif defined(__linux__)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

static bool create_directory(std::string const& strPath)
{
    struct stat sb;

    if (stat(strPath.c_str(), &sb) != 0)
    {
        return mkdir(strPath.c_str(), 0755) == 0;
    }
    if (S_ISDIR(sb.st_mode))
        return true;

    return false;
}

#endif

static void generate_achievements(CurlEasy &easy)
{
    std::string url = "https://api.steampowered.com/ISteamUserStats/GetSchemaForGame/v2/?key=";
    url += steam_apikey;
    url += "&appid=";
    url += app_id;
    easy.set_url(url);
    easy.perform();
    try
    {
        std::ofstream ach_file(output_path + "/achievements.json", std::ios::trunc | std::ios::out);
        nlohmann::json json = nlohmann::json::parse(easy.get_answer());
        nlohmann::json output_json = nlohmann::json::array();

        bool first = true;
        int i = 0;
        for (auto& item : json["game"]["availableGameStats"]["achievements"].items())
        {
            output_json[i]["name"] = item.value()["name"];
            output_json[i]["displayName"] = item.value()["displayName"];
            output_json[i]["hidden"] = std::to_string(item.value()["hidden"].get<int>());
            try
            {
                if( !item.value()["description"].is_null() )
                    output_json[i]["description"] = item.value()["description"];
                else
                    output_json[i]["description"] = "";
            }
            catch (...)
            {
                output_json[i]["description"] = "";
            }
            
            {
                std::string icon_path = "images/" + item.value()["name"].get<std::string>() + ".jpg";
                std::ofstream achievement_icon(output_path + "/" + icon_path, std::ios::out | std::ios::trunc | std::ios::binary);
                if (!achievement_icon)
                {
                    std::cerr << "Cannot create achievement icon \"" << icon_path << "\"" << std::endl;
                    return;
                }
                easy.set_url(item.value()["icon"]);
                easy.perform();

                std::string picture = easy.get_answer();
                achievement_icon.write(picture.c_str(), picture.length());

                output_json[i]["icon"] = icon_path;
                
            }
            {
                std::string icon_path = "images/" + item.value()["name"].get<std::string>() + "_gray.jpg";
                std::ofstream achievement_icon(output_path + "/" + icon_path, std::ios::out | std::ios::trunc | std::ios::binary);
                if (!achievement_icon)
                {
                    std::cerr << "Cannot create achievement icon \"" << icon_path << "\"" << std::endl;
                    return;
                }
                easy.set_url(item.value()["icongray"]);
                easy.perform();
                
                std::string picture = easy.get_answer();
                achievement_icon.write(picture.c_str(), picture.length());

                output_json[i]["icongray"] = icon_path;
            }
            ++i;
        }
        ach_file << std::setw(2) << output_json;
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to get infos: ";
        long code;
        if (easy.get_html_code(code) == CURLE_OK && code == 403)
        {
            std::cerr << "Error in webapi key";
        }
        else
        {
            std::cerr << "Error while parsing json. Try to go at " << url << " and see what you can do to build your achivements.json";
        }
        std::cerr << std::endl;
    }
}

template<class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using fifo_json = nlohmann::basic_json<my_workaround_fifo_map>;


static void generate_items(CurlEasy& easy)
{
    std::string url = "https://api.steampowered.com/IInventoryService/GetItemDefMeta/v1?key=";
    url += steam_apikey;
    url += "&appid=";
    url += app_id;

    easy.set_url(url);
    easy.perform();

    try
    {
        nlohmann::json json = nlohmann::json::parse(easy.get_answer());
        std::string digest = json["response"]["digest"];

        url = "https://api.steampowered.com/IGameInventory/GetItemDefArchive/v0001?appid=";
        url += app_id;
        url += "&digest=";
        url += digest;

        easy.set_url(url);
        easy.perform();

        fifo_json item_json;
        fifo_json default_item_json;

        json = nlohmann::json::parse(easy.get_answer());
        std::ofstream items_file(output_path + "/items.json", std::ios::trunc | std::ios::out);
        std::ofstream default_items_file(output_path + "/default_items.json", std::ios::trunc | std::ios::out);

        for (auto &i : json)
        {
            for (auto j = i.begin(); j != i.end(); ++j)
            {
                //if (j.key() == "itemdefid")
                //{
                //    j.value() = std::stoll(j.value().get<std::string>());
                //}
                //else
                {
                    nlohmann::json& v = j.value();
                    switch (v.type())
                    {
                    case nlohmann::json::value_t::boolean:
                        v = (v.get<bool>() ? "true" : "false");
                        break;

                    case nlohmann::json::value_t::number_float:
                        v = std::to_string(v.get<double>());
                        break;

                    case nlohmann::json::value_t::number_integer:
                        v = std::to_string(v.get<int64_t>());
                        break;

                    case nlohmann::json::value_t::number_unsigned:
                        v = std::to_string(v.get<uint64_t>());
                        break;
                    }
                }
            }
            item_json[i["itemdefid"].get<std::string>()] = i;
            default_item_json[i["itemdefid"].get<std::string>()] = 1;
        }

        items_file << std::setw(2) << item_json;
        default_items_file << std::setw(2) << default_item_json;
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to get infos: ";
        long code;
        if (easy.get_html_code(code) == CURLE_OK && code == 403)
        {
            std::cerr << "Error in webapi key";
        }
        else
        {
            std::cerr << "Error while parsing json. Try to go at " << url << " and see what you can do to build your items.json";
        }
        std::cerr << std::endl;
    }
}

static std::string get_appid_name(CurlEasy& easy, uint32_t appid)
{
    static std::map<uint32_t, std::string> appid_names;
    static bool done;

    if (!done) {
        std::string url = "https://api.steampowered.com/ISteamApps/GetAppList/v2/";
        std::cout << "getting app list" << std::endl;
        easy.set_url(url);
        easy.perform();
        try
        {
            nlohmann::json json = nlohmann::json::parse(easy.get_answer());
            for (auto &app : json["applist"]["apps"]) {
                appid_names[app["appid"].get<uint32_t>()] = app["name"].get<std::string>();
            }

            done = true;
        }
        catch (std::exception& e)
        {
            std::cerr << "Failed to get infos: ";
            long code;
            if (easy.get_html_code(code) == CURLE_OK && code == 403)
            {
                std::cerr << "Error 403 while getting app list";
            }
            else
            {
                std::cerr << "Error while parsing json. With " << url << "";
            }
            std::cerr << std::endl;
        }
    }

    if (done) {
        if (!appid_names.count(appid)) {
            std::cout << "getting app name: " << appid << std::endl;
            std::string s_appid = std::to_string(appid);
            std::string url = "https://store.steampowered.com/api/appdetails/?appids=" + s_appid;
            easy.set_url(url);
            easy.perform();
            nlohmann::json json = nlohmann::json::parse(easy.get_answer());
            appid_names[appid] = json[s_appid]["data"]["name"].get<std::string>();
        }

        return appid_names[appid];
    }

    return "";
}

static void generate_dlcs(CurlEasy& easy)
{
    std::string base_url = "https://store.steampowered.com/api/appdetails/?appids=";
    std::string url = base_url + app_id;
    easy.set_url(url);
    easy.perform();

    try
    {
        nlohmann::json json = nlohmann::json::parse(easy.get_answer());
        std::list<uint32_t> dlcs;
        std::map<uint32_t, std::string> dlc_names;

        for (auto& dlc : json[app_id]["data"]["dlc"])
        {
            dlcs.push_back(dlc.get<uint32_t>());
        }

        std::ofstream dlc_file(output_path + "/DLC.txt", std::ios::trunc | std::ios::out);
        for (auto &dlc: dlcs) {
            dlc_file << dlc << "=" << get_appid_name(easy, dlc) << std::endl;
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Failed to get infos: ";
        long code;
        if (easy.get_html_code(code) == CURLE_OK && code == 403)
        {
            std::cerr << "Error 403 while getting dlcs";
        }
        else
        {
            std::cerr << "Error while parsing json. With " << url << "";
        }
        std::cerr << std::endl;
    }
}

int main(int argc, char **argv)
{
    CurlGlobal& cglobal = CurlGlobal::Inst();
    cglobal.init();

    CurlEasy easy;
    if (easy.init())
    {
        easy.skip_verifypeer();

        if (argc > 2) {
            app_id = argv[2];
            steam_apikey = argv[1];
        } else {
            std::cout << "Usage: " << argv[0] << " steam_api_key app_id <output_path (default is folder with app_id/steam_settings)>" << std::endl;
            std::cout << "Enter the game appid: ";
            std::cin >> app_id;
            std::cout << "Enter your webapi key: ";
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cin >> steam_apikey;
        }

        if (argc > 3) {
            output_path = argv[3];
        } else {
            output_path = app_id;
            create_directory(output_path);
            output_path += "/steam_settings";
        }

        if (!create_directory(output_path))
        {
            std::cerr << "Cannot create directory: " << output_path << std::endl;
            return -1;
        }

        if (!create_directory(output_path + "/images"))
        {
            std::cerr << "Cannot create directory \"images\"" << std::endl;
            return -1;
        }

        {
            std::ofstream appid_file(output_path + "/steam_appid.txt", std::ios::trunc | std::ios::out);
            appid_file << app_id;
        }

        std::cout << "Generating DLC.txt" << std::endl;
        generate_dlcs(easy);
        std::cout << "Generating achievements" << std::endl;
        generate_achievements(easy);
        std::cout << "Generating items" << std::endl;
        generate_items(easy);
    }
}
