#ifndef MEDIAFLAGS_H
#define MEDIAFLAGS_H

#include <QWidget>
#include "data/StreamDetails.h"

namespace Ui {
class MediaFlags;
}

/**
 * @brief The MediaFlags class
 *        A widget where flags based on the streamdetails are shown
 */
class MediaFlags : public QWidget
{
    Q_OBJECT

public:
    explicit MediaFlags(QWidget *parent = 0);
    ~MediaFlags();
    void setStreamDetails(StreamDetails *streamDetails);

private:
    Ui::MediaFlags *ui;
    int m_height;
    void setupResolution(StreamDetails *streamDetails);
    void setupAspect(StreamDetails *streamDetails);
    void setupCodec(StreamDetails *streamDetails);
    void setupAudio(StreamDetails *streamDetails);
    void setupChannels(StreamDetails *streamDetails);
};

#endif // MEDIAFLAGS_H
