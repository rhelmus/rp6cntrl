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


#ifndef SENSORPLOT_H
#define SENSORPLOT_H

#include <map>
#include <vector>

#include <QWidget>

class QLCDNumber;
class QToolButton;
class QVBoxLayout;

class QwtPlot;
class QwtPlotCurve;
class QwtPlotMagnifier;
class QwtPlotPanner;
class QwtPlotPicker;
class QwtPlotZoomer;

class CSensorPlot: public QWidget
{
    Q_OBJECT
    
    struct SSensor
    {
        QwtPlotCurve *sensorCurve;
        QLCDNumber *LCD;
        std::vector<double> xdata, ydata;
        SSensor(QwtPlotCurve *c, QLCDNumber *l) : sensorCurve(c), LCD(l) { }
        SSensor(void) : sensorCurve(0), LCD(0) { } // For stl
    };

    typedef std::map<std::string, SSensor> TSensorMap;
    TSensorMap sensorMap;

    QwtPlot *sensorPlot;
    QwtPlotMagnifier *magnifier;
    QwtPlotPanner *panner;
    QwtPlotPicker *picker;
    QwtPlotZoomer *zoomer;
    QVBoxLayout *LCDLayout;
    
    QWidget *createPlotTools(void);
    QToolButton *createToolButton(const QString &text);

private slots:
    void toolToggled(const QString &name);
    
public:
    CSensorPlot(const QString &title, QWidget *parent = 0, Qt::WindowFlags f = 0);

    void addSensor(const std::string &name, const QColor &color=Qt::black);
    void addData(const std::string &name, const double x, const double y);
    void addData(const std::string &name, const double y);
};

#endif
