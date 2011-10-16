#ifndef RP6SIMUL_H
#define RP6SIMUL_H

#include <QtGui/QMainWindow>

#include <SDL/SDL.h>

#include <QList>
#include <QMutex>
#include <QString>

#include "lua.h"
#include "robotscene.h"

enum EMotor { MOTOR_LEFT=0, MOTOR_RIGHT, MOTOR_END };
enum EMotorDirection { MOTORDIR_FWD=0, MOTORDIR_BWD, MOTORDIR_END };
enum EM32Slot { SLOT_FRONT=0, SLOT_BACK, SLOT_END };

Q_DECLARE_METATYPE(EMotor)
Q_DECLARE_METATYPE(EMotorDirection)

class QActionGroup;
class QCheckBox;
class QComboBox;
class QGraphicsView;
class QPushButton;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QSpinBox;
class QStackedWidget;
class QTableWidget;
class QTableWidgetItem;
class QTabWidget;
class QToolButton;
class QTreeWidget;
class QTreeWidgetItem;

class QextSerialPort;

class CBumper;
class CLED;
class CProjectWizard;
class CRobotWidget;
class CSimulator;

class CRP6Simulator : public QMainWindow
{
    Q_OBJECT

    enum ELogType { LOG_LOG, LOG_WARNING, LOG_ERROR };
    enum { AUDIO_SAMPLERATE = 22050 };
    enum EHandClapMode { CLAP_SOFT, CLAP_NORMAL, CLAP_LOUD };
    enum { EXTEEPROM_PAGESIZE = 64, EXTEEPROM_CAPACITY = 32 * 1024,
           EXTEEPROM_TABLEDIMENSION = 8 };
    enum EExtEEPROMMode { EXTEEPROMMODE_NUMERICAL=0, EXTEEPROMMODE_CHAR=1 };

    QextSerialPort *robotSerialDevice, *m32SerialDevice;
    float audioCurrentCycle, audioFrequency;
    int beeperPitch; // As set by plugin code
    bool playingBeep;
    SDL_AudioCVT handClapCVT;
    EHandClapMode handClapMode;
    bool playingHandClap;
    int handClapSoundPos;
    EExtEEPROMMode extEEPROMMode;
    CSimulator *robotSimulator, *m32Simulator;
    QString currentProjectFile;
    QString currentMapFile;
    bool currentMapIsTemplate;

    CProjectWizard *projectWizard;
    QList<QAction *> mapMenuActionList;
    QAction *runPluginAction, *stopPluginAction;
    QToolButton *handClapButton;
    QWidget *keyPadWidget;
    QToolBar *robotToolBar;
    QMenu *dataPlotMenu;
    QToolBar *editMapToolBar;
    QActionGroup *editMapActionGroup;
    QStackedWidget *mainStackedWidget;
    QTabWidget *projectTabWidget;
    QStackedWidget *mapStackedWidget;
    CRobotWidget *robotWidget;
    CRobotScene *robotScene;
    QGraphicsView *graphicsView;
    QPlainTextEdit *logWidget;
    QPlainTextEdit *robotSerialOutputWidget;
    QLineEdit *robotSerialInputWidget;
    QPushButton *robotSerialSendButton;
    QPlainTextEdit *m32SerialOutputWidget;
    QLineEdit *m32SerialInputWidget;
    QPushButton *m32SerialSendButton;
    QPlainTextEdit *IRCOMMOutputWidget;
    QSpinBox *IRCOMMAddressWidget, *IRCOMMKeyWidget;
    QCheckBox *IRCOMMToggleWidget;
    QPushButton *IRCOMMSendButton;
    QTreeWidget *robotStatusTreeWidget;
    QTreeWidget *mapSelectorTreeWidget;
    QTreeWidgetItem *mapTemplatesTreeItem;
    QTreeWidgetItem *mapHistoryTreeItem;
    QDockWidget *ADCDockWidget;
    QTableWidget *robotADCTableWidget, *m32ADCTableWidget;
    QComboBox *IORegisterTableSelector;
    QTableWidget *IORegisterTableWidget;
    QComboBox *extEEPROMModeComboBox;
    QSpinBox *extEEPROMPageSpinBox;
    QTableWidget *extEEPROMTableWidget;
    QStackedWidget *extEEPROMInputStackedWidget;
    QSpinBox *extEEPROMInputSpinBox;
    QLineEdit *extEEPROMInputLineEdit;

    QString logTextBuffer;
    QMutex logBufferMutex;
    QString robotSerialTextBuffer, m32SerialTextBuffer;
    QMutex robotSerialBufferMutex, m32SerialBufferMutex;
    QList<QStringList> robotStatusUpdateBuffer;
    QMutex robotStatusMutex;
    QMap<CLED *, bool> changedLEDs;
    QMutex robotLEDMutex;    
#ifdef USEATOMIC
    QAtomicInt changedMotorPower[MOTOR_END];
    QAtomicInt changedMotorMoveSpeed[MOTOR_END];
    QAtomicInt changedMotorDirection[MOTORDIR_END];
#else
    int changedMotorPower[MOTOR_END];
    QMutex motorPowerMutex;
    int changedMotorMoveSpeed[MOTOR_END];
    QMutex motorMoveSpeedMutex;
    int changedMotorDirection[MOTORDIR_END];
    QMutex motorDirectionMutex;
#endif
    volatile bool robotIsBlocked;
    QMap<CBumper *, int> bumperLuaCallbacks;
    int robotSerialSendLuaCallback, m32SerialSendLuaCallback;
    int IRCOMMSendLuaCallback;
    int luaHandleExtInt1Callback;
    int luaHandleSoundCallback;
    int luaHandleKeyPressCallback;
    QTimer *pluginUpdateUITimer;
    QTimer *pluginUpdateLEDsTimer;

