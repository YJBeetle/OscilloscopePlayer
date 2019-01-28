#ifndef DECODE_H
#define DECODE_H

#include <QThread>

class Decode : public QThread
{
    Q_OBJECT
public:
    explicit Decode(QObject *parent = nullptr);

signals:

public slots:

private:

};

#endif // DECODE_H
