#include "TMDbConcerts.h"

#include <QDebug>
#include <QtScript/QScriptValue>
#include <QtScript/QScriptValueIterator>
#include <QtScript/QScriptEngine>
#include <QSettings>

#include "data/Storage.h"
#include "globals/Globals.h"
#include "globals/Helper.h"

/**
 * @brief TMDbConcerts::TMDbConcerts
 * @param parent
 */
TMDbConcerts::TMDbConcerts(QObject *parent)
{
    setParent(parent);
    m_apiKey = "5d832bdf69dcb884922381ab01548d5b";
    m_language = "en";

    m_scraperSupports << ConcertScraperInfos::Title
                      << ConcertScraperInfos::Tagline
                      << ConcertScraperInfos::Rating
                      << ConcertScraperInfos::Released
                      << ConcertScraperInfos::Runtime
                      << ConcertScraperInfos::Certification
                      << ConcertScraperInfos::Trailer
                      << ConcertScraperInfos::Overview
                      << ConcertScraperInfos::Poster
                      << ConcertScraperInfos::Backdrop
                      << ConcertScraperInfos::Genres
                      << ConcertScraperInfos::ExtraArts;

    m_baseUrl = "http://cf2.imgobject.com/t/p/";
    setup();
}

TMDbConcerts::~TMDbConcerts()
{
}

/**
 * @brief languages
 * @return
 */
QMap<QString, QString> TMDbConcerts::languages()
{
    QMap<QString, QString> m;

    m.insert(tr("Bulgarian"), "bg");
    m.insert(tr("Chinese"), "zh");
    m.insert(tr("Croatian"), "hr");
    m.insert(tr("Czech"), "cs");
    m.insert(tr("Danish"), "da");
    m.insert(tr("Dutch"), "nl");
    m.insert(tr("English"), "en");
    m.insert(tr("Finnish"), "fi");
    m.insert(tr("French"), "fr");
    m.insert(tr("German"), "de");
    m.insert(tr("Greek"), "el");
    m.insert(tr("Hebrew"), "he");
    m.insert(tr("Hungarian"), "hu");
    m.insert(tr("Italian"), "it");
    m.insert(tr("Japanese"), "ja");
    m.insert(tr("Korean"), "ko");
    m.insert(tr("Norwegian"), "no");
    m.insert(tr("Polish"), "pl");
    m.insert(tr("Portuguese"), "pt");
    m.insert(tr("Russian"), "ru");
    m.insert(tr("Slovene"), "sl");
    m.insert(tr("Spanish"), "es");
    m.insert(tr("Swedish"), "sv");
    m.insert(tr("Turkish"), "tr");

    return m;
}

/**
 * @brief language
 * @return
 */
QString TMDbConcerts::language()
{
    return m_language;
}

/**
 * @brief TMDbConcerts::setLanguage
 * @param language
 */
void TMDbConcerts::setLanguage(QString language)
{
    m_language = language;
}

/**
 * @brief Returns the name of the scraper
 * @return Name of the Scraper
 */
QString TMDbConcerts::name()
{
    return QString("The Movie DB (Concerts)");
}

/**
 * @brief Returns if the scraper has settings
 * @return Scraper has settings
 */
bool TMDbConcerts::hasSettings()
{
    return true;
}

/**
 * @brief Loads scrapers settings
 */
void TMDbConcerts::loadSettings()
{
    QSettings settings;
    m_language = settings.value("Scrapers/TMDbConcerts/Language", "en").toString();
}

/**
 * @brief Saves scrapers settings
 */
void TMDbConcerts::saveSettings()
{
    QSettings settings;
    settings.setValue("Scrapers/TMDbConcerts/Language", m_language);
}

/**
 * @brief Just returns a pointer to the scrapers network access manager
 * @return Network Access Manager
 */
QNetworkAccessManager *TMDbConcerts::qnam()
{
    return &m_qnam;
}

