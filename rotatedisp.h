#ifndef ROTATEDISP_H
#define ROTATEDISP_H

#include <QString>
#include <QSize>

class RotateDisp
{
public:
    enum ROTATE {
        ROTATE_DEFAULT,
        ROTATE_90,
        ROTATE_180,
        ROTATE_270
    };

    typedef struct _mode {
        ROTATE r;
        int w;
        int h;
    }Mode;

    RotateDisp();

    QString errmsg();

    bool rotate(RotateDisp::ROTATE r);
    bool rotate(QString name, RotateDisp::ROTATE r);

    RotateDisp::ROTATE getRotate();
    RotateDisp::ROTATE getRotate(QString name);

    int getMonitorCount();
    QString getMonitorName(int index);

private:
    QSize m_defSzie;
    QString m_sErrinf;
};

#endif // ROTATEDISP_H
