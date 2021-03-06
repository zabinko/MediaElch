#ifndef TVSHOWFILESWIDGET_H
#define TVSHOWFILESWIDGET_H

#include <QMenu>
#include <QModelIndex>
#include <QWidget>
#include "globals/Globals.h"
#include "data/TvShow.h"
#include "data/TvShowEpisode.h"
#include "data/TvShowProxyModel.h"
#include "data/TvShowDelegate.h"
#include "globals/Filter.h"

namespace Ui {
class TvShowFilesWidget;
}

/**
 * @brief The TvShowFilesWidget class
 */
class TvShowFilesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TvShowFilesWidget(QWidget *parent = 0);
    ~TvShowFilesWidget();
    void setFilter(QList<Filter*> filters, QString text);
    static TvShowFilesWidget *instance();
    QList<TvShowEpisode*> selectedEpisodes();
    QList<TvShow*> selectedShows();

public slots:
    void renewModel();
    void emitLastSelection();

signals:
    void sigEpisodeSelected(TvShowEpisode *episode);
    void sigTvShowSelected(TvShow *show);
    void sigSeasonSelected(TvShow *show, int season);
    void sigNothingSelected();

private slots:
    void onItemActivated(QModelIndex index, QModelIndex previous);
    void onItemClicked(QModelIndex index);
    void showContextMenu(QPoint point);
    void scanForEpisodes();
    void markAsWatched();
    void markAsUnwatched();
    void loadStreamDetails();
    void markForSync();
    void unmarkForSync();
    void openFolder();

private:
    Ui::TvShowFilesWidget *ui;
    TvShowProxyModel *m_tvShowProxyModel;
    TvShowDelegate *m_tvShowDelegate;
    static TvShowFilesWidget *m_instance;
    QMenu *m_contextMenu;
    TvShow *m_lastTvShow;
    TvShowEpisode *m_lastEpisode;
    int m_lastSeason;
};

#endif // TVSHOWFILESWIDGET_H
