#include "principal.h"
#include "ui_principal.h"
#include <QKeyEvent>

Principal::Principal(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Principal)
{
    ui->setupUi(this);

    connect(ui->sliderMin, SIGNAL(valueChanged(int)), this, SLOT(slot_algunSliderModificado()));
    connect(ui->sliderMax, SIGNAL(valueChanged(int)), this, SLOT(slot_algunSliderModificado()));

    slot_algunSliderModificado();  // Solo para setear los valores inicialmente
}

Principal::~Principal()
{
    delete ui;
}

void Principal::keyPressEvent(QKeyEvent *event)  {
    switch(event->key())  {
    case Qt::Key_Escape:
        this->close();
        break;
    default:;
    }
}

void Principal::slot_algunSliderModificado()  {
    ui->scene->refSkin->addMinHue(ui->sliderMin->value());
    ui->scene->refSkin->addMaxHue(ui->sliderMax->value());
    ui->lSliderMin->setText("Min: " + QString::number(ui->sliderMin->value()));
    ui->lSliderMax->setText("Max: " + QString::number(ui->sliderMax->value()));
}
