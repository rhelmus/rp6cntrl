#ifndef RP6SIMUL_H
#define RP6SIMUL_H

#include <QtGui/QMainWindow>

#include <QList>
#include <QMutex>
#include <QString>

#include "lua.h"
#include "robotscene.h"

class QActionGroup;
class QGraphicsView;
class QPushButton;
class QLCDNumber;
class QLineEdit;
class QPlainTextEdit;
class QStackedWidget;
class QTableWidget;
class QTreeWidget;

class CProjectWizard;
class CSimulator;

class CRP6Simulator : public QMainWindow
{
    Q_OBJECT

    enum ELogType { LOG_LOG, LOG_WARNING, LOG_ERROR };

    CSimulator *simulator;
    QString currentProjectFile;

    CProjectWizard *projectWizard;
    QList<QAction *> mapMenuActionList;
    QAction *runPluginAction, *stopPluginAction;
    QToolBar *editMapToolBar;
    QActionGroup *editMapActionGroup;
    QStackedWidget *mainStackedWidget;
    CRobotScene *robotScene;
    QGraphicsView *graphicsView;
    QPlainTextEdit *logWidget;
    QPlainTextEdit *serialOutputWidget;
    QLineEdit *serialInputWidget;
    QPushButton *serialSendButton;
    QTreeWidget *robotStatusTreeWidget;
    QTableWidget *IORegisterTableWidget;

    QString serialTextBuffer;
    QMutex serialBufferMutex;
    QList<QStringList> robotStatusUpdateBuffer;
    QMutex robotStatusMutex;
    QTimer *pluginUpdateUITimer;

    static CRP6Simulator *instance;

    void createMenus(void);
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
    void changeSceneMouseMode(QAction *a);
    void sceneMouseModeChanged(CRobotScene::EMouseMode mode);
    void editMapSettings(void);
    void saveMap(void);
    void exportMap(void);
    void importMap(void);
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
