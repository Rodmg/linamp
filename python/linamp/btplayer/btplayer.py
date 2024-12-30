from linamp.baseplayer import BasePlayer, PlayerStatus
from linamp.btplayer.btadapter import BTPlayerAdapter, is_empty_player_track

EMPTY_TRACK_INFO = (
    0,
    '',
    '',
    '',
    0,
    '',
    0,
    44100
)

class BTPlayer(BasePlayer):

    message: str
    show_message: bool
    message_timeout: int

    player: BTPlayerAdapter
    track_info: tuple[int, str, str, str, int, str, int, int]

    def __init__(self) -> None:
        self.player = BTPlayerAdapter()
        # tuple with format (tracknumber: int, artist, album, title, duration: int, codec: str, bitrate_bps: int, samplerate_hz: int)
        self.track_info = EMPTY_TRACK_INFO

        self.clear_message()

        self.player.setup_sync()

    def _display_connection_info(self):
        if self.player.connected:
            self.message = f'CONNNECTED TO: {self.player.device_alias}'
            self.show_message = True
            self.message_timeout = 5000
        else:
            self.message = 'DISCONNECTED'
            self.show_message = True
            self.message_timeout = 5000

    # -------- Control Functions --------

    def load(self) -> None:
        self.player.find_player_sync()
        if self.player.connected:
            track = self.player.track
            if not track or is_empty_player_track(track):
                self._display_connection_info()
                return
            self.track_info = (
                track.track_number,
                track.artist,
                track.album,
                track.title,
                track.duration,
                self.player.get_codec_str(),
                0, # No simple way to know bitrate from BT
                44100
            )
        else:
            self.track_info = EMPTY_TRACK_INFO
            self._display_connection_info()

    def unload(self) -> None:
        self.track_info = EMPTY_TRACK_INFO
        self.clear_message()

    def play(self) -> None:
        self.player.play()

    def stop(self) -> None:
        self.player.stop()

    def pause(self) -> None:
        self.player.pause()

    def next(self) -> None:
        self.player.next()

    def prev(self) -> None:
        self.player.previous()

    # Go to a specific time in a track while playing
    def seek(self, ms: int) -> None:
        self.message = 'NOT SUPPORTED'
        self.show_message = True
        self.message_timeout = 3000

    def set_shuffle(self, enabled: bool) -> None:
        self.player.set_shuffle(enabled)

    def set_repeat(self, enabled: bool) -> None:
        self.player.set_repeat(enabled)

    def eject(self) -> None:
        self.message = 'NOT SUPPORTED'
        self.show_message = True
        self.message_timeout = 3000

    # -------- Status Functions --------

    def get_postition(self) -> int:
        return self.player.position

    def get_shuffle(self) -> bool:
        return self.player.shuffle != 'off'

    def get_repeat(self) -> bool:
        return self.player.repeat != 'off'

    # Returns the str representation of PlayerStatus enum
    def get_status(self) -> str:
        status = PlayerStatus.Idle
        btstatus = self.player.status
        if btstatus == 'playing':
            status = PlayerStatus.Playing
        if btstatus == 'stopped':
            status = PlayerStatus.Stopped
        if btstatus == 'paused':
            status = PlayerStatus.Paused
        if btstatus == 'error':
            status = PlayerStatus.Error
        if btstatus == 'forward-seek':
            status = PlayerStatus.Loading
        if btstatus == 'reverse-seek':
            status = PlayerStatus.Loading
        return status.value

    def get_track_info(self) -> tuple[int, str, str, str, int, str, int, int]:
        return self.track_info

    # Return any message you want to show to the user. tuple with format: (show_message: bool, message: str, message_timeout_ms: int)
    def get_message(self) -> tuple[bool, str, int]:
        return (self.show_message, self.message, self.message_timeout)

    def clear_message(self) -> None:
        self.show_message = False
        self.message = ''
        self.message_timeout = 0

    # -------- Events to be called by a timer --------

    def poll_events(self) -> bool:
        was_connected = self.player.connected
        self.load()

        # Should tell UI to refresh if we are connected and were not connected before
        return self.player.connected and not was_connected