/**
 * @brief Returns a list of infos available from the scraper
 * @return List of supported infos
 */
QList<int> TMDbConcerts::scraperSupports()
{
    return m_scraperSupports;
}

/**
 * @brief Loads the setup parameters from TMDb
 * @see TMDbConcerts::setupFinished
 */
void TMDbConcerts::setup()
{
    QUrl url(QString("http://api.themoviedb.org/3/configuration?api_key=%1").arg(m_apiKey));
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    QNetworkReply *reply = qnam()->get(request);
    connect(reply, SIGNAL(finished()), this, SLOT(setupFinished()));
}

/**
 * @brief Called when setup parameters were got
 *        Parses json and assigns the baseUrl
 */
void TMDbConcerts::setupFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    if (reply->error() != QNetworkReply::NoError ) {
        reply->deleteLater();
        return;
    }
    QString msg = QString::fromUtf8(reply->readAll());
    reply->deleteLater();
    QScriptValue sc;
    QScriptEngine engine;
    sc = engine.evaluate("(" + QString(msg) + ")");

    m_baseUrl = sc.property("images").property("base_url").toString();
}

/**
 * @brief Searches for a concert
 * @param searchStr The Concert name/search string
 * @see TMDbConcerts::searchFinished
 */
void TMDbConcerts::search(QString searchStr)
{
    qDebug() << "Entered, searchStr=" << searchStr;
    searchStr = QUrl::toPercentEncoding(searchStr);
    QUrl url(QString("http://api.themoviedb.org/3/search/movie?api_key=%1&language=%2&query=%3").arg(m_apiKey).arg(m_language).arg(searchStr));
    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");
    QNetworkReply *reply = qnam()->get(request);
    reply->setProperty("searchString", searchStr);
    reply->setProperty("results", Storage::toVariant(reply, QList<ScraperSearchResult>()));
    connect(reply, SIGNAL(finished()), this, SLOT(searchFinished()));
}

/**
 * @brief Called when the search result was downloaded
 *        Emits "searchDone" if there are no more pages in the result set
 * @see TMDbConcerts::parseSearch
 */
void TMDbConcerts::searchFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    QList<ScraperSearchResult> results = reply->property("results").value<Storage*>()->results();

    if (reply->error() != QNetworkReply::NoError ) {
        qWarning() << "Network Error" << reply->errorString();
        reply->deleteLater();
        emit searchDone(results);
        return;
    }

    QString searchString = reply->property("searchString").toString();
    QString msg = QString::fromUtf8(reply->readAll());
    int nextPage = -1;
    results.append(parseSearch(msg, &nextPage));
    reply->deleteLater();

    if (nextPage == -1) {
        emit searchDone(results);
    } else {
        QUrl url(QString("http://api.themoviedb.org/3/search/movie?api_key=%1&language=%2&page=%3&query=%4").arg(m_apiKey).arg(m_language).arg(nextPage).arg(searchString));
        QNetworkRequest request(url);
        request.setRawHeader("Accept", "application/json");
        QNetworkReply *reply = qnam()->get(request);
        reply->setProperty("searchString", searchString);
        reply->setProperty("results", Storage::toVariant(reply, results));
        connect(reply, SIGNAL(finished()), this, SLOT(searchFinished()));
    }
}

/**
 * @brief Parses the JSON search results
 * @param json JSON string
 * @param nextPage This will hold the next page to get, -1 if there are no more pages
 * @return List of search results
 */
