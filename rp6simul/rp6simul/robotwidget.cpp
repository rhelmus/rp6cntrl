#include "bumper.h"
#include "dataplotwidget.h"
#include "irsensor.h"
#include "led.h"
#include "robotwidget.h"
#include "simulator.h"
#include "utils.h"

#include <QtGui>

namespace {

void drawBumper(QPainter &painter, CBumper *bumper, const QTransform &tr,
                qreal scale)
{
    QPolygonF points = bumper->getPoints();

    // Account for scaling/transformation
    for (QPolygonF::iterator it=points.begin(); it!=points.end(); ++it)
    {
        it->rx() *= scale;
        it->ry() *= scale;
        *it = tr.map(*it);
    }

    painter.setBrush(bumper->getColor());
    painter.setPen(Qt::NoPen);
    painter.drawPolygon(points);
}

void drawMotorIndicator(QPainter &painter, const QRect &rect, int gradh,
                        EMotorDirection dir)
{
    const int tailwidth = 0.5 * (float)rect.width();
    const int tailx = rect.left() + ((rect.width()-tailwidth) / 2);
    const int headheight = 0.25 * rect.height();

    QPolygon arrowp;
    arrowp << QPoint(tailx, rect.bottom());
    arrowp << QPoint(tailx, rect.top() + headheight);
    arrowp << QPoint(rect.left(), arrowp.last().y());
    arrowp << QPoint(rect.center().x(), rect.top());
    arrowp << QPoint(rect.right(), rect.top() + headheight);
    arrowp << QPoint(tailx + tailwidth, arrowp.last().y());
    arrowp << QPoint(arrowp.last().x(), rect.bottom());

    // NOTE: drawn from bottom to top
    // gradh is used here to have a constant gradient, regardless of the
    // height (power) of the indicator. Thus a high arrow (high power)
    // has a more redish head.
    QLinearGradient lg(rect.center().x(), rect.bottom(), rect.center().x(),
                       rect.bottom() - gradh);
    lg.setColorAt(0.0, Qt::yellow);
    lg.setColorAt(1.0, Qt::red);
    painter.setBrush(lg);
    painter.setPen(Qt::NoPen);

    if (dir == MOTORDIR_FWD)
        painter.drawPolygon(arrowp);
    else
    {
        QTransform tr;
        tr.translate(rect.center().x(), rect.center().y());
        tr.rotate(180);
        tr.translate(-rect.center().x(), -rect.center().y());
        painter.drawPolygon(tr.map(arrowp));
    }
}

}

CRobotWidget::CRobotWidget(QWidget *parent) :
    QMdiArea(parent), motorArrowWidth(25), motorArrowXSpacing(10)
{
    const QPixmap p("../resource/rp6-top.png");
    origRobotSize = p.size();
    robotPixmap = p.scaledToWidth(250, Qt::SmoothTransformation);
    widgetMinSize = robotPixmap.size();
    widgetMinSize.rwidth() += (2 * (motorArrowWidth + motorArrowXSpacing));

    setBackground(QBrush(Qt::darkCyan)); // Use normal background (instead of darkened default)

#if 0
    QwtPlot *plot = new QwtPlot;
    plot->setTitle("Motor");
    plot->setAxisTitle(QwtPlot::xBottom, "time (ms)");
    plot->setAxisTitle(QwtPlot::yLeft, "AU");
//    plot->setAttribute(Qt::WA_TranslucentBackground);
//    plot->setAutoFillBackground(true);
//    plot->canvas()->setPalette(QColor(127, 127, 127, 127));
//    plot->canvas()->setAutoFillBackground(true);
//    plot->setCanvasBackground(QColor(255, 0, 0, 0));
//    plot->canvas()->setAutoFillBackground(false);

    plot->canvas()->setPaintAttribute(QwtPlotCanvas::BackingStore, false);
    plot->canvas()->setPaintAttribute(QwtPlotCanvas::Opaque, false);
    plot->canvas()->setAttribute( Qt::WA_OpaquePaintEvent, false );
    plot->canvas()->setAutoFillBackground( false );

    QwtPlotCurve *curve = new QwtPlotCurve;
    QVector<double> xdata, ydata;
    xdata << 0.0 << 1.0 << 2.0;
    ydata << 0.0 << 10.0 << 15.0;
    curve->attach(plot);
    curve->setSamples(xdata, ydata);

    QMdiSubWindow *subw = addSubWindow(plot);
    subw->setAttribute(Qt::WA_TranslucentBackground);
    subw->setWindowTitle("Motor");
    subw->setWindowFlags(subw->windowFlags() & ~Qt::WindowMaximizeButtonHint &
                         ~Qt::WindowCloseButtonHint);
    subw->setPalette(QColor(127, 127, 127, 127));
//    subw->setAutoFillBackground(true);
#endif
    CDataPlotWidget *plot = new CDataPlotWidget;
    plot->setMaxYScale(10.0);
    plot->addDataPoint(0.0, 10.0);
    plot->addDataPoint(1.0, 15.0);
    plot->addDataPoint(2.0, 50.0);

    QMdiSubWindow *subw = addSubWindow(plot);
    subw->setAttribute(Qt::WA_TranslucentBackground);
    subw->setWindowTitle("Motor");
    subw->setWindowFlags(subw->windowFlags() & ~Qt::WindowMaximizeButtonHint &
                         ~Qt::WindowCloseButtonHint);
    subw->setPalette(QColor(127, 127, 127, 127));
}

