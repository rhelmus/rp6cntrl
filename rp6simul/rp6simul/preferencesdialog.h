#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QDialog>

class CPreferencesDialog : public QDialog
{
    Q_OBJECT

    QWidget *createGeneralTab(void);
    QWidget *createSoundTab(void);
    QWidget *createSerialTab(void);
    QWidget *createSimulationTab(void);

public:
    explicit CPreferencesDialog(QWidget *parent = 0);
};

class CSerialPreferencesWidget : public QWidget
{
public:
    CSerialPreferencesWidget(QWidget *parent=0, Qt::WindowFlags f=0);
};

#endif // PREFERENCES_H
