/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2021 - 2025                                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "ui_window.h"

#include <algorithm>
#include <cassert>
#include <cstddef>

#include "agg_image.h"
#include "icn.h"
#include "settings.h"
#include "ui_button.h"
#include "ui_constants.h"

namespace
{
    const int32_t borderSize{ fheroes2::borderWidthPx };

    // Offset from border edges (size of evil interface corners is 43 pixels) - these edges (corners) will not be copied to fill the border.
    const int32_t borderEdgeOffset{ 43 };

    // Size in pixels of dithered transition from one image to another.
    const int32_t transitionSize{ 10 };

    // Offset from window edges to background copy area.
    const int32_t backgroundOffset{ 22 };

    // Spaces between buttons in symmetric button groups.
    const int32_t buttonsHorizontalGap = 37;
    const int32_t buttonsVerticalGap = 10;

    fheroes2::Rect getSymmetricDialogActiveArea( fheroes2::ButtonGroup & buttons, const bool isSingleColumn, const int32_t extraHeight, const fheroes2::Image & output )
    {
        if ( buttons.getButtonsCount() == 0 ) {
            return {};
        }

        const fheroes2::Rect & buttonArea = buttons.button( 0 ).area();
        const int32_t buttonCount = static_cast<int32_t>( buttons.getButtonsCount() );

        const int32_t widthPadding = isSingleColumn ? 52 : 60;
        int32_t dialogWidth = widthPadding;

        const int32_t heightPadding = isSingleColumn ? 43 : 26;
        // We assume that the cancel button height for multiple columns is 25 px because this button should contain only a single line of text.
        const int32_t cancelButtonAreaHeight = isSingleColumn ? 0 : 25 + buttonsVerticalGap + 10 + 1;
        int32_t dialogHeight = cancelButtonAreaHeight + heightPadding + extraHeight;

        // When there's an odd number of buttons we always make a dialog for a single column of buttons.
        if ( isSingleColumn || buttonCount % 2 != 0 ) {
            dialogWidth += buttonArea.width;
            dialogHeight += buttonArea.height * buttonCount + ( buttonCount - 1 ) * buttonsVerticalGap;
        }
        else if ( buttonCount == 2 ) {
            dialogWidth += buttonArea.width * 2 + buttonsHorizontalGap;
            dialogHeight += buttonArea.height;
        }
        else {
            const int32_t buttonGaps = 2 * buttonsVerticalGap;
            dialogWidth += ( buttonCount / 2 ) * buttonArea.width + ( ( buttonCount / 2 - 1 ) * buttonGaps );
            // We apply equal gaps between buttons vertically and horizontally.
            dialogHeight += buttonArea.height * 2 + buttonGaps;
        }
        fheroes2::Point placement;
        if ( isSingleColumn ) {
            const fheroes2::Sprite mainMenuBackground = fheroes2::AGG::GetICN( ICN::HEROES, 0 );
            const int32_t panelXPos = output.width() - mainMenuBackground.x() - ( dialogWidth + fheroes2::borderWidthPx ) - 8;
            const int32_t panelYPos = mainMenuBackground.y() + fheroes2::borderWidthPx + 8;
            placement.x = panelXPos;
            placement.y = panelYPos;
        }
        else {
            placement.x = ( output.width() - dialogWidth ) / 2;
            placement.y = ( output.height() - dialogHeight ) / 2;
        }

        return { placement.x, placement.y, dialogWidth, dialogHeight };
    }
}

namespace fheroes2
{
    StandardWindow::StandardWindow( const int32_t width, const int32_t height, const bool renderBackground, Image & output )
        : _output( output )
        , _activeArea( ( output.width() - width ) / 2, ( output.height() - height ) / 2, width, height )
        , _windowArea( _activeArea.x - borderSize, _activeArea.y - borderSize, _activeArea.width + 2 * borderSize, _activeArea.height + 2 * borderSize )
        , _totalArea( _windowArea.x - borderSize, _windowArea.y, _windowArea.width + borderSize, _windowArea.height + borderSize )
        , _restorer( output, _windowArea.x - borderSize, _windowArea.y, _windowArea.width + borderSize, _windowArea.height + borderSize )
        , _hasBackground{ renderBackground }
    {
        render();
    }

    StandardWindow::StandardWindow( const int32_t x, const int32_t y, const int32_t width, const int32_t height, const bool renderBackground, Image & output )
        : _output( output )
        , _activeArea( x, y, width, height )
        , _windowArea( _activeArea.x - borderSize, _activeArea.y - borderSize, _activeArea.width + 2 * borderSize, _activeArea.height + 2 * borderSize )
        , _totalArea( _windowArea.x - borderSize, _windowArea.y, _windowArea.width + borderSize, _windowArea.height + borderSize )
        , _restorer( output, _windowArea.x - borderSize, _windowArea.y, _windowArea.width + borderSize, _windowArea.height + borderSize )
        , _hasBackground{ renderBackground }
    {
        render();
    }

