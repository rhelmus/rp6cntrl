#include "dataplotwidget.h"

#include <QVBoxLayout>

#include <qwt_plot.h>
#include <qwt_plot_canvas.h>
#include <qwt_plot_curve.h>
#include <qwt_legend.h>
#include <qwt_legend_item.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_zoomer.h>

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
    plotWidget->setAxisMaxMajor(QwtPlot::xBottom, 5);
    plotWidget->setAxisMaxMinor(QwtPlot::xBottom, 2);
    plotWidget->setAxisMaxMajor(QwtPlot::yLeft, 5);
    plotWidget->setAxisMaxMinor(QwtPlot::yLeft, 2);
    plotWidget->updateAxes();

    new QwtPlotMagnifier(plotWidget->canvas());
    new QwtPlotZoomer(plotWidget->canvas());

    // Make it transparent: see http://www.qtcentre.org/threads/39684-Setting-QwtPlot-s-canvas-as-Transparent-wthout-stylesheet-way
    plotWidget->canvas()->setPaintAttribute(QwtPlotCanvas::BackingStore, false);
    plotWidget->canvas()->setPaintAttribute(QwtPlotCanvas::Opaque, false);
    plotWidget->canvas()->setAttribute(Qt::WA_OpaquePaintEvent, false);
    plotWidget->canvas()->setAutoFillBackground(false);
}

void CDataPlotWidget::showCurve(QwtPlotItem *item, bool on)
{
    // From Qwt cpluplot example
    item->setVisible(on);
    QWidget *w = plotWidget->legend()->find(item);
    if (w && w->inherits("QwtLegendItem"))
        ((QwtLegendItem *)w)->setChecked(on);

    plotWidget->replot();
}

void CDataPlotWidget::setYAxisRange(double min, double max)
{
    plotWidget->setAxisScale(QwtPlot::yLeft, min, max);
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

    if (curveData.size() > 1)
    {
        if (!plotWidget->legend())
        {
            QwtLegend *legend = new QwtLegend;
            legend->setItemMode(QwtLegend::CheckableItem);
            plotWidget->insertLegend(legend, QwtPlot::TopLegend);

            // Enable all items
            foreach (QWidget *w, legend->legendItems())
            {
                if (w && w->inherits("QwtLegendItem"))
                    ((QwtLegendItem *)w)->setChecked(true);
            }

            connect(plotWidget, SIGNAL(legendChecked(QwtPlotItem*,bool)),
                    SLOT(showCurve(QwtPlotItem*,bool)));
        }
        else
            showCurve(plotcurve, true);
    }
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

    plotWidget->setAxisScale(QwtPlot::xBottom, cdata.dataX[0], cdata.dataX.last());

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