QList<ScraperSearchResult> TMDbConcerts::parseSearch(QString json, int *nextPage)
{
    qDebug() << "Entered";
    QList<ScraperSearchResult> results;
    QScriptValue sc;
    QScriptEngine engine;
    sc = engine.evaluate("(" + QString(json) + ")");

    // only get the first 3 pages
    if (sc.property("page").toInteger() < sc.property("total_pages").toInteger() && sc.property("page").toInteger() < 3)
        *nextPage = sc.property("page").toInteger()+1;

    if (sc.property("results").isArray() ) {
        QScriptValueIterator it(sc.property("results"));
        while (it.hasNext() ) {
            it.next();
            if (it.value().property("id").toString().isEmpty()) {
                continue;
            }
            ScraperSearchResult result;
            result.name     = it.value().property("title").toString();
            if (result.name.isEmpty())
                it.value().property("original_title").toString();
            result.id       = it.value().property("id").toString();
            result.released = QDate::fromString(it.value().property("release_date").toString(), "yyyy-MM-dd");
            results.append(result);
        }
    }

    return results;
}

/**
 * @brief Starts network requests to download infos from TMDb
 * @param id TMDb movie ID
 * @param concert Concert object
 * @param infos List of infos to load
 * @see TMDbConcerts::loadFinished
 * @see TMDbConcerts::loadCastsFinished
 * @see TMDbConcerts::loadTrailersFinished
 * @see TMDbConcerts::loadImagesFinished
 * @see TMDbConcerts::loadReleasesFinished
 */
void TMDbConcerts::loadData(QString id, Concert *concert, QList<int> infos)
{
    qDebug() << "Entered, id=" << id << "concert=" << concert->name();
    concert->setTmdbId(id);
    concert->clear(infos);

    QUrl url;
    QNetworkRequest request;
    request.setRawHeader("Accept", "application/json");

    QList<ScraperData> loadsLeft;

    // Infos
    loadsLeft.append(DataInfos);
    url.setUrl(QString("http://api.themoviedb.org/3/movie/%1?api_key=%2&language=%3").arg(id).arg(m_apiKey).arg(m_language));
    request.setUrl(url);
    QNetworkReply *reply = qnam()->get(QNetworkRequest(request));
    reply->setProperty("storage", Storage::toVariant(reply, concert));
    reply->setProperty("infosToLoad", Storage::toVariant(reply, infos));
    connect(reply, SIGNAL(finished()), this, SLOT(loadFinished()));

    // Trailers
    if (infos.contains(ConcertScraperInfos::Trailer)) {
        loadsLeft.append(DataTrailers);
        url.setUrl(QString("http://api.themoviedb.org/3/movie/%1/trailers?api_key=%2").arg(id).arg(m_apiKey));
        request.setUrl(url);
        QNetworkReply *reply = qnam()->get(QNetworkRequest(request));
        reply->setProperty("storage", Storage::toVariant(reply, concert));
        reply->setProperty("infosToLoad", Storage::toVariant(reply, infos));
        connect(reply, SIGNAL(finished()), this, SLOT(loadTrailersFinished()));
    }

    // Images
    if (infos.contains(ConcertScraperInfos::Poster) || infos.contains(ConcertScraperInfos::Backdrop)) {
        loadsLeft.append(DataImages);
        url.setUrl(QString("http://api.themoviedb.org/3/movie/%1/images?api_key=%2").arg(id).arg(m_apiKey));
        request.setUrl(url);
        QNetworkReply *reply = qnam()->get(QNetworkRequest(request));
        reply->setProperty("storage", Storage::toVariant(reply, concert));
        reply->setProperty("infosToLoad", Storage::toVariant(reply, infos));
        connect(reply, SIGNAL(finished()), this, SLOT(loadImagesFinished()));
    }

    // Releases
    if (infos.contains(ConcertScraperInfos::Certification)) {
        loadsLeft.append(DataReleases);
        url.setUrl(QString("http://api.themoviedb.org/3/movie/%1/releases?api_key=%2").arg(id).arg(m_apiKey));
        request.setUrl(url);
        QNetworkReply *reply = qnam()->get(QNetworkRequest(request));
        reply->setProperty("storage", Storage::toVariant(reply, concert));
        reply->setProperty("infosToLoad", Storage::toVariant(reply, infos));
        connect(reply, SIGNAL(finished()), this, SLOT(loadReleasesFinished()));
    }
    concert->setLoadsLeft(loadsLeft);
}