    StandardWindow::StandardWindow( ButtonGroup & buttons, const bool isSingleColumn, const int32_t extraHeight, Image & output )
        : _output( output )
        , _activeArea( getSymmetricDialogActiveArea( buttons, isSingleColumn, extraHeight, output ) )
        , _windowArea( _activeArea.x - borderSize, _activeArea.y - borderSize, _activeArea.width + 2 * borderSize, _activeArea.height + 2 * borderSize )
        , _totalArea( _windowArea.x - borderSize, _windowArea.y, _windowArea.width + borderSize, _windowArea.height + borderSize )
        , _restorer( output, _windowArea.x - borderSize, _windowArea.y, _windowArea.width + borderSize, _windowArea.height + borderSize )
    {
        if ( buttons.getButtonsCount() == 0 ) {
            // What are you trying to achieve with no buttons?!
            assert( 0 );
            return;
        }

        render();
    }

    void StandardWindow::render()
    {
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        // Notice: ICN::SURDRBKE and ICN::SURDRBKG has 16 (equals to borderWidthPx) pixels shadow from the left and the bottom sides.
        const Sprite & horizontalSprite = AGG::GetICN( ( isEvilInterface ? ICN::SURDRBKE : ICN::SURDRBKG ), 0 );
        const Sprite & verticalSprite = AGG::GetICN( ( isEvilInterface ? ICN::WINLOSEE : ICN::WINLOSE ), 0 );

        // Offset from window edges to background copy area and also the size of corners to render.
        const int32_t cornerSize = _hasBackground ? backgroundOffset : borderSize;

        const int32_t horizontalSpriteWidth = horizontalSprite.width() - borderSize;
        const int32_t horizontalSpriteHeight = horizontalSprite.height() - borderSize;
        const int32_t verticalSpriteHeight = verticalSprite.height();
        const int32_t verticalSpriteWidth = verticalSprite.width();

        // Render window corners. The corners are the same in used original images, so we use only 'verticalSprite'.
        const int32_t rightCornerOffsetX = _windowArea.x + _windowArea.width - cornerSize;
        const int32_t bottomCornerOffsetY = _windowArea.y + _windowArea.height - cornerSize;
        const int32_t rightCornerSpriteOffsetX = verticalSpriteWidth - cornerSize;
        const int32_t bottomCornerSpriteOffsetY = verticalSpriteHeight - cornerSize;
        Blit( verticalSprite, 0, 0, _output, _windowArea.x, _windowArea.y, cornerSize, cornerSize );
        Blit( verticalSprite, rightCornerSpriteOffsetX, 0, _output, rightCornerOffsetX, _windowArea.y, cornerSize, cornerSize );
        Blit( verticalSprite, 0, bottomCornerSpriteOffsetY, _output, _windowArea.x, bottomCornerOffsetY, cornerSize, cornerSize );
        Blit( verticalSprite, rightCornerSpriteOffsetX, bottomCornerSpriteOffsetY, _output, rightCornerOffsetX, bottomCornerOffsetY, cornerSize, cornerSize );

        // Render additional part of border corners. This part will not be repeated to fill the border length.
        const int32_t extraCornerSize = borderEdgeOffset - cornerSize;
        const Point cornerOffset( _windowArea.x + cornerSize, _windowArea.y + cornerSize );
        const int32_t rightBorderEdgeOffset = verticalSpriteWidth - borderEdgeOffset;
        const int32_t bottomBorderEdgeOffset = verticalSpriteHeight - borderEdgeOffset;

        Blit( verticalSprite, cornerSize, 0, _output, cornerOffset.x, _windowArea.y, extraCornerSize, cornerSize );
        Blit( verticalSprite, 0, cornerSize, _output, _windowArea.x, cornerOffset.y, cornerSize, extraCornerSize );

        Blit( verticalSprite, rightBorderEdgeOffset, 0, _output, rightCornerOffsetX - extraCornerSize, _windowArea.y, extraCornerSize, cornerSize );
        Blit( verticalSprite, rightCornerSpriteOffsetX, cornerSize, _output, rightCornerOffsetX, cornerOffset.y, cornerSize, extraCornerSize );

        Blit( verticalSprite, cornerSize, bottomCornerSpriteOffsetY, _output, cornerOffset.x, bottomCornerOffsetY, extraCornerSize, cornerSize );
        Blit( verticalSprite, 0, bottomBorderEdgeOffset, _output, _windowArea.x, bottomCornerOffsetY - extraCornerSize, cornerSize, extraCornerSize );

        Blit( verticalSprite, rightBorderEdgeOffset, bottomCornerSpriteOffsetY, _output, rightCornerOffsetX - extraCornerSize, bottomCornerOffsetY, extraCornerSize,
              cornerSize );
        Blit( verticalSprite, rightCornerSpriteOffsetX, bottomBorderEdgeOffset, _output, rightCornerOffsetX, bottomCornerOffsetY - extraCornerSize, cornerSize,
              extraCornerSize );

        if ( _hasBackground ) {
            // Render the background image.
            renderBackgroundImage( _output, _windowArea, backgroundOffset, isEvilInterface );

            // Make a transition from borders to the background in the corners.
            CreateDitheringTransition( verticalSprite, cornerSize, cornerSize, _output, cornerOffset.x, cornerOffset.y, extraCornerSize, transitionSize, false, true );
            CreateDitheringTransition( verticalSprite, cornerSize, cornerSize, _output, cornerOffset.x, cornerOffset.y, transitionSize, extraCornerSize, true, true );

            CreateDitheringTransition( verticalSprite, rightBorderEdgeOffset, cornerSize, _output, rightCornerOffsetX - extraCornerSize, cornerOffset.y, extraCornerSize,
                                       transitionSize, false, true );
            CreateDitheringTransition( verticalSprite, rightCornerSpriteOffsetX - transitionSize, cornerSize, _output, rightCornerOffsetX - transitionSize,
                                       cornerOffset.y, transitionSize, extraCornerSize, true, false );

            CreateDitheringTransition( verticalSprite, cornerSize, bottomCornerSpriteOffsetY - transitionSize, _output, cornerOffset.x,
                                       bottomCornerOffsetY - transitionSize, extraCornerSize, transitionSize, false, false );
            CreateDitheringTransition( verticalSprite, cornerSize, bottomBorderEdgeOffset, _output, cornerOffset.x, bottomCornerOffsetY - extraCornerSize, transitionSize,
                                       extraCornerSize, true, true );

            CreateDitheringTransition( verticalSprite, rightBorderEdgeOffset, bottomCornerSpriteOffsetY - transitionSize, _output, rightCornerOffsetX - extraCornerSize,
                                       bottomCornerOffsetY - transitionSize, extraCornerSize, transitionSize, false, false );
            CreateDitheringTransition( verticalSprite, rightCornerSpriteOffsetX - transitionSize, bottomBorderEdgeOffset, _output, rightCornerOffsetX - transitionSize,
                                       bottomCornerOffsetY - extraCornerSize, transitionSize, extraCornerSize, true, false );
        }

        // Render vertical borders.
        const int32_t doubleBorderEdgeOffset = borderEdgeOffset * 2;
        const int32_t verticalSpriteCopyHeight = std::min( _windowArea.height, verticalSpriteHeight ) - doubleBorderEdgeOffset;
        const int32_t verticalSpriteCopies
            = ( _windowArea.height - doubleBorderEdgeOffset - 1 - transitionSize ) / ( bottomBorderEdgeOffset - borderEdgeOffset - transitionSize );
        const int32_t topBorderEdgeOffset = _windowArea.y + borderEdgeOffset;

        Blit( verticalSprite, 0, borderEdgeOffset, _output, _windowArea.x, topBorderEdgeOffset, cornerSize, verticalSpriteCopyHeight );
        Blit( verticalSprite, rightCornerSpriteOffsetX, borderEdgeOffset, _output, rightCornerOffsetX, topBorderEdgeOffset, cornerSize, verticalSpriteCopyHeight );

        // Render a transition to the background.
        if ( _hasBackground ) {
            CreateDitheringTransition( verticalSprite, cornerSize, borderEdgeOffset, _output, cornerOffset.x, topBorderEdgeOffset, transitionSize,
                                       verticalSpriteCopyHeight, true, true );
            CreateDitheringTransition( verticalSprite, rightCornerSpriteOffsetX - transitionSize, borderEdgeOffset, _output, rightCornerOffsetX - transitionSize,
                                       topBorderEdgeOffset, transitionSize, verticalSpriteCopyHeight, true, false );
        }

        // If we need more copies to fill vertical borders we make a transition and copy the central part of the border.
        if ( verticalSpriteCopies > 0 ) {
            int32_t toOffsetY = borderEdgeOffset + verticalSpriteCopyHeight;
            const int32_t outputY = _windowArea.y + toOffsetY - transitionSize;
            CreateDitheringTransition( verticalSprite, 0, borderEdgeOffset, _output, _windowArea.x, outputY, cornerSize, transitionSize, false, false );
            CreateDitheringTransition( verticalSprite, rightCornerSpriteOffsetX, borderEdgeOffset, _output, rightCornerOffsetX, outputY, cornerSize, transitionSize,
                                       false, false );

            const int32_t stepY = verticalSpriteCopyHeight - transitionSize;
            const int32_t fromOffsetY = borderEdgeOffset + transitionSize;

            for ( int32_t i = 0; i < verticalSpriteCopies; ++i ) {
                const int32_t copyHeight = std::min( verticalSpriteCopyHeight, _windowArea.height - borderEdgeOffset - toOffsetY );
                const int32_t toY = _windowArea.y + toOffsetY;

                Blit( verticalSprite, 0, fromOffsetY, _output, _windowArea.x, toY, cornerSize, copyHeight );
                Blit( verticalSprite, rightCornerSpriteOffsetX, fromOffsetY, _output, rightCornerOffsetX, toY, cornerSize, copyHeight );

                // Render a transition to the background.
                if ( _hasBackground ) {
                    CreateDitheringTransition( verticalSprite, cornerSize, fromOffsetY, _output, cornerOffset.x, toY, transitionSize, copyHeight, true, true );
                    CreateDitheringTransition( verticalSprite, rightCornerSpriteOffsetX - transitionSize, fromOffsetY, _output, rightCornerOffsetX - transitionSize, toY,
                                               transitionSize, copyHeight, true, false );
                }

                toOffsetY += stepY;
            }
        }

        // Make a transition to the bottom corners.
        const int32_t verticalSpriteBottomCornerEdgeY = bottomBorderEdgeOffset - transitionSize;
        const int32_t optputBottomCornerEdgeY = _windowArea.y + _windowArea.height - borderEdgeOffset - transitionSize;
        CreateDitheringTransition( verticalSprite, 0, verticalSpriteBottomCornerEdgeY, _output, _windowArea.x, optputBottomCornerEdgeY, cornerSize, transitionSize, false,
                                   false );
        CreateDitheringTransition( verticalSprite, rightCornerSpriteOffsetX, verticalSpriteBottomCornerEdgeY, _output, rightCornerOffsetX, optputBottomCornerEdgeY,
                                   cornerSize, transitionSize, false, false );

        // Render horizontal borders. We have to remember that 'verticalSprite' has 16 (equals to borderWidthPx) pixels of shadow at the left and bottom sides.
        const int32_t horizontalSpriteCopyWidth = std::min( _windowArea.width, horizontalSpriteWidth ) - doubleBorderEdgeOffset;
        const int32_t horizontalSpriteCopies
            = ( _windowArea.width - doubleBorderEdgeOffset - 1 - transitionSize ) / ( horizontalSpriteWidth - doubleBorderEdgeOffset - transitionSize );
        const int32_t bottomBorderSpriteOffsetY = horizontalSpriteHeight - cornerSize;
        const int32_t horizontalSpriteCopyStartX = borderEdgeOffset + borderSize;
        const int32_t leftBorderEdgeOffset = _windowArea.x + borderEdgeOffset;

        Blit( horizontalSprite, horizontalSpriteCopyStartX, 0, _output, leftBorderEdgeOffset, _windowArea.y, horizontalSpriteCopyWidth, cornerSize );
        Blit( horizontalSprite, horizontalSpriteCopyStartX, bottomBorderSpriteOffsetY, _output, leftBorderEdgeOffset, bottomCornerOffsetY, horizontalSpriteCopyWidth,
              cornerSize );

        // Render a transition to the background.
        if ( _hasBackground ) {
            CreateDitheringTransition( horizontalSprite, horizontalSpriteCopyStartX, cornerSize, _output, leftBorderEdgeOffset, cornerOffset.y, horizontalSpriteCopyWidth,
                                       transitionSize, false, true );
            CreateDitheringTransition( horizontalSprite, horizontalSpriteCopyStartX, bottomBorderSpriteOffsetY - transitionSize, _output, leftBorderEdgeOffset,
                                       bottomCornerOffsetY - transitionSize, horizontalSpriteCopyWidth, transitionSize, false, false );
        }

        // If we need more copies to fill horizontal borders we make a transition and copy the central part of the border.
        if ( horizontalSpriteCopies > 0 ) {
            int32_t toOffsetX = borderEdgeOffset + horizontalSpriteCopyWidth;
            const int32_t outputX = _windowArea.x + toOffsetX - transitionSize;
            CreateDitheringTransition( horizontalSprite, horizontalSpriteCopyStartX, 0, _output, outputX, _windowArea.y, transitionSize, cornerSize, true, false );
            CreateDitheringTransition( horizontalSprite, horizontalSpriteCopyStartX, bottomBorderSpriteOffsetY, _output, outputX, bottomCornerOffsetY, transitionSize,
                                       cornerSize, true, false );

            const int32_t stepX = horizontalSpriteCopyWidth - transitionSize;
            const int32_t fromOffsetX = horizontalSpriteCopyStartX + transitionSize;

            for ( int32_t i = 0; i < horizontalSpriteCopies; ++i ) {
                const int32_t copyWidth = std::min( horizontalSpriteCopyWidth, _windowArea.width - borderEdgeOffset - toOffsetX );
                const int32_t toX = _windowArea.x + toOffsetX;

                Blit( horizontalSprite, fromOffsetX, 0, _output, toX, _windowArea.y, copyWidth, cornerSize );
                Blit( horizontalSprite, fromOffsetX, bottomBorderSpriteOffsetY, _output, toX, bottomCornerOffsetY, copyWidth, cornerSize );

                // Render a transition to the background.
                if ( _hasBackground ) {
                    CreateDitheringTransition( horizontalSprite, fromOffsetX, cornerSize, _output, toX, cornerOffset.y, copyWidth, transitionSize, false, true );
                    CreateDitheringTransition( horizontalSprite, fromOffsetX, bottomBorderSpriteOffsetY - transitionSize, _output, toX,
                                               bottomCornerOffsetY - transitionSize, copyWidth, transitionSize, false, false );
                }

                toOffsetX += stepX;
            }
        }

        // Make a transition to the right corners.
        const int32_t horizontalSpriteRightCornerEdgeX = horizontalSprite.width() - borderEdgeOffset - transitionSize;
        const int32_t optputRightCornerEdgeX = _windowArea.x + _windowArea.width - borderEdgeOffset - transitionSize;
        CreateDitheringTransition( horizontalSprite, horizontalSpriteRightCornerEdgeX, 0, _output, optputRightCornerEdgeX, _windowArea.y, transitionSize, cornerSize,
                                   true, false );
        CreateDitheringTransition( horizontalSprite, horizontalSpriteRightCornerEdgeX, bottomBorderSpriteOffsetY, _output, optputRightCornerEdgeX, bottomCornerOffsetY,
                                   transitionSize, cornerSize, true, false );

        fheroes2::addGradientShadowForArea( _output, { _windowArea.x, _windowArea.y }, _windowArea.width, _windowArea.height, borderSize );
    }

