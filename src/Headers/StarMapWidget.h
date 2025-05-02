// StarMapWidget.h

#ifndef STARMAPWIDGET_H
#define STARMAPWIDGET_H

#include <QWidget>
#include <QImage>
#include <vector>
#include <cstdint>
#include "StarCatalog.h"
#include "SettingsDialog.h"

class StarMapWidget : public QWidget {
    Q_OBJECT

public:
    StarMapWidget(
        const std::vector<double>&   x,
        const std::vector<double>&   y,
        const std::vector<double>&   m,
        const std::vector<uint64_t>& ids,
        const StarCatalog::Sun&      sunInfo,
        BlurParams            blurParams,
        bool                         blurEnabled,
        FlareParams           flareParams,
        bool                         flareEnabled,
        QWidget*                     parent = nullptr
        );

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void renderStars();

    std::vector<double>   xCoords, yCoords, magnitudes;
    std::vector<uint64_t> starIds;
    StarCatalog::Sun      sun;
    BlurParams            blurParams;
    bool        m_blurEnabled;
    FlareParams           flareParams;
    bool        m_flareEnabled;

    QImage starMapImage;
    QImage blurredImage;
};

#endif // STARMAPWIDGET_H
