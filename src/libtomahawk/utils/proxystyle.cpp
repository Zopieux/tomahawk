#include "proxystyle.h"

#include <QDebug>
#include <QPainter>
#include <QSplitter>
#include <QStyleOption>
#include <QWidget>


void
ProxyStyle::drawPrimitive( PrimitiveElement pe, const QStyleOption* opt, QPainter* p, const QWidget* w ) const
{
    if ( pe != PE_FrameStatusBar )
        QProxyStyle::drawPrimitive( pe, opt, p, w );
}


void
ProxyStyle::drawControl( ControlElement ce, const QStyleOption* opt, QPainter* p, const QWidget* w ) const
{
    if ( ce == CE_Splitter )
    {
        const QSplitter* splitter = qobject_cast< const QSplitter* >( w );
        if ( !splitter->sizes().contains( 0 ) )
        {
            p->setPen( QColor( 0x8c, 0x8c, 0x8c ) );
            p->drawLine( opt->rect.topLeft(), opt->rect.bottomRight() );
        }
    }
    else
        QProxyStyle::drawControl( ce, opt, p, w );
}
