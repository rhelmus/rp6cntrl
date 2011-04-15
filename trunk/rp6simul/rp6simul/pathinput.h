#ifndef DIRINPUT_H
#define DIRINPUT_H

#include <QWidget>

class QLineEdit;

class CPathInput : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString path READ getPath WRITE setPath NOTIFY pathTextChanged USER true)

public:
    enum EPathInputMode { PATH_EXISTDIR, PATH_EXISTFILE };

private:
    QString pathDescription;
    EPathInputMode pathInputMode;
    QLineEdit *pathEdit;

private slots:
    void openBrowser(void);

public:
    explicit CPathInput(const QString &desc, EPathInputMode mode,
                        const QString &path=QString(), QWidget *parent = 0);

    QString getPath(void) const;
    void setPath(const QString &dir);

signals:
    void pathTextChanged(const QString &);
    void pathTextEdited(const QString &);
};

#endif // DIRINPUT_H
