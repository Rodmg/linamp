from enum import Enum

class PlayerStatus(Enum):
    Idle = 'idle'
    Playing = 'playing'
    Stopped = 'stopped'
    Paused = 'paused'
    Error = 'error'
    Loading = 'loading'


# Base Player abstract class
class BasePlayer:
    # Called when the source is selected in the UI or whenever the full track info needs to be refreshed
    def load(self) -> None:
        pass

    # Called when the source is deselected in the UI
    def unload(self) -> None:
        pass

    # Basic playback control funcions called on button press
    def play(self) -> None:
        pass

    def stop(self) -> None:
        pass

    def pause(self) -> None:
        pass

    def next(self) -> None:
        pass

    def prev(self) -> None:
        pass

    # Go to a specific time in a track while playing
    def seek(self, ms: int) -> None:
        pass

    # Playback mode toggles
    def set_shuffle(self, enabled: bool) -> None:
        pass

    def set_repeat(self, enabled: bool) -> None:
        pass

    # Eject button
    def eject(self) -> None:
        pass

    # -------- Status Functions --------

    # Called constantly during playback to update the time progress
    def get_postition(self) -> int:
        return 0

    def get_shuffle(self) -> bool:
        return False

    def get_repeat(self) -> bool:
        return False

    # Returns the str representation of PlayerStatus enum
    def get_status(self) -> str:
        status = PlayerStatus.Idle
        return status.value

    # tuple with format (tracknumber: int, artist, album, title, duration_ms: int, codec: str, bitrate_bps: int, samplerate_hz: int)
    def get_track_info(self) -> tuple[int, str, str, str, int, str, int, int]:
        return (1, '', '', '', 1, 'AAC', 256000, 44100)

    # Return any message you want to show to the user. tuple with format: (show_message: bool, message: str, message_timeout_ms: int)
    def get_message(self) -> tuple[bool, str, int]:
        return (False, '', 0)

    # Called whenever the UI wants to force clear the message
    def clear_message(self) -> None:
        pass

    # -------- Polling functions to be called by a timer --------

    def poll_events(self) -> bool:
        return False
