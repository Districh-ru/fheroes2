/***************************************************************************
 *   fheroes2: https://github.com/ihhub/fheroes2                           *
 *   Copyright (C) 2023 - 2025                                             *
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

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <utility>

namespace Maps::Map_Format
{
    struct MapFormat;
}

namespace fheroes2
{
    class HistoryManager;

    class Action
    {
    public:
        virtual ~Action() = default;

        virtual bool redo() = 0;

        virtual bool undo() = 0;
    };

    // Remember the map state and create an action if the map has changed.
    class ActionCreator
    {
    public:
        explicit ActionCreator( HistoryManager & manager, Maps::Map_Format::MapFormat & mapFormat );

        ~ActionCreator()
        {
            if ( _action ) {
                // The action wasn't committed. Undo all the changes.
                _action->undo();
            }
        }

        ActionCreator( const ActionCreator & ) = delete;

        ActionCreator & operator=( const ActionCreator & ) = delete;

        void commit();

    private:
        HistoryManager & _manager;

        std::unique_ptr<Action> _action;
    };

    class HistoryManager
    {
    public:
        void setStateCallback( std::function<void( const bool, const bool )> stateCallback )
        {
            _stateCallback = std::move( stateCallback );
        }

        void reset()
        {
            _actions.clear();
            _lastActionId = 0;

            if ( _stateCallback ) {
                _stateCallback( false, false );
            }
        }

        void add( std::unique_ptr<Action> action )
        {
            _actions.resize( _lastActionId );

            _actions.push_back( std::move( action ) );

            ++_lastActionId;

            if ( _actions.size() > maxActions ) {
                --_lastActionId;
                _actions.pop_front();
            }

            if ( _stateCallback ) {
                _stateCallback( isUndoAvailable(), isRedoAvailable() );
            }

            assert( _actions.size() <= maxActions );
        }

        bool isUndoAvailable() const
        {
            return _lastActionId > 0;
        }

        bool isRedoAvailable() const
        {
            return _lastActionId < _actions.size();
        }

        bool undo()
        {
            if ( !isUndoAvailable() ) {
                // Nothing to do.
                return false;
            }

            --_lastActionId;
            const bool result = _actions[_lastActionId]->undo();

            if ( _stateCallback ) {
                _stateCallback( isUndoAvailable(), isRedoAvailable() );
            }

            return result;
        }

        bool redo()
        {
            if ( !isRedoAvailable() ) {
                // Nothing to do.
                return false;
            }

            const bool result = _actions[_lastActionId]->redo();
            ++_lastActionId;

            if ( _stateCallback ) {
                _stateCallback( isUndoAvailable(), isRedoAvailable() );
            }

            return result;
        }

    private:
        // We shouldn't store too many actions. It is extremely rare when there is a need to revert so many changes.
        static const size_t maxActions{ 500 };

        std::deque<std::unique_ptr<Action>> _actions;

        size_t _lastActionId{ 0 };

        std::function<void( const bool, const bool )> _stateCallback;
    };
}
