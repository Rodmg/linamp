#include "cdnativediscservice.h"

#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/cdrom.h>

CDNativeDiscService::CDNativeDiscService(QString devicePath)
    : m_devicePath(std::move(devicePath))
{
}

bool CDNativeDiscService::isDiscInserted() const
{
    const QByteArray deviceBytes = m_devicePath.toLocal8Bit();
    const int fd = open(deviceBytes.constData(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        return false;
    }

    const int status = ioctl(fd, CDROM_DRIVE_STATUS, 0);
    close(fd);
    return status == CDS_DISC_OK;
}

bool CDNativeDiscService::eject() const
{
    const QByteArray deviceBytes = m_devicePath.toLocal8Bit();
    const int fd = open(deviceBytes.constData(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        return false;
    }

    // Some drives refuse eject while door lock is set or while spinning.
    ioctl(fd, CDROM_LOCKDOOR, 0);
    ioctl(fd, CDROMSTOP, 0);

    const int result = ioctl(fd, CDROMEJECT, 0);
    close(fd);
    return result == 0;
}

QList<CDNativeTrack> CDNativeDiscService::readTrackLayout() const
{
    QList<CDNativeTrack> tracks;

    const QByteArray deviceBytes = m_devicePath.toLocal8Bit();
    const int fd = open(deviceBytes.constData(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        return tracks;
    }

    cdrom_tochdr hdr;
    if (ioctl(fd, CDROMREADTOCHDR, &hdr) != 0) {
        close(fd);
        return tracks;
    }

    struct TrackBoundary {
        int trackNumber;
        int control;
        qint64 startLba;
    };

    QList<TrackBoundary> boundaries;

    for (int track = hdr.cdth_trk0; track <= hdr.cdth_trk1; ++track) {
        cdrom_tocentry entry;
        memset(&entry, 0, sizeof(entry));
        entry.cdte_track = static_cast<unsigned char>(track);
        entry.cdte_format = CDROM_MSF;

        if (ioctl(fd, CDROMREADTOCENTRY, &entry) != 0) {
            continue;
        }

        const qint64 lba = msfToLba(
            entry.cdte_addr.msf.minute,
            entry.cdte_addr.msf.second,
            entry.cdte_addr.msf.frame);

        boundaries.append({track, entry.cdte_ctrl, lba});
    }

    cdrom_tocentry leadout;
    memset(&leadout, 0, sizeof(leadout));
    leadout.cdte_track = CDROM_LEADOUT;
    leadout.cdte_format = CDROM_MSF;

    if (ioctl(fd, CDROMREADTOCENTRY, &leadout) == 0) {
        boundaries.append({hdr.cdth_trk1 + 1, 0, msfToLba(
            leadout.cdte_addr.msf.minute,
            leadout.cdte_addr.msf.second,
            leadout.cdte_addr.msf.frame)});
    }

    close(fd);

    if (boundaries.size() < 2) {
        return tracks;
    }

    for (int i = 0; i < boundaries.size() - 1; ++i) {
        const TrackBoundary current = boundaries.at(i);
        const TrackBoundary next = boundaries.at(i + 1);
        const qint64 lengthLba = qMax<qint64>(0, next.startLba - current.startLba);

        CDNativeTrack track;
        track.trackNumber = current.trackNumber;
        track.startLba = current.startLba;
        track.lengthLba = lengthLba;
        track.durationMs = (lengthLba * 1000) / 75;
        track.isDataTrack = (current.control & CDROM_DATA_TRACK) != 0;
        track.title = QString("Track %1").arg(track.trackNumber);
        tracks.append(track);
    }

    return tracks;
}

qint64 CDNativeDiscService::msfToLba(int minute, int second, int frame)
{
    const qint64 absoluteFrames = (minute * 60 * 75) + (second * 75) + frame;
    return absoluteFrames - 150;
}
