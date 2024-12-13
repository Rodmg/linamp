import asyncio

from linamp.btplayer.btadapter import BTPlayerAdapter

loop = asyncio.get_event_loop()

class BTPlayer:

    def __init__(self) -> None:
        self.player = BTPlayerAdapter()
        # Array of tuples with format (tracknumber: int, artist, album, title, duration: int, is_data_track: bool)
        self.track_info = []

        loop.run_until_complete(self.player.setup())

    def _do_set_repeat(self):
        pass

    def _do_set_shuffle(self):
        pass

    # -------- Control Functions --------

    def load(self):
        loop.run_until_complete(self.player.find_player())
        if self.player.connected:
            track = self.player.track
            if not track:
                return
            self.track_info = [(
                track.track_number,
                track.artist,
                track.album,
                track.title,
                track.duration
            )]

    def unload(self):
        self.track_info = []

    def play(self):
        self.player.play()

    def stop(self):
        self.player.stop()

    def pause(self):
        self.player.pause()

    def next(self):
        self.player.next()

    def prev(self):
        self.player.previous()

    # Jump to a specific track
    def jump(self, index):
        pass

    # Go to a specific time in a track while playing
    def seek(self, ms):
        pass

    def set_shuffle(self, enabled):
        self._do_set_shuffle()

    def set_repeat(self, enabled):
        self._do_set_repeat()

    def eject(self):
        pass

    # -------- Status Functions --------

    def get_postition(self):
        return self.player.position

    def get_shuffle(self):
        return self.player.shuffle != "off"

    def get_repeat(self):
        return self.player.repeat != "off"

    def get_status(self):
        status = "disconnected"
        btstatus = self.player.status
        if btstatus == "playing":
            status = "playing"
        if btstatus == "stopped":
            status = "stopped"
        if btstatus == "paused":
            status = "paused"
        if btstatus == "error":
            status = "error"
        if btstatus == "forward-seek":
            status = "loading"
        if btstatus == "reverse-seek":
            status = "loading"
        return status

    def get_all_tracks_info(self):
        return self.track_info

    def get_track_info(self, index):
        if index >= len(self.track_info) or index < 0:
            raise Exception("Invalid track number")
        return self.track_info[index]

    def get_current_track_info(self):
        index = 0
        if index < 0:
            index = 0
        return self.get_track_info(index)

    # -------- Events to be called by a timer --------

    def poll_changes(self):
        self.load()
        return self.player.connected
