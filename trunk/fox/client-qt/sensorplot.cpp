/***************************************************************************
 *   Copyright (C) 2010 by Rick Helmus   *
 *   rhelmus_AT_gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLCDNumber>
#include <QSignalMapper>
#include <QToolButton>
#include <QVBoxLayout>

#include <qwt_legend.h>
#include <qwt_plot.h>
#include <qwt_plot_curve.h>
#include <qwt_plot_magnifier.h>
#include <qwt_plot_panner.h>
#include <qwt_plot_picker.h>
#include <qwt_plot_zoomer.h>
#include <qwt_symbol.h>

#include "sensorplot.h"

CSensorPlot::CSensorPlot(const QString &title, QWidget *parent,
                         Qt::WindowFlags flags) : QWidget(parent, flags)
{
    QHBoxLayout *hbox = new QHBoxLayout(this);
    
    QWidget *plotW = new QWidget;
    hbox->addWidget(plotW);
    QVBoxLayout *vbox = new QVBoxLayout(plotW);
    
    vbox->addWidget(sensorPlot = new QwtPlot(title));
    sensorPlot->setCanvasBackground(QColor(Qt::cyan));
    sensorPlot->setAxisTitle(QwtPlot::xBottom, "time");
    sensorPlot->setAxisTitle(QwtPlot::yLeft, "ADC");
    //sensorPlot->setAxisScale(QwtPlot::yLeft, 0.0, 1000.0);
    sensorPlot->setAxisMaxMajor(QwtPlot::yLeft, 5);
    sensorPlot->updateAxes();
//     sensorPlot->setAxisAutoScale(QwtPlot::yLeft);
    vbox->insertWidget(0, createPlotTools(), 0, Qt::AlignCenter);
    
    
    hbox->addLayout(LCDLayout = new QVBoxLayout);
}

QWidget *CSensorPlot::createPlotTools()
{
    QFrame *ret = new QFrame;
    ret->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    
    QHBoxLayout *hbox = new QHBoxLayout(ret);
    
    QSignalMapper *signalmapper = new QSignalMapper(this);
    connect(signalmapper, SIGNAL(mapped(const QString &)), this,
            SLOT(toolToggled(const QString &)));
    
    // Zoomer
    zoomer = new QwtPlotZoomer(QwtPlot::xBottom, QwtPlot::yLeft,
                               sensorPlot->canvas());
    QToolButton *button = createToolButton("Zoom");
    hbox->addWidget(button);
    connect(button, SIGNAL(toggled(bool)), signalmapper, SLOT(map()));
    signalmapper->setMapping(button, "zoomer");
    button->setChecked(false);
    
    // Panner
    panner = new QwtPlotPanner(sensorPlot->canvas());
    panner->setMouseButton(Qt::MidButton);
    hbox->addWidget(button = createToolButton("Panner"));
    connect(button, SIGNAL(toggled(bool)), signalmapper, SLOT(map()));
    signalmapper->setMapping(button, "panner");
    button->setChecked(true);

    // Picker
    picker = new QwtPlotPicker(sensorPlot->canvas());
    picker->setRubberBand(QwtPicker::CrossRubberBand);
    picker->setTrackerMode(QwtPicker::AlwaysOn);
    picker->setSelectionFlags(QwtPicker::PointSelection | QwtPicker::DragSelection);
    hbox->addWidget(button = createToolButton("Picker"));
    connect(button, SIGNAL(toggled(bool)), signalmapper, SLOT(map()));
    signalmapper->setMapping(button, "picker");
    button->setChecked(true);

    // Magnifier
    magnifier = new QwtPlotMagnifier(sensorPlot->canvas());
    hbox->addWidget(button = createToolButton("Magnifier"));
    connect(button, SIGNAL(toggled(bool)), signalmapper, SLOT(map()));
    signalmapper->setMapping(button, "magnifier");
    button->setChecked(true);

    return ret;
}

QToolButton *CSensorPlot::createToolButton(const QString &text)
{
    QToolButton *ret = new QToolButton;
    ret->setText(text);
    ret->setCheckable(true);
    return ret;
}

void CSensorPlot::toolToggled(const QString &name)
{
    if (name == "zoomer")
        zoomer->setEnabled(!zoomer->isEnabled());
    else if (name == "panner")
        zoomer->setEnabled(!panner->isEnabled());
    else if (name == "picker")
        zoomer->setEnabled(!picker->isEnabled());
    else if (name == "magnifier")
        zoomer->setEnabled(!magnifier->isEnabled());
}

void CSensorPlot::addSensor(const std::string &name, const QColor &color)
{
    QwtPlotCurve *curve = new QwtPlotCurve(name.c_str());
    curve->setRenderHint(QwtPlotItem::RenderAntialiased);
    curve->setPen(QPen(color));
    curve->attach(sensorPlot);

    sensorPlot->replot();

    QFrame *frame = new QFrame;
    frame->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    frame->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    LCDLayout->addWidget(frame);

    QVBoxLayout *vbox = new QVBoxLayout(frame);

    QLabel *label = new QLabel(name.c_str());
    label->setAlignment(Qt::AlignCenter);
    vbox->addWidget(label);
    
    QLCDNumber *LCD = new QLCDNumber(frame);
    LCD->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    vbox->addWidget(LCD);

    sensorMap.insert(std::make_pair(name, SSensor(curve, LCD)));

    if ((sensorMap.size() > 1) && !sensorPlot->legend())
        sensorPlot->insertLegend(new QwtLegend, QwtPlot::RightLegend);
}

void CSensorPlot::addData(const std::string &name, const double x, const double y)
{
    TSensorMap::iterator it = sensorMap.find(name);

    if (it == sensorMap.end())
        return; // UNDONE: Error handling and stuff
    
    SSensor &sensor = it->second;
    
    // UNDONE: Size restraining
    sensor.xdata.push_back(x);
    sensor.ydata.push_back(y);

    sensor.sensorCurve->setRawData(&sensor.xdata[0], &sensor.ydata[0], sensor.xdata.size());

    // Update last value
    sensor.LCD->display(y);

    sensorPlot->replot();
}

void CSensorPlot::addData(const std::string &name, const double y)
{
    addData(name, sensorMap[name].xdata.size()+1, y);
}
