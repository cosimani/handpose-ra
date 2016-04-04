#ifndef PRINCIPAL_H
#define PRINCIPAL_H

#include <QWidget>

namespace Ui {
class Principal;
}

class Principal : public QWidget
{
    Q_OBJECT

public:
    explicit Principal(QWidget *parent = 0);
    ~Principal();

private:
    Ui::Principal *ui;

protected:
    void keyPressEvent( QKeyEvent *event );

public slots:
    void slot_algunSliderModificado();

};

#endif // PRINCIPAL_H
