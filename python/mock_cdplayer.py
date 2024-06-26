import vlc
import time


class CDPlayer:

    def __init__(self, device="/dev/cdrom") -> None:
        self._device = device
        self.vlc_instance = vlc.Instance()
        self.player = None
        self.media_list = None
        self.list_player = None
        self.disc_loaded = False
        # Array of tuples with format (tracknumber: int, artist, album, title, duration: int, is_data_track: bool)
        self.track_info = []
        self.shuffle = False
        self.repeat = False

        self._mock_status = "no-disc"
        self._mock_current_track_idx = 0

    def _set_track_info(self, artists, track_titles, album, durations, is_data_tracks):
        tracks = []
        for i in range(0, len(track_titles)):
            track = (
                i + 1,
                artists[i],
                album,
                track_titles[i],
                durations[i],
                is_data_tracks[i],
            )
            tracks.append(track)
        self.track_info = tracks

    def _do_set_repeat(self):
        if self.list_player is not None:
            if self.repeat:
                self.list_player.set_playback_mode(vlc.PlaybackMode.loop)
            else:
                self.list_player.set_playback_mode(vlc.PlaybackMode.default)

    def _do_set_shuffle(self):
        pass

    # -------- Control Functions --------

    def load(self):

        # simulate slow data fetch
        time.sleep(5)

        artists = ["Test Artist"] * 4
        track_titles = ["Track 1", "Track 2", "Track 3", "Track 4"]
        album = "Test Album"
        durations = [180000, 182000, 185000, 184000]
        is_data_tracks = [False] * 4

        self._set_track_info(artists, track_titles, album, durations, is_data_tracks)

        self.disc_loaded = True
        self._mock_status = "stopped"

    def unload(self):
        self.player = None
        self.media_list = None
        self.list_player = None
        self.disc_loaded = False
        self.track_info = []

    def play(self):
        self._mock_status = "playing"

    def stop(self):
        self._mock_status = "stopped"

    def pause(self):
        self._mock_status = "paused"

    def next(self):
        if self._mock_current_track_idx < len(self.track_info) - 1:
            self._mock_current_track_idx = self._mock_current_track_idx + 1

    def prev(self):
        if self._mock_current_track_idx > 0:
            self._mock_current_track_idx = self._mock_current_track_idx - 1

    # Jump to a specific track
    def jump(self, index):
        self._mock_current_track_idx = index

    # Go to a specific time in a track while playing
    def seek(self, ms):
        if self.player is None:
            return

        track_duration = self.player.get_media().get_duration()
        percentage = ms / track_duration
        self.player.set_position(percentage)

    def set_shuffle(self, enabled):
        # self.shuffle = enabled
        print("WARNING: Shuffle not supported by vlc backend")
        self._do_set_shuffle()

    def set_repeat(self, enabled):
        self.repeat = enabled
        self._do_set_repeat()

    def eject(self):
        if self.disc_loaded:
            self.stop()
            self._mock_status = "no-disc"
            self.unload()
        else:
            self.load()
            self._mock_status = "stopped"

    # -------- Status Functions --------

    def get_postition(self):
        if self.player is None:
            return 0
        time = self.player.get_time()
        if time < 0:
            time = 0
        return time

    def get_shuffle(self):
        return self.shuffle

    def get_repeat(self):
        return self.repeat

    def get_status(self):
        return self._mock_status

    def get_all_tracks_info(self):
        # Filter data tracks
        tracks = []
        for track in self.track_info:
            if track[5] == False:
                tracks.append(track)
        return tracks

    def get_track_info(self, index):
        if index >= len(self.track_info) or index < 0:
            raise Exception("Invalid track number")
        return self.track_info[index]

    def get_current_track_info(self):
        if not self.disc_loaded:
            raise Exception("Not playing")
        index = self._mock_current_track_idx
        if index < 0:
            index = 0
        return self.get_track_info(index)

    # -------- Events to be called by a timer --------

    def detect_disc_insertion(self):
        if not self.disc_loaded:
            self.load()
            return True

        return False
