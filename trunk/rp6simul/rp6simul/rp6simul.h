#ifndef RP6SIMUL_H
#define RP6SIMUL_H

#include <QtGui/QMainWindow>

#include <QList>
#include <QMutex>
#include <QString>

#include "lua.h"

class QGraphicsView;
class QPushButton;
class QLCDNumber;
class QLineEdit;
class QPlainTextEdit;
class QTableWidget;
class QTreeWidget;

class CProjectWizard;
class CRobotGraphicsItem;
class CSimulator;

class CRP6Simulator : public QMainWindow
{
    Q_OBJECT

    enum ELogType { LOG_LOG, LOG_WARNING, LOG_ERROR };

    CSimulator *simulator;

    CProjectWizard *projectWizard;
    QAction *runPluginAction, *stopPluginAction;

    QGraphicsView *graphicsView;
    CRobotGraphicsItem *robotGraphicsItem;
    QPlainTextEdit *logWidget;
    QPlainTextEdit *serialOutputWidget;
    QLineEdit *serialInputWidget;
    QPushButton *serialSendButton;
    QLCDNumber *clockDisplay;
    QTreeWidget *robotStatusTreeWidget;
    QTableWidget *IORegisterTableWidget;

    QString serialTextBuffer;
    QMutex serialBufferMutex;
    QList<QStringList> robotStatusUpdateBuffer;
    QMutex robotStatusMutex;
    QTimer *pluginUpdateUITimer;

    static CRP6Simulator *instance;

    void createMenus(void);
    QPixmap createAddLightImage(void) const;
    QPixmap createLightSettingsImage(void) const;
    void setToolBarToolTips(void);
    void createToolbars(void);
    QWidget *createMainWidget(void);
    QWidget *createLogWidgets(void);
    QDockWidget *createStatusDock(void);
    QDockWidget *createRegisterDock(void);

    void openProjectFile(const QString &file);
    void initLua(void);
    QString getLogOutput(ELogType type, QString text) const;
    void appendLogOutput(ELogType type, const QString &text);
    void scaleGraphicsView(qreal f);

    // Lua bindings
    static int luaAppendLogOutput(lua_State *l);
    static int luaAppendSerialOutput(lua_State *l);
    static int luaUpdateRobotStatus(lua_State *l);

private slots:
    void updateClockDisplay(unsigned long hz);
    void timedUpdate(void);
    void newProject(void);
    void openProject(void);
    void runPlugin(void);
    void stopPlugin(void);
    void zoomSceneIn(void);
    void zoomSceneOut(void);
    void sendSerialPressed(void);
    void debugSetRobotLeftPower(int power);
    void debugSetRobotRightPower(int power);

public:
    CRP6Simulator(QWidget *parent = 0);

    static CRP6Simulator *getInstance(void) { return instance; }

signals:
    void logTextReady(const QString &text);
};

#endif // RP6SIMUL_H