    void StandardWindow::applyTextBackgroundShading( const Rect & roi )
    {
        const Rect shadingRoi = roi ^ _activeArea;

        applyTextBackgroundShading( _output, shadingRoi );
    }

    void StandardWindow::applyTextBackgroundShading( Image & output, const Rect & roi )
    {
        // The text background is darker than original background. The shadow strength 2 is too much so we do two shading transforms: 3 and 5.
        ApplyTransform( output, roi.x + 2, roi.y + 2, roi.width - 4, roi.height - 4, 3 );
        ApplyTransform( output, roi.x + 2, roi.y + 2, roi.width - 4, roi.height - 4, 5 );

        // Make text background borders: it consists of rectangles with different transform shading.
        auto applyRectTransform = [&roi, &output]( const int32_t offset, const int32_t size, const uint8_t transformId ) {
            // Top horizontal line.
            ApplyTransform( output, roi.x + offset, roi.y + offset, roi.width - 2 * offset, size, transformId );
            // Left vertical line without pixels that are parts of horizontal lines.
            ApplyTransform( output, roi.x + offset, roi.y + offset + size, size, roi.height - 2 * ( offset + size ), transformId );
            // Bottom horizontal line.
            ApplyTransform( output, roi.x + offset, roi.y + roi.height - offset - size, roi.width - 2 * offset, size, transformId );
            // Right vertical line without pixels that are parts of horizontal lines.
            ApplyTransform( output, roi.x + roi.width - offset - size, roi.y + offset + size, size, roi.height - 2 * ( offset + size ), transformId );
        };

        // Outer rectangle is slightly bright.
        applyRectTransform( 0, 1, 9 );
        // Next shaded rectangles have these shadow strengths: 4, 3, 2, 2, 2, 3, 4, 5.
        applyRectTransform( 1, 1, 4 );
        applyRectTransform( 2, 1, 3 );
        applyRectTransform( 3, 3, 2 );
        applyRectTransform( 6, 1, 3 );
        applyRectTransform( 7, 1, 4 );
        applyRectTransform( 8, 1, 5 );
    }

