#include "SimulationPlayerWidget.h"

#include <QPainter>
#include <QTimer>
#include <algorithm>
#include <utility>

SimulationPlayerWidget::SimulationPlayerWidget(QWidget* parent)
    : QWidget(parent)
    , m_timer(new QTimer(this))
{
    setMinimumSize(1081, 761);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(m_timer, &QTimer::timeout, this, &SimulationPlayerWidget::onTick);
}

void SimulationPlayerWidget::setFrames(QVector<QImage> frames, int playbackMsPerFrame)
{
    m_frames = std::move(frames);
    m_playbackMsPerFrame = std::max(10, playbackMsPerFrame);
    m_index = 0;
    update();
}

void SimulationPlayerWidget::start()
{
    if (m_frames.isEmpty()) {
        emit finished();
        return;
    }

    m_index = 0;
    update();

    if (m_frames.size() <= 1) {
        QTimer::singleShot(m_playbackMsPerFrame, this, [this]() { emit finished(); });
        return;
    }

    m_timer->start(m_playbackMsPerFrame);
}

void SimulationPlayerWidget::stop()
{
    m_timer->stop();
}

void SimulationPlayerWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)

    QPainter p(this);
    p.fillRect(rect(), Qt::black);

    if (m_frames.isEmpty())
        return;

    const int lastIndex = std::max(0, static_cast<int>(m_frames.size()) - 1);
    const QImage& frame = m_frames[std::clamp(m_index, 0, lastIndex)];
    if (frame.isNull())
        return;

    const QSize targetSize = frame.size().scaled(size(), Qt::KeepAspectRatio);
    const QPoint topLeft((width() - targetSize.width()) / 2,
                         (height() - targetSize.height()) / 2);

    p.drawImage(QRect(topLeft, targetSize), frame);
}

void SimulationPlayerWidget::onTick()
{
    ++m_index;
    if (m_index >= m_frames.size()) {
        m_timer->stop();
        emit finished();
        return;
    }
    update();
}
