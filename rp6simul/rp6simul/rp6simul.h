#ifndef RP6SIMUL_H
#define RP6SIMUL_H

#include <QtGui/QMainWindow>

#include <QList>
#include <QMutex>
#include <QString>

#include "lua.h"
#include "robotscene.h"

enum ELEDType { LED1, LED2, LED3, LED4, LED5, LED6, ACSL, ACSR };

class QActionGroup;
class QGraphicsView;
class QPushButton;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QStackedWidget;
class QTableWidget;
class QTreeWidget;
class QTreeWidgetItem;

class CProjectWizard;
class CRobotWidget;
class CSimulator;

class CRP6Simulator : public QMainWindow
{
    Q_OBJECT

    enum ELogType { LOG_LOG, LOG_WARNING, LOG_ERROR };

    CSimulator *simulator;
    QString currentProjectFile;
    QString currentMapFile;
    bool currentMapIsTemplate;    

    CProjectWizard *projectWizard;
    QList<QAction *> mapMenuActionList;
    QAction *runPluginAction, *stopPluginAction;
    QToolBar *editMapToolBar;
    QActionGroup *editMapActionGroup;
    QStackedWidget *mainStackedWidget;
    CRobotWidget *robotWidget;
    CRobotScene *robotScene;
    QGraphicsView *graphicsView;
    QPlainTextEdit *logWidget;
    QPlainTextEdit *serialOutputWidget;
    QLineEdit *serialInputWidget;
    QPushButton *serialSendButton;
    QTreeWidget *robotStatusTreeWidget;
    QTreeWidget *mapSelectorTreeWidget;
    QTreeWidgetItem *mapTemplatesTreeItem;
    QTreeWidgetItem *mapHistoryTreeItem;
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
    QWidget *createProjectPlaceHolderWidget(void);
    QWidget *createRobotWidget(void);
    QWidget *createRobotSceneWidget(void);
    QWidget *createLogWidgets(void);
    QDockWidget *createStatusDock(void);
    QDockWidget *createRegisterDock(void);

    void updateMainStackedWidget(void);
    void openProjectFile(const QString &file);
    void loadMapFile(const QString &file, bool istemplate);
    void loadMapTemplatesTree(void);
    bool mapItemIsTemplate(QTreeWidgetItem *item) const;
    void syncMapHistoryTree(const QStringList &l);
    void loadMapHistoryTree(void);
    void addMapHistoryFile(const QString &file);
    void initLua(void);
    QString getLogOutput(ELogType type, QString text) const;
    void appendLogOutput(ELogType type, const QString &text);
    void appendRobotStatusUpdate(const QStringList &strtree);

    // Lua bindings
    static int luaAppendLogOutput(lua_State *l);
    static int luaAppendSerialOutput(lua_State *l);
    static int luaUpdateRobotStatus(lua_State *l);
    static int luaEnableLED(lua_State *l);

private slots:
    void updateClockDisplay(unsigned long hz);
    void timedUpdate(void);
    void newProject(void);
    void openProject(void);
    void newMap(void);
    void saveMap(void);
    void saveMapAs(void);
    void loadMap(void);
    void runPlugin(void);
    void stopPlugin(void);
    void changeSceneMouseMode(QAction *a);
    void sceneMouseModeChanged(CRobotScene::EMouseMode mode);
    void editMapSettings(void);
    void mapSelectorItemActivated(QTreeWidgetItem *item);
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