/**
 * @brief Called when the concert infos are downloaded
 * @see TMDbConcerts::parseAndAssignInfos
 */
void TMDbConcerts::loadFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    Concert *concert = reply->property("storage").value<Storage*>()->concert();
    QList<int> infos = reply->property("infosToLoad").value<Storage*>()->infosToLoad();
    reply->deleteLater();
    if (!concert)
        return;

    if (reply->error() == QNetworkReply::NoError ) {
        QString msg = QString::fromUtf8(reply->readAll());
        parseAndAssignInfos(msg, concert, infos);
    } else {
        qWarning() << "Network Error (load)" << reply->errorString();
    }
    concert->removeFromLoadsLeft(DataInfos);
}

/**
 * @brief Called when the concert trailers are downloaded
 * @see TMDbConcerts::parseAndAssignInfos
 */
void TMDbConcerts::loadTrailersFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    Concert *concert = reply->property("storage").value<Storage*>()->concert();
    QList<int> infos = reply->property("infosToLoad").value<Storage*>()->infosToLoad();
    reply->deleteLater();
    if (!concert)
        return;

    if (reply->error() == QNetworkReply::NoError ) {
        QString msg = QString::fromUtf8(reply->readAll());
        parseAndAssignInfos(msg, concert, infos);
    } else {
        qDebug() << "Network Error (trailers)" << reply->errorString();
    }
    concert->removeFromLoadsLeft(DataTrailers);
}

/**
 * @brief Called when the concert images are downloaded
 * @see TMDbConcerts::parseAndAssignInfos
 */
void TMDbConcerts::loadImagesFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    Concert *concert = reply->property("storage").value<Storage*>()->concert();
    QList<int> infos = reply->property("infosToLoad").value<Storage*>()->infosToLoad();
    reply->deleteLater();
    if (!concert)
        return;

    if (reply->error() == QNetworkReply::NoError ) {
        QString msg = QString::fromUtf8(reply->readAll());
        parseAndAssignInfos(msg, concert, infos);
    } else {
        qWarning() << "Network Error (images)" << reply->errorString();
    }
    concert->removeFromLoadsLeft(DataImages);
}

/**
 * @brief Called when the concert releases are downloaded
 * @see TMDbConcerts::parseAndAssignInfos
 */
void TMDbConcerts::loadReleasesFinished()
{
    QNetworkReply *reply = static_cast<QNetworkReply*>(QObject::sender());
    Concert *concert = reply->property("storage").value<Storage*>()->concert();
    QList<int> infos = reply->property("infosToLoad").value<Storage*>()->infosToLoad();
    reply->deleteLater();
    if (!concert)
        return;

    if (reply->error() == QNetworkReply::NoError ) {
        QString msg = QString::fromUtf8(reply->readAll());
        parseAndAssignInfos(msg, concert, infos);
    } else {
        qWarning() << "Network Error (releases)" << reply->errorString();
    }
    concert->removeFromLoadsLeft(DataReleases);
}

/**
 * @brief Parses JSON data and assigns it to the given concert object
 *        Handles all types of data from TMDb (info, releases, trailers, images)
 * @param json JSON data
 * @param concert Concert object
 * @param infos List of infos to load
 */
