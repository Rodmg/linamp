#include "titlebar.h"
#include "ui_titlebar.h"
#include "scale.h"
#include <QMouseEvent>

TitleBar::TitleBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TitleBar)
{
    ui->setupUi(this);
    scale();
    connect(ui->closeButton, &QPushButton::clicked, window(), &QWidget::close);
    connect(ui->minimizeButton, &QPushButton::clicked, window(), &QWidget::showMinimized);
}

TitleBar::~TitleBar()
{
    delete ui;
}


void TitleBar::scale()
{
    QRect geo = this->geometry();
    geo.setHeight(geo.height() * UI_SCALE);
    this->setGeometry(geo);

    ui->closeButton->setMaximumSize(ui->closeButton->maximumSize() * UI_SCALE);
    ui->closeButton->setMinimumSize(ui->closeButton->minimumSize() * UI_SCALE);

    ui->minimizeButton->setMaximumSize(ui->minimizeButton->maximumSize() * UI_SCALE);
    ui->minimizeButton->setMinimumSize(ui->minimizeButton->minimumSize() * UI_SCALE);

    QFont titleFont = ui->windowTitle->font();
    titleFont.setPointSize(titleFont.pointSize() * UI_SCALE);
    ui->windowTitle->setFont(titleFont);

    ui->decorationL->layout()->setContentsMargins(ui->decorationL->layout()->contentsMargins() * UI_SCALE);
    ui->decorationL->layout()->setSpacing(ui->decorationL->layout()->spacing() * UI_SCALE);

    ui->decorationR->layout()->setContentsMargins(ui->decorationR->layout()->contentsMargins() * UI_SCALE);
    ui->decorationR->layout()->setSpacing(ui->decorationR->layout()->spacing() * UI_SCALE);

    ui->lineLT->setMaximumHeight(ui->lineLT->maximumHeight() * UI_SCALE);
    ui->lineLT->setMinimumHeight(ui->lineLT->minimumHeight() * UI_SCALE);

    ui->lineLB->setMaximumHeight(ui->lineLB->maximumHeight() * UI_SCALE);
    ui->lineLB->setMinimumHeight(ui->lineLB->minimumHeight() * UI_SCALE);

    ui->lineRT->setMaximumHeight(ui->lineRT->maximumHeight() * UI_SCALE);
    ui->lineRT->setMinimumHeight(ui->lineRT->minimumHeight() * UI_SCALE);

    ui->lineRB->setMaximumHeight(ui->lineRB->maximumHeight() * UI_SCALE);
    ui->lineRB->setMinimumHeight(ui->lineRB->minimumHeight() * UI_SCALE);

    QSize sh = ui->horizontalSpacer->sizeHint();
    QSizePolicy sp = ui->horizontalSpacer->sizePolicy();
    ui->horizontalSpacer->changeSize(sh.width()*UI_SCALE, sh.height(), sp.horizontalPolicy(), sp.verticalPolicy());

    QSize sh2 = ui->horizontalSpacer_2->sizeHint();
    QSizePolicy sp2 = ui->horizontalSpacer_2->sizePolicy();
    ui->horizontalSpacer_2->changeSize(sh2.width()*UI_SCALE, sh2.height(), sp2.horizontalPolicy(), sp2.verticalPolicy());

    QSize sh4 = ui->horizontalSpacer_4->sizeHint();
    QSizePolicy sp4 = ui->horizontalSpacer_4->sizePolicy();
    ui->horizontalSpacer_4->changeSize(sh4.width()*UI_SCALE, sh4.height(), sp4.horizontalPolicy(), sp4.verticalPolicy());
}

void TitleBar::mousePressEvent(QMouseEvent *event)
{
    if(event->button() == Qt::LeftButton)
    {
        m_pCursor = event->globalPosition() - window()->geometry().topLeft();
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent *event)
{
    if(event->buttons() & Qt::LeftButton)
    {
        window()->move((event->globalPosition() - m_pCursor).toPoint());
        event->accept();
    }
}