void CRobotWidget::paintEvent(QPaintEvent *event)
{
    QMdiArea::paintEvent(event);

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    // center
    const int imgx = (width() - robotPixmap.width()) / 2;
    const int imgy = (height() - robotPixmap.height()) / 2;
    painter.drawPixmap(imgx, imgy, robotPixmap);

    QTransform tr;
    tr.translate(imgx, imgy);

    // Equal aspect ratio, so scaling equals for width and height
    // UNDONE: move to transform
    const qreal scale = (qreal)robotPixmap.width() / (qreal)origRobotSize.width();

    foreach (CLED *l, LEDs)
    {
        if (l->isEnabled())
            drawLED(painter, l, tr, scale);
    }

    foreach (CBumper *b, bumpers)
    {
        if (b->isHit())
            drawBumper(painter, b, tr, scale);
    }

    foreach (CIRSensor *ir, IRSensors)
    {
        if (ir->getHitDistance() < ir->getTraceDistance())
        {
            QPointF pos = ir->getPosition();
            pos.rx() *= scale;
            pos.ry() *= scale;
            pos = tr.map(pos);

            const float rad = ir->getRadius() * scale;
            painter.setBrush(ir->getColor());
            painter.drawEllipse(pos, rad, rad);
        }
    }

    const int arrowyoffset = robotPixmap.height() * 0.2;
    const int arrowmaxheight = robotPixmap.height() - (2 * arrowyoffset);
    const int arrowtexth = painter.fontMetrics().height();

    if (motorPower[MOTOR_LEFT] != 0)
    {
        const int arrowheight = motorPower[MOTOR_LEFT] * arrowmaxheight / 200;
        const int bottom = (imgy + robotPixmap.height()) - arrowyoffset;
        const int y = bottom - arrowheight;
        QRect rect(imgx - (motorArrowWidth + motorArrowXSpacing), y,
                   motorArrowWidth, arrowheight);
        drawMotorIndicator(painter, rect, arrowmaxheight,
                           motorDirection[MOTOR_LEFT]);

        const QString text(QString::number(motorPower[MOTOR_LEFT]));
        QRect trect(rect.left(), rect.top() - arrowtexth, rect.width(),
                    arrowtexth);
        painter.setPen(Qt::blue);
        painter.drawText(trect, Qt::AlignCenter, text);
    }

    if (motorPower[MOTOR_RIGHT] != 0)
    {
        const int arrowheight = motorPower[MOTOR_RIGHT] * arrowmaxheight / 200;
        const int bottom = (imgy + robotPixmap.height()) - arrowyoffset;
        const int y = bottom - arrowheight;
        QRect rect(imgx + robotPixmap.width() + motorArrowXSpacing, y,
                   motorArrowWidth, arrowheight);
        drawMotorIndicator(painter, rect, arrowmaxheight,
                           motorDirection[MOTOR_RIGHT]);

        QRect trect(rect.left(), rect.top() - arrowtexth, rect.width(),
                    arrowtexth);
        painter.setPen(Qt::blue);
        painter.drawText(trect, Qt::AlignCenter,
                         QString::number(motorPower[MOTOR_RIGHT]));
    }
}

void CRobotWidget::addBumper(CBumper *b)
{
    bumpers << b;
}

void CRobotWidget::removeBumper(CBumper *b)
{
    bumpers.removeOne(b);
}

void CRobotWidget::addLED(CLED *l)
{
    LEDs << l;
}

void CRobotWidget::removeLED(CLED *l)
{
    LEDs.removeOne(l);
}

void CRobotWidget::addIRSensor(CIRSensor *ir)
{
    IRSensors << ir;
}

void CRobotWidget::removeIRSensor(CIRSensor *ir)
{
    IRSensors.removeOne(ir);
}