void TMDbConcerts::parseAndAssignInfos(QString json, Concert *concert, QList<int> infos)
{
    qDebug() << "Entered";
    QScriptValue sc;
    QScriptEngine engine;
    sc = engine.evaluate("(" + QString(json) + ")");

    // Infos
    if (sc.property("imdb_id").isValid() && !sc.property("imdb_id").toString().isEmpty())
        concert->setId(sc.property("imdb_id").toString());
    if (infos.contains(ConcertScraperInfos::Title) && sc.property("title").isValid())
        concert->setName(sc.property("title").toString());
    if (infos.contains(ConcertScraperInfos::Overview) && sc.property("overview").isValid() && !sc.property("overview").isNull())
        concert->setOverview(sc.property("overview").toString());
    if (infos.contains(ConcertScraperInfos::Rating) && sc.property("vote_average").isValid())
        concert->setRating(sc.property("vote_average").toNumber());
    if (infos.contains(ConcertScraperInfos::Tagline) && sc.property("tagline").isValid() && !sc.property("tagline").isNull())
        concert->setTagline(sc.property("tagline").toString());
    if (infos.contains(ConcertScraperInfos::Released) && sc.property("release_date").isValid())
        concert->setReleased(QDate::fromString(sc.property("release_date").toString(), "yyyy-MM-dd"));
    if (infos.contains(ConcertScraperInfos::Runtime) && sc.property("runtime").isValid())
        concert->setRuntime(sc.property("runtime").toInteger());
    if (infos.contains(ConcertScraperInfos::Genres) && sc.property("genres").isArray()) {
        QScriptValueIterator itC(sc.property("genres"));
        while (itC.hasNext()) {
            itC.next();
            QScriptValue vC = itC.value();
            if (vC.property("id").toString().isEmpty())
                continue;
            concert->addGenre(vC.property("name").toString());
        }
    }

    // Trailers
    if (infos.contains(ConcertScraperInfos::Trailer) && sc.property("youtube").isArray()) {
        QScriptValueIterator itC(sc.property("youtube"));
        while (itC.hasNext()) {
            itC.next();
            QScriptValue vC = itC.value();
            if (vC.property("source").toString().isEmpty())
                continue;
            concert->setTrailer(QUrl(Helper::formatTrailerUrl(QString("http://www.youtube.com/watch?v=%1").arg(vC.property("source").toString()))));
            break;
        }
    }

    // Images
    if (infos.contains(ConcertScraperInfos::Backdrop) && sc.property("backdrops").isArray()) {
        QScriptValueIterator itB(sc.property("backdrops"));
        while (itB.hasNext()) {
            itB.next();
            QScriptValue vB = itB.value();
            if (vB.property("file_path").toString().isEmpty())
                continue;
            Poster b;
            b.thumbUrl = m_baseUrl + "w780" + vB.property("file_path").toString();
            b.originalUrl = m_baseUrl + "original" + vB.property("file_path").toString();
            b.originalSize.setWidth(vB.property("width").toString().toInt());
            b.originalSize.setHeight(vB.property("height").toString().toInt());
            concert->addBackdrop(b);
        }
    }

    if (infos.contains(ConcertScraperInfos::Poster) && sc.property("posters").isArray()) {
        QScriptValueIterator itB(sc.property("posters"));
        while (itB.hasNext()) {
            itB.next();
            QScriptValue vB = itB.value();
            if (vB.property("file_path").toString().isEmpty())
                continue;
            Poster b;
            b.thumbUrl = m_baseUrl + "w342" + vB.property("file_path").toString();
            b.originalUrl = m_baseUrl + "original" + vB.property("file_path").toString();
            b.originalSize.setWidth(vB.property("width").toString().toInt());
            b.originalSize.setHeight(vB.property("height").toString().toInt());
            concert->addPoster(b);
        }
    }

    // Releases
    if (infos.contains(ConcertScraperInfos::Certification) && sc.property("countries").isArray()) {
        QString locale;
        QString us;
        QString gb;
        QScriptValueIterator itB(sc.property("countries"));
        while (itB.hasNext()) {
            itB.next();
            QScriptValue vB = itB.value();
            if (vB.property("iso_3166_1").toString() == "US")
                us = vB.property("certification").toString();
            if (vB.property("iso_3166_1").toString() == "GB")
                gb = vB.property("certification").toString();
            if (vB.property("iso_3166_1").toString().toLower() == m_language)
                locale = vB.property("certification").toString();
        }
        if (!locale.isEmpty())
            concert->setCertification(locale);
        else if (!us.isEmpty())
            concert->setCertification(us);
        else if (!gb.isEmpty())
            concert->setCertification(gb);
    }

}
