/*
 * Copyright 2014  Martin Gräßlin <mgraesslin@kde.org>
 * Copyright 2014  Hugo Pereira Da Costa <hugo.pereira@free.fr>
 * Copyright 2017  Igor Shovkun <igshov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "breezebutton.h"

#include <KDecoration2/DecoratedClient>
#include <KColorUtils>

#include <QPainter>

namespace SierraBreeze
{

    using KDecoration2::ColorRole;
    using KDecoration2::ColorGroup;
    using KDecoration2::DecorationButtonType;


    //__________________________________________________________________
    Button::Button(DecorationButtonType type, Decoration* decoration, QObject* parent)
        : DecorationButton(type, decoration, parent)
        , m_animation( new QPropertyAnimation( this ) )
    {

        // setup animation
        m_animation->setStartValue( 0 );
        m_animation->setEndValue( 1.0 );
        m_animation->setTargetObject( this );
        m_animation->setPropertyName( "opacity" );
        m_animation->setEasingCurve( QEasingCurve::InOutQuad );

        // setup default geometry
        const int height = decoration->buttonHeight();
        setGeometry(QRect(0, 0, height, height));
        setIconSize(QSize( height, height ));

        // connections
        connect(decoration->client().data(), SIGNAL(iconChanged(QIcon)), this, SLOT(update()));
        connect(decoration->settings().data(), &KDecoration2::DecorationSettings::reconfigured, this, &Button::reconfigure);
        connect( this, &KDecoration2::DecorationButton::hoveredChanged, this, &Button::updateAnimationState );

        reconfigure();

    }

    //__________________________________________________________________
    Button::Button(QObject *parent, const QVariantList &args)
        : DecorationButton(args.at(0).value<DecorationButtonType>(), args.at(1).value<Decoration*>(), parent)
        , m_flag(FlagStandalone)
        , m_animation( new QPropertyAnimation( this ) )
    {}

    //__________________________________________________________________
    Button *Button::create(DecorationButtonType type, KDecoration2::Decoration *decoration, QObject *parent)
    {
        if (auto d = qobject_cast<Decoration*>(decoration))
        {
            Button *b = new Button(type, d, parent);
            switch( type )
            {

                case DecorationButtonType::Close:
                b->setVisible( d->client().data()->isCloseable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::closeableChanged, b, &SierraBreeze::Button::setVisible );
                break;

                case DecorationButtonType::Maximize:
                b->setVisible( d->client().data()->isMaximizeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::maximizeableChanged, b, &SierraBreeze::Button::setVisible );
                break;

                case DecorationButtonType::Minimize:
                b->setVisible( d->client().data()->isMinimizeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::minimizeableChanged, b, &SierraBreeze::Button::setVisible );
                break;

                case DecorationButtonType::ContextHelp:
                b->setVisible( d->client().data()->providesContextHelp() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::providesContextHelpChanged, b, &SierraBreeze::Button::setVisible );
                break;

                case DecorationButtonType::Shade:
                b->setVisible( d->client().data()->isShadeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::shadeableChanged, b, &SierraBreeze::Button::setVisible );
                break;

                case DecorationButtonType::Menu:
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::iconChanged, b, [b]() { b->update(); });
                break;

                default: break;

            }

            return b;
        }

        return nullptr;

    }

    //__________________________________________________________________
    void Button::paint(QPainter *painter, const QRect &repaintRegion)
    {
        Q_UNUSED(repaintRegion)

        if (!decoration()) return;

        painter->save();

        // translate from offset
        if( m_flag == FlagFirstInList ) painter->translate( m_offset );
        else painter->translate( 0, m_offset.y() );

        if( !m_iconSize.isValid() ) m_iconSize = geometry().size().toSize();

        // menu button
        if (type() == DecorationButtonType::Menu)
        {

            const QRectF iconRect( geometry().topLeft(), m_iconSize );
            const QPixmap pixmap = decoration()->client().data()->icon().pixmap( m_iconSize );
            painter->drawPixmap(iconRect.center() - QPoint(pixmap.width()/2, pixmap.height()/2)/pixmap.devicePixelRatio(), pixmap);

        } else {

            drawIcon( painter );

        }

        painter->restore();

    }

    //__________________________________________________________________
    void Button::drawIcon( QPainter *painter ) const
    {

        painter->setRenderHints( QPainter::Antialiasing );

        /*
        scale painter so that its window matches QRect( -1, -1, 20, 20 )
        this makes all further rendering and scaling simpler
        all further rendering is preformed inside QRect( 0, 0, 18, 18 )
        */
        painter->translate( geometry().topLeft() );

        const qreal width( m_iconSize.width() );
        painter->scale( width/20, width/20 );
        painter->translate( 1, 1 );

        // render background
        const QColor backgroundColor( this->backgroundColor() );
        if( backgroundColor.isValid() )
        {
            painter->setPen( Qt::NoPen );
            painter->setBrush( backgroundColor );
            painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
        }

        // render mark
        const QColor foregroundColor( this->foregroundColor() );
        if( foregroundColor.isValid() )
        {

            // setup painter
            QPen pen( foregroundColor );
            pen.setCapStyle( Qt::RoundCap );
            pen.setJoinStyle( Qt::MiterJoin );
            pen.setWidthF( 1.1*qMax((qreal)1.0, 20/width ) );

            painter->setPen( pen );
            painter->setBrush( Qt::NoBrush );
            // auto d = qobject_cast<Decoration*>( decoration() );
            // auto c = d->client().data();

            const auto hover_hint_color = QColor(41, 43, 50, 200);
            QPen hint_pen(hover_hint_color);
            hint_pen.setCapStyle( Qt::RoundCap );
            hint_pen.setJoinStyle( Qt::MiterJoin );
            hint_pen.setWidthF( 1.5*qMax((qreal)1.0, 20/width ) );

            switch( type() )
            {

                case DecorationButtonType::Close:
                {
                    painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 12 ) );
                    painter->drawLine( 12, 6, 6, 12 );
                    break;
                }

                case DecorationButtonType::Maximize:
                {
                    if ( isChecked() )
                    {
                      painter->drawPolyline( QPolygonF()
                          << QPointF( 5, 8 )
                          << QPointF( 9, 12 )
                          << QPointF( 13, 8 ) );
                    } else {
                      painter->drawPolyline( QPolygonF()
                          << QPointF( 5, 10 )
                          << QPointF( 9, 6 )
                          << QPointF( 13, 10 ) );
                    }
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                    painter->drawLine( QLineF( 5, 9, 13, 9) );
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                    if ( isChecked() || (isChecked() && isHovered()) )
                    {
                      painter->setPen( QColor(255, 255, 255, 220) );
                      painter->drawEllipse(5, 5, 8, 8);
                    }
                    else if ( isHovered() )
                    {
                      painter->setPen( QColor(0, 0, 0, 100) );
                      painter->drawEllipse(4, 4, 9, 9);
                    }
                    else
                    {
                      painter->setPen( QColor(0, 0, 0, 0) );
                      painter->drawEllipse(4, 4, 9, 9);
                    }
                    break;
                }

                case DecorationButtonType::Shade:
                {
                    if (isChecked())
                    {
                        painter->drawLine( 5, 6, 13, 6 );
                        painter->drawPolyline( QPolygonF()
                            << QPointF( 5, 9 )
                            << QPointF( 9, 13 )
                            << QPointF( 13, 9 ) );
                    } else {
                        painter->drawLine( 5, 6, 13, 6 );
                        painter->drawPolyline( QPolygonF()
                            << QPointF( 5, 13 )
                            << QPointF( 9, 9 )
                            << QPointF( 13, 13 ) );
                    }
                    break;
                }

                case DecorationButtonType::KeepBelow:
                {
                    painter->drawPolyline( QPolygonF()
                        << QPointF( 5, 5 )
                        << QPointF( 9, 9 )
                        << QPointF( 13, 5 ) );

                    painter->drawPolyline( QPolygonF()
                        << QPointF( 5, 9 )
                        << QPointF( 9, 13 )
                        << QPointF( 13, 9 ) );
                    break;

                }

                case DecorationButtonType::KeepAbove:
                {
                    painter->drawPolyline( QPolygonF()
                        << QPointF( 5, 8 )
                        << QPointF( 9, 4 )
                        << QPointF( 13, 8 ) );

                    painter->drawPolyline( QPolygonF()
                        << QPointF( 5, 12 )
                        << QPointF( 9, 8 )
                        << QPointF( 13, 12 ) );
                    break;
                }


                case DecorationButtonType::ApplicationMenu:
                {
                    painter->drawLine( QPointF( 3.5, 5 ), QPointF( 14.5, 5 ) );
                    painter->drawLine( QPointF( 3.5, 9 ), QPointF( 14.5, 9 ) );
                    painter->drawLine( QPointF( 3.5, 13 ), QPointF( 14.5, 13 ) );
                    break;
                }

                case DecorationButtonType::ContextHelp:
                {
                    QPainterPath path;
                    path.moveTo( 5, 6 );
                    path.arcTo( QRectF( 5, 3.5, 8, 5 ), 180, -180 );
                    path.cubicTo( QPointF(12.5, 9.5), QPointF( 9, 7.5 ), QPointF( 9, 11.5 ) );
                    painter->drawPath( path );
                    painter->drawPoint( 9, 15 );
                    break;
                }

                default: break;

            }

        }

    }

    //__________________________________________________________________
    QColor Button::foregroundColor() const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        if( !d ) {

            return QColor();

        } else if( isPressed() ) {

            return QColor(0, 0, 0, 100);

        } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove || type() == DecorationButtonType::OnAllDesktops ) && isChecked() ) {

            return QColor(255, 255, 255, 150);

        } else if( m_animation->state() == QPropertyAnimation::Running ) {

            return KColorUtils::mix( QColor(0, 0, 0, 0), QColor(0, 0, 0, 100) );

        } else if( isHovered() ) {

            return QColor(0, 0, 0, 80);

        } else {

            // By default black with no opacity
            return QColor(0, 0, 0, 0);

        }

    }

    //__________________________________________________________________
    QColor Button::backgroundColor() const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        if( !d ) {
            return QColor();
        }

        // The colors are hard coded into the following  if (type()== x)
        // Ideally they should be defined as reusable variables somewhere
        // and modified with QColor::lighter() or QColor::darker()

        // Close button
        // red base    QColor(220, 100, 60)

        // Maximize button
        // green base  QColor(110, 180, 50)

        // Minimize button
        // yellow base QColor(250, 190, 10)

        // On all desktops button
        // teal base   QColor(125, 210, 200)

        // Shade button
        // brown base  QColor(180, 140, 90)

        // Keep above button
        // pink base   QColor(255, 140, 240)

        // Keep below button
        // pink base   QColor(200, 140, 190)


        // auto c = d->client().data();
        if( isPressed() ) {

            // close button pressed color
            if( type() == DecorationButtonType::Close ) return QColor(220, 100, 60).darker(120);
            // maximize button pressed color
            else if( type() == DecorationButtonType::Maximize ) return QColor(110, 180, 50).darker(120);
            // minimize button pressed color
            else if( type() == DecorationButtonType::Minimize ) return QColor(250, 190, 10).darker(120);
            // on all desktopts button pressed color
            else if( type() == DecorationButtonType::OnAllDesktops ) return QColor(125, 210, 200).darker(120);
            // shade button pressed color
            else if( type() == DecorationButtonType::Shade ) return QColor(180, 140, 90).darker(120);
            // keep below button pressed color
            else if( type() == DecorationButtonType::KeepBelow ) return QColor(200, 140, 190).darker(120);
            // keep above button pressed color
            else if( type() == DecorationButtonType::KeepAbove ) return QColor(255, 140, 240).darker(120);
            // any other button pressed gets its color from theme
            else return d->fontColor();

        // background color when keep below or above are checked
        } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove ) && isChecked() ) {

            if( type() == DecorationButtonType::KeepBelow ) return QColor(200, 140, 190).lighter(120);
            else return QColor(255, 140, 240).lighter(120);

        // animate background color
        // } else if( m_animation->state() == QPropertyAnimation::Running ) {

        //     if( type() == DecorationButtonType::Close )
        //     {
        //         if( d->internalSettings()->outlineCloseButton() )
        //         {

        //             return KColorUtils::mix( d->fontColor(), c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter(), m_opacity );

        //         } else {

        //             QColor color( c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter() );
        //             color.setAlpha( color.alpha()*m_opacity );
        //             return color;

        //         }

        //     } else {

        //         QColor color( d->fontColor() );
        //         color.setAlpha( color.alpha()*m_opacity );
        //         return color;

        //     }

        } else if( isHovered() ) {

            // close button hovered color
            if( type() == DecorationButtonType::Close ) return QColor(220, 100, 60).lighter(130);
            // maximize button hovered color
            else if( type() == DecorationButtonType::Maximize ) return QColor(110, 180, 50).lighter(120);
            // minimize button hovered color
            else if( type() == DecorationButtonType::Minimize ) return QColor(250, 190, 10).lighter(140);
            // on all desktopts button hovered color
            else if( type() == DecorationButtonType::OnAllDesktops ) return QColor(125, 210, 200).lighter(110);
            // shade button hovered color
            else if( type() == DecorationButtonType::Shade ) return QColor(180, 140, 90).lighter(120);
            // keep below button hovered color
            else if( type() == DecorationButtonType::KeepBelow ) return QColor(200, 140, 190).lighter(120);
            // keep above button hovered color
            else if( type() == DecorationButtonType::KeepAbove ) return QColor(255, 140, 240).lighter(120);
            // any other button gets theme colors mixed
            else return KColorUtils::mix( d->titleBarColor(), d->fontColor(), 0.5 );

        } else {

            // close button normal color
            if( type() == DecorationButtonType::Close ) return QColor(220, 100, 60);
            // maximize button normal color
            else if( type() == DecorationButtonType::Maximize ) return QColor(110, 180, 50);
            // minimize button normal color
            else if( type() == DecorationButtonType::Minimize ) return QColor(250, 190, 10);
            // on all desktopts button normal color
            else if( type() == DecorationButtonType::OnAllDesktops ) return QColor(125, 210, 200);
            // shade button normal color
            else if( type() == DecorationButtonType::Shade ) return QColor(180, 140, 90);
            // keep below button normal color
            else if( type() == DecorationButtonType::KeepBelow ) return QColor(200, 140, 190);
            // keep above button normal color
            else if( type() == DecorationButtonType::KeepAbove ) return QColor(255, 140, 240);
            // any other button without background color
            else return QColor();

        }

    }

    //________________________________________________________________
    void Button::reconfigure()
    {

        // animation
        auto d = qobject_cast<Decoration*>(decoration());
        if( d )  m_animation->setDuration( d->internalSettings()->animationsDuration() );

    }

    //__________________________________________________________________
    void Button::updateAnimationState( bool hovered )
    {

        auto d = qobject_cast<Decoration*>(decoration());
        if( !(d && d->internalSettings()->animationsEnabled() ) ) return;

        m_animation->setDirection( hovered ? QPropertyAnimation::Forward : QPropertyAnimation::Backward );
        if( m_animation->state() != QPropertyAnimation::Running ) m_animation->start();

    }

} // namespace
