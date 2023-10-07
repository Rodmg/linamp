#include "titlebar.h"
#include "ui_titlebar.h"
#include <QMouseEvent>

TitleBar::TitleBar(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::TitleBar)
{
    ui->setupUi(this);
    connect(ui->closeButton, &QPushButton::clicked, window(), &QWidget::close);
    connect(ui->minimizeButton, &QPushButton::clicked, window(), &QWidget::showMinimized);
}

TitleBar::~TitleBar()
{
    delete ui;
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
