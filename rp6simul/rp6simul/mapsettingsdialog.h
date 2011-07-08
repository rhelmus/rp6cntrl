#ifndef MAPSETTINGSDIALOG_H
#define MAPSETTINGSDIALOG_H

#include "robotscene.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QSlider;
class QSpinBox;

class CMapSettingsPreviewWidget;

class CMapSettingsDialog : public QDialog
{
    Q_OBJECT

    QSpinBox *widthSpinBox, *heightSpinBox;
    QCheckBox *refreshLightCheckBox;
    QSlider *ambientLightSlider;
    QComboBox *shadowQualityComboBox;
    QSpinBox *gridSizeSpinBox;
    QCheckBox *snapCheckBox;
    CMapSettingsPreviewWidget *previewWidget;

    QWidget *createLightSlider(void);

public:
    explicit CMapSettingsDialog(QWidget *parent = 0);

    void setMapSize(const QSizeF &s);
    void setAutoRefreshLight(bool a);
    void setAmbientLight(float l);
    void setShadowQuality(CRobotScene::EShadowQuality q);
    void setGridSize(float s);
    void setAutoGridSnap(bool s);

    QSizeF getMapSize(void) const;
    bool getAutoRefreshLight(void) const;
    float getAmbientLight(void) const;
    CRobotScene::EShadowQuality getShadowQuality(void) const;
    float getGridSize(void) const;
    bool getAutoGridSnap(void) const;
};

class CMapSettingsPreviewWidget : public QWidget
{
    Q_OBJECT

    QPixmap backGroundPixmap;
    int gridSize;
    int ambientLight;

protected:
    void paintEvent(QPaintEvent *);

public:
    explicit CMapSettingsPreviewWidget(QWidget *parent = 0);

    QSize minimumSizeHint(void) const { return QSize(50, 50); }
    QSize sizeHint(void) const { return QSize(150, 150); }

public slots:
    void setGridSize(int s) { gridSize = s; update(); }
    void setAmbientLight(int l) { ambientLight = l; update(); }
};

#endif // MAPSETTINGSDIALOG_H
