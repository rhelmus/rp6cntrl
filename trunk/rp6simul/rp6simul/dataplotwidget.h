#ifndef DATAPLOTWIDGET_H
#define DATAPLOTWIDGET_H

#include <QMap>
#include <QString>
#include <QWidget>

class QwtPlot;
class QwtPlotCurve;
class QwtPlotItem;

class CDataPlotWidget : public QWidget
{
    Q_OBJECT

    struct SCurveData
    {
        QVector<double> dataX, dataY;
        QwtPlotCurve *plotCurve;
        SCurveData(QwtPlotCurve *c) : plotCurve(c) { }
        SCurveData(void) : plotCurve(0) { }
    };

    int maxDataPoints;
    QwtPlot *plotWidget;
    QMap<QString, SCurveData> curveData;

private slots:
    void showCurve(QwtPlotItem *item, bool on);

public:
    CDataPlotWidget(QWidget *parent=0, Qt::WindowFlags flags=0);

    void setMaxDataPoints(int m) { maxDataPoints = m; }
    void setYAxisRange(double min, double max);
    void addCurve(const QString &name, const QColor &color);
    void addDataPoint(const QString &curve, double x, double y);
    void clearData(void);
};

#endif // DATAPLOTWIDGET_H
