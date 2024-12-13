import asyncio

from linamp.btplayer.btadapter import BTPlayerAdapter

loop = asyncio.get_event_loop()

class BTPlayer:

    def __init__(self) -> None:
        self.player = BTPlayerAdapter()
        # Array of tuples with format (tracknumber: int, artist, album, title, duration: int, is_data_track: bool)
        self.track_info = []
        self.shuffle = False
        self.repeat = False

        loop.run_until_complete(self.player.setup())

    def _do_set_repeat(self):
        pass

    def _do_set_shuffle(self):
        pass

    # -------- Control Functions --------

    def load(self):
        loop.run_until_complete(self.player.find_player())
        if self.player.connected:
            self.shuffle = self.player.shuffle
            self.repeat = self.player.repeat
            track = self.player.track
            if not track:
                return
            self.track_info = [{
                "tracknumber": track.track_number,
                "artist": track.artist,
                "album": track.album,
                "title": track.title,
                "duration": track.duration
            }]



    def unload(self):
        self.track_info = []

    def play(self):
        pass

    def stop(self):
        pass

    def pause(self):
        pass

    def next(self):
        pass

    def prev(self):
        pass

    # Jump to a specific track
    def jump(self, index):
        pass

    # Go to a specific time in a track while playing
    def seek(self, ms):
        pass

    def set_shuffle(self, enabled):
        self.shuffle = enabled
        self._do_set_shuffle()

    def set_repeat(self, enabled):
        self.repeat = enabled
        self._do_set_repeat()

    def eject(self):
        pass

    # -------- Status Functions --------

    def get_postition(self):
        return 0

    def get_shuffle(self):
        return self.shuffle

    def get_repeat(self):
        return self.repeat

    def get_status(self):
        status = "no-disc"
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