    void StandardWindow::applyGemDecoratedCorners()
    {
        fheroes2::Image gem;
        const int32_t gemSideLength = 9;
        gem.resize( gemSideLength, gemSideLength );
        gem.reset();
        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();
        if ( !isEvilInterface ) {
            const fheroes2::Sprite & gemDialog = fheroes2::AGG::GetICN( ICN::REDBACK, 0 );
            Copy( gemDialog, 20, 2, gem, 0, 0, gemSideLength, gemSideLength );
        }
        else {
            const fheroes2::Sprite & corners = fheroes2::AGG::GetICN( ICN::EVIL_DIALOG_PLAIN_CORNERS, 0 );
            const int32_t cornerSideLength = 43;
            Copy( corners, 0, 0, _output, _windowArea.x, _windowArea.y, cornerSideLength, cornerSideLength );
            Copy( corners, cornerSideLength, 0, _output, _windowArea.x + _windowArea.width - cornerSideLength, _windowArea.y, cornerSideLength, cornerSideLength );
            Copy( corners, 0, cornerSideLength, _output, _windowArea.x, _windowArea.y + _windowArea.height - cornerSideLength, cornerSideLength, cornerSideLength );
            Copy( corners, cornerSideLength, cornerSideLength, _output, _windowArea.x + _windowArea.width - cornerSideLength,
                  _windowArea.y + _windowArea.height - cornerSideLength, cornerSideLength, cornerSideLength );

            const fheroes2::Sprite & gemDialog = fheroes2::AGG::GetICN( ICN::WINLOSEE, 0 );
            Copy( gemDialog, 32, 2, gem, 0, 0, gemSideLength, gemSideLength );
            FillTransform( gem, 0, 0, 1, 1, 1 );
            FillTransform( gem, gemSideLength - 1, 0, 1, 1, 1 );
            FillTransform( gem, 0, gemSideLength - 1, 1, 1, 1 );
            FillTransform( gem, gemSideLength - 1, gemSideLength - 1, 1, 1, 1 );
        }
        Blit( gem, 0, 0, _output, _windowArea.x + 4, _windowArea.y + 2, gemSideLength, gemSideLength );
        Blit( gem, 0, 0, _output, _windowArea.x + _windowArea.width - 2 - gemSideLength, _windowArea.y + 2, gemSideLength, gemSideLength );
        Blit( gem, 0, 0, _output, _windowArea.x + 4, _windowArea.y + _windowArea.height - gemSideLength - 4, gemSideLength, gemSideLength );
        Blit( gem, 0, 0, _output, _windowArea.x + _windowArea.width - 2 - gemSideLength, _windowArea.y + _windowArea.height - gemSideLength - 4, gemSideLength,
              gemSideLength );
    }

