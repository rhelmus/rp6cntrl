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
    QMdiArea(parent), motorArrowWidth(25), motorArrowXSpacing(10),
    m32Scale(1.0), activeM32Slot(SLOT_END), m32PixmapDirty(false),
    dataPlotClosedSignalMapper(this)
{
    const QPixmap p("../resource/rp6-top.png");
    origRobotSize = p.size();
    robotPixmap = p.scaledToWidth(225, Qt::SmoothTransformation);
    widgetMinSize = robotPixmap.size();
    widgetMinSize.rwidth() += (2 * (motorArrowWidth + motorArrowXSpacing));

    origM32Size = QImage("../resource/m32-top.png").size();
    m32Rotations[SLOT_FRONT] = m32Rotations[SLOT_BACK] = 0.0;

    setBackground(QBrush()); // Clear background: interferes with drawing

    connect(&dataPlotClosedSignalMapper, SIGNAL(mapped(int)),
            SIGNAL(dataPlotClosed(int)));

    // Create timer before plot subwindows!
    dataPlotUpdateTimer = new QTimer;
    dataPlotUpdateTimer->setInterval(250); // UNDONE: Configurable?
    connect(dataPlotUpdateTimer, SIGNAL(timeout()), SLOT(dataPlotTimedUpdate()));

    for (int i=0; i<DATAPLOT_MAX; ++i)
        createDataPlotSubWindow(static_cast<EDataPlotType>(i));
}

void CRobotWidget::updateM32Pixmap()
{
    if (activeM32Slot == SLOT_END)
        return;

    const float w = robotPixmap.width() * m32Scale;
    m32Pixmap = QPixmap("../resource/m32-top.png").scaledToWidth(w, Qt::SmoothTransformation);

    const QPointF c(m32Pixmap.rect().center());
    QTransform tr;
    tr.translate(c.x(), c.y());
    tr.rotate(m32Rotations[activeM32Slot]);
    tr.translate(-c.x(), -c.y());
    m32Pixmap = m32Pixmap.transformed(tr, Qt::SmoothTransformation);

    m32PixmapDirty = false;
}

void CRobotWidget::createDataPlotSubWindow(EDataPlotType plot)
{
    CDataPlotWidget *plotw = new CDataPlotWidget;
    // Restrict history to 10 secs
    plotw->setMaxDataPoints(1000.0 / (double)dataPlotUpdateTimer->interval() * 10.0);

    CDataPlotSubWindow *subw =
            new CDataPlotSubWindow(this, Qt::SubWindow | Qt::CustomizeWindowHint |
                                   Qt::WindowTitleHint |
                                   Qt::WindowMinimizeButtonHint |
                                   Qt::WindowCloseButtonHint);
    subw->setWidget(plotw);
    addSubWindow(subw);
    subw->setAttribute(Qt::WA_TranslucentBackground);
    subw->setPalette(QColor(127, 127, 127, 127));
    subw->resize(subw->minimumSizeHint());
    subw->close();

    switch (plot)
    {
    case DATAPLOT_MOTORSPEED:
        subw->setWindowTitle("Motor Speed");
        plotw->addCurve("left", Qt::blue);
        plotw->addCurve("right", Qt::red);
        break;
    case DATAPLOT_MOTORPOWER:
        subw->setWindowTitle("Motor Power");
        plotw->addCurve("left", Qt::blue);
        plotw->addCurve("right", Qt::red);
        break;
    case DATAPLOT_LIGHT:
        subw->setWindowTitle("Light Sensors");
        plotw->addCurve("left", Qt::blue);
        plotw->addCurve("right", Qt::red);
        break;
    case DATAPLOT_PIEZO:
        subw->setWindowTitle("Piezo");
        plotw->addCurve("piezo", Qt::blue);
        break;
    case DATAPLOT_MIC:
        subw->setWindowTitle("Microphone");
        plotw->addCurve("mic", Qt::blue);
        break;
    case DATAPLOT_ROBOTADC:
        subw->setWindowTitle("ADC robot");
        plotw->addCurve("ADC0", Qt::red);
        plotw->addCurve("ADC1", Qt::yellow);
        plotw->addCurve("E_INT1", Qt::green);
        plotw->addCurve("MCURRENT_L", Qt::black);
        plotw->addCurve("MCURRENT_R", Qt::magenta);
        plotw->addCurve("UBAT", Qt::cyan);
        break;
    case DATAPLOT_M32ADC:
        subw->setWindowTitle("ADC M32");
        plotw->addCurve("KEYPAD", Qt::red);
        plotw->addCurve("ADC2", Qt::yellow);
        plotw->addCurve("ADC3", Qt::green);
        plotw->addCurve("ADC4", Qt::black);
        plotw->addCurve("ADC5", Qt::magenta);
        plotw->addCurve("ADC6", Qt::cyan);
        plotw->addCurve("ADC7", Qt::darkRed);
        break;
    default: Q_ASSERT(false);
    }

    dataPlotClosedSignalMapper.setMapping(subw, plot);
    connect(subw, SIGNAL(closed()), &dataPlotClosedSignalMapper, SLOT(map()));

    dataPlots.insert(plot, SDataPlotInfo(subw, plotw));
}

