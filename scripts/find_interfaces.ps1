$fi = $args[0]

if( ! $fi )
{
  $fi = "steam_api.dll"

  if( !(Test-Path $fi) )
  {
    $fi = "steam_api64.dll"
  }

  if( !(Test-Path $fi) )
  {
    Write-Output "Failed to find steam_api or steam_api64"
    Return 1
  }
}
else
{
  if( !(Test-Path $fi) )
  {
    Write-Output "Failed to find $fi"
    Return 1
  }
}

function findinterface

($api)
{
  $str = Select-String "$api[0-9][0-9][0-9]" $fi
  if( $str -match "$api[0-9]{3}" )
  {
    Write-Output "$($matches[0])"
  }
}

findinterface SteamClient                            | Out-File -Encoding ASCII -FilePath steam_interfaces.txt
findinterface SteamGameServer                        | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface SteamGameServerStats                   | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface SteamUser                              | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface SteamFriends                           | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface SteamUtils                             | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface SteamMatchMaking                       | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface SteamMatchMakingServers                | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMUSERSTATS_INTERFACE_VERSION       | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMAPPS_INTERFACE_VERSION            | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface SteamNetworking                        | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMREMOTESTORAGE_INTERFACE_VERSION   | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMSCREENSHOTS_INTERFACE_VERSION     | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMHTTP_INTERFACE_VERSION            | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMUNIFIEDMESSAGES_INTERFACE_VERSION | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMCONTROLLER_INTERFACE_VERSION      | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMUGC_INTERFACE_VERSION             | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMAPPLIST_INTERFACE_VERSION         | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMMUSIC_INTERFACE_VERSION           | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMMUSICREMOTE_INTERFACE_VERSION     | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMHTMLSURFACE_INTERFACE_VERSION_    | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface STEAMINVENTORY_INTERFACE_V             | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt
findinterface SteamController                        | Out-File -Encoding ASCII -Append -FilePath steam_interfaces.txt