#include "dataplotwidget.h"

#include <QVBoxLayout>

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>

CDataPlotWidget::CDataPlotWidget(QWidget *parent, Qt::WindowFlags flags) :
    QWidget(parent, flags), maxDataPoints(10)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);

    vbox->addWidget(plotWidget = new QwtPlot);
    plotWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QwtText titlex("time (ms)");
    QFont f(font());
    f.setPointSize(10);
    titlex.setFont(f);
    plotWidget->setAxisTitle(QwtPlot::xBottom, titlex);

    f.setPointSize(9);
    plotWidget->setAxisFont(QwtPlot::xBottom, f);
    plotWidget->setAxisFont(QwtPlot::yLeft, f);
//    plotWidget->setAxisTitle(QwtPlot::yLeft, "AU"); UNDONE

    // Make it transparent: see http://www.qtcentre.org/threads/39684-Setting-QwtPlot-s-canvas-as-Transparent-wthout-stylesheet-way
    plotWidget->canvas()->setPaintAttribute(QwtPlotCanvas::BackingStore, false);
    plotWidget->canvas()->setPaintAttribute(QwtPlotCanvas::Opaque, false);
    plotWidget->canvas()->setAttribute( Qt::WA_OpaquePaintEvent, false);
    plotWidget->canvas()->setAutoFillBackground(false);

    plotCurve = new QwtPlotCurve;
    plotCurve->attach(plotWidget);
}

void CDataPlotWidget::setMaxYScale(double s)
{
    const int maxmajordiv = 4; // Only 5 major axis ticks (keeps it small)
    plotWidget->setAxisMaxMajor(QwtPlot::yLeft, maxmajordiv);
    plotWidget->setAxisMaxMinor(QwtPlot::yLeft, 2);
    plotWidget->updateAxes();
}

void CDataPlotWidget::addDataPoint(double x, double y)
{
    dataX << x;
    dataY << y;

    if (dataX.size() > maxDataPoints)
    {
        dataX.pop_front();
        dataY.pop_front();
    }

    plotCurve->setRawSamples(dataX.data(), dataY.data(), dataX.size());
    plotWidget->replot();
}
