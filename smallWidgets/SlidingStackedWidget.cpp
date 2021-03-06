#include "SlidingStackedWidget.h"

SlidingStackedWidget::SlidingStackedWidget(QWidget *parent) :
    QStackedWidget(parent)
{
    if (parent!=0)
        m_mainWindow = parent;
    else
        m_mainWindow = this;

    m_vertical = false;
    m_speed = 500;
    m_animationType = QEasingCurve::OutBack;
    m_now = 0;
    m_next = 0;
    m_wrap = false;
    m_pNow = QPoint(0,0);
    m_active = false;
    m_expanded = false;
}


SlidingStackedWidget::~SlidingStackedWidget()
{
}

void SlidingStackedWidget::setVerticalMode(bool vertical)
{
    m_vertical=vertical;
}

void SlidingStackedWidget::setSpeed(int speed)
{
    m_speed = speed;
}

void SlidingStackedWidget::setAnimation(enum QEasingCurve::Type animationType)
{
    m_animationType = animationType;
}

void SlidingStackedWidget::setWrap(bool wrap)
{
    m_wrap=wrap;
}

void SlidingStackedWidget::slideInNext()
{
    int now = currentIndex();
    if (m_wrap || (now < count()-1))
        slideInIdx(now+1);
}


void SlidingStackedWidget::slideInPrev()
{
    int now = currentIndex();
    if (m_wrap || (now>0))
        slideInIdx(now-1);
}

void SlidingStackedWidget::slideInIdx(int idx, enum t_direction direction)
{
    if (idx > count()-1) {
        direction = m_vertical ? TOP2BOTTOM : RIGHT2LEFT;
        idx = (idx)%count();
    } else if (idx < 0) {
        direction = m_vertical ? BOTTOM2TOP: LEFT2RIGHT;
        idx = (idx+count())%count();
    }
    slideInWgt(widget(idx), direction);
}

void SlidingStackedWidget::slideInWgt(QWidget *newWidget, enum t_direction direction)
{
    if (m_active)
        return;
    else
        m_active = true;

    enum t_direction directionHint;
    int now = currentIndex();
    int next = indexOf(newWidget);
    if (now == next) {
        m_active = false;
        return;
    } else if (now < next){
        directionHint=m_vertical ? TOP2BOTTOM : RIGHT2LEFT;
    } else {
        directionHint=m_vertical ? BOTTOM2TOP : LEFT2RIGHT;
    }
    if (direction == AUTOMATIC)
        direction=directionHint;

    int offsetX = frameRect().width();
    int offsetY = frameRect().height();

    widget(next)->setGeometry(0, 0, offsetX, offsetY);

    if (direction == BOTTOM2TOP) {
        offsetX=0;
        offsetY=-offsetY;
    } else if (direction == TOP2BOTTOM) {
        offsetX=0;
    } else if (direction == RIGHT2LEFT) {
        offsetX=-offsetX;
        offsetY=0;
    } else if (direction == LEFT2RIGHT) {
        offsetY=0;
    }

    QPoint pNext = widget(next)->pos();
    QPoint pNow = widget(now)->pos();
    m_pNow = pNow;

    widget(next)->move(pNext.x()-offsetX,pNext.y()-offsetY);
    widget(next)->show();
    widget(next)->raise();

    QPropertyAnimation *animNow = new QPropertyAnimation(widget(now), "pos");

    animNow->setDuration(m_speed);
    animNow->setEasingCurve(m_animationType);
    animNow->setStartValue(QPoint(pNow.x(), pNow.y()));
    animNow->setEndValue(QPoint(offsetX+pNow.x(), offsetY+pNow.y()));
    QPropertyAnimation *animNext = new QPropertyAnimation(widget(next), "pos");
    animNext->setDuration(m_speed);
    animNext->setEasingCurve(m_animationType);
    animNext->setStartValue(QPoint(-offsetX+pNext.x(), offsetY+pNext.y()));
    animNext->setEndValue(QPoint(pNext.x(), pNext.y()));

    QParallelAnimationGroup *animGroup = new QParallelAnimationGroup;

    animGroup->addAnimation(animNow);
    animGroup->addAnimation(animNext);

    QObject::connect(animGroup, SIGNAL(finished()),this,SLOT(animationDoneSlot()));
    m_next = next;
    m_now = now;
    m_active = true;
    animGroup->start();
}

void SlidingStackedWidget::animationDoneSlot(void)
{
    setCurrentIndex(m_next);
    widget(m_now)->hide();
    widget(m_now)->move(m_pNow);
    m_active = false;
    emit animationFinished();
}

void SlidingStackedWidget::expandToOne()
{
    if (m_expanded)
        return;

    m_expanded = true;

    setFixedWidth(frameRect().width()*count()+((count()-1)*24));

    m_widgets.clear();
    for (int i=0, n=count() ; i<n ; ++i)
        m_widgets.append(widget(i));

    foreach (QWidget *widget, m_widgets)
        removeWidget(widget);

    QWidget *pWidget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(pWidget);
    layout->setSpacing(24);
    layout->setContentsMargins(0, 0, 0, 0);

    foreach (QWidget *w, m_widgets) {
        w->setParent(pWidget);
        w->show();
        layout->addWidget(w);
    }
    pWidget->setLayout(layout);
    addWidget(pWidget);
}

void SlidingStackedWidget::collapse()
{
    if (!m_expanded)
        return;

    m_expanded = false;

    QList<QWidget*> widgetsToDelete;
    for (int i=0, n=count() ; i<n ; ++i)
        widgetsToDelete.append(widget(i));

    setFixedWidth((frameRect().width()-((m_widgets.count()-1)*24))/m_widgets.count());

    foreach (QWidget *w, m_widgets)
        addWidget(w);

    foreach (QWidget *w, widgetsToDelete) {
        removeWidget(w);
        w->deleteLater();
    }
}

bool SlidingStackedWidget::isExpanded() const
{
    return m_expanded;
}
