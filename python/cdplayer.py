# With code from https://github.com/trybula/RpiPythonCDPlayer

import vlc
import cdio, pycdio
import discid
import musicbrainzngs


def search_exact_tracklist(data):  # function to find exalctly which cd was inserted
    for x in range(0, len(data["disc"]["release-list"])):
        for y in range(0, len(data["disc"]["release-list"][x]["medium-list"])):
            for z in range(
                0, len(data["disc"]["release-list"][x]["medium-list"][y]["disc-list"])
            ):
                if (
                    data["disc"]["release-list"][x]["medium-list"][y]["disc-list"][z][
                        "id"
                    ]
                    == data["disc"]["id"]
                ):
                    return x, y


def fetchdata():
    try:
        d = cdio.Device(driver_id=pycdio.DRIVER_UNKNOWN)
        i_tracks = d.get_num_tracks()
        artists = [None] * i_tracks
        track_list = [None] * i_tracks
        album = "Unknown"
        durations = [None] * i_tracks  # list of int
        is_data_tracks = [False] * i_tracks
    except IOError as e:
        print("Problem finding a CD-ROM")
        raise e

    i_first_track = pycdio.get_first_track_num(d.cd)

    # Detect any data track
    for t in range(i_first_track, i_tracks + i_first_track):
        track = d.get_track(t)
        if track.get_format() == "data":
            is_data_tracks[t - i_first_track] = True

    musicbrainzngs.set_useragent("Small_diy_cd_player", "0.1")
    disc = discid.read()  # id read
    try:
        result = musicbrainzngs.get_releases_by_discid(
            disc.id, includes=["artists", "recordings"]
        )  # get data from Musicbrainz
    except musicbrainzngs.ResponseError:
        print(
            "disc not found or bad response, using cdtxt instead"
        )  # if not available search for cdtext
        cdt = d.get_cdtext()

        for t in range(i_first_track, i_tracks + i_first_track):
            # Get track duration
            track = d.get_track(t)
            # msf: time in format minutes:seconds:frames, example '03:33:45'
            sector_len = track.get_track_sec_count()
            # On CD, one second equals 75 frames
            duration_ms = (sector_len * 1000) / 75
            durations[t - i_first_track] = int(duration_ms)

            for i in range(pycdio.MIN_CDTEXT_FIELD, pycdio.MAX_CDTEXT_FIELDS):
                # print(pycdio.cdtext_field2str(i)) ##0-TITLE 1-PERFORMER 2-SONGWRITER 3-COMPOSER 4-MESSAGE 5-ARRANGER 6-ISRC 7-UPC_EAN  8-GENERE 9-DISC_ID
                value = cdt.get(i, t)
                if value is not None:
                    if i == 0:
                        track_list[t - i_first_track] = value
                        pass
                    elif i == 1:
                        artists[t - i_first_track] = value
                        pass
                    pass
                pass
            if track_list[t - i_first_track] == None:
                track_list[t - i_first_track] = "Track " + str(t)
                artists[t - i_first_track] = "Unknown"
    else:  # Artist and album info
        a, b = search_exact_tracklist(result)
        if result.get("disc"):
            artists = [
                result["disc"]["release-list"][a]["artist-credit-phrase"]
            ] * i_tracks
            album = result["disc"]["release-list"][a]["title"]
        elif result.get("cdstub"):
            artists = [result["cdstub"]["artist"]] * i_tracks
            album = result["cdstub"]["title"]
        for t in range(0, i_tracks):
            try:
                track_list[t] = result["disc"]["release-list"][a]["medium-list"][b][
                    "track-list"
                ][t]["recording"]["title"]
            except Exception as e:
                # This happens with multi-session CDs, one way to avoid this would be to check if
                # d.get_track(t).get_format() == 'data' and remove it from the list
                print(f"WARNING: got exception while parsing track name: {e}")

                if is_data_tracks[t]:
                    track_list[t] = "Data Track"
                else:
                    track_list[t] = f"Track {t + 1}"

            try:
                durations[t] = int(
                    result["disc"]["release-list"][a]["medium-list"][b]["track-list"][
                        t
                    ]["recording"]["length"]
                )
            except Exception as e:
                print(f"WARNING: got exception while parsing track duration: {e}")
                durations[t] = 0

        if artists[0] == "Various Artists":
            cdt = d.get_cdtext()
            for t in range(i_first_track, i_tracks + i_first_track):
                value = cdt.get(1, t)
                if value is not None:
                    artists[t - i_first_track] = value
                pass
    return artists, track_list, album, i_tracks, durations, is_data_tracks