void CRobotWidget::dataPlotTimedUpdate()
{
    const double elapsed = static_cast<double>(runTime.elapsed()) / 1000.0;

    // Motor speed
    int lspeed = motorSpeed[MOTOR_LEFT], rspeed = motorSpeed[MOTOR_RIGHT];
    if (motorDirection[MOTOR_LEFT] == MOTORDIR_BWD)
        lspeed *= -1;
    if (motorDirection[MOTOR_RIGHT] == MOTORDIR_BWD)
        rspeed *= -1;

    dataPlots[DATAPLOT_MOTORSPEED].plotWidget->addDataPoint("left", elapsed,
                                                            lspeed);
    dataPlots[DATAPLOT_MOTORSPEED].plotWidget->addDataPoint("right", elapsed,
                                                            rspeed);

    // Motor power
    dataPlots[DATAPLOT_MOTORPOWER].plotWidget->addDataPoint("left", elapsed,
                                                            motorPower[MOTOR_LEFT]);
    dataPlots[DATAPLOT_MOTORPOWER].plotWidget->addDataPoint("right", elapsed,
                                                            motorPower[MOTOR_RIGHT]);

    // Beeper / Piezo
    dataPlots[DATAPLOT_PIEZO].plotWidget->addDataPoint("piezo", elapsed,
                                                       beeperPitch);


    // Light
    dataPlots[DATAPLOT_LIGHT].plotWidget->addDataPoint("left", elapsed,
                                                       robotADCValues["LS_L"]);
    dataPlots[DATAPLOT_LIGHT].plotWidget->addDataPoint("right", elapsed,
                                                       robotADCValues["LS_R"]);

    // Microphone
    dataPlots[DATAPLOT_MIC].plotWidget->addDataPoint("mic", elapsed,
                                                     m32ADCValues["MIC"]);

    // (Other) ADC - robot
    dataPlots[DATAPLOT_ROBOTADC].plotWidget->addDataPoint("ADC0", elapsed,
                                                          robotADCValues["ADC0"]);
    dataPlots[DATAPLOT_ROBOTADC].plotWidget->addDataPoint("ADC1", elapsed,
                                                          robotADCValues["ADC1"]);
    dataPlots[DATAPLOT_ROBOTADC].plotWidget->addDataPoint("E_INT1", elapsed,
                                                          robotADCValues["E_INT1"]);
    dataPlots[DATAPLOT_ROBOTADC].plotWidget->addDataPoint("MCURRENT_L", elapsed,
                                                          robotADCValues["MCURRENT_L"]);
    dataPlots[DATAPLOT_ROBOTADC].plotWidget->addDataPoint("MCURRENT_R", elapsed,
                                                          robotADCValues["MCURRENT_R"]);
    dataPlots[DATAPLOT_ROBOTADC].plotWidget->addDataPoint("UBAT", elapsed,
                                                          robotADCValues["UBAT"]);

    // (Other) ADC - m32
    dataPlots[DATAPLOT_M32ADC].plotWidget->addDataPoint("KEYPAD", elapsed,
                                                          m32ADCValues["KEYPAD"]);
    dataPlots[DATAPLOT_M32ADC].plotWidget->addDataPoint("ADC2", elapsed,
                                                          m32ADCValues["ADC2"]);
    dataPlots[DATAPLOT_M32ADC].plotWidget->addDataPoint("ADC3", elapsed,
                                                          m32ADCValues["ADC3"]);
    dataPlots[DATAPLOT_M32ADC].plotWidget->addDataPoint("ADC4", elapsed,
                                                          m32ADCValues["ADC4"]);
    dataPlots[DATAPLOT_M32ADC].plotWidget->addDataPoint("ADC5", elapsed,
                                                          m32ADCValues["ADC5"]);
    dataPlots[DATAPLOT_M32ADC].plotWidget->addDataPoint("ADC6", elapsed,
                                                          m32ADCValues["ADC6"]);
    dataPlots[DATAPLOT_M32ADC].plotWidget->addDataPoint("ADC7", elapsed,
                                                          m32ADCValues["ADC7"]);
}

