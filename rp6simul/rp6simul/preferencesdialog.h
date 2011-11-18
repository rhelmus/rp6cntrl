#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>

class QComboBox;
class QGroupBox;
class QLineEdit;
class QSettings;
class QSlider;

class CPathInput;
class CSerialPreferencesWidget;

class CPreferencesDialog : public QDialog
{
    Q_OBJECT

    CPathInput *defaultProjectDirPathInput;
    QGroupBox *soundGroupBox;
    QSlider *piezoVolumeSlider;
    QGroupBox *robotSerialGroupBox, *m32SerialGroupBox;
    CSerialPreferencesWidget *robotSerialWidget, *m32SerialWidget;
    QComboBox *simulationCPUComboBox;

    QWidget *createGeneralTab(void);
    QWidget *createSoundTab(void);
    QWidget *createSerialTab(void);
    QWidget *createSimulationTab(void);

public:
    explicit CPreferencesDialog(QWidget *parent = 0);

    void loadPreferences(QSettings &settings);
    void savePreferences(QSettings &settings);

public slots:
    void accept();
};

class CSerialPreferencesWidget : public QWidget
{
    Q_OBJECT

    QLineEdit *portEdit;
    QComboBox *baudRateComboBox, *parityComboBox;
    QComboBox *dataBitsComboBox, *stopBitsComboBox;

private slots:
    void showSerialPortSelection(void);

public:
    CSerialPreferencesWidget(QWidget *parent=0, Qt::WindowFlags f=0);

    void loadPreferences(QSettings &settings);
    void savePreferences(QSettings &settings);
    QString getPort(void) const;
};

#endif // PREFERENCES_H