    void StandardWindow::renderScrollbarBackground( const Rect & roi, const bool isEvilInterface )
    {
        const Sprite & scrollBar = AGG::GetICN( isEvilInterface ? ICN::ADVBORDE : ICN::ADVBORD, 0 );

        const int32_t topPartHeight = 19;
        const int32_t scrollBarWidth = 16;
        const int32_t middlePartHeight = 88;
        const int32_t icnOffsetX = 536;
        const int32_t middleAndBottomPartsHeight = roi.height - topPartHeight;

        // Top part of scrollbar background.
        Copy( scrollBar, icnOffsetX, 176, _output, roi.x, roi.y, scrollBarWidth, topPartHeight );

        // Middle part of scrollbar background.
        const int32_t middlePartCount = ( roi.height - 2 * topPartHeight + middlePartHeight - 1 ) / middlePartHeight;
        int32_t offsetY = topPartHeight;

        for ( int32_t i = 0; i < middlePartCount; ++i ) {
            Copy( scrollBar, icnOffsetX, 196, _output, roi.x, roi.y + offsetY, scrollBarWidth, std::min( middlePartHeight, middleAndBottomPartsHeight - offsetY ) );
            offsetY += middlePartHeight;
        }

        // Bottom part of scrollbar background.
        Copy( scrollBar, icnOffsetX, 285, _output, roi.x, roi.y + middleAndBottomPartsHeight, scrollBarWidth, topPartHeight );

        // Make scrollbar shadow.
        for ( uint8_t i = 0; i < 4; ++i ) {
            const uint8_t transformId = i + 1;
            ApplyTransform( _output, roi.x - transformId, roi.y + transformId, 1, roi.height - transformId, transformId );
            ApplyTransform( _output, roi.x - transformId, roi.y + roi.height + i, scrollBarWidth, 1, transformId );
        }
    }

