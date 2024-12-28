#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

if [[ $PLAYER_EVENT == 'session_connected' || $PLAYER_EVENT == 'session_disconnected' ]]; then
  dbus-send --system /org/freedesktop/librespot/event org.freedesktop.Librespot.Event dict:string:string:'event',"$PLAYER_EVENT",'user_name',"$USER_NAME",'connection_id',"$CONNECTION_ID"
fi

if [[ $PLAYER_EVENT == 'shuffle_changed' ]]; then
  dbus-send --system /org/freedesktop/librespot/event org.freedesktop.Librespot.Event dict:string:string:'event',"$PLAYER_EVENT",'shuffle',"$SHUFFLE"
fi

if [[ $PLAYER_EVENT == 'repeat_changed' ]]; then
  dbus-send --system /org/freedesktop/librespot/event org.freedesktop.Librespot.Event dict:string:string:'event',"$PLAYER_EVENT",'repeat',"$REPEAT"
fi

if [[ $PLAYER_EVENT == 'track_changed' ]]; then
  dbus-send --system /org/freedesktop/librespot/event org.freedesktop.Librespot.Event dict:string:string:'event',"$PLAYER_EVENT",\
'item_type',"${ITEM_TYPE:-''}",\
'track_id',"${TRACK_ID:-''}",\
'uri',"${URI:-''}",\
'name',"${NAME:-''}",\
'duration_ms',"${DURATION_MS:-''}",\
'is_explicit',"${IS_EXPLICIT:-''}",\
'language',"${LANGUAGE:-''}",\
'covers',"${COVERS:-''}",\
'number',"${NUMBER:-''}",\
'disc_number',"${DISC_NUMBER:-''}",\
'popularity',"${POPULARITY:-''}",\
'album',"${ALBUM:-''}",\
'artists',"${ARTISTS:-''}",\
'album_artists',"${ALBUM_ARTISTS:-''}",\
'show_name',"${SHOW_NAME:-''}",\
'publish_time',"${PUBLISH_TIME:-''}",\
'description',"${DESCRIPTION:-''}"
fi

if [[ $PLAYER_EVENT == 'playing' || $PLAYER_EVENT == 'paused' || $PLAYER_EVENT == 'seeked' || $PLAYER_EVENT == 'position_correction' ]]; then
  dbus-send --system /org/freedesktop/librespot/event org.freedesktop.Librespot.Event dict:string:string:'event',"$PLAYER_EVENT",'track_id',"$TRACK_ID",'position_ms',"$POSITION_MS"
fi