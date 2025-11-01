// StarMapWidget.h

#ifndef STARMAPWIDGET_H
#define STARMAPWIDGET_H

#include <QWidget>
#include <QImage>
#include <QMouseEvent>
#include <QPointF>
#include <QString>
#include <vector>
#include <cstdint>
#include "StarCatalog.h"
#include "SettingsDialog.h"

class QLabel;

class StarMapWidget : public QWidget {
    Q_OBJECT

public:
    StarMapWidget(
        std::vector<StarProjection> projections,
        const StarCatalog::Sun&      sunInfo,
        BlurParams                   blurParams,
        bool                         blurEnabled,
        FlareParams                  flareParams,
        bool                         flareEnabled,
        QWidget*                     parent = nullptr
        );

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    QString formatStarInfo(const StarProjection& projection) const;
    void showInfoPopup(const QPoint& globalPos, const QString& text);
    void hideInfoPopup();
    void renderStars();

    std::vector<StarProjection> m_projections;
    std::vector<QPointF>        m_pixelPositions;
    StarCatalog::Sun            sun;
    BlurParams                  blurParams;
    bool                        m_blurEnabled;
    FlareParams                 flareParams;
    bool                        m_flareEnabled;

    QImage starMapImage;
    QImage blurredImage;

    double m_minXi = 0.0;
    double m_maxXi = 0.0;
    double m_minEta = 0.0;
    double m_maxEta = 0.0;
    double m_centerXi = 0.0;
    double m_centerEta = 0.0;
    double m_scale = 1.0;
    bool   m_hasGeometry = false;

    QWidget* m_infoPopup = nullptr;
    QLabel*  m_infoLabel = nullptr;
};

#endif // STARMAPWIDGET_H
