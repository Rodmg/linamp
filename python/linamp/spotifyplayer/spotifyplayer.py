from linamp.baseplayer import BasePlayer, PlayerStatus
from linamp.spotifyplayer.spotifyadapter import SpotifyPlayerAdapter, is_empty_player_track

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

class SpotifyPlayer(BasePlayer):

    message: str
    show_message: bool
    message_timeout: int

    player: SpotifyPlayerAdapter
    track_info: tuple[int, str, str, str, int, str, int, int]

    def __init__(self) -> None:
        self.player = SpotifyPlayerAdapter()
        # tuple with format (tracknumber: int, artist, album, title, duration: int, codec: str, bitrate_bps: int, samplerate_hz: int)
        self.track_info = EMPTY_TRACK_INFO

        self.clear_message()

        # self.player.setup_sync() TODO: should run this on a specific thread as it won't end (asyncio)

    def _display_connection_info(self):
        if self.player.connected:
            self.message = 'CONNNECTED'
            self.show_message = True
            self.message_timeout = 5000
        else:
            self.message = 'DISCONNECTED'
            self.show_message = True
            self.message_timeout = 5000

    def _not_supported(self):
        self.message = 'NOT SUPPORTED'
        self.show_message = True
        self.message_timeout = 3000

    # -------- Control Functions --------

    def load(self) -> None:
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
                '',
                320,
                44100
            )
        else:
            self.track_info = EMPTY_TRACK_INFO
            self._display_connection_info()

    def unload(self) -> None:
        self.track_info = EMPTY_TRACK_INFO
        self.clear_message()

    def play(self) -> None:
        self._not_supported()

    def stop(self) -> None:
        self._not_supported()

    def pause(self) -> None:
        self._not_supported()

    def next(self) -> None:
        self._not_supported()

    def prev(self) -> None:
        self._not_supported()

    # Go to a specific time in a track while playing
    def seek(self, ms: int) -> None:
        self._not_supported()

    def set_shuffle(self, enabled: bool) -> None:
        self._not_supported()

    def set_repeat(self, enabled: bool) -> None:
        self._not_supported()

    def eject(self) -> None:
        self._not_supported()

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
        spotstatus = self.player.status
        if spotstatus == 'playing':
            status = PlayerStatus.Playing
        if spotstatus == 'stopped':
            status = PlayerStatus.Stopped
        if spotstatus == 'paused':
            status = PlayerStatus.Paused
        if spotstatus == 'error':
            status = PlayerStatus.Error
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
