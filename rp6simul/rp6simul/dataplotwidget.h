#ifndef DATAPLOTWIDGET_H
#define DATAPLOTWIDGET_H

#include <QWidget>

class QwtPlot;
class QwtPlotCurve;

class CDataPlotWidget : public QWidget
{
    QVector<double> dataX, dataY;
    int maxDataPoints;
    QwtPlot *plotWidget;
    QwtPlotCurve *plotCurve;

public:
    CDataPlotWidget(QWidget *parent=0, Qt::WindowFlags flags=0);

    void setMaxDataPoints(int m) { maxDataPoints = m; }
    void setMaxYScale(double s);
    void addDataPoint(double x, double y);
};

#endif // DATAPLOTWIDGET_H
