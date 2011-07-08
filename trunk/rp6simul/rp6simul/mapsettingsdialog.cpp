#include "mapsettingsdialog.h"

#include <QtGui>

namespace {

QSpinBox *createSpinBox(int min, int max)
{
    QSpinBox *ret = new QSpinBox;
    ret->setRange(min, max);
    return ret;
}

}

CMapSettingsDialog::CMapSettingsDialog(QWidget *parent) :
    QDialog(parent)
{
    setModal(true);

    QGridLayout *grid = new QGridLayout(this);

    QFormLayout *form = new QFormLayout;
    grid->addLayout(form, 0, 0);

    form->addRow("Width", widthSpinBox = createSpinBox(100, 20000));
    form->addRow("Height", heightSpinBox = createSpinBox(100, 20000));
    form->addRow("Auto refresh light map", refreshLightCheckBox = new QCheckBox);
    form->addRow("Ambient light", createLightSlider());
    form->addRow("Grid size", gridSizeSpinBox = createSpinBox(5, 100));
    form->addRow("Auto snap to grid", snapCheckBox = new QCheckBox);

    refreshLightCheckBox->setToolTip("If necessary refresh lighting automatically when leaving edit mode.");
    gridSizeSpinBox->setSingleStep(5);

    QVBoxLayout *vbox = new QVBoxLayout;
    grid->addLayout(vbox, 0, 1);

    QLabel *l = new QLabel("Preview");
    l->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    vbox->addWidget(l);

    vbox->addWidget(previewWidget = new CMapSettingsPreviewWidget);
    connect(ambientLightSlider, SIGNAL(valueChanged(int)),
            previewWidget, SLOT(setAmbientLight(int)));
    connect(gridSizeSpinBox, SIGNAL(valueChanged(int)), previewWidget,
            SLOT(setGridSize(int)));

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok |
                                                  QDialogButtonBox::Cancel);
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));
    grid->addWidget(bbox, 1, 0, 1, -1);
}

QWidget *CMapSettingsDialog::createLightSlider()
{
    QWidget *ret = new QWidget;
    QGridLayout *grid = new QGridLayout(ret);
    grid->setHorizontalSpacing(20);

    grid->addWidget(ambientLightSlider = new QSlider(Qt::Horizontal), 0, 0, 1, -1);
    ambientLightSlider->setRange(0, 200);
    ambientLightSlider->setSingleStep(10);
    ambientLightSlider->setTickPosition(QSlider::TicksBelow);

    grid->addWidget(new QLabel("Dark"), 1, 0);
    grid->addWidget(new QLabel("Normal"), 1, 1);
    grid->addWidget(new QLabel("Bright"), 1, 2);

    return ret;
}

void CMapSettingsDialog::setMapSize(const QSizeF &s)
{
    widthSpinBox->setValue(s.width());
    heightSpinBox->setValue(s.height());
}

void CMapSettingsDialog::setAutoRefreshLight(bool a)
{
    refreshLightCheckBox->setChecked(a);
}

void CMapSettingsDialog::setAmbientLight(float l)
{
    ambientLightSlider->setValue(qRound(l * 100.0));
}

void CMapSettingsDialog::setGridSize(float s)
{
    gridSizeSpinBox->setValue(s);
}

void CMapSettingsDialog::setAutoGridSnap(bool s)
{
    snapCheckBox->setChecked(s);
}

QSizeF CMapSettingsDialog::getMapSize() const
{
    return QSizeF(widthSpinBox->value(), heightSpinBox->value());
}

bool CMapSettingsDialog::getAutoRefreshLight() const
{
    return refreshLightCheckBox->isChecked();
}

float CMapSettingsDialog::getAmbientLight() const
{
    return static_cast<float>(ambientLightSlider->value()) / 100.0;
}

float CMapSettingsDialog::getGridSize() const
{
    return gridSizeSpinBox->value();
}

bool CMapSettingsDialog::getAutoGridSnap() const
{
    return snapCheckBox->isChecked();
}


// Background pixmap: keep in sync with CRobotScene
CMapSettingsPreviewWidget::CMapSettingsPreviewWidget(QWidget *parent)
    : QWidget(parent),
      backGroundPixmap(QPixmap("../resource/floor.jpg").scaled(300, 300,
                                                               Qt::IgnoreAspectRatio,
                                                               Qt::SmoothTransformation)),
      gridSize(0), ambientLight(0)
{
}

void CMapSettingsPreviewWidget::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.drawPixmap(0, 0, backGroundPixmap);

    if (ambientLight > 100)
    {
        painter.setCompositionMode(QPainter::CompositionMode_Overlay);
        painter.setPen(Qt::NoPen);
        const int c = ambientLight * 127 / 100;
        painter.setBrush(QColor(c, c, c));
        painter.drawRect(0, 0, size().width(), size().height());
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }
    else if (ambientLight < 100)
    {
        const int alpha = 255 - (ambientLight * 255 / 100);
        painter.setBrush(QColor(0, 0, 0, alpha));
        painter.drawRect(0, 0, size().width(), size().height());
    }

    if (gridSize)
    {
        painter.setPen(QPen(Qt::blue, 0.25));
        for (int x=0; x<=size().width(); x+=gridSize)
            painter.drawLine(x, 0, x, size().height());

        for (int y=0; y<=size().height(); y+=gridSize)
            painter.drawLine(0, y, size().width(), y);
    }
}
