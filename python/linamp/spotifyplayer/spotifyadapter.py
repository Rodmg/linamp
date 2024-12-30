import asyncio
import time

from dbus_next.aio import MessageBus
from dbus_next.service import ServiceInterface, method

DBUS_INTERFACE = 'org.linamp.LibrespotInterface'

class SpotifyTrackInfo():
    def __init__(self, title: str = '', track_number: int = 0, number_of_tracks: int = 0, duration: int = 0, album: str = '', artist: str = ''):
        self.title = title
        self.track_number = track_number
        self.number_of_tracks = number_of_tracks
        self.duration = duration
        self.album = album
        self.artist = artist

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

def is_empty_player_track(track: SpotifyTrackInfo) -> bool:
    return track.duration <= 0


class SpotifyPlayerAdapter(ServiceInterface):
    bus = None

    connected = False
    status = 'stopped'
    track = SpotifyTrackInfo()
    position = 0
    last_updated_position = None # time
    repeat = 'off'
    shuffle = 'off'

    def __init__(self):
        super().__init__(DBUS_INTERFACE)

    @method()
    def send_event(self, data: 'a{ss}'):
        event = data.get('event')
        if event == 'session_connected':
            self.connected = True
        if event == 'session_disconnected':
            self.connected = False
            self.status = 'stopped'
            self.track = SpotifyTrackInfo()
            self._set_position(0)
            self.repeat = 'off'
            self.shuffle = 'off'
        if event == 'shuffle_changed':
            self.connected = True
            shuffle_str = data.get('shuffle', '')
            self.shuffle = 'on' if shuffle_str == 'true' else 'off'
        if event == 'repeat_changed':
            self.connected = True
            repeat_str = data.get('repeat', '')
            self.repeat = 'on' if repeat_str == 'true' else 'off'
        if event == 'playing' or event == 'paused':
            self.connected = True
            self.status = event
            self._set_position(int(data.get('position_ms', '0')))
        if event == 'seeked' or event == 'position_correction':
            self.connected = True
            self._set_position(int(data.get('position_ms', '0')))
        if event == 'track_changed':
            self.connected = True
            item_type = data.get('item_type')

            title = data.get('name', '')
            number_of_tracks = 0
            duration = int(data.get('duration_ms', '0'))

            track_number = int(data.get('number', '0'))
            album = data.get('album', '')
            artist = data.get('artists', '')

            if item_type == 'Episode':
                # Special case for podcasts
                track_number = 0
                album = ''
                artist = data.get('show_name', '')

            self.track = SpotifyTrackInfo(title, track_number, number_of_tracks, duration, album, artist)

        
        # TODO remove
        self.print_state()


    def _set_position(self, pos: int):
        self.position = pos
        self.last_updated_position = time.time_ns()

    def get_postition(self) -> int:
        """Updates position if it hasn't been updated by an event"""
        now = time.time_ns()
        then = self.last_updated_position()
        diff_ms = (now - then)/1000000
        self._set_position(int(self.position + diff_ms))
        return self.position

    async def setup(self):
        """Initialize DBus"""
        self.bus = await MessageBus().connect()
        self.bus.export('/org/linamp/librespot', self)
        
    async def loop(self):
        await self.setup()
        await self.bus.request_name('org.linamp.Librespot')
        await self.bus.wait_for_disconnect()

    def print_state(self):
        print(f'Connected: {self.connected}')
        print(f'Status: {self.status}')

        seconds_total = self.position/1000
        minutes = int(seconds_total/60)
        seconds = int(seconds_total - (minutes * 60))

        print(f'Position: {str(minutes).zfill(2)}:{str(seconds).zfill(2)} ({self.position})')
        print(f'Repeat: {self.repeat}')
        print(f'Shuffle: {self.shuffle}')
        print(f'Track: {self.track}')

    # Runs the asyncio event loop, should be called from a new thread
    def run_loop(self):
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        loop.run_until_complete(self.loop())
