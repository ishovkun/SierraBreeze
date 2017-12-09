/*
 * Copyright 2014  Martin Gräßlin <mgraesslin@kde.org>
 * Copyright 2014  Hugo Pereira Da Costa <hugo.pereira@free.fr>
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

namespace Breeze
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
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::closeableChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::Maximize:
                b->setVisible( d->client().data()->isMaximizeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::maximizeableChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::Minimize:
                b->setVisible( d->client().data()->isMinimizeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::minimizeableChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::ContextHelp:
                b->setVisible( d->client().data()->providesContextHelp() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::providesContextHelpChanged, b, &Breeze::Button::setVisible );
                break;

                case DecorationButtonType::Shade:
                b->setVisible( d->client().data()->isShadeable() );
                QObject::connect(d->client().data(), &KDecoration2::DecoratedClient::shadeableChanged, b, &Breeze::Button::setVisible );
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
            auto d = qobject_cast<Decoration*>( decoration() );
            auto c = d->client().data();

            const auto hover_hint_color = QColor(41, 43, 50);
            QPen hint_pen(hover_hint_color);

            switch( type() )
            {

                case DecorationButtonType::Close:
                {
                  QColor button_color = QColor(242, 80, 86);
                  if (!c->isActive())
                    button_color = QColor(199, 199, 199);
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                  // painter->drawEllipse( QRectF( 3, 3, 12, 12 ) );
                  painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                  painter->setBrush( Qt::NoBrush );
                  if ( isHovered() )
                  {
                    painter->setPen( hint_pen );
                    // it's a cross
                    painter->drawLine( QPointF( 6, 6 ), QPointF( 12, 12 ) );
                    painter->drawLine( QPointF( 6, 12 ), QPointF( 12, 6 ) );
                  }
                  painter->setPen( pen );

                  break;
                }

                case DecorationButtonType::Maximize:
                {
                  QColor button_color = QColor(19, 209, 61);
                  if (!c->isActive())
                    button_color = QColor(199, 199, 199);

                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                  // painter->drawEllipse( QRectF( 3, 3, 12, 12 ) );
                  painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                  painter->setBrush( Qt::NoBrush );
                  if ( isHovered() )
                  {
                    painter->setPen( hint_pen );
                    // two triangles
                    QPainterPath path1, path2;
                    path1.moveTo(5, 13);
                    path1.lineTo(11, 13);
                    path1.lineTo(5, 7);

                    path2.moveTo(13, 4);
                    path2.lineTo(7, 4);
                    path2.lineTo(13, 10);


                    painter->fillPath(path1, QBrush(hover_hint_color));
                    painter->fillPath(path2, QBrush(hover_hint_color));
                  }
                  painter->setPen( pen );
                    // if( isChecked() )
                    // {
                    //     pen.setJoinStyle( Qt::RoundJoin );
                    //     painter->setPen( pen );

                    //     painter->drawPolygon( QPolygonF()
                    //         << QPointF( 4, 9 )
                    //         << QPointF( 9, 4 )
                    //         << QPointF( 14, 9 )
                    //         << QPointF( 9, 14 ) );

                    // } else {
                    //     painter->drawPolyline( QPolygonF()
                    //         << QPointF( 4, 11 )
                    //         << QPointF( 9, 6 )
                    //         << QPointF( 14, 11 ) );
                    // }
                    break;
                }

                case DecorationButtonType::Minimize:
                {
                  QColor button_color = QColor(252, 190, 7);
                  if (!c->isActive())
                    button_color = QColor(199, 199, 199);
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                  // painter->drawEllipse( QRectF( 3, 3, 12, 12 ) );
                  painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                  painter->setBrush( Qt::NoBrush );
                  if ( isHovered() )
                    {
                      painter->setPen( hint_pen );
                      painter->drawLine( QPointF( 6, 9 ), QPointF( 12, 9 ) );
                    }
                  painter->setPen( pen );
                    // painter->drawPolyline( QPolygonF()
                    //     << QPointF( 4, 7 )
                    //     << QPointF( 9, 12 )
                    //     << QPointF( 14, 7 ) );
                    break;
                }

                case DecorationButtonType::OnAllDesktops:
                {
                  QColor button_color = QColor(125, 209, 200);
                  if (!c->isActive())
                    button_color = QColor(199, 199, 199);
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                  // painter->drawEllipse( QRectF( 3, 3, 12, 12 ) );
                  painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                  painter->setPen( pen );
                  painter->setBrush( Qt::NoBrush );
                    // painter->setPen( Qt::NoPen );
                    // painter->setBrush( foregroundColor );

                    // if( isChecked())
                    // {

                    //     // outer ring
                    //     painter->drawEllipse( QRectF( 3, 3, 12, 12 ) );

                    //     // center dot
                    //     QColor backgroundColor( this->backgroundColor() );
                    //     auto d = qobject_cast<Decoration*>( decoration() );
                    //     if( !backgroundColor.isValid() && d ) backgroundColor = d->titleBarColor();

                    //     if( backgroundColor.isValid() )
                    //     {
                    //         painter->setBrush( backgroundColor );
                    //         painter->drawEllipse( QRectF( 8, 8, 2, 2 ) );
                    //     }

                    // } else {

                    //     painter->drawPolygon( QPolygonF()
                    //         << QPointF( 6.5, 8.5 )
                    //         << QPointF( 12, 3 )
                    //         << QPointF( 15, 6 )
                    //         << QPointF( 9.5, 11.5 ) );

                    //     painter->setPen( pen );
                    //     painter->drawLine( QPointF( 5.5, 7.5 ), QPointF( 10.5, 12.5 ) );
                    //     painter->drawLine( QPointF( 12, 6 ), QPointF( 4.5, 13.5 ) );
                    // }
                    break;
                }

                case DecorationButtonType::Shade:
                {

                    if (isChecked())
                    {

                        painter->drawLine( 4, 5, 14, 5 );
                        painter->drawPolyline( QPolygonF()
                            << QPointF( 4, 8 )
                            << QPointF( 9, 13 )
                            << QPointF( 14, 8 ) );

                    } else {

                        painter->drawLine( 4, 5, 14, 5 );
                        painter->drawPolyline( QPolygonF()
                            << QPointF( 4, 13 )
                            << QPointF( 9, 8 )
                            << QPointF( 14, 13 ) );
                    }

                    break;

                }

                case DecorationButtonType::KeepBelow:
                {

                    painter->drawPolyline( QPolygonF()
                        << QPointF( 4, 5 )
                        << QPointF( 9, 10 )
                        << QPointF( 14, 5 ) );

                    painter->drawPolyline( QPolygonF()
                        << QPointF( 4, 9 )
                        << QPointF( 9, 14 )
                        << QPointF( 14, 9 ) );
                    break;

                }

                case DecorationButtonType::KeepAbove:
                {
                  QColor button_color = QColor(204, 176, 213);
                  if (!c->isActive())
                    button_color = QColor(199, 199, 199);
                  painter->setBrush( button_color );
                  painter->setPen( Qt::NoPen );
                  // painter->drawEllipse( QRectF( 3, 3, 12, 12 ) );
                  painter->drawEllipse( QRectF( 0, 0, 18, 18 ) );
                  painter->setPen( pen );
                  painter->setBrush( Qt::NoBrush );
                    // painter->drawPolyline( QPolygonF()
                    //     << QPointF( 4, 9 )
                    //     << QPointF( 9, 4 )
                    //     << QPointF( 14, 9 ) );

                    // painter->drawPolyline( QPolygonF()
                    //     << QPointF( 4, 13 )
                    //     << QPointF( 9, 8 )
                    //     << QPointF( 14, 13 ) );
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
    QColor Button::foregroundColor( void ) const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        if( !d ) {

            return QColor();

        } else if( isPressed() ) {

            return d->titleBarColor();

        } else if( type() == DecorationButtonType::Close && d->internalSettings()->outlineCloseButton() ) {

            return d->titleBarColor();

        } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove ) && isChecked() ) {

            return d->titleBarColor();

        } else if( m_animation->state() == QPropertyAnimation::Running ) {

            return KColorUtils::mix( d->fontColor(), d->titleBarColor(), m_opacity );

        } else if( isHovered() ) {

            return d->titleBarColor();

        } else {

            return d->fontColor();

        }

    }

    //__________________________________________________________________
    QColor Button::backgroundColor( void ) const
    {
        auto d = qobject_cast<Decoration*>( decoration() );
        if( !d ) {

            return QColor();

        }

        auto c = d->client().data();
        if( isPressed() ) {

            if( type() == DecorationButtonType::Close ) return c->color( ColorGroup::Warning, ColorRole::Foreground );
            else return KColorUtils::mix( d->titleBarColor(), d->fontColor(), 0.3 );

        } else if( ( type() == DecorationButtonType::KeepBelow || type() == DecorationButtonType::KeepAbove ) && isChecked() ) {

            return d->fontColor();

        } else if( m_animation->state() == QPropertyAnimation::Running ) {

            if( type() == DecorationButtonType::Close )
            {
                if( d->internalSettings()->outlineCloseButton() )
                {

                    return KColorUtils::mix( d->fontColor(), c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter(), m_opacity );

                } else {

                    QColor color( c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter() );
                    color.setAlpha( color.alpha()*m_opacity );
                    return color;

                }

            } else {

                QColor color( d->fontColor() );
                color.setAlpha( color.alpha()*m_opacity );
                return color;

            }

        } else if( isHovered() ) {

            if( type() == DecorationButtonType::Close ) return c->color( ColorGroup::Warning, ColorRole::Foreground ).lighter();
            else return d->fontColor();

        } else if( type() == DecorationButtonType::Close && d->internalSettings()->outlineCloseButton() ) {

            return d->fontColor();

        } else {

            return QColor();

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