    static CRP6Simulator *instance;

    void initSerialPort(QextSerialPort *port);
    void openSerialPort(QextSerialPort *port);
    void initSDL(void);
    void initSimulators(void);
    void registerLuaGeneric(lua_State *l);
    void registerLuaRobot(lua_State *l);
    void registerLuaM32(lua_State *l);

    void createMenus(void);
    void setToolBarToolTips(void);
    void createToolbars(void);
    QWidget *createMainWidget(void);
    QWidget *createProjectPlaceHolderWidget(void);
    QWidget *createRobotWidget(void);
    QWidget *createRobotSceneWidget(void);
    QWidget *createBottomTabs(void);
    QWidget *createLogTab(void);
    QWidget *createSerialRobotTab(void);
    QWidget *createSerialM32Tab(void);
    QWidget *createIRCOMMTab(void);
    QDockWidget *createStatusDock(void);
    QDockWidget *createADCDock(void);
    QDockWidget *createRegisterDock(void);
    QDockWidget *createExtEEPROMDock(void);

    void updateMainStackedWidget(void);
    void updateMapStackedWidget(void);
    void openProjectFile(const QString &file);
    void loadMapFile(const QString &file, bool istemplate);
    void loadMapTemplatesTree(void);
    bool mapItemIsTemplate(QTreeWidgetItem *item) const;
    void syncMapHistoryTree(const QStringList &l);
    void loadMapHistoryTree(void);
    void addMapHistoryFile(const QString &file);
    QString getLogOutput(ELogType type, QString text) const;
    void appendRobotStatusUpdate(const QStringList &strtree);
    QVariantList getExtEEPROM(void) const;
    QVariantHash getADCValues(lua_State *l);

    // SDL audio callback
    static void SDLAudioCallback(void *, Uint8 *stream, int length);

    // Lua bindings
    static int luaAppendLogOutput(lua_State *l);
    static int luaAppendRobotSerialOutput(lua_State *l);
    static int luaAppendM32SerialOutput(lua_State *l);
    static int luaSetDriverLists(lua_State *l);
    static int luaSetCmPerPixel(lua_State *l);
    static int luaSetRobotLength(lua_State *l);
    static int luaSetRobotM32Slot(lua_State *l);
    static int luaSetRobotM32Scale(lua_State *l);
    static int luaLogIRCOMM(lua_State *l);
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
    static int luaSetRobotSerialSendCallback(lua_State *l);
    static int luaSetM32SerialSendCallback(lua_State *l);
    static int luaSetIRCOMMSendCallback(lua_State *l);
    static int luaSetExtInt1Callback(lua_State *l);
    static int luaSetExtInt1Enabled(lua_State *l);
    static int luaSetExtEEPROM(lua_State *l);
    static int luaGetExtEEPROM(lua_State *l);
    static int luaSetBeeperEnabled(lua_State *l);
    static int luaSetBeeperFrequency(lua_State *l);
    static int luaSetSoundCallback(lua_State *l);
    static int luaSetKeyPressCallback(lua_State *l);

private slots:
    void handleRobotSerialDeviceData(void);
    void handleM32SerialDeviceData(void);
    void updateRobotClockDisplay(unsigned long hz);
    void updateM32ClockDisplay(unsigned long hz);
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
    void doHandClap(void);
    void setHandClapMode(int m);
    void handleKeyPadPress(int key);
    void resetKeyPress(void);
    void handleDataPlotSelection(QAction *a);
    void enableDataPlotSelection(int plot);
    void tabViewChanged(int index);
    void changeSceneMouseMode(QAction *a);
    void sceneMouseModeChanged(CRobotScene::EMouseMode mode);
    void editMapSettings(void);
    void mapSelectorItemActivated(QTreeWidgetItem *item);
    void resetADCTable(void);
    void applyADCTable(void);
    void updateExtEEPROM(int page);
    void updateExtEEPROMMode(int mode);
    void updateExtEEPROMSelection(QTableWidgetItem *item);
    void updateExtEEPROMByte(int address, quint8 data);
    void applyExtEEPROMChange(void);
    void setRobotIsBlocked(bool b) { robotIsBlocked = b; }
    void setLuaBumper(CBumper *b, bool e);
    void sendRobotSerialText(void);
    void sendM32SerialText(void);
    void sendIRCOMM(void);
    void debugSetRobotLeftPower(int power);
    void debugSetRobotRightPower(int power);

public:
    CRP6Simulator(QWidget *parent = 0);
    ~CRP6Simulator(void);

    static CRP6Simulator *getInstance(void) { return instance; }
    bool loadCustomDriverInfo(const QString &file, QString &name,
                              QString &desc);

signals:
    // These are emitted by the class itself to easily thread-synchronize
    // function calls
    void motorDriveSpeedChanged(EMotor, int);
    void motorDirectionChanged(EMotor, EMotorDirection);
    void newIRCOMMText(const QString &);
    void beeperPitchChanged(int);
    void receivedRobotSerialSendCallback(bool);
    void receivedM32SerialSendCallback(bool);
    void receivedIRCOMMSendCallback(bool);
    void receivedSoundCallback(bool);
    void receivedKeyPressCallback(bool);
    void extEEPROMByteChanged(int, quint8);
};

#endif // RP6SIMUL_H
