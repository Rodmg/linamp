import asyncio
from enum import Enum

from dbus_next.aio import MessageBus
from dbus_next import BusType

SERVICE_NAME = 'org.bluez'
DEVICE_IFACE = SERVICE_NAME + '.Device1'
PLAYER_IFACE = SERVICE_NAME + '.MediaPlayer1'
TRANSPORT_IFACE = SERVICE_NAME + '.MediaTransport1'

# Useful docs:
# https://manpages.ubuntu.com/manpages/oracular/man5/org.bluez.MediaPlayer.5.html
# https://manpages.ubuntu.com/manpages/oracular/man5/org.bluez.MediaTransport.5.html

# Possible values for player status:
# - playing
# - stopped
# - paused
# - forward-seek
# - reverse-seek
# - error

# Possible values for transport state:
# - idle
# - pending
# - active

# Possible values for repeat:
# - off
# - singletrack
# - alltracks
# - group

# Possible values for shuffle:
# - off
# - alltracks
# - group

# Please note: these are unconfirmed
class BTCodec(Enum):
    SBC = 0
    MP3 = 1
    AAC = 2
    APTX = 3
    APTXHD = 4

class BTTrackInfo():
    def __init__(self, title: str = '', track_number: int = 0, number_of_tracks: int = 0, duration: int = 0, album: str = '', artist: str = ''):
        self.title = title
        self.track_number = track_number
        self.number_of_tracks = number_of_tracks
        self.duration = duration
        self.album = album
        self.artist = artist

        self.loop = asyncio.new_event_loop()
        asyncio.set_event_loop(self.loop)

    def __str__(self):
        repr = '\n'
        repr = repr + f'  Title: {self.title}\n'
        repr = repr + f'  Album: {self.album}\n'
        repr = repr + f'  Artist: {self.artist}\n'

        seconds_total = self.duration/1000
        minutes = int(seconds_total/60)
        seconds = int(seconds_total - (minutes * 60))

        repr = repr + f'  Duration: {str(minutes).zfill(2)}:{str(seconds).zfill(2)} ({self.duration})\n'
        repr = repr + f'  Track Number: {self.track_number}\n'
        repr = repr + f'  Number of Tracks: {self.number_of_tracks}\n'

        return repr

def wait_for_loop() -> None:
    """Antipattern: waits the asyncio running loop to end"""
    asyncio.set_event_loop(self.loop)
    while self.loop.is_running():
        pass

def is_empty_player_track(track: BTTrackInfo) -> bool:
    return track.duration <= 0


