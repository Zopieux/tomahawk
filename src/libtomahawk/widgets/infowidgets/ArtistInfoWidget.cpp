/* === This file is part of Tomahawk Player - <http://tomahawk-player.org> ===
 *
 *   Copyright 2010-2011, Christian Muehlhaeuser <muesli@tomahawk-player.org>
 *   Copyright 2010-2011, Jeff Mitchell <jeff@tomahawk-player.org>
 *
 *   Tomahawk is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   Tomahawk is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with Tomahawk. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ArtistInfoWidget.h"
#include "ArtistInfoWidget_p.h"
#include "ui_ArtistInfoWidget.h"

#include <QScrollArea>
#include <QPainter>
#include <QEvent>

#include "audio/AudioEngine.h"
#include "playlist/PlayableModel.h"
#include "playlist/TreeModel.h"
#include "playlist/PlaylistModel.h"
#include "playlist/TreeProxyModel.h"
#include "Source.h"

#include "database/DatabaseCommand_AllTracks.h"
#include "database/DatabaseCommand_AllAlbums.h"

#include "Pipeline.h"
#include "utils/StyleHelper.h"
#include "utils/TomahawkUtilsGui.h"
#include "utils/Logger.h"

using namespace Tomahawk;


ArtistInfoWidget::ArtistInfoWidget( const Tomahawk::artist_ptr& artist, QWidget* parent )
    : QWidget( parent )
    , ui( new Ui::ArtistInfoWidget )
    , m_artist( artist )
{
    m_mainWidget = new QWidget;
    ui->setupUi( m_mainWidget );

    QPalette pal = palette();
    pal.setColor( QPalette::WindowText, QColor( "#949494" ) );

    m_mainWidget->setPalette( pal );
    m_mainWidget->setAutoFillBackground( true );
    m_mainWidget->installEventFilter( this );

    m_plInterface = Tomahawk::playlistinterface_ptr( new MetaPlaylistInterface( this ) );

/*    TomahawkUtils::unmarginLayout( ui->layoutWidget->layout() );
    TomahawkUtils::unmarginLayout( ui->layoutWidget1->layout() );
    TomahawkUtils::unmarginLayout( ui->layoutWidget2->layout() );
    TomahawkUtils::unmarginLayout( ui->albumHeader->layout() );*/

    m_albumsModel = new PlayableModel( ui->albums );
    ui->albums->setPlayableModel( m_albumsModel );
    ui->topHits->setEmptyTip( tr( "Sorry, we could not find any albums for this artist!" ) );

    m_relatedModel = new PlayableModel( ui->relatedArtists );
    ui->relatedArtists->setPlayableModel( m_relatedModel );
    ui->relatedArtists->proxyModel()->sort( -1 );
    ui->topHits->setEmptyTip( tr( "Sorry, we could not find any related artists!" ) );

    m_topHitsModel = new PlaylistModel( ui->topHits );
    m_topHitsModel->setStyle( PlayableModel::Short );
    ui->topHits->setPlayableModel( m_topHitsModel );
    ui->topHits->setSortingEnabled( false );
    ui->topHits->setEmptyTip( tr( "Sorry, we could not find any top hits for this artist!" ) );

    ui->relatedArtists->setAutoFitItems( false );
    ui->relatedArtists->setWrapping( false );
    ui->relatedArtists->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    ui->relatedArtists->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    m_relatedModel->setItemSize( QSize( 170, 170 ) );
    ui->albums->setAutoFitItems( false );
    ui->albums->setWrapping( false );
    ui->albums->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    ui->albums->setHorizontalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    m_albumsModel->setItemSize( QSize( 170, 170 ) );

    ui->topHits->setFrameShape( QFrame::StyledPanel );
    ui->topHits->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    m_pixmap = TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::ScaledCover, QSize( 48, 48 ) );
    ui->cover->setPixmap( TomahawkUtils::defaultPixmap( TomahawkUtils::DefaultArtistImage, TomahawkUtils::ScaledCover, QSize( ui->cover->sizeHint() ) ) );

    connect( m_albumsModel, SIGNAL( loadingStarted() ), SLOT( onLoadingStarted() ) );
    connect( m_albumsModel, SIGNAL( loadingFinished() ), SLOT( onLoadingFinished() ) );

    ui->biography->setStyleSheet( "QTextBrowser#biography { background-color: transparent; }" );
    ui->biography->setFrameShape( QFrame::NoFrame );
    ui->biography->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    QPalette p = ui->biography->palette();
    p.setColor( QPalette::Foreground, QColor( "#252525" ) );
    p.setColor( QPalette::Text, QColor( "#252525" ) );

    ui->biography->setPalette( p );
    ui->label->setPalette( p );
    ui->label_2->setPalette( p );
    ui->label_3->setPalette( p );
    
    ui->label->setContentsMargins( 0, 0, 0, 0 );
    ui->label_2->setContentsMargins( 0, 16, 0, 0 );
    ui->label_3->setContentsMargins( 0, 16, 0, 0 );

    QScrollArea* area = new QScrollArea();
    area->setWidgetResizable( true );
    area->setWidget( m_mainWidget );

    area->setStyleSheet( "QScrollArea { background-color: #323435; }" );
    area->setFrameShape( QFrame::NoFrame );
    area->setAttribute( Qt::WA_MacShowFocusRect, 0 );

    QVBoxLayout* layout = new QVBoxLayout();
    layout->addWidget( area );
    setLayout( layout );
    TomahawkUtils::unmarginLayout( layout );

    m_bgTile = TomahawkUtils::createTiledBackground( RESPATH "images/artistpage-background-tile.png", m_mainWidget->width(), m_mainWidget->height() );

    load( artist );
}


