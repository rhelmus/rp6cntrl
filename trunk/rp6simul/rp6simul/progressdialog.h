#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>

// Own version of QProgressDialog: adds support for rolling progressbar and
// un-closable

class QLabel;
class QProgressBar;

class CProgressDialog : public QDialog
{
    Q_OBJECT

    QProgressBar *progressBar;
    QLabel *statusText;
    bool closable;
    bool canceled;

private slots:
    void cancelPressed(void) { canceled = true; reject(); }

protected:
    void closeEvent(QCloseEvent *e);
    void keyPressEvent(QKeyEvent *e);

public:
    explicit CProgressDialog(bool c, QWidget *parent = 0);

    void setValue(int v);
    void setRange(int min, int max);
    void setLabelText(const QString &t);
    bool wasCanceled(void) const { return canceled; }
};

#endif // PROGRESSDIALOG_H