    void StandardWindow::renderButton( Button & button, const int icnId, const uint32_t releasedIndex, const uint32_t pressedIndex, const Point & offset,
                                       const Padding padding )
    {
        const Sprite & buttonSprite = AGG::GetICN( icnId, 0 );

        const Point pos = _getRenderPos( offset, { buttonSprite.width(), buttonSprite.height() }, padding );

        button.setICNInfo( icnId, releasedIndex, pressedIndex );
        button.setPosition( pos.x, pos.y );
        addGradientShadow( buttonSprite, _output, button.area().getPosition(), { -5, 5 } );
        button.draw();
    }

    void StandardWindow::renderOkayCancelButtons( Button & buttonOk, Button & buttonCancel )
    {
        const Point gapsFromEdges( 20, 7 );

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        const int buttonOkIcn = isEvilInterface ? ICN::BUTTON_SMALL_OKAY_EVIL : ICN::BUTTON_SMALL_OKAY_GOOD;
        renderButton( buttonOk, buttonOkIcn, 0, 1, gapsFromEdges, Padding::BOTTOM_LEFT );

        const int buttonCancelIcn = isEvilInterface ? ICN::BUTTON_SMALL_CANCEL_EVIL : ICN::BUTTON_SMALL_CANCEL_GOOD;
        renderButton( buttonCancel, buttonCancelIcn, 0, 1, gapsFromEdges, Padding::BOTTOM_RIGHT );
    }