ArtistInfoWidget::~ArtistInfoWidget()
{
    delete ui;
}


Tomahawk::playlistinterface_ptr
ArtistInfoWidget::playlistInterface() const
{
    return m_plInterface;
}


void
ArtistInfoWidget::onLoadingStarted()
{
}


void
ArtistInfoWidget::onLoadingFinished()
{
}


bool
ArtistInfoWidget::isBeingPlayed() const
{
    if ( ui->albums->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( ui->relatedArtists->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    if ( ui->topHits->playlistInterface() == AudioEngine::instance()->currentTrackPlaylist() )
        return true;

    return false;
}


bool
ArtistInfoWidget::jumpToCurrentTrack()
{
    if ( ui->albums->jumpToCurrentTrack() )
        return true;

    if ( ui->relatedArtists->jumpToCurrentTrack() )
        return true;

    if ( ui->topHits->jumpToCurrentTrack() )
        return true;

    return false;
}


void
ArtistInfoWidget::load( const artist_ptr& artist )
{
    if ( !m_artist.isNull() )
    {
        disconnect( m_artist.data(), SIGNAL( updated() ), this, SLOT( onArtistImageUpdated() ) );
        disconnect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), this, SLOT( onSimilarArtistsLoaded() ) );
        disconnect( m_artist.data(), SIGNAL( biographyLoaded() ), this, SLOT( onBiographyLoaded() ) );
        disconnect( m_artist.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ),
                    this,              SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ) );
        disconnect( m_artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                    this,              SLOT( onTracksFound( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode ) ) );
    }

    m_artist = artist;
    m_title = artist->name();

    connect( m_artist.data(), SIGNAL( biographyLoaded() ), SLOT( onBiographyLoaded() ) );
    connect( m_artist.data(), SIGNAL( similarArtistsLoaded() ), SLOT( onSimilarArtistsLoaded() ) );
    connect( m_artist.data(), SIGNAL( updated() ), SLOT( onArtistImageUpdated() ) );
    connect( m_artist.data(), SIGNAL( albumsAdded( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ),
                                SLOT( onAlbumsFound( QList<Tomahawk::album_ptr>, Tomahawk::ModelMode ) ) );
    connect( m_artist.data(), SIGNAL( tracksAdded( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode, Tomahawk::collection_ptr ) ),
                                SLOT( onTracksFound( QList<Tomahawk::query_ptr>, Tomahawk::ModelMode ) ) );

    if ( !m_artist->albums( Mixed ).isEmpty() )
        onAlbumsFound( m_artist->albums( Mixed ), Mixed );
    
    if ( !m_artist->tracks().isEmpty() )
        onTracksFound( m_artist->tracks(), Mixed );
    
    if ( !m_artist->similarArtists().isEmpty() )
        onSimilarArtistsLoaded();
    
    if ( !m_artist->biography().isEmpty() )
        onBiographyLoaded();

    onArtistImageUpdated();
}


void
ArtistInfoWidget::onAlbumsFound( const QList<Tomahawk::album_ptr>& albums, ModelMode mode )
{
    Q_UNUSED( mode );

    m_albumsModel->append( albums );
}


void
ArtistInfoWidget::onTracksFound( const QList<Tomahawk::query_ptr>& queries, ModelMode mode )
{
    Q_UNUSED( mode );

    m_topHitsModel->append( queries );
}


void
ArtistInfoWidget::onSimilarArtistsLoaded()
{
    m_relatedModel->append( m_artist->similarArtists() );
}


void
ArtistInfoWidget::onBiographyLoaded()
{
    m_longDescription = m_artist->biography();
    emit longDescriptionChanged( m_longDescription );
    
    ui->biography->setHtml( m_artist->biography() );
}


void
ArtistInfoWidget::onArtistImageUpdated()
{
    if ( m_artist->cover( QSize( 0, 0 ) ).isNull() )
        return;

    m_pixmap = m_artist->cover( QSize( 0, 0 ) );
    emit pixmapChanged( m_pixmap );
    
    ui->cover->setPixmap( m_artist->cover( ui->cover->sizeHint() ) );
}


void
ArtistInfoWidget::changeEvent( QEvent* e )
{
    QWidget::changeEvent( e );
    switch ( e->type() )
    {
        case QEvent::LanguageChange:
            ui->retranslateUi( this );
            break;

        default:
            break;
    }
}


bool
ArtistInfoWidget::eventFilter( QObject* obj, QEvent* event )
{
    if ( obj == m_mainWidget && event->type() == QEvent::Paint )
    {
        if ( m_bgTile.isNull() || m_mainWidget->width() > m_bgTile.width() || m_mainWidget->height() > m_bgTile.height() )
            m_bgTile = TomahawkUtils::createTiledBackground( RESPATH "images/artistpage-background-tile.png", m_mainWidget->width() * 1.1, m_mainWidget->height() * 1.1 );

        if ( m_bgTile.isNull() )
            return false;

        QPainter p( m_mainWidget );

        // Truncate bg pixmap and paint into bg
        p.drawPixmap( m_mainWidget->rect(), m_bgTile, m_mainWidget->rect() );
    }

    return false;
}
