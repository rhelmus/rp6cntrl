#ifndef RP6SIMUL_H
#define RP6SIMUL_H

#include <QtGui/QMainWindow>

#include <QList>
#include <QMutex>
#include <QString>

#include "lua.h"
#include "robotscene.h"

enum EMotor { MOTOR_LEFT, MOTOR_RIGHT };
enum EMotorDirection { MOTORDIR_FWD, MOTORDIR_BWD };

Q_DECLARE_METATYPE(EMotor)
Q_DECLARE_METATYPE(EMotorDirection)

class QActionGroup;
class QCheckBox;
class QGraphicsView;
class QPushButton;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QTabWidget;
class QTreeWidget;
class QTreeWidgetItem;

class CBumper;
class CLED;
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
    QTabWidget *projectTabWidget;
    QStackedWidget *mapStackedWidget;
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
    QDockWidget *ADCDockWidget;
    QTableWidget *ADCTableWidget;
    QTableWidget *IORegisterTableWidget;

    QString logTextBuffer;
    QMutex logBufferMutex;
    QString serialTextBuffer;
    QMutex serialBufferMutex;
    QList<QStringList> robotStatusUpdateBuffer;
    QMutex robotStatusMutex;
    QMap<CLED *, bool> changedLEDs;
    QMutex robotLEDMutex;
    QMap<EMotor, int> changedMotorPower;
    QMutex motorPowerMutex;
    QMap<EMotor, int> changedMotorMoveSpeed;
    QMutex motorMoveSpeedMutex;
    QMap<EMotor, EMotorDirection> changedMotorDirection;
    QMutex motorDirectionMutex;
    volatile bool robotIsBlocked;
    QMap<CBumper *, int> bumperLuaCallbacks;
    QList<QCheckBox *> ADCOverrideCheckBoxes;
    QList<QSpinBox *> ADCOverrideSpinBoxes;
    QTimer *pluginUpdateUITimer;
    QTimer *pluginUpdateLEDsTimer;

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
    QDockWidget *createADCDock(void);
    QDockWidget *createRegisterDock(void);

    void updateMainStackedWidget(void);
    void updateMapStackedWidget(void);
    void openProjectFile(const QString &file);
    void loadMapFile(const QString &file, bool istemplate);
    void loadMapTemplatesTree(void);
    bool mapItemIsTemplate(QTreeWidgetItem *item) const;
    void syncMapHistoryTree(const QStringList &l);
    void loadMapHistoryTree(void);
    void addMapHistoryFile(const QString &file);
    void initLua(void);
    QString getLogOutput(ELogType type, QString text) const;
    void appendRobotStatusUpdate(const QStringList &strtree);

    // Lua bindings
    static int luaAppendLogOutput(lua_State *l);
    static int luaAppendSerialOutput(lua_State *l);
    static int luaUpdateRobotStatus(lua_State *l);
    static int luaRobotIsBlocked(lua_State *l);
    static int luaCreateBumper(lua_State *l);
    static int luaBumperSetCallback(lua_State *l);
    static int luaBumperDestr(lua_State *l);
    static int luaCreateLED(lua_State *l);
    static int luaLEDSetEnabled(lua_State *l);
    static int luaLEDDestr(lua_State *l);
    static int luaSetMotorPower(lua_State *l);
    static int luaSetMotorDriveSpeed(lua_State *l);
    static int luaSetMotorMoveSpeed(lua_State *l);
    static int luaSetMotorDir(lua_State *l);
    static int luaCreateIRSensor(lua_State *l);
    static int luaIRSensorGetHitDistance(lua_State *l);
    static int luaIRSensorDestr(lua_State *l);
    static int luaCreateLightSensor(lua_State *l);
    static int luaLightSensorGetLight(lua_State *l);
    static int luaLightSensorDestr(lua_State *l);

private slots:
    void updateClockDisplay(unsigned long hz);
    void timedUIUpdate(void);
    void timedLEDUpdate(void);
    void newProject(void);
    void openProject(void);
    void newMap(void);
    void saveMap(void);
    void saveMapAs(void);
    void loadMap(void);
    void runPlugin(void);
    void stopPlugin(void);
    void tabViewChanged(int index);
    void changeSceneMouseMode(QAction *a);
    void sceneMouseModeChanged(CRobotScene::EMouseMode mode);
    void editMapSettings(void);
    void mapSelectorItemActivated(QTreeWidgetItem *item);
    void resetADCTable(void);
    void applyADCTable(void);
    void setRobotIsBlocked(bool b) { robotIsBlocked = b; }
    void setLuaBumper(CBumper *b, bool e);
    void sendSerialPressed(void);
    void debugSetRobotLeftPower(int power);
    void debugSetRobotRightPower(int power);

public:
    CRP6Simulator(QWidget *parent = 0);
    ~CRP6Simulator(void);

    static CRP6Simulator *getInstance(void) { return instance; }

signals:
    // These are emitted by the class itself to easily thread-synchronize
    // function calls
    void motorDriveSpeedChanged(EMotor, int);
    void motorDirectionChanged(EMotor, EMotorDirection);
};

#endif // RP6SIMUL_H
