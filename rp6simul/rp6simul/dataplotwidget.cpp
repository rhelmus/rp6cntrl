#include "dataplotwidget.h"

#include <QVBoxLayout>

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_legend.h>

CDataPlotWidget::CDataPlotWidget(QWidget *parent, Qt::WindowFlags flags) :
    QWidget(parent, flags), maxDataPoints(10)
{
    QVBoxLayout *vbox = new QVBoxLayout(this);

    vbox->addWidget(plotWidget = new QwtPlot);
    plotWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QwtText titlex("time (s)");
    QFont f(font());
    f.setPointSize(8);
    titlex.setFont(f);
    plotWidget->setAxisTitle(QwtPlot::xBottom, titlex);

    f.setPointSize(7);
    plotWidget->setAxisFont(QwtPlot::xBottom, f);
    plotWidget->setAxisFont(QwtPlot::yLeft, f);
//    plotWidget->setAxisTitle(QwtPlot::yLeft, "AU"); UNDONE
    plotWidget->setAxisMaxMajor(QwtPlot::xBottom, 5);
    plotWidget->setAxisMaxMinor(QwtPlot::xBottom, 2);
    plotWidget->setAxisMaxMajor(QwtPlot::yLeft, 5);
    plotWidget->setAxisMaxMinor(QwtPlot::yLeft, 2);
    plotWidget->updateAxes();

    // Make it transparent: see http://www.qtcentre.org/threads/39684-Setting-QwtPlot-s-canvas-as-Transparent-wthout-stylesheet-way
    plotWidget->canvas()->setPaintAttribute(QwtPlotCanvas::BackingStore, false);
    plotWidget->canvas()->setPaintAttribute(QwtPlotCanvas::Opaque, false);
    plotWidget->canvas()->setAttribute(Qt::WA_OpaquePaintEvent, false);
    plotWidget->canvas()->setAutoFillBackground(false);
}

void CDataPlotWidget::addCurve(const QString &name, const QColor &color)
{
    Q_ASSERT(!curveData.contains(name));

    QwtText t(name);
    QFont f(font());
    f.setPointSize(9);
    t.setFont(f);

    QwtPlotCurve *plotcurve = new QwtPlotCurve(t);
    plotcurve->setPen(color);
    plotcurve->attach(plotWidget);

    curveData.insert(name, SCurveData(plotcurve));

    if ((curveData.size() > 1) && !plotWidget->legend())
        plotWidget->insertLegend(new QwtLegend, QwtPlot::TopLegend);
}

void CDataPlotWidget::addDataPoint(const QString &curve, double x, double y)
{
    Q_ASSERT(curveData.contains(curve));

    SCurveData &cdata = curveData[curve];

    cdata.dataX << x;
    cdata.dataY << y;

    if (cdata.dataX.size() > maxDataPoints)
    {
        cdata.dataX.pop_front();
        cdata.dataY.pop_front();
    }

    cdata.plotCurve->setRawSamples(cdata.dataX.data(), cdata.dataY.data(),
                                   cdata.dataX.size());
    plotWidget->replot();
}

void CDataPlotWidget::clearData()
{
    for (QMap<QString, SCurveData>::iterator it=curveData.begin();
         it!=curveData.end(); ++it)
    {
        it.value().dataX.clear();
        it.value().dataY.clear();
        it.value().plotCurve->setRawSamples(it.value().dataX.data(),
                                            it.value().dataY.data(),
                                            it.value().dataX.size());
    }
    plotWidget->replot();
}
