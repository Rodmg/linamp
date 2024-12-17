import asyncio
from enum import Enum

from linamp.btplayer.btadapter import BTPlayerAdapter

loop = asyncio.get_event_loop()

class PlayerStatus(Enum):
    Idle = 'idle'
    Playing = 'playing'
    Stopped = 'stopped'
    Paused = 'paused'
    Error = 'error'
    Loading = 'loading'

class BTPlayer:

    message: str
    show_message: bool
    message_timeout: int

    player: BTPlayerAdapter
    track_info: tuple[int, str, str, str, int]

    def __init__(self) -> None:
        self.player = BTPlayerAdapter()
        # tuple with format (tracknumber: int, artist, album, title, duration: int)
        self.track_info = ()

        self.message = ''
        self.show_message = False
        self.message_timeout = 0

        loop.run_until_complete(self.player.setup())

    # -------- Control Functions --------

    def load(self) -> None:
        if loop.is_running():
            print('WARNING: loop is running and tried to run again on load')
            return
        loop.run_until_complete(self.player.find_player())
        if self.player.connected:
            track = self.player.track
            if not track:
                return
            self.track_info = (
                track.track_number,
                track.artist,
                track.album,
                track.title,
                track.duration
            )

    def unload(self) -> None:
        self.track_info = ()
        self.message = ''
        self.show_message = False
        self.message_timeout = 0

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
        pass

    def set_shuffle(self, enabled: bool) -> None:
        self.player.set_shuffle(enabled)

    def set_repeat(self, enabled: bool) -> None:
        self.player.set_repeat(enabled)

    def eject(self) -> None:
        pass

    # -------- Status Functions --------

    def get_postition(self) -> int:
        return self.player.position

    def get_shuffle(self) -> bool:
        return self.player.shuffle != "off"

    def get_repeat(self) -> bool:
        return self.player.repeat != "off"

    # Returns the str representation of PlayerStatus enum
    def get_status(self) -> str:
        status = PlayerStatus.Idle
        btstatus = self.player.status
        if btstatus == "playing":
            status = PlayerStatus.Playing
        if btstatus == "stopped":
            status = PlayerStatus.Stopped
        if btstatus == "paused":
            status = PlayerStatus.Paused
        if btstatus == "error":
            status = PlayerStatus.Error
        if btstatus == "forward-seek":
            status = PlayerStatus.Loading
        if btstatus == "reverse-seek":
            status = PlayerStatus.Loading
        return status.value

    def get_track_info(self) -> tuple[int, str, str, str, int]:
        return self.track_info

    # -------- Events to be called by a timer --------

    def poll_events(self) -> bool:
        self.load()
        return self.player.connected