    void StandardWindow::renderTextAdaptedButtonSprite( ButtonSprite & button, const char * buttonText, const Point & offset, const Padding padding )
    {
        Sprite released;
        Sprite pressed;

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        getTextAdaptedSprite( released, pressed, buttonText, isEvilInterface ? ICN::EMPTY_EVIL_BUTTON : ICN::EMPTY_GOOD_BUTTON,
                              isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

        const Point pos = _getRenderPos( offset, { released.width(), released.height() }, padding );
        button.setSprite( released, pressed );
        button.setPosition( pos.x, pos.y );
        addGradientShadow( released, _output, button.area().getPosition(), { -5, 5 } );
        button.draw();
    }

    void StandardWindow::renderCustomButtonSprite( ButtonSprite & button, const std::string & buttonText, const fheroes2::Size buttonSize, const Point & offset,
                                                   const Padding padding )
    {
        Sprite released;
        Sprite pressed;

        const bool isEvilInterface = Settings::Get().isEvilInterfaceEnabled();

        makeButtonSprites( released, pressed, buttonText, buttonSize, isEvilInterface, isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK );

        const Point pos = _getRenderPos( offset, { released.width(), released.height() }, padding );

        button.setSprite( released, pressed );
        button.setPosition( pos.x, pos.y );
        addGradientShadow( released, _output, button.area().getPosition(), { -5, 5 } );
        button.draw();
    }

    void StandardWindow::renderSymmetricButtons( ButtonGroup & buttons, const int32_t offsetY, const bool isSingleColumn )
    {
        const int32_t buttonsWidth = buttons.button( 0 ).area().width;
        const int32_t buttonsHeight = buttons.button( 0 ).area().height;

        int32_t rows = 0;
        int32_t columns = 0;
        Point buttonsOffset;

        const int32_t buttonCount = static_cast<int32_t>( buttons.getButtonsCount() );
        int32_t horizontalGapBetweenButtons = buttonsHorizontalGap;
        // An odd number of buttons will be arranged on a single column.
        if ( isSingleColumn || buttonCount % 2 != 0 ) {
            rows = buttonCount;
            columns = 1;
            buttonsOffset = { 25, 22 };
        }
        else if ( buttonCount == 2 ) {
            rows = 1;
            columns = 2;
            buttonsOffset = { 30, 15 };
        }
        else {
            rows = 2;
            columns = buttonCount / 2;
            buttonsOffset = { 30, 15 };
            // We apply equal gaps between buttons vertically and horizontally.
            horizontalGapBetweenButtons = 2 * buttonsVerticalGap;
        }
        // This assumes that the extra height always gets added above the buttons.
        buttonsOffset.y += offsetY;

        const int32_t verticalGapOffset = isSingleColumn ? buttonsVerticalGap : 2 * buttonsVerticalGap;

        size_t buttonId = 0;
        for ( int32_t row = 0; row < rows; ++row ) {
            for ( int32_t column = 0; column < columns; ++column ) {
                buttons.button( buttonId )
                    .setPosition( _activeArea.x + column * buttonsWidth + buttonsOffset.x + column * horizontalGapBetweenButtons,
                                  _activeArea.y + ( row * ( buttonsHeight + verticalGapOffset ) ) + buttonsOffset.y );
                ++buttonId;
            }
        }
        buttons.drawShadows( _output );
        buttons.draw( _output );
    }

    Point StandardWindow::_getRenderPos( const Point & offset, const Size & itemSize, const Padding padding ) const
    {
        Point pos( _activeArea.x, _activeArea.y );

        // Apply horizontal padding.
        switch ( padding ) {
        case Padding::TOP_LEFT:
        case Padding::CENTER_LEFT:
        case Padding::BOTTOM_LEFT:
            pos.x += offset.x;
            break;
        case Padding::TOP_CENTER:
        case Padding::CENTER_CENTER:
        case Padding::BOTTOM_CENTER:
            pos.x += ( _activeArea.width - itemSize.width ) / 2 + offset.x;
            break;
        case Padding::TOP_RIGHT:
        case Padding::CENTER_RIGHT:
        case Padding::BOTTOM_RIGHT:
            pos.x += _activeArea.width - itemSize.width - offset.x;
            break;
        default:
            // Have you added a new padding? Add a logic for it.
            assert( 0 );
            break;
        }

        // Apply vertical padding.
        switch ( padding ) {
        case Padding::TOP_LEFT:
        case Padding::TOP_CENTER:
        case Padding::TOP_RIGHT:
            pos.y += offset.y;
            break;
        case Padding::CENTER_LEFT:
        case Padding::CENTER_CENTER:
        case Padding::CENTER_RIGHT:
            pos.y += ( _activeArea.height - itemSize.height ) / 2 + offset.y;
            break;
        case Padding::BOTTOM_LEFT:
        case Padding::BOTTOM_CENTER:
        case Padding::BOTTOM_RIGHT:
            pos.y += _activeArea.height - itemSize.height - offset.y;
            break;
        default:
            // Have you added a new padding? Add a logic for it.
            assert( 0 );
            break;
        }

        return pos;
    }

    void StandardWindow::renderBackgroundImage( Image & output, const Rect & roi, const int32_t borderOffset, const bool isEvilInterface )
    {
        const Sprite & backgroundSprite = AGG::GetICN( ( isEvilInterface ? ICN::STONEBAK_EVIL : ICN::STONEBAK ), 0 );
        const int32_t backgroundSpriteWidth{ backgroundSprite.width() };
        const int32_t backgroundSpriteHeight{ backgroundSprite.height() };

        const int32_t backgroundWidth = roi.width - borderOffset * 2;
        const int32_t backgroundHeight = roi.height - borderOffset * 2;
        const int32_t backgroundHorizontalCopies = ( backgroundWidth - 1 - transitionSize ) / ( backgroundSpriteWidth - transitionSize );
        const int32_t backgroundVerticalCopies = ( backgroundHeight - 1 - transitionSize ) / ( backgroundSpriteHeight - transitionSize );

        const int32_t backgroundCopyWidth = std::min( backgroundSpriteWidth, backgroundWidth );
        const int32_t backgroundCopyHeight = std::min( backgroundSpriteHeight, backgroundHeight );
        const int32_t backgroundOffsetX = roi.x + borderOffset;
        const int32_t backgroundOffsetY = roi.y + borderOffset;

        // We do a copy as the background image does not have transparent pixels.
        Copy( backgroundSprite, 0, 0, output, backgroundOffsetX, backgroundOffsetY, backgroundCopyWidth, backgroundCopyHeight );

        // If we need more copies to fill background horizontally we make a transition and copy existing image.
        if ( backgroundHorizontalCopies > 0 ) {
            int32_t toOffsetX = borderOffset + backgroundSpriteWidth;
            CreateDitheringTransition( backgroundSprite, 0, 0, output, roi.x + toOffsetX - transitionSize, backgroundOffsetY, transitionSize, backgroundCopyHeight, true,
                                       false );

            const int32_t stepX = backgroundSpriteWidth - transitionSize;
            const int32_t fromOffsetX = borderOffset + transitionSize;

            for ( int32_t i = 0; i < backgroundHorizontalCopies; ++i ) {
                Copy( output, roi.x + fromOffsetX, backgroundOffsetY, output, roi.x + toOffsetX, backgroundOffsetY,
                      std::min( backgroundSpriteWidth, roi.width - borderOffset - toOffsetX ), backgroundCopyHeight );
                toOffsetX += stepX;
            }
        }

        // If we need more copies to fill background vertically we make a transition and copy existing image in full background width.
        if ( backgroundVerticalCopies > 0 ) {
            int32_t toOffsetY = borderOffset + backgroundSpriteHeight;
            CreateDitheringTransition( output, backgroundOffsetX, backgroundOffsetY, output, backgroundOffsetX, roi.y + toOffsetY - transitionSize, backgroundWidth,
                                       transitionSize, false, false );

            const int32_t stepY = backgroundSpriteHeight - transitionSize;
            const int32_t fromOffsetY = borderOffset + transitionSize;

            for ( int32_t i = 0; i < backgroundVerticalCopies; ++i ) {
                Copy( output, backgroundOffsetX, roi.y + fromOffsetY, output, backgroundOffsetX, roi.y + toOffsetY, backgroundWidth,
                      std::min( backgroundSpriteHeight, roi.height - borderOffset - toOffsetY ) );
                toOffsetY += stepY;
            }
        }
    }
}
