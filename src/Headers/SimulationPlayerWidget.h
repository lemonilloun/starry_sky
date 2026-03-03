#pragma once

#include <QWidget>
#include <QVector>
#include <QImage>

class QTimer;

class SimulationPlayerWidget : public QWidget {
    Q_OBJECT

public:
    explicit SimulationPlayerWidget(QWidget* parent = nullptr);

    void setFrames(QVector<QImage> frames, int playbackMsPerFrame);
    void start();
    void stop();

signals:
    void finished();

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onTick();

private:
    QVector<QImage> m_frames;
    int m_playbackMsPerFrame = 40;
    int m_index = 0;
    QTimer* m_timer = nullptr;
};