class BTPlayerAdapter():
    bus = None
    manager = None

    device = None
    device_alias = None
    player = None
    player_interface = None
    transport = None

    connected = False
    transport_state = None
    codec = None
    codec_configuration = None
    status = None
    track = BTTrackInfo()
    position = 0
    repeat = 'off'
    shuffle = 'off'

    def __init__(self):
        pass

    async def _get_dbus_object(self, path):
        introspection = await self.bus.introspect(SERVICE_NAME, path)
        obj = self.bus.get_proxy_object(SERVICE_NAME, path, introspection)
        return obj

    async def _get_manager(self):
        obj = await self._get_dbus_object('/')
        manager = obj.get_interface('org.freedesktop.DBus.ObjectManager')
        return manager

    async def _get_player(self, path):
        return await self._get_dbus_object(path)

    async def _get_device(self, path):
        return await self._get_dbus_object(path)

    async def _get_transport(self, path):
        return await self._get_dbus_object(path)

    def _parse_track_info(self, track_raw):
        title = track_raw['Title'].value if 'Title' in track_raw else 'Unknown'
        track_number = track_raw['TrackNumber'].value if 'TrackNumber' in track_raw else 0
        number_of_tracks = track_raw['NumberOfTracks'].value if 'NumberOfTracks' in track_raw else 0
        duration = track_raw['Duration'].value if 'Duration' in track_raw else 0
        album = track_raw['Album'].value if 'Album' in track_raw else 'Unknown'
        artist = track_raw['Artist'].value if 'Artist' in track_raw else 'Unknown'
        track_info = BTTrackInfo(title, track_number, number_of_tracks, duration, album, artist)
        return track_info

    async def setup(self):
        """Initialize DBus"""
        self.bus = await MessageBus(bus_type=BusType.SYSTEM).connect()
        self.manager = await self._get_manager()

    def print_state(self):
        print(f'Connected: {self.connected}')
        print(f'State: {self.transport_state}')
        print(f'Codec: {self.codec}')
        print(f'Configuration: {self.codec_configuration}')
        print(f'Status: {self.status}')
        print(f'Device Alias: {self.device_alias}')

        seconds_total = self.position/1000
        minutes = int(seconds_total/60)
        seconds = int(seconds_total - (minutes * 60))

        print(f'Position: {str(minutes).zfill(2)}:{str(seconds).zfill(2)} ({self.position})')
        print(f'Repeat: {self.repeat}')
        print(f'Shuffle: {self.shuffle}')
        print(f'Track: {self.track}')

    async def find_player(self):
        """Identify current player and device"""
        objects = await self.manager.call_get_managed_objects()

        player_path = None
        transport_path = None
        for path in objects:
            interfaces = objects[path]
            if PLAYER_IFACE in interfaces:
                player_path = path
            if TRANSPORT_IFACE in interfaces:
                transport_path = path

        if player_path:
            #print(f'Found player: {player_path}')
            # Setup connected state
            self.connected = True
            self.player = await self._get_player(player_path)
            self.player_interface = self.player.get_interface(PLAYER_IFACE)
            player_properties = self.player.get_interface('org.freedesktop.DBus.Properties')
            device_path_raw = await player_properties.call_get(PLAYER_IFACE, 'Device')
            device_path = device_path_raw.value
            self.device = await self._get_device(device_path)
            device_properties = self.device.get_interface('org.freedesktop.DBus.Properties')
            device_alias_raw = await device_properties.call_get(DEVICE_IFACE, 'Alias')
            self.device_alias = device_alias_raw.value

            all_player_properties = await player_properties.call_get_all(PLAYER_IFACE)
            if 'Status' in all_player_properties:
                self.status = all_player_properties['Status'].value
            if 'Track' in all_player_properties:
                track_raw = all_player_properties['Track'].value
                self.track = self._parse_track_info(track_raw)
            if 'Position' in all_player_properties:
                self.position = all_player_properties['Position'].value
            if 'Repeat' in all_player_properties:
                self.repeat = all_player_properties['Repeat'].value
            if 'Shuffle' in all_player_properties:
                self.shuffle = all_player_properties['Shuffle'].value

        else:
            self.connected = False
            self.player = None
            self.player_interface = None
            self.device = None
            self.device_alias = None
            self.status = None
            self.track = BTTrackInfo()
            self.position = 0
            self.repeat = 'off'
            self.shuffle = 'off'
            #print('No Player found')

        if transport_path:
            #print(f'Found transport: {transport_path}')
            self.transport = await self._get_transport(transport_path)
            transport_properties = self.transport.get_interface('org.freedesktop.DBus.Properties')
            all_transport_properties = await transport_properties.call_get_all(TRANSPORT_IFACE)
            if 'State' in all_transport_properties:
                self.transport_state = all_transport_properties['State'].value
            if 'Codec' in all_transport_properties:
                self.codec = BTCodec(all_transport_properties['Codec'].value)
            if 'Configuration' in all_transport_properties:
                self.codec_configuration = all_transport_properties['Configuration'].value
        else:
            self.transport = None
            self.transport_state = None
            self.codec = None
            self.codec_configuration = None

    def setup_sync(self):
        wait_for_loop()
        self.loop.run_until_complete(self.setup())

    def find_player_sync(self):
        wait_for_loop()
        self.loop.run_until_complete(self.find_player())

    def play(self):
        if not self.player_interface:
            return
        wait_for_loop()
        self.loop.run_until_complete(self.player_interface.call_play())

    def pause(self):
        if not self.player_interface:
            return
        wait_for_loop()
        self.loop.run_until_complete(self.player_interface.call_pause())

    def stop(self):
        if not self.player_interface:
            return
        wait_for_loop()
        self.loop.run_until_complete(self.player_interface.call_stop())

    def next(self):
        if not self.player_interface:
            return
        wait_for_loop()
        self.loop.run_until_complete(self.player_interface.call_next())

    def previous(self):
        if not self.player_interface:
            return
        wait_for_loop()
        self.loop.run_until_complete(self.player_interface.call_previous())

    def set_shuffle(self, enabled: bool) -> None:
        if not self.player_interface:
            return
        wait_for_loop()
        self.shuffle = 'alltracks' if enabled else 'off'
        self.loop.run_until_complete(self.player_interface.set_shuffle(self.shuffle))

    def set_repeat(self, enabled: bool) -> None:
        if not self.player_interface:
            return
        wait_for_loop()
        self.repeat = 'alltracks' if enabled else 'off'
        self.loop.run_until_complete(self.player_interface.set_repeat(self.repeat))

    def get_codec_str(self) -> str:
        if not self.codec:
            return ''
        return self.codec.name