void CRobotWidget::paintEvent(QPaintEvent *event)
{
    QMdiArea::paintEvent(event);

    QPainter painter(viewport());
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);

    // Background
    painter.fillRect(rect(), Qt::darkCyan);

    // center
    const int imgx = (width() - robotPixmap.width()) / 2;
    const int imgy = (height() - robotPixmap.height()) / 2;
    painter.drawPixmap(imgx, imgy, robotPixmap);

    QTransform tr;
    tr.translate(imgx, imgy);

    // Equal aspect ratio, so scaling equals for width and height
    // UNDONE: move to transform
    const qreal scale = (qreal)robotPixmap.width() / (qreal)origRobotSize.width();

    foreach (CLED *l, robotLEDs)
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

    if (m32PixmapDirty)
        updateM32Pixmap();

    if (!m32Pixmap.isNull()) // null if active slot isn't set yet
    {
        const QPointF m32pos(m32Positions[activeM32Slot]);
        painter.drawPixmap(tr.map(m32pos), m32Pixmap);

        tr.translate(m32pos.x(), m32pos.y());
        tr.translate(m32Pixmap.width()/2.0, m32Pixmap.height()/2.0);
        tr.rotate(m32Rotations[activeM32Slot]);
        tr.translate(-m32Pixmap.width()/2.0, -m32Pixmap.height()/2.0);

        const qreal m32scale =
                (qreal)m32Pixmap.width() / (qreal)origM32Size.width();
        foreach (CLED *l, m32LEDs)
        {
            if (l->isEnabled())
                drawLED(painter, l, tr, m32scale);
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

void CRobotWidget::start()
{
    runTime.start();
    dataPlotUpdateTimer->start();
    foreach (SDataPlotInfo di, dataPlots)
        di.plotWidget->clearData();
}

void CRobotWidget::stop()
{
    dataPlotUpdateTimer->stop();
    robotLEDs.clear();
    bumpers.clear();
    IRSensors.clear();
    motorPower.clear();
    motorSpeed.clear();
    motorDirection.clear();
    update();
}

void CRobotWidget::showDataPlot(EDataPlotType plot)
{
    dataPlots[plot].subWindow->show();
    dataPlots[plot].plotWidget->show();
}

void CRobotWidget::addBumper(CBumper *b)
{
    bumpers << b;
}

void CRobotWidget::removeBumper(CBumper *b)
{
    bumpers.removeOne(b);
}

void CRobotWidget::addRobotLED(CLED *l)
{
    robotLEDs << l;
}

void CRobotWidget::removeRobotLED(CLED *l)
{
    robotLEDs.removeOne(l);
}

void CRobotWidget::addIRSensor(CIRSensor *ir)
{
    IRSensors << ir;
}

void CRobotWidget::removeIRSensor(CIRSensor *ir)
{
    IRSensors.removeOne(ir);
}

void CRobotWidget::setM32Slot(EM32Slot s, const QPointF &p, float r)
{
    const qreal scale = (qreal)robotPixmap.width() / (qreal)origRobotSize.width();
    m32Positions[s] = p * scale;
    m32Rotations[s] = r;
    m32PixmapDirty = true;
}

void CRobotWidget::addM32LED(CLED *l)
{
    m32LEDs << l;
}

void CRobotWidget::removeM32LED(CLED *l)
{
    m32LEDs.removeOne(l);
}