class CDPlayer:

    def __init__(self, device="/dev/cdrom") -> None:
        self._device = device
        self._detect_disc_insertion_is_first_call = True
        self.vlc_instance = vlc.Instance()
        self.player = None
        self.media_list = None
        self.list_player = None
        self.disc_loaded = False
        # Array of tuples with format (tracknumber: int, artist, album, title, duration: int, is_data_track: bool)
        self.track_info = []
        self.shuffle = False
        self.repeat = False

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
        # Make sure to start fresh
        self.unload()

        try:
            artists, track_titles, album, n_tracks, durations, is_data_tracks = (
                fetchdata()
            )
        except IOError:
            print("No CD found")
            return
        except Exception as e:
            print(f"Unknown exception: {e}")
            return

        #print(f">>>> {artists}, {track_titles}, {album}, {n_tracks}")

        self._set_track_info(artists, track_titles, album, durations, is_data_tracks)

        self.player = self.vlc_instance.media_player_new()
        self.media_list = self.vlc_instance.media_list_new()
        self.list_player = self.vlc_instance.media_list_player_new()
        self.list_player.set_media_player(self.player)

        self._do_set_shuffle()
        self._do_set_repeat()

        for i in range(1, n_tracks + 1):
            if is_data_tracks[i - 1]:
                # Skip any data tracks
                continue
            track = self.vlc_instance.media_new(
                f"cdda://{self._device}", (":cdda-track=" + str(i))
            )
            self.media_list.add_media(track)

        self.list_player.set_media_list(self.media_list)
        self.disc_loaded = True

    def unload(self):
        if self.player is not None:
            self.player.release()
        if self.media_list is not None:
            self.media_list.release()
        if self.list_player is not None:
            self.list_player.release()

        self.player = None
        self.media_list = None
        self.list_player = None
        self.disc_loaded = False
        self.track_info = []

    def play(self):
        if self.list_player is not None:
            self.list_player.play()

    def stop(self):
        if self.list_player is not None:
            self.list_player.stop()

    def pause(self):
        if self.list_player is not None:
            self.list_player.pause()

    def next(self):
        if self.list_player is not None:
            self.list_player.next()

    def prev(self):
        if self.list_player is not None:
            self.list_player.previous()

    # Jump to a specific track
    def jump(self, index):
        if self.list_player is not None:
            self.list_player.play_item_at_index(index)

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
        self.stop()
        try:
            drive = cdio.Device(driver_id=pycdio.DRIVER_UNKNOWN)
            drive.eject_media()
        except Exception as e:
            print(f"Problem finding a CD-ROM. {e}")
        self.unload()

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
        if self.disc_loaded is False:
            return "no-disc"

        if self.player is None:
            return "stopped"
        state = self.player.get_state()

        if (
            state == vlc.State.Playing
            or state == vlc.State.Buffering
            or state == vlc.State.Opening
        ):
            return "playing"
        if state == vlc.State.Stopped:
            return "stopped"
        if state == vlc.State.Paused:
            return "paused"

        if state == vlc.State.Buffering or state == vlc.State.Opening:
            return "loading"
        if state == vlc.State.Error:
            return "error"

        return "stopped"

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
        if self.media_list is None or self.list_player is None:
            raise Exception("Not playing")
        index = self.media_list.index_of_item(
            self.list_player.get_media_player().get_media()
        )
        if index < 0:
            index = 0
        return self.get_track_info(index)

    # -------- Events to be called by a timer --------

    def detect_disc_insertion(self):
        if self.disc_loaded:
            # Don't try to detect if disc is already loaded
            return False

        try:
            d = cdio.Device(driver_id=pycdio.DRIVER_UNKNOWN)
            # This is True every time media changed between the last time you called it and now
            # If this is the first tiem we call this, force check
            if d.get_media_changed() or self._detect_disc_insertion_is_first_call:
                self._detect_disc_insertion_is_first_call = False
                # This raises an exception "++ WARN: error in ioctl CDROMREADTOCHDR: No medium found" if no disc is inserted
                try:
                    num_tracks = d.get_num_tracks()
                    if num_tracks > 0:
                        return True
                except Exception:
                    return False
        except Exception as e:
            print(f"Problem finding a CD-ROM. {e}")

        return False

    def detect_disc_insertion_and_load(self):
        if self.detect_disc_insertion():
            self.load()
